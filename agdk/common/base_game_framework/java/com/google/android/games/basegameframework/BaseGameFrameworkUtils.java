/*
 * Copyright 2023 Google LLC
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
package com.google.android.games.basegameframework;

import android.app.Activity;
import android.app.usage.StorageStatsManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.Display;
import android.view.Surface;

import java.io.IOException;
import java.util.UUID;

public class BaseGameFrameworkUtils {
    private class FreeSpace {
        FreeSpace() {
            cacheSpace = 0;
            internalSpace = 0;
            externalSpace = 0;
            retrieved = false;
        }
        long cacheSpace;
        long internalSpace;
        long externalSpace;
        boolean retrieved;
    }

    private static final int kFeatureAndroidExtensionPack = 1;

    // Must match RootPathType in filesystem_manager.h
    private static final int kRootPathInternalStorage = 0;
    private static final int kRootPathExternalStorage = 1;
    private static final int kRootPathCacheStorage = 2;

    private static final int kOrientationLandscape = 0;
    private static final int kOrientationPortrait = 1;

    public static final int FREE_SPACE_MIN_API = Build.VERSION_CODES.O;

    public static final int CONTEXT_GET_DISPLAY_MIN_API = Build.VERSION_CODES.R;

    private Activity mGameActivity;
    private FreeSpace mFreeSpace;

    public BaseGameFrameworkUtils(Activity activity) {
        mGameActivity = activity;
    }

    public boolean GetAndroidFeatureSupported(int featureId) {
        if (featureId == kFeatureAndroidExtensionPack) {
            boolean deviceSupportsAEP = mGameActivity.getPackageManager().hasSystemFeature
                    (PackageManager.FEATURE_OPENGLES_EXTENSION_PACK);
            return deviceSupportsAEP;
        }
        return false;
    }

    public int GetExternalStorageEmulated() {
        if (Environment.isExternalStorageEmulated(mGameActivity.getExternalFilesDir(null))) {
            return 1;
        }
        return 0;
    }

    public int GetDisplayDPI() {
        return mGameActivity.getResources().getDisplayMetrics().densityDpi;
    }

    public int GetApiLevel() {
        return Build.VERSION.SDK_INT;
    }

    public String GetRootPath(int pathType) {
        String returnPath = "";
        switch (pathType) {
            case kRootPathInternalStorage:
                returnPath = mGameActivity.getFilesDir().getAbsolutePath();
                break;
            case kRootPathExternalStorage:
                returnPath = mGameActivity.getExternalFilesDir(null).getAbsolutePath();
                break;
            case kRootPathCacheStorage:
                returnPath = mGameActivity.getCacheDir().getAbsolutePath();
                break;
        }
        return returnPath;
    }

    public long GetEstimatedFreeSpace(int pathType) {
        long freeSpace = 0;
        synchronized (mFreeSpace) {
            if (mFreeSpace.retrieved) {
                switch (pathType) {
                    case kRootPathInternalStorage:
                        freeSpace = mFreeSpace.internalSpace;
                        break;
                    case kRootPathExternalStorage:
                        freeSpace = mFreeSpace.externalSpace;
                        break;
                    case kRootPathCacheStorage:
                        freeSpace = mFreeSpace.cacheSpace;
                        break;
                }
            } else {
                freeSpace = -1;
            }
        }
        return freeSpace;
    }

    public void Initialize() {
        retrieveFreeSpace();
        registerUtilObject();
    }

    private void retrieveFreeSpace() {
        mFreeSpace = new FreeSpace();
        if (Build.VERSION.SDK_INT >= FREE_SPACE_MIN_API) {
            synchronized (mFreeSpace) {
                new Thread(new Runnable() {
                    public void run() {
                        try {
                            StorageManager storageManager = (StorageManager)
                                    mGameActivity.getSystemService(Context.STORAGE_SERVICE);

                            UUID cacheUUID = storageManager.getUuidForPath(
                                    mGameActivity.getCacheDir());
                            long quota = storageManager.getCacheQuotaBytes(cacheUUID);
                            mFreeSpace.cacheSpace = quota;

                            UUID internalUUID = storageManager.getUuidForPath(
                                    mGameActivity.getFilesDir());
                            mFreeSpace.internalSpace =
                                    storageManager.getAllocatableBytes(internalUUID);

                            UUID externalUUID = storageManager.getUuidForPath(
                                    mGameActivity.getExternalFilesDir(null));
                            mFreeSpace.externalSpace = storageManager.getAllocatableBytes(
                                    externalUUID);
                            mFreeSpace.retrieved = true;
                        } catch (IOException e) {
                            mFreeSpace.retrieved = true;
                            e.printStackTrace();
                        }
                    }
                }).start();
            }
        } else {
            // If no API support, set retrieved, but all sizes are 0
            mFreeSpace.retrieved = true;
        }
    }

    @SuppressWarnings("deprecation")
    public int getDeviceRotation()
    {
        int rotation = Surface.ROTATION_0;
        if (Build.VERSION.SDK_INT >= CONTEXT_GET_DISPLAY_MIN_API) {
            Display display = mGameActivity.getDisplay();
            if (display != null) {
                rotation = display.getRotation();
            }
        } else {
            rotation = mGameActivity.getWindowManager().getDefaultDisplay().getRotation();
        }
        return rotation;
    }
    public native void registerUtilObject();
}
