// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package com.gamesamples.trivialkartunity;

import android.accounts.Account;
import android.content.Context;
import android.util.Log;
import android.os.CancellationSignal;

import androidx.credentials.CredentialManager;
import androidx.credentials.CredentialManagerCallback;
import androidx.credentials.GetCredentialRequest;
import androidx.credentials.GetCredentialResponse;
import androidx.credentials.exceptions.GetCredentialException;

import com.google.android.libraries.identity.googleid.GetGoogleIdOption;
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential;

import com.google.android.gms.auth.api.identity.AuthorizationClient;
import com.google.android.gms.auth.api.identity.AuthorizationRequest;
import com.google.android.gms.auth.api.identity.AuthorizationResult;
import com.google.android.gms.auth.api.identity.Identity;
import com.google.android.gms.common.api.Scope;

import com.unity3d.player.UnityPlayer;

import java.util.Collections;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;


// [START google_signin_credman_example]

public class CredManBridge {

    private static final List<Scope> REQUESTED_SCOPES =
        Collections.singletonList(new Scope("https://www.googleapis.com/auth/games_lite"));

    // --- MODE 1: SILENT SIGN-IN (Called on Awake) ---
    // Tries to auto-select an authorized account. If it fails, it does NOT show UI.
    public static void signInSilent(Context context, String webClientId) {
        ExecutorService executor = null;
        try {
            CredentialManager credentialManager = CredentialManager.create(context);
            CancellationSignal cancellationSignal = new CancellationSignal();

            Log.d("CredMan", "Attempting Silent Sign-In...");

            GetGoogleIdOption silentOption = new GetGoogleIdOption.Builder()
                .setFilterByAuthorizedAccounts(true) // Strict: Only authorized accounts
                .setServerClientId(webClientId)
                .setAutoSelectEnabled(true)          // Auto-select if possible
                .build();

            GetCredentialRequest silentRequest = new GetCredentialRequest.Builder()
                .addCredentialOption(silentOption)
                .build();

            executor = Executors.newSingleThreadExecutor();
            final ExecutorService finalExecutor = executor;
            credentialManager.getCredentialAsync(
                context,
                silentRequest,
                cancellationSignal,
                finalExecutor,
                new CredentialManagerCallback<GetCredentialResponse, GetCredentialException>() {
                    @Override
                    public void onResult(GetCredentialResponse result) {
                        try {
                            Log.d("CredMan", "Silent Sign-In Successful!");
                            handleSignInResult(context, result, webClientId);
                        } finally {
                            finalExecutor.shutdown();
                        }
                    }

                    @Override
                    public void onError(GetCredentialException e) {
                        try {
                            // Send a specific error code so Unity knows to just stay on the Start Screen
                            Log.d("CredMan", "Silent sign-in failed. Keeping UI hidden.");
                            UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "SilentFailed");
                        } finally {
                            finalExecutor.shutdown();
                        }
                    }
                }
            );
        } catch (Exception e) {
            if (executor != null) {
                executor.shutdown();
            }
            Log.e("CredMan", "Silent Sign-In invocation failed", e);
            UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "SilentFailed");
        }
    }

    // --- MODE 2: INTERACTIVE SIGN-IN (Called on Button Click) ---
    // Forces the Account Selection / "Add Account" sheet to appear.
    public static void signInInteractive(Context context, String webClientId) {
        ExecutorService executor = null;
        try {
            CredentialManager credentialManager = CredentialManager.create(context);
            CancellationSignal cancellationSignal = new CancellationSignal();

            Log.d("CredMan", "Starting Interactive Sign-In...");

            GetGoogleIdOption interactiveOption = new GetGoogleIdOption.Builder()
                .setFilterByAuthorizedAccounts(false) // Show ALL accounts (and "Add Account")
                .setServerClientId(webClientId)
                .setAutoSelectEnabled(false)          // Force the UI to show
                .build();

            GetCredentialRequest interactiveRequest = new GetCredentialRequest.Builder()
                .addCredentialOption(interactiveOption)
                .build();

            executor = Executors.newSingleThreadExecutor();
            final ExecutorService finalExecutor = executor;
            credentialManager.getCredentialAsync(
                context,
                interactiveRequest,
                cancellationSignal,
                finalExecutor,
                new CredentialManagerCallback<GetCredentialResponse, GetCredentialException>() {
                    @Override
                    public void onResult(GetCredentialResponse result) {
                        try {
                            Log.d("CredMan", "Interactive Sign-In Successful!");
                            handleSignInResult(context, result, webClientId);
                        } finally {
                            finalExecutor.shutdown();
                        }
                    }

                    @Override
                    public void onError(GetCredentialException e) {
                        try {
                            Log.e("CredMan", "Interactive Sign-In Canceled or Failed", e);
                            UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "Canceled");
                        } finally {
                            finalExecutor.shutdown();
                        }
                    }
                }
            );
        } catch (Exception e) {
            if (executor != null) {
                executor.shutdown();
            }
            Log.e("CredMan", "Interactive Sign-In invocation failed", e);
            UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "Canceled");
        }
    }

    
    private static void handleSignInResult(Context context, GetCredentialResponse result, String webClientId) {
        try {
            GoogleIdTokenCredential credential = GoogleIdTokenCredential.createFrom(result.getCredential().getData());
            String email = credential.getId();
            
            if (email == null || email.trim().isEmpty()) {
                Log.e("CredMan", "Invalid email/ID from credential");
                UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "Invalid email/ID from credential");
                return;
            }

            Account account = new Account(email, "com.google");
            // Requesting GAMES_LITE scope to check for pre-existing V1 grants
            AuthorizationRequest authRequest = new AuthorizationRequest.Builder()
                .setRequestedScopes(REQUESTED_SCOPES)
                .setAccount(account)
                .requestOfflineAccess(webClientId)
                .build();
    
            AuthorizationClient authClient = Identity.getAuthorizationClient(context);
            
            authClient.authorize(authRequest)
                .addOnSuccessListener(authorizationResult -> {
                    if (authorizationResult.getServerAuthCode() != null) {
                        // CASE 1: RETURNING USER (Success)
                        // The user has already granted GAMES_LITE in the past. 
                        // We got the code directly without showing UI.
                        Log.i("CredMan", "PGS v1: Existing grant found. Returning user detected. Auth Code retrieved.");
                        UnityPlayer.UnitySendMessage("AuthManager", "OnSignInSuccess", authorizationResult.getServerAuthCode());
                    } 
                    else if (authorizationResult.hasResolution()) {
                        // CASE 2: NEW USER (PendingIntent)
                        // The user has NOT granted GAMES_LITE before. The API returned a PendingIntent 
                        // (authorizationResult.getPendingIntent()) to show the consent screen.
                        // As per your flow, we DISCARD this intent and do not show UI.
                        Log.i("CredMan", "PGS v1: No existing grant (PendingIntent returned). This is a NEW user or they revoked access.");
                        Log.i("CredMan", "PGS v1: Discarding PendingIntent. Proceeding as New User.");
                        
                        // Notify Unity that this is a "New User" so it can trigger V2 logic instead of failing
                        UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "NewUser_NoGrant");
                    } 
                    else {
                        // Edge Case: No code and no resolution?
                        Log.e("CredMan", "PGS v1: Authorization success but no Auth Code or Resolution returned.");
                        UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "No Auth Code returned");
                    }
                })
                .addOnFailureListener(e -> {
                    // CASE 3: GENERIC FAILURE
                    Log.e("CredMan", "PGS v1: Authorization failed completely.", e);
                    String errorMessage = e.getMessage() != null ? e.getMessage() : e.toString();
                    UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "Authorization Failed: " + errorMessage);
                });
    
        } catch (Exception e) {
            String errorMessage = e.getMessage() != null ? e.getMessage() : e.toString();
            UnityPlayer.UnitySendMessage("AuthManager", "OnSignInError", "Parsing Error: " + errorMessage);
        }
    }
}
// [END google_signin_credman_example]