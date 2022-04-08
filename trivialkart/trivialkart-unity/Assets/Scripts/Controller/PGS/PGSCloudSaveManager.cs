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

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

#if PLAY_GAMES_SERVICES
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using GooglePlayGames.BasicApi.SavedGame;
using UnityEditor;
#endif

public class PGSCloudSaveManager : MonoBehaviour
{
    private const string CloudSaveFilename = "TKCloud.save";

    public enum PgsCloudSaveStatus
    {
        // Cloud save is not available
        PgsCloudDisabled = 0,
        // Cloud save is waiting for PGS initialization
        PgsCloudStartup,
        // Cloud save is retrieving cloud save metadata
        PgsCloudRetrievingMetadata,
        // Cloud save is opening the cloud save file
        PgsCloudOpening,
        // Cloud save is ready for load/save requests
        PgsCloudReady,
        // Cloud save is loading the cloud save file
        PgsCloudReadingFile,
        // Cloud save is saving the cloud save file
        PgsCloudWritingFile,
        // Cloud save is checking for cloud save updates after
        // returning from the background
        PgsCloudResuming,
        // Cloud save has a later cloud save file available
        PgsCloudLaterSaveAvailable
    }

    public String CloudSaveData { get; private set; }

    public PgsCloudSaveStatus CloudSaveStatus { get; private set; }

    public bool HasCloudSave { get; private set; }

#if PLAY_GAMES_SERVICES
    private ISavedGameMetadata _cloudSaveMetadata;

    // When we resume from the background, we check to see if a later cloud save is present. If so
    // we store its metadata here and prompt the user if they want to load the later cloud save.
    // If they confirm, we use this metadata to read the later cloud and replace the existing game data.
    private ISavedGameMetadata _resumedCloudSaveMetadata;

    private bool _cloudSaveOpen;
#endif

    void Awake()
    {
#if PLAY_GAMES_SERVICES
        CloudSaveStatus = PgsCloudSaveStatus.PgsCloudStartup;
        _cloudSaveMetadata = null;
        _resumedCloudSaveMetadata = null;
        _cloudSaveOpen = false;
#else
        CloudSaveStatus = PgsCloudSaveStatus.PgsCloudDisabled;
#endif
        CloudSaveData = "";
        HasCloudSave = false;
    }

#if PLAY_GAMES_SERVICES
    // Called after successful signin, retrieve any available cloud save metadata
    // to look for a cloud save file to load
    public void RetrieveCloudMetadata()
    {
        CloudSaveData = "";
        CloudSaveStatus = PgsCloudSaveStatus.PgsCloudRetrievingMetadata;
        var savedGameClient = PlayGamesPlatform.Instance.SavedGame;
        // Retrieve the list of saved games, the count should be zero or one, as we do
        // not maintain multiple save game snapshots in the cloud
        savedGameClient.FetchAllSavedGames(DataSource.ReadNetworkOnly, (status, list) =>
        {
            if (status == SavedGameRequestStatus.Success)
            {
                Debug.Log("Cloud metadata count: " + list.Count);
                LogMetadataInfo(list);
                if (list.Count > 0)
                {
                    HasCloudSave = true;
                }
                // Open the file even if it doesn't exist, since it is a
                // prerequisite for saving to it
                OpenCloudSaveFile(PgsCloudSaveStatus.PgsCloudOpening,
                    PgsCloudSaveStatus.PgsCloudReady);
            }
            else
            {
                CloudSaveStatus = PgsCloudSaveStatus.PgsCloudDisabled;
                Debug.Log("FetchAllSavedGames returned error status: " + status);
            }
        });
    }

    // This operation must be performed prior to loading or saving a cloud save. Even if we don't currently
    // have a cloud save to load, we still need to perform the open operation before writing a save.
    private void OpenCloudSaveFile(PgsCloudSaveStatus openingStatus, PgsCloudSaveStatus openedStatus)
    {
        var savedGameClient = PlayGamesPlatform.Instance.SavedGame;
        CloudSaveStatus = openingStatus;
        savedGameClient.OpenWithAutomaticConflictResolution(CloudSaveFilename, DataSource.ReadCacheOrNetwork,
            ConflictResolutionStrategy.UseLongestPlaytime, (status, metadata) =>
            {
                if (status == SavedGameRequestStatus.Success)
                {
                    if (openedStatus == PgsCloudSaveStatus.PgsCloudLaterSaveAvailable)
                    {
                        _resumedCloudSaveMetadata = metadata;
                    }
                    else
                    {
                        _cloudSaveMetadata = metadata;
                    }
                    _cloudSaveOpen = true;
                    CloudSaveStatus = openedStatus;
                    Debug.Log("OpenCloudSaveFile success");
                }
                else
                {
                    CloudSaveStatus = PgsCloudSaveStatus.PgsCloudDisabled;
                    Debug.Log("OpenWithAutomaticConflictResolution returned error status: "
                              + status);
                }
            });
    }

    public void LoadCloudSave()
    {
        if (HasCloudSave && CloudSaveStatus == PgsCloudSaveStatus.PgsCloudReady && _cloudSaveOpen)
        {
            ReadCloudSaveData(_cloudSaveMetadata);
        }
        else
        {
            Debug.Log("LoadCloudSave called without a valid open cloud save file");
        }
    }

    private void ReadCloudSaveData(ISavedGameMetadata savedGameMetadata)
    {
        CloudSaveStatus = PgsCloudSaveStatus.PgsCloudReadingFile;
        var savedGameClient = PlayGamesPlatform.Instance.SavedGame;
        savedGameClient.ReadBinaryData(savedGameMetadata, (status, bytes) =>
        {
            if (status == SavedGameRequestStatus.Success)
            {
                CloudSaveData = Encoding.ASCII.GetString(bytes);
                CloudSaveStatus = PgsCloudSaveStatus.PgsCloudReady;
                _cloudSaveMetadata = savedGameMetadata;
                Debug.Log("ReadCloudSaveData success");
                Debug.Log(CloudSaveData);
            }
            else
            {
                CloudSaveStatus = PgsCloudSaveStatus.PgsCloudDisabled;
                Debug.Log("ReadBinaryData returned error status: " + status);
            }
        });
    }

    public void SaveToCloud(string saveData, TimeSpan timeSpan)
    {
        if (CloudSaveStatus == PgsCloudSaveStatus.PgsCloudReady)
        {
            _resumedCloudSaveMetadata = null;
            CloudSaveStatus = PgsCloudSaveStatus.PgsCloudWritingFile;
            var savedGameClient = PlayGamesPlatform.Instance.SavedGame;
            byte[] saveBytes = Encoding.ASCII.GetBytes(saveData);
            var builder = new SavedGameMetadataUpdate.Builder();
            builder = builder.WithUpdatedPlayedTime(timeSpan);
            var metadataUpdate = builder.Build();
            savedGameClient.CommitUpdate(_cloudSaveMetadata, metadataUpdate, saveBytes,
                (status, metadata) =>
            {
                if (status == SavedGameRequestStatus.Success)
                {
                    _cloudSaveOpen = false;
                    _cloudSaveMetadata = metadata;
                    HasCloudSave = true;
                    CloudSaveStatus = PgsCloudSaveStatus.PgsCloudReady;
                    Debug.Log("SaveToCloud success");
                    // Save closes the file, reopen before our next save
                    OpenCloudSaveFile(PgsCloudSaveStatus.PgsCloudOpening,
                        PgsCloudSaveStatus.PgsCloudReady);
                }
                else
                {
                    CloudSaveStatus = PgsCloudSaveStatus.PgsCloudDisabled;
                    Debug.Log("CommitUpdate returned error status: " + status);
                }
            });
        }
    }

    // When we return from the background, we need to check the cloud save to see
    // if another device might have made a cloud save while we were paused, if the
    // cloud has a 'later' save, we prompt the user if they want to load it.
    public void CheckForCloudUpdates()
    {
        if (CloudSaveStatus != PgsCloudSaveStatus.PgsCloudDisabled &&
            CloudSaveStatus != PgsCloudSaveStatus.PgsCloudResuming && _cloudSaveMetadata != null)
        {
            _resumedCloudSaveMetadata = null;
            CloudSaveStatus = PgsCloudSaveStatus.PgsCloudResuming;
            var savedGameClient = PlayGamesPlatform.Instance.SavedGame;
            // Retrieve the list of saved games, the count should be zero or one, as we do
            // not maintain multiple save game snapshots in the cloud
            savedGameClient.FetchAllSavedGames(DataSource.ReadNetworkOnly, (status, list) =>
            {
                if (status == SavedGameRequestStatus.Success)
                {
                    LogMetadataInfo(list);
                    CloudSaveStatus = PgsCloudSaveStatus.PgsCloudReady;
                    if (list.Count > 0)
                    {
                        var metadata = list[0];
                        if (metadata.TotalTimePlayed > _cloudSaveMetadata.TotalTimePlayed)
                        {
                            Debug.Log("Found later cloud save");
                            _resumedCloudSaveMetadata = metadata;
                            CloudSaveStatus = PgsCloudSaveStatus.PgsCloudLaterSaveAvailable;
                        }
                        // If we saved when going to the background, the cloud save file might
                        // be closed, make sure the cloud save file is open
                        if (!metadata.IsOpen)
                        {
                            OpenCloudSaveFile(PgsCloudSaveStatus.PgsCloudResuming,
                                PgsCloudSaveStatus.PgsCloudLaterSaveAvailable);
                        }
                    }
                }
                else
                {
                    CloudSaveStatus = PgsCloudSaveStatus.PgsCloudDisabled;
                    Debug.Log("CommitUpdate returned error status: " + status);
                }
            });
        }
    }

    private void LogMetadataInfo(List<ISavedGameMetadata> metadataList)
    {
        foreach (var metadata in metadataList)
        {
            float distance = GameDataController.ConvertTimespanToDistance(metadata.TotalTimePlayed);
            Debug.Log("Metadata entry");
            Debug.Log(metadata.Filename + " : isOpen " + metadata.IsOpen +
                      " Distance : " + distance + " TimePlayed: " +
                      metadata.TotalTimePlayed.Ticks);
            Debug.Log(metadata.LastModifiedTimestamp.ToString());
        }
    }

    public TimeSpan GetLaterCloudSaveTimespan()
    {
        if (_resumedCloudSaveMetadata != null)
        {
            return _resumedCloudSaveMetadata.TotalTimePlayed;
        }
        return new TimeSpan(0);
    }

    public void LoadLaterCloudSave()
    {
        if (CloudSaveStatus == PgsCloudSaveStatus.PgsCloudLaterSaveAvailable)
        {
            ReadCloudSaveData(_resumedCloudSaveMetadata);
        }
    }

    public void IgnoreLaterCloudSave()
    {
        if (CloudSaveStatus == PgsCloudSaveStatus.PgsCloudLaterSaveAvailable)
        {
            CloudSaveStatus = PgsCloudSaveStatus.PgsCloudReady;
        }
    }
#endif
}
