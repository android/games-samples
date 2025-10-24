const express = require('express');
const { OAuth2Client } = require('google-auth-library');

// --- CONFIGURATION ---
const YOUR_WEB_CLIENT_ID = "";
const YOUR_WEB_CLIENT_SECRET = "";
const PORT = 3000;
// ---------------------

const app = express();
app.use(express.json()); // Middleware to parse JSON bodies

// This is our simple, in-memory "database".
// In a real app, you would use a real database (like PostgreSQL, MySQL, or MongoDB).
// It will store: { "user-email@gmail.com": "ingame-1001" }
const userDatabase = new Map();
let nextInGameAccountId = 1001;

/**
 * Main endpoint to verify the auth code and link the account.
 */
app.post('/verify_and_link', async (req, res) => {
    const { authCode } = req.body;

    if (!authCode) {
        return res.status(400).json({ error: "authCode is required" });
    }

    try {
        // 1. Set up the Google Auth Client
        const client = new OAuth2Client(
            YOUR_WEB_CLIENT_ID,
            YOUR_WEB_CLIENT_SECRET,
            "postmessage" // This redirect_uri is required for the v1 server auth code flow
        );

        // 2. Exchange the code for tokens
        const { tokens } = await client.getToken(authCode);
        const idToken = tokens.id_token;

        // 3. Verify the ID token to get the user's info
        const ticket = await client.verifyIdToken({
            idToken: idToken,
            audience: YOUR_WEB_CLIENT_ID,
        });

        const payload = ticket.getPayload();
        const email = payload.email;
        const googleId = payload.sub; // This is the user's unique GAIA ID

        console.log(`Successfully verified user: ${email} (Google ID: ${googleId})`);

        // 4. Find or create the in-game account
        let inGameAccountId;
        if (userDatabase.has(email)) {
            // User already exists, retrieve their ID
            inGameAccountId = userDatabase.get(email);
            console.log(`Existing user. In-Game ID: ${inGameAccountId}`);
        } else {
            // New user, create a new in-game ID and store it
            inGameAccountId = `ingame-${nextInGameAccountId++}`;
            userDatabase.set(email, inGameAccountId);
            console.log(`New user. Created In-Game ID: ${inGameAccountId}`);
        }

        // 5. Send the success response back to the client
        res.status(200).json({
            email: email,
            inGameAccountId: inGameAccountId
        });

    } catch (error) {
        console.error("Error during token verification:", error.message);
        res.status(500).json({ error: "Failed to verify authentication" });
    }
});

app.listen(PORT, () => {
    console.log(`Game server listening on http://localhost:${PORT}`);
    console.log('Waiting for a client to connect...');
});