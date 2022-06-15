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

#include "data_loader_machine.hpp"

DataLoaderStateMachine::DataLoaderStateMachine(bool isCloudSaveEnabled, char *savePath) {
    mIsCloudSaveEnabled = isCloudSaveEnabled;
    int len = strlen(savePath) + strlen(SAVE_FILE_NAME) + 2;
    mSaveFileName = new char[len];
    strcpy(mSaveFileName, savePath);
    strcat(mSaveFileName, "/");
    strcat(mSaveFileName, SAVE_FILE_NAME);
    mLevelLoaded = 0;
    mCurrentState = LOAD_NOT_STARTED;
}

DataLoaderStateMachine::~DataLoaderStateMachine() {
    delete mSaveFileName;
}

int DataLoaderStateMachine::getTotalSteps() {
    return DATA_LOADED;
}

int DataLoaderStateMachine::getStepsCompleted() {
    return mCurrentState;
}

bool DataLoaderStateMachine::isLoadingDataCompleted() {
    // At the beginning, we assume that the level 0 has been loaded
    return mCurrentState == DATA_LOADED || mCurrentState == LOAD_NOT_STARTED;
}

int DataLoaderStateMachine::getLevelLoaded() {
    return mLevelLoaded;
}

void DataLoaderStateMachine::init() {
    MY_ASSERT(mCurrentState == LOAD_NOT_STARTED || mCurrentState == DATA_LOADED);
    ALOGI("Loading data initialized.");
    mCurrentState = DATA_INITIALIZED;
}

void DataLoaderStateMachine::authenticationCompleted() {
    MY_ASSERT(mCurrentState == CLOUD_USER_AUTHENTICATED - 1);
    ALOGI("Cloud authentication completed, searching saved cloud data.");
    mCurrentState = CLOUD_USER_AUTHENTICATED;
}

void DataLoaderStateMachine::savedStateCloudDataFound() {
    MY_ASSERT(mCurrentState == CLOUD_DATA_FOUND - 1);
    ALOGI("Cloud saved data found, continuing reading data.");
    mCurrentState = CLOUD_DATA_FOUND;
}

void DataLoaderStateMachine::savedStateLoadingCompleted(int level) {
    MY_ASSERT(mIsCloudSaveEnabled && mCurrentState == DATA_LOADED - 1);
    ALOGI("Using cloud save data. Level loaded: %d", level);
    mLevelLoaded = level;
    mCurrentState = DATA_LOADED;
}

void DataLoaderStateMachine::authenticationFailed() {
    ALOGE("Error authenticating the user. Using local data instead.");
    LoadLocalProgress();
}

void DataLoaderStateMachine::savedStateSnapshotNotFound() {
    ALOGI("Cloud saved data not found. Using local data instead.");
    LoadLocalProgress();
}

void DataLoaderStateMachine::savedStateLoadingFailed() {
    ALOGE("Error on loading cloud data. Using local data instead.");
    LoadLocalProgress();
}

void DataLoaderStateMachine::LoadLocalProgress() {
    ALOGI("Attempting to load locally: %s", mSaveFileName);
    mLevelLoaded = 0;
    FILE *f = fopen(mSaveFileName, "r");
    if (f) {
        ALOGI("Local file found. Loading data.");
        if (1 != fscanf(f, "v1 %d", &mLevelLoaded)) {
            ALOGE("Error parsing save file.");
            mLevelLoaded = 0;
        } else {
            ALOGI("Loaded. Level = %d", mLevelLoaded);
        }
        fclose(f);
    } else {
        ALOGI("Save file not present.");
    }
    mCurrentState = DATA_LOADED;
}

void DataLoaderStateMachine::SaveLocalProgress(int level) {
    mLevelLoaded = level;
    ALOGI("Saving progress (level %d) to file: %s", level, mSaveFileName);
    FILE *f = fopen(mSaveFileName, "w");
    if (!f) {
        ALOGE("Error writing to save game file.");
        return;
    }
    fprintf(f, "v1 %d", level);
    fclose(f);
    ALOGI("Save file written.");
}