const express = require('express');
const cors = require('cors');
const fs = require('fs');
const { JWT } = require('google-auth-library');
const axios = require('axios');
const { v4: uuidv4 } = require('uuid'); // Used for generating unique tokens
require('dotenv').config();

// --- Simulated Database ---
// In a real application, this would be a real database like PostgreSQL, MongoDB, etc.
// We use the unique 'recallToken' as the key.
const playerDatabase = new Map();

// --- Google Auth Initialization (Unchanged) ---
const KEY_FILE_PATH = process.env.KEY_FILE_PATH;
const SCOPES = ['https://www.googleapis.com/auth/androidpublisher'];

let auth;
try {
    if (!fs.existsSync(KEY_FILE_PATH)) {
        throw new Error(`Service account key file not found at path: ${KEY_FILE_PATH}`);
    }
    const keyFileContent = fs.readFileSync(KEY_FILE_PATH, 'utf8');
    const credentials = JSON.parse(keyFileContent);

    auth = new JWT({
        email: credentials.client_email,
        key: credentials.private_key,
        scopes: SCOPES,
    });
} catch (error) {
    console.error('FATAL ERROR: Could not initialize Google Authentication.');
    console.error('Original error:', error.message);
    process.exit(1);
}

const app = express();
const port = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());


// --- Google API Functions ---

async function getAccessToken() {
    const tokenResponse = await auth.getAccessToken();
    const accessToken = tokenResponse.token;
    if (!accessToken) {
        throw new Error('Failed to retrieve a valid access token');
    }
    console.log('Successfully obtained Google API access token.');
    return accessToken;
}

// Function to link a new persona
async function linkNewPersona(recallSessionId, persona, token) {
    console.log(`Attempting to link persona '${persona}'...`);
    const accessToken = await getAccessToken();
    const apiUrl = `https://games.googleapis.com/games/v1/recall:linkPersona`;

    try {
        await axios.post(apiUrl, {
            token: token,
            persona: persona,
            // cardinality_constraint: 'ONE_TOKEN_PER_PERSONA_PER_GAME',
            conflicting_links_resolution_policy: 'CREATE_NEW_LINK'
        }, {
            headers: {
                'Authorization': `Bearer ${accessToken}`,
                'Content-Type': 'application/json'
            },
            params: {
                sessionId: recallSessionId
            }
        });
        console.log(`Successfully linked persona '${persona}'.`);
        return true;
    } catch (error) {
        console.error('Error calling Google Link Persona API:', error.response ? JSON.stringify(error.response.data, null, 2) : error.message);
        throw new Error('Failed to link new persona.');
    }
}


// --- Routes ---
app.get('/', (req, res) => {
    res.send('Node.js server for Unity Recall API is running!');
});

// MODIFIED: This route now handles new and existing players
app.post('/recall-session', async (req, res) => {
    console.log('\nReceived a request on /recall-session');
    const { token: recallSessionId } = req.body;

    if (!recallSessionId) {
        return res.status(400).json({ status: 'error', message: 'No session ID provided.' });
    }

    try {
        const accessToken = await getAccessToken();
        const encodedSessionId = encodeURIComponent(recallSessionId);
        const apiUrl = `https://games.googleapis.com/games/v1/recall/tokens/${encodedSessionId}`;

        const response = await axios.get(apiUrl, {
            headers: { 'Authorization': `Bearer ${accessToken}` },
            // Important: Expect a 404 if no tokens are found, don't treat it as an error
            validateStatus: (status) => (status >= 200 && status < 300) || status === 404,
        });

        if (response.status === 404 || !response.data.tokens || response.data.tokens.length === 0) {
            console.log('No tokens found for session. This is a new player.');
            return res.status(200).json({ status: 'NewPlayer' });
        }

        const playerRecallToken = response.data.tokens[0].token;
        console.log(`Found recall token: ${playerRecallToken}`);

        if (playerDatabase.has(playerRecallToken)) {
            const playerData = playerDatabase.get(playerRecallToken);
            console.log('Player found in database. Sending data to client.');
            res.status(200).json({ status: 'AccountFound', playerData });
        } else {
            console.warn('Orphaned token found. Google has a link, but DB has no record.');
            res.status(200).json({ status: 'NewPlayer', message: 'Orphaned token detected.' });
        }

    } catch (error) {
        console.error('An error occurred during the recall process:', error.message);
        res.status(500).json({ status: 'error', message: 'An internal server error occurred.' });
    }
});

// NEW: This route handles creating a new linked account
app.post('/create-account', async (req, res) => {
    console.log('\nReceived a request on /create-account');
    const { recallSessionId, username } = req.body;

    if (!recallSessionId || !username) {
        return res.status(400).json({ status: 'error', message: 'Session ID and username are required.' });
    }

    try {
        // 1. Generate a new unique token for our database. UUID is a good choice.
        const newRecallToken = uuidv4();
        // 2. The persona should be a stable, non-sensitive identifier. We'll use the token itself.
        const newPersona = newRecallToken;

        // 3. Link the persona on Google's side
        await linkNewPersona(recallSessionId, newPersona, newRecallToken);

        // 4. Create the player record in our database
        const newPlayerData = {
            username: username,
            coinsOwned: 1,
            distanceTraveled: 100,
            createdAt: new Date().toISOString()
        };
        playerDatabase.set(newRecallToken, newPlayerData);

        console.log(`Successfully created and linked account for ${username}.`);

        // 5. Send the new player data back to the client
        res.status(201).json({ status: 'AccountCreated', playerData: newPlayerData });

    } catch (error) {
        console.error('An error occurred during account creation:', error.message);
        res.status(500).json({ status: 'error', message: 'Failed to create account.' });
    }
});

// --- Server Activation ---
app.listen(port, () => {
    console.log(`Server listening at http://localhost:${port}`);
});