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

#include "OnlineExternalUIGooglePlayExtension.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineAsyncTaskGooglePlayExtensionShowProfileUI.h"
#include "OnlineSubsystemGooglePlayExtension.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

//Implementation for FOnlineExternalUIGooglePlayExtension

FOnlineExternalUIGooglePlayExtension::FOnlineExternalUIGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem, const IOnlineExternalUIPtr& InExternalUI)
: Subsystem(InSubsystem), 
BaseExternalUI(InExternalUI)
{}

bool FOnlineExternalUIGooglePlayExtension::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly,
	bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	return BaseExternalUI->ShowLoginUI(ControllerIndex, bShowOnlineOnly, bShowSkipButton, Delegate);
}

bool FOnlineExternalUIGooglePlayExtension::ShowAccountCreationUI(const int ControllerIndex,
	const FOnAccountCreationUIClosedDelegate& Delegate)
{
	return BaseExternalUI->ShowAccountCreationUI(ControllerIndex, Delegate);
}

bool FOnlineExternalUIGooglePlayExtension::ShowFriendsUI(int32 LocalUserNum)
{
	return BaseExternalUI->ShowFriendsUI(LocalUserNum);
}

bool FOnlineExternalUIGooglePlayExtension::ShowInviteUI(int32 LocalUserNum, FName SessionName)
{
	return BaseExternalUI->ShowInviteUI(LocalUserNum, SessionName);
}

bool FOnlineExternalUIGooglePlayExtension::ShowAchievementsUI(int32 LocalUserNum)
{
	return BaseExternalUI->ShowAchievementsUI(LocalUserNum);
}

bool FOnlineExternalUIGooglePlayExtension::ShowLeaderboardUI(const FString& LeaderboardName)
{
	return BaseExternalUI->ShowLeaderboardUI(LeaderboardName);
}

bool FOnlineExternalUIGooglePlayExtension::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams,
	const FOnShowWebUrlClosedDelegate& Delegate)
{
	return BaseExternalUI->ShowWebURL(Url, ShowParams, Delegate);
}

bool FOnlineExternalUIGooglePlayExtension::CloseWebURL()
{
	return BaseExternalUI->CloseWebURL();
}

bool FOnlineExternalUIGooglePlayExtension::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee,
	const FOnProfileUIClosedDelegate& Delegate)
{
	if (!Subsystem) return false;

	FString TargetPlayerId = Requestee.ToString();
	FString CurrentPlayerName = TEXT("");
	FString OtherPlayerName = TEXT("");

	// 1. Get the Local Player's Name from the Identity Interface
	if (IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
	{
		// Assuming LocalUserNum 0 for mobile
		CurrentPlayerName = IdentityInterface->GetPlayerNickname(0); 
	}

	// 2. Get the Friend's Name from the Friends Interface cache
	if (IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface())
	{
		// "Default" is the standard list name used in UE
		TSharedPtr<FOnlineFriend> Friend = FriendsInterface->GetFriend(0, Requestee, TEXT("Default"));
		if (Friend.IsValid())
		{
			OtherPlayerName = Friend->GetDisplayName();
		}
	}

	TriggerOnExternalUIChangeDelegates(true);

	// 3. Pass the resolved names into the Async Task
	Subsystem->QueueAsyncTask(new FOnlineAsyncTaskGooglePlayExtensionShowProfileUI(
		Subsystem, 
		TargetPlayerId, 
		CurrentPlayerName, // Pass local name
		OtherPlayerName,   // Pass friend name
		Delegate
	));
	return true;
}

bool FOnlineExternalUIGooglePlayExtension::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	return BaseExternalUI->ShowAccountUpgradeUI(UniqueId);
}

bool FOnlineExternalUIGooglePlayExtension::ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams,
	const FOnShowStoreUIClosedDelegate& Delegate)
{
	return BaseExternalUI->ShowStoreUI(LocalUserNum, ShowParams, Delegate);
}

bool FOnlineExternalUIGooglePlayExtension::ShowSendMessageUI(int32 LocalUserNum,
	const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	return BaseExternalUI->ShowSendMessageUI(LocalUserNum, ShowParams, Delegate);
}
