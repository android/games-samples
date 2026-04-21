/*
*	Copyright 2026 The Android Open Source Project
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*		https://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/

#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID
#include <jni.h>
#endif

DECLARE_DELEGATE_TwoParams(FOnSaveGameUIClosed, bool /*bSelected*/, FString /*SnapshotName*/);

class FOnlineAsyncTaskGooglePlayExtensionWriteSave;
class FOnlineAsyncTaskGooglePlayExtensionReadSave;
class FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID;
class FOnlineAsyncTaskGooglePlayExtensionReadFriendsList;
class FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats;
class FOnlineAsyncTaskGooglePlayExtensionShowProfileUI;

const FName GooglePlayExtensionSubsystemName = FName("GOOGLEPLAYEXTENSION");
const FString SubsystemThreadName = TEXT("OnlineAsyncTaskThreadGooglePlayExt");
/**
 * Wrapper Class used for handling calls for Java Function in Game Activity via JNI Interface
 */
class FGooglePlayGamesExtensionWrapper
{
public:
	void Init();
	void Reset();
    
    void ShowSavedGamesUI(const FString& Title, bool bAllowAdd, bool bAllowDelete, int32 MaxSaves, FOnSaveGameUIClosed OnCloseDelegate);
    void SetSnapshotMetadata(class UTexture2D* CoverImage, const FString& Description, int64 PlayedTimeMillis, int64 ProgressValue);
	bool SaveSnapshot(FOnlineAsyncTaskGooglePlayExtensionWriteSave* Task, const FString& FileName, const TArray<uint8>& Data);
	bool LoadSnapshot(FOnlineAsyncTaskGooglePlayExtensionReadSave* Task, const FString& FileName);
	
	bool GetRecallSessionId(FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID* Task);
    
    void SetFriendsConfig(int32 PageSize, bool bForceReload);
	
	bool ReadFriendsList(FOnlineAsyncTaskGooglePlayExtensionReadFriendsList* Task);
	
	void ShowProfileUI(FOnlineAsyncTaskGooglePlayExtensionShowProfileUI* Task, const FString& PlayerId, const FString& CurrentPlayerName, const FString& OtherPlayerName);
	
	bool GetPlayerStats(FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats* Task, bool bForceReload);
	
	void IncrementEvent(const FString& EventId, int32 IncrementAmount);
    
    static FOnSaveGameUIClosed SaveUIClosedDelegate;

private:
#if PLATFORM_ANDROID
	jclass WrapperClass = nullptr;
    jmethodID ShowSavedGamesUIId = nullptr;
    jmethodID SetSnapshotMetadataId = nullptr;
	jmethodID SaveId = nullptr;
	jmethodID LoadId = nullptr;
	jmethodID GetRecallId = nullptr;
	jmethodID ReadFriendsListId = nullptr;
    jmethodID SetFriendsConfigId = nullptr;
	jmethodID ShowProfileUIId = nullptr;
	jmethodID GetPlayerStatsId = nullptr;
	jmethodID IncrementEventId = nullptr;
#endif
};
