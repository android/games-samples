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
#include "OnlineAsyncTaskManager.h"

#include "OnlineSubsystemImpl.h"

class FGooglePlayGamesExtensionWrapper;

class FOnlineAsyncTaskManagerGooglePlayExtension : public FOnlineAsyncTaskManager
{
public:
	FOnlineAsyncTaskManagerGooglePlayExtension() = default;
	virtual ~FOnlineAsyncTaskManagerGooglePlayExtension() = default;

	// This must be overridden to link the manager to the subsystem
	virtual void OnlineTick() override {}
};
/**
 * Subsystem Implementation for the PGS SDK
 */
class ONLINESUBSYSTEMGOOGLEPLAYEXTENSION_API FOnlineSubsystemGooglePlayExtension : 
	public FOnlineSubsystemImpl
{
public:

	virtual ~FOnlineSubsystemGooglePlayExtension() = default;

	FOnlineSubsystemGooglePlayExtension() = delete;
	explicit FOnlineSubsystemGooglePlayExtension(FName InInstanceName);
	
	virtual bool Init() override;
	virtual bool Tick(float DeltaTime) override;
	virtual bool Shutdown() override;
	
	
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;

	virtual FString GetAppId() const override;
	virtual FText GetOnlineServiceName() const override;
	
	void QueueAsyncTask(class FOnlineAsyncTask* AsyncTask);
	
	// The JNI Wrapper Instance owned by the subsystem
	TSharedPtr<FGooglePlayGamesExtensionWrapper, ESPMode::ThreadSafe> GPGWrapper;
	
	// --- Threading Members ---
	// The "Brain" that runs the tasks
	TUniquePtr<FOnlineAsyncTaskManagerGooglePlayExtension> OnlineAsyncTaskThreadRunnable;
    
	// The actual OS Thread
	TUniquePtr<FRunnableThread> OnlineAsyncTaskThread;

private:
	IOnlineFriendsPtr FriendsInterface;
	IOnlineExternalUIPtr ExternalUIPtr;
	IOnlineUserCloudPtr CloudInterface;
	IOnlineStatsPtr StatsInterface;
	IOnlineIdentityPtr IdentityInterface;
	IOnlineEventsPtr EventsInterface;
	
};

typedef TSharedPtr<FOnlineSubsystemGooglePlayExtension, ESPMode::ThreadSafe> FOnlineSubsystemGooglePlayExtensionPtr;
