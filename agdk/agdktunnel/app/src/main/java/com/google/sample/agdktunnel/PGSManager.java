/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.sample.agdktunnel;

import android.app.Activity;
import android.util.Log;

import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.games.AuthenticationResult;
import com.google.android.gms.games.GamesClientStatusCodes;
import com.google.android.gms.games.GamesSignInClient;
import com.google.android.gms.games.PlayGames;
import com.google.android.gms.games.PlayGamesSdk;
import com.google.android.gms.games.SnapshotsClient;
import com.google.android.gms.games.snapshot.Snapshot;
import com.google.android.gms.games.snapshot.SnapshotMetadata;
import com.google.android.gms.games.snapshot.SnapshotMetadataChange;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Tasks;

public class PGSManager {

    private final Activity mActivity;
    private final SnapshotsClient mSnapshotClient;
    private final GamesSignInClient mGamesSignInClient;
    private final String mSnapshotName = "AGDKTunnelLevel";
    // In the case of a conflict, the snapshot version with the highest progress will be used
    private static final int mConflictResolutionPolicy =
            SnapshotsClient.RESOLUTION_POLICY_HIGHEST_PROGRESS;

    private final String TAG = "AGDKTunnel (PGS)";

    PGSManager(Activity activity) {
        mActivity = activity;
        mSnapshotClient = PlayGames.getSnapshotsClient(mActivity);
        mGamesSignInClient = PlayGames.getGamesSignInClient(mActivity);
        PlayGamesSdk.initialize(mActivity);
    }

    public void onResume() {
        loadCheckpoint();
    }

    void loadCheckpoint() {
        if (isLoadingWorkInProgress()) {
            // Nothing to do
            Log.i(TAG, "Loading data task in progress detected.");
            return;
        }
        Log.i(TAG, "Initializing loading data task.");
        savedStateInitLoading();
        // Make sure the user is authenticated before loading data
        Log.i(TAG, "authenticating user to load cloud data.");
        Task<AuthenticationResult> task = mGamesSignInClient.isAuthenticated();
        task.continueWithTask(authTask -> {
            if (!authTask.isSuccessful() || !authTask.getResult().isAuthenticated()) {
                // try to sign-in
                return mGamesSignInClient.signIn();
            }
            return authTask;
        }).addOnCompleteListener(mActivity, authTask -> {
            if (!authTask.isSuccessful() || !authTask.getResult().isAuthenticated()) {
                Log.e(TAG, "the user can't be authenticated.");
                authenticationFailed();
                return;
            }
            // Signal authentication completed and schedule loading data task
            Log.i(TAG, "authentication completed, loading data.");
            authenticationCompleted();
            loadSnapshot();
        });
    }

    void saveCheckpoint(int level) {
        // Make sure the user is authenticated before saving data
        Log.i(TAG, "authenticating user to save cloud data.");
        Task<AuthenticationResult> task = mGamesSignInClient.isAuthenticated();
        task.continueWithTask(authTask -> {
            if (!authTask.isSuccessful() || !authTask.getResult().isAuthenticated()) {
                // try to sign-in
                return mGamesSignInClient.signIn();
            }
            return authTask;
        }).addOnCompleteListener(mActivity, authTask -> {
            if (!authTask.isSuccessful() || !authTask.getResult().isAuthenticated()) {
                Log.e(TAG, "the user can't be authenticated.");
                return;
            }
            Log.i(TAG, "authentication completed, loading data.");
            saveSnapshot(level);
        });
    }

    // Signal the game to update status on cloud load
    private native void savedStateInitLoading();
    private native void authenticationCompleted();
    private native void authenticationFailed();
    private native void savedStateSnapshotNotFound();
    private native void savedStateCloudDataFound();
    private native void savedStateLoadingFailed();
    private native void savedStateLoadingCompleted(int level);
    private native boolean isLoadingWorkInProgress();

    private Task<Void> loadSnapshot() {
        // Open Snapshot using its name
        return mSnapshotClient.open(
                mSnapshotName, /* createIfNotFound= */ false, mConflictResolutionPolicy)
            .addOnFailureListener(mActivity, e -> {
                // Check if snapshot was not found
                if (e instanceof ApiException) {
                    int status = ((ApiException) e).getStatusCode();
                    if (status == GamesClientStatusCodes.SNAPSHOT_NOT_FOUND) {
                        Log.i(TAG, "Level hasn't been saved");
                        savedStateSnapshotNotFound();
                        return;
                    }
                }
                Log.e(TAG, "Error while opening Snapshot to load data.", e);
                savedStateLoadingFailed();
            }).addOnSuccessListener(mActivity, task-> {
                Log.i(TAG, "Snapshot has been found.");
                savedStateCloudDataFound();
            }).continueWithTask(task -> {
                if (!task.isSuccessful()) {
                    return Tasks.forResult(null);
                }

                Log.i(TAG, "Attempting to load level from the cloud.");
                Snapshot snapshot = task.getResult().getData();
                // Opening the snapshot was a success and any conflicts have been resolved.
                try {
                    // Extract the level from the Snapshot metadata
                    Log.i(TAG, "Reading the Snapshot content.");
                    SnapshotMetadata snapshotMetadata = snapshot.getMetadata();
                    int level = (int)snapshotMetadata.getProgressValue();
                    Log.i(TAG, "level to load:" + level);
                    savedStateLoadingCompleted(level);
                } catch (NullPointerException e) {
                    Log.e(TAG, "Error while reading Snapshot content.", e);
                    savedStateLoadingFailed();
                }
                return mSnapshotClient.discardAndClose(snapshot);
            });
    }

    private Task<SnapshotMetadata> saveSnapshot(int level) {
        // Open Snapshot using its name
        return mSnapshotClient.open(
                mSnapshotName, /* createIfNotFound= */true, mConflictResolutionPolicy)
            .addOnFailureListener(mActivity, e ->
                    Log.e(TAG, "Error while opening Snapshot to save data.", e))
            .continueWithTask(task -> {
                if (!task.isSuccessful()) {
                    return Tasks.forResult(null);
                }

                Log.i(TAG, "Attempting to save level on the cloud.");
                Snapshot snapshot = task.getResult().getData();
                if (snapshot == null) {
                    // Snapshot not available
                    return Tasks.forResult(null);
                }

                String description = "AGDK Tunnel checkpoint level.";
                // Create the change operation
                SnapshotMetadataChange metadataChange =
                        new SnapshotMetadataChange.Builder()
                        .setDescription(description)
                        .setProgressValue(level)
                        .build();
                // Commit the operation
                Task<SnapshotMetadata> metadataTask =
                        mSnapshotClient.commitAndClose(snapshot, metadataChange);
                metadataTask.addOnCompleteListener(mActivity, saveTask -> {
                    if (saveTask.isSuccessful()) {
                        Log.i(TAG, "Snapshot saved!");
                    } else {
                        Log.e(TAG, "Snapshot was not saved");
                    }
                });
                return metadataTask;
            });
    }
}
