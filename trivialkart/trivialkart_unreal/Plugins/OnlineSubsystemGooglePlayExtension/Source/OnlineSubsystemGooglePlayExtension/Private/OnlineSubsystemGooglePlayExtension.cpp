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


#include "OnlineSubsystemGooglePlayExtension.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "Online.h"
#include "OnlineEventsGooglePlayExtension.h"
#include "OnlineExternalUIGooglePlayExtension.h"
#include "OnlineFriendsGooglePlayExtension.h"
#include "OnlineIdentityGooglePlayExtension.h"
#include "OnlineStatsGooglePlayExtension.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineUserCloudGooglePlayExtension.h"

//Implementation for FOnlineSubsystemGooglePlayExtension

FOnlineSubsystemGooglePlayExtension::FOnlineSubsystemGooglePlayExtension(FName InInstanceName)
: FOnlineSubsystemImpl(GooglePlayExtensionSubsystemName, InInstanceName)
{
	GPGWrapper = MakeShareable(new FGooglePlayGamesExtensionWrapper());
}

bool FOnlineSubsystemGooglePlayExtension::Init()
{
	// 1. Start the Async Thread
	OnlineAsyncTaskThreadRunnable = MakeUnique<FOnlineAsyncTaskManagerGooglePlayExtension>();
	OnlineAsyncTaskThread.Reset(FRunnableThread::Create(
		OnlineAsyncTaskThreadRunnable.Get(), 
		*FString::Printf(TEXT("%s %s"), *SubsystemThreadName, *InstanceName.ToString())
	));

	// 2. Init Wrappers
	GPGWrapper->Init();

	// 3. Init Interfaces
	
	FriendsInterface = MakeShareable(new FOnlineFriendsGooglePlayExtension(this));
	ExternalUIPtr = MakeShareable(new FOnlineExternalUIGooglePlayExtension(this, Online::GetExternalUIInterface(TEXT("GooglePlay"))));
	CloudInterface = MakeShareable(new FOnlineUserCloudGooglePlayExtension(this));
	StatsInterface = MakeShareable(new FOnlineStatsGooglePlayExtension(this));
	TSharedPtr<FOnlineIdentityGooglePlayExtension> CurrentIdentityInterface = MakeShareable(new FOnlineIdentityGooglePlayExtension(this, Online::GetIdentityInterface(TEXT("GooglePlay"))));
	CurrentIdentityInterface->Initialize();
	IdentityInterface = CurrentIdentityInterface;
	EventsInterface = MakeShareable(new FOnlineEventsGooglePlayExtension(this));

	return true;
}

bool FOnlineSubsystemGooglePlayExtension::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		// This processes the "OutQueue", firing the delegates for completed tasks
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

	return true;
}

bool FOnlineSubsystemGooglePlayExtension::Shutdown()
{
	if (GPGWrapper.IsValid()) { GPGWrapper->Reset(); }
	return FOnlineSubsystemImpl::Shutdown();
}

IOnlineSessionPtr FOnlineSubsystemGooglePlayExtension::GetSessionInterface() const { return nullptr; }

IOnlineFriendsPtr FOnlineSubsystemGooglePlayExtension::GetFriendsInterface() const
{
	return FriendsInterface;
}

IOnlinePartyPtr FOnlineSubsystemGooglePlayExtension::GetPartyInterface() const { return nullptr; }
IOnlineGroupsPtr FOnlineSubsystemGooglePlayExtension::GetGroupsInterface() const { return nullptr; }
IOnlineSharedCloudPtr FOnlineSubsystemGooglePlayExtension::GetSharedCloudInterface() const { return nullptr; }
IOnlineVoicePtr FOnlineSubsystemGooglePlayExtension::GetVoiceInterface() const { return nullptr; }

IOnlineExternalUIPtr FOnlineSubsystemGooglePlayExtension::GetExternalUIInterface() const
{
	return ExternalUIPtr;
}

IOnlineTimePtr FOnlineSubsystemGooglePlayExtension::GetTimeInterface() const { return nullptr; }
IOnlineTitleFilePtr FOnlineSubsystemGooglePlayExtension::GetTitleFileInterface() const { return nullptr; }
IOnlineEntitlementsPtr FOnlineSubsystemGooglePlayExtension::GetEntitlementsInterface() const { return nullptr; }

IOnlineAchievementsPtr FOnlineSubsystemGooglePlayExtension::GetAchievementsInterface() const
{
	return Online::GetAchievementsInterface(TEXT("GooglePlay"));
}

IOnlineLeaderboardsPtr FOnlineSubsystemGooglePlayExtension::GetLeaderboardsInterface() const
{
	return Online::GetLeaderboardsInterface(TEXT("GooglePlay"));
}

IOnlinePresencePtr FOnlineSubsystemGooglePlayExtension::GetPresenceInterface() const { return nullptr; }
IOnlineUserPtr FOnlineSubsystemGooglePlayExtension::GetUserInterface() const { return nullptr; }

IOnlineStatsPtr FOnlineSubsystemGooglePlayExtension::GetStatsInterface() const
{
	return StatsInterface;
}

IOnlineIdentityPtr FOnlineSubsystemGooglePlayExtension::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineUserCloudPtr FOnlineSubsystemGooglePlayExtension::GetUserCloudInterface() const
{
	return CloudInterface;
}

IOnlineStoreV2Ptr FOnlineSubsystemGooglePlayExtension::GetStoreV2Interface() const
{
	if(const IOnlineSubsystem* GooglePlaySubsystem = Online::GetSubsystem(nullptr, TEXT("GooglePlay")))
	{
		return GooglePlaySubsystem->GetStoreV2Interface();
	}
	return nullptr;
}

IOnlinePurchasePtr FOnlineSubsystemGooglePlayExtension::GetPurchaseInterface() const
{
	return Online::GetPurchaseInterface(TEXT("GooglePlay"));
}

IOnlineEventsPtr FOnlineSubsystemGooglePlayExtension::GetEventsInterface() const
{
	return EventsInterface;
}

FString FOnlineSubsystemGooglePlayExtension::GetAppId() const
{
	return TEXT("GooglePlayExtension");
}

FText FOnlineSubsystemGooglePlayExtension::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemGooglePlayExtension", "OnlineServiceName", "Google Play Extension");
}

void FOnlineSubsystemGooglePlayExtension::QueueAsyncTask(class FOnlineAsyncTask* AsyncTask)
{
	check(OnlineAsyncTaskThreadRunnable);
	OnlineAsyncTaskThreadRunnable->AddToInQueue(AsyncTask);
}
