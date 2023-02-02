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
#ifndef agdktunnel_data_loader_machine_hpp
#define agdktunnel_data_loader_machine_hpp

#include "common.hpp"

// save file name
#define SAVE_FILE_NAME "tunnel.dat"

// checkpoint (save progress) every how many levels?
#define LEVELS_PER_CHECKPOINT 4

class DataLoaderStateMachine {
private:

    enum DataLoadStates {
        // loading has not been initialized
        LOAD_NOT_STARTED,
        // a new loading work has been initialized and is in progress
        DATA_INITIALIZED,
        // the user has been authenticated to Google Play Games Services
        CLOUD_USER_AUTHENTICATED,
        // the saved data has been found
        CLOUD_DATA_FOUND,
        // the loading data work has finished
        DATA_LOADED
    };

    // pointer to the current status, initialized in LOAD_NOT_STARTED
    DataLoadStates mCurrentState;

    // level to start from in play scene
    int mLevelLoaded;

    // name of the save file
    char *mSaveFileName;

    // flag to know if cloud save is enabled
    bool mIsCloudSaveEnabled;

public:

    DataLoaderStateMachine(bool isCloudSaveEnabled, char *savePath);

    ~DataLoaderStateMachine();

    // starts loading work by moving LOAD_NOT_STARTED or DATA_LOADED to
    // DATA_INITIALIZED
    void init();

    // moves state DATA_INITIALIZED to CLOUD_USER_AUTHENTICATED
    void authenticationCompleted();

    // moves state CLOUD_USER_AUTHENTICATED to CLOUD_DATA_FOUND
    void savedStateCloudDataFound();

    // finishes loading work by setting the level found on cloud and
    // moves state CLOUD_DATA_FOUND to DATA_LOADED
    void savedStateLoadingCompleted(int level);

    // finishes loading work when the user can't be authenticated, instead
    // loads local data and moves state DATA_INITIALIZED to DATA_LOADED
    void authenticationFailed();

    // finishes loading work when cloud data can't be found, instead loads
    // local data and moves state CLOUD_USER_AUTHENTICATED to DATA_LOADED
    void savedStateSnapshotNotFound();

    // finishes loading work when an error loading data occurs, instead
    // loads local data and moves DATA_LOADED to DATA_LOADED
    void savedStateLoadingFailed();

    // retrieve the level loaded after a loading operation
    int getLevelLoaded();

    // query for the total steps to get done to finish with the load operation
    int getTotalSteps();

    // query for the steps completed of the current load operation
    int getStepsCompleted();

    // asks if a loading operation is in progress
    bool isLoadingDataCompleted();

    // load progress saved in internal storage and moves state to DATA_LOADED
    void LoadLocalProgress();

    // save progress to internal storage
    void SaveLocalProgress(int level);

};

#endif //agdktunnel_data_loader_machine_hpp
