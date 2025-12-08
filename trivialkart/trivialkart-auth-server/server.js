const express = require('express');
const { OAuth2Client } = require('google-auth-library');
const jwt = require('jsonwebtoken');

require('dotenv').config();

const WEB_CLIENT_ID = process.env.WEB_CLIENT_ID;
const WEB_CLIENT_SECRET = process.env.WEB_CLIENT_SECRET;
const FB_APP_ID = process.env.FB_APP_ID;
const FB_APP_SECRET = process.env.FB_APP_SECRET;

const JWT_SECRET = process.env.JWT_SECRET;

const PORT = 3000;

if (!WEB_CLIENT_ID || !WEB_CLIENT_SECRET || !FB_APP_ID || !FB_APP_SECRET || !JWT_SECRET) {
    console.error("FATAL ERROR: Google or Facebook .env or JWT_SECRET variables are not set.");
    process.exit(1);
}

const FB_APP_ACCESS_TOKEN = `${FB_APP_ID}|${FB_APP_SECRET}`;
const client = new OAuth2Client(WEB_CLIENT_ID, WEB_CLIENT_SECRET);

const app = express();
app.use(express.json());

//{ "openID": "ingame-1001" }
const userDatabase = new Map();

//{ "ingame-1001": 10 }
const inGameDatabase = new Map();

const verifyToken = (req, res, next) => {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1]; // Get just the token part

    if (token == null) {
        return res.status(401).json({ error: "No token provided" }); // 401 Unauthorized
    }

    // Verify the token is valid and was signed by us
    jwt.verify(token, JWT_SECRET, (err, userPayload) => {
        if (err) {
            console.error("Token verification failed:", err.message);
            return res.status(403).json({ error: "Invalid token" }); // 403 Forbidden
        }

        // Token is valid!
        req.user = userPayload;
        next();
    });
};

let nextInGameAccountId = 1001;

// ---
// PGS v1 (v0.10.x) ENDPOINT
// This endpoint receives an ID Token directly from the client.
// ---
app.post('/verify_and_link_google', async (req, res) => {
    const { idToken } = req.body;

    if (!idToken) {
        return res.status(400).json({ error: "idToken is required" });
    }

    try {
        const ticket = await client.verifyIdToken({
            idToken: idToken,
            audience: WEB_CLIENT_ID,
        });

        const payload = ticket.getPayload();
        const email = payload.email;

        // 1. Get the Google Subject ID (OpenID)
        const openId = payload.sub;

        // 2. Create the consistent DB Key (matches PGS v2 implementation)
        const dbKey = `google-${openId}`;

        console.log(`(PGS v1) Successfully verified idToken for: ${email}, OpenID: ${openId}`);

        // 3. Find or create the in-game account using the DB Key
        let inGameAccountID;

        if (userDatabase.has(dbKey)) {
            // User already exists, retrieve their ID
            inGameAccountID = userDatabase.get(dbKey);
            console.log(`(PGS v1) Existing user. In-Game ID: ${inGameAccountID}`);
        } else {
            // New user, create a new in-game ID and store it
            inGameAccountID = `ingame-${nextInGameAccountId++}`;

            userDatabase.set(dbKey, inGameAccountID);
            inGameDatabase.set(inGameAccountID, 0);

            console.log(`(PGS v1) New user. Created In-Game ID: ${inGameAccountID}`);
        }

        const tokenPayload = {
            playerID: dbKey, // Use the dbKey as the playerID for consistency
            inGameAccountID: inGameAccountID
        };
        const customJwtToken = jwt.sign(tokenPayload, JWT_SECRET, { expiresIn: '7d' });

        // 4. Send the success response back to the client
        res.status(200).json({
            playerID: dbKey,
            email: email,
            inGameAccountID: inGameAccountID,
            inGameCount : inGameDatabase.get(inGameAccountID),
            jwtToken: customJwtToken
        });

    } catch (error) {
        console.error("Error during token verification:", error.message);
        res.status(500).json({ error: "Failed to verify authentication" });
    }
});

// ---
// NEW: PGS v2 (v0.11.x+) ENDPOINT
// This endpoint receives a one-time Auth Code from the client,
// exchanges it for tokens, verifies the token, and links the account
// ---
app.post('/exchange_authcode_and_link', async (req, res) => {
    const { authCode } = req.body;

    if (!authCode) {
        return res.status(400).json({ error: "authCode is required" });
    }

    try {
        // 1. Exchange the one-time auth code for tokens
        console.log(`(PGS v2) Exchanging authCode for tokens...`);
        const { tokens } = await client.getToken(authCode);
        const idToken = tokens.id_token;

        if (!idToken) {
            throw new Error("Failed to retrieve id_token from authCode exchange.");
        }

        // 2. We now have an ID Token, so we can verify it to get the payload
        const ticket = await client.verifyIdToken({
            idToken: idToken,
            audience: WEB_CLIENT_ID, // Ensures the token was meant for this server
        });

        const payload = ticket.getPayload();
        const email = payload.email;
        const openId = payload.sub;

        console.log(`(PGS v2) Verified. Email: ${email}, OpenID: ${openId}`);

        const dbKey = `google-${openId}`;

        let inGameAccountID;

        if (userDatabase.has(dbKey)) {
            // User exists, fetch their internal ID
            inGameAccountID = userDatabase.get(dbKey);
            console.log(`(PGS v2) Existing user linked to OpenID. In-Game ID: ${inGameAccountID}`);
        } else {
            // New user, create internal ID
            inGameAccountID = `ingame-${nextInGameAccountId++}`;

            // Link the OpenID to the In-Game ID
            userDatabase.set(dbKey, inGameAccountID);
            inGameDatabase.set(inGameAccountID, 0); // Init score

            console.log(`(PGS v2) New user linked to OpenID. Created: ${inGameAccountID}`);
        }

        // 3. Create our custom JWT
        const tokenPayload = {
            playerID: dbKey,
            inGameAccountID: inGameAccountID
        };
        const customJwtToken = jwt.sign(tokenPayload, JWT_SECRET, { expiresIn: '7d' });

        // 4. Send the success response back to the client
        res.status(200).json({
            playerID: dbKey,
            email: email,
            inGameAccountID: inGameAccountID,
            inGameCount: inGameDatabase.get(inGameAccountID),
            jwtToken: customJwtToken
        });

    } catch (error) {
        console.error("Error during authCode exchange:", error.message);
        res.status(500).json({ error: "Failed to verify authentication" });
    }
});


app.post('/verify_and_link_facebook', async (req, res) => {
    const { accessToken } = req.body;

    if (!accessToken) {
        return res.status(400).json({ error: "accessToken is required" });
    }

    try {
        // 1. Verify the client's token by calling Facebook's debug_token endpoint
        const debugUrl = `https://graph.facebook.com/debug_token?input_token=${accessToken}&access_token=${FB_APP_ACCESS_TOKEN}`;

        const debugResponse = await fetch(debugUrl);
        const debugData = await debugResponse.json();

        if (!debugData.data || !debugData.data.is_valid) {
            console.error("Facebook token is invalid:", debugData);
            throw new Error("Invalid Facebook token.");
        }

        // 2. Token is valid, get the unique Facebook User ID
        const facebookUserId = debugData.data.user_id;

        // 3. (Optional but recommended) Get the user's name and email
        const userUrl = `https://graph.facebook.com/${facebookUserId}?fields=name,email&access_token=${FB_APP_ACCESS_TOKEN}`;
        const userResponse = await fetch(userUrl);
        const userData = await userResponse.json();

        const email = userData.email || null; // Email is not always guaranteed
        const name = userData.name || "Facebook User";
        console.log(`Successfully verified FB user: ${name} (ID: ${facebookUserId})`);

        // 4. Find or create the in-game account
        // We MUST prefix the ID to avoid collisions with Google IDs
        const prefixedFacebookId = `fb-${facebookUserId}`;

        let inGameAccountID;
        if (userDatabase.has(prefixedFacebookId)) {
            // User already exists
            inGameAccountID = userDatabase.get(prefixedFacebookId);
            console.log(`Existing FB user. In-Game ID: ${inGameAccountID}`);
        } else {
            // New user, create a new in-game ID
            inGameAccountID = `ingame-${nextInGameAccountId++}`;
            userDatabase.set(prefixedFacebookId, inGameAccountID);
            inGameDatabase.set(inGameAccountID, 0); // Default score
            console.log(`New FB user. Created In-Game ID: ${inGameAccountID}`);
        }

        const tokenPayload = {
            facebookId: prefixedFacebookId,
            inGameAccountID: inGameAccountID
        };
        const customJwtToken = jwt.sign(tokenPayload, JWT_SECRET, { expiresIn: '7d' });

        // 5. Send the success response back to the client
        res.status(200).json({
            playerID: prefixedFacebookId, // We re-use this field
            email: email,
            inGameAccountID: inGameAccountID,
            inGameCount : inGameDatabase.get(inGameAccountID),
            jwtToken: customJwtToken
        });

    } catch (error) {
        console.error("Error during Facebook token verification:", error.message);
        res.status(500).json({ error: "Failed to verify Facebook authentication" });
    }
});

app.post('/post_count', verifyToken, async (req, res) => {
    // 1. Get the count from the body
    const { count } = req.body;

    // 2. Get the user's info FROM THE MIDDLEWARE (req.user)
    // This is 100% secure because the token was already verified.
    const { inGameAccountID, playerID, facebookId } = req.user;

    if (typeof count === 'undefined') {
        return res.status(400).json({ error: "count is required" });
    }

    try {
        // 3. Update the database for the correct user
        inGameDatabase.set(inGameAccountID, count);

        console.log(`Updated count for ${inGameAccountID} to: ${count}`);

        // 4. Send back the success response
        res.status(200).json({
            playerID: playerID || facebookId,
            email: "",
            inGameAccountID: inGameAccountID,
            inGameCount : inGameDatabase.get(inGameAccountID)
        });

    } catch (error) {
        console.error("Error during posting count:", error.message);
        res.status(500).json({ error: "Failed to post count" });
    }
});

app.listen(PORT, () => {
    console.log(`Game server listening on http://localhost:${PORT}`);
    console.log('Waiting for a client to connect...');
});