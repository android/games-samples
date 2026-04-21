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

#include "OnlineAsyncTaskGooglePlayExtensionReadFriendsList.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineFriendsGooglePlayExtension.h"
#include "OnlineSubsystemGooglePlayExtension.h"

//Implementation for FOnlineAsyncTaskGooglePlayExtensionReadFriendsList

FOnlineAsyncTaskGooglePlayExtensionReadFriendsList::FOnlineAsyncTaskGooglePlayExtensionReadFriendsList(
	FOnlineSubsystemGooglePlayExtension* Subsystem, int32 InLocalUserNum, const FString& InListName, const FOnReadFriendsListComplete& InDelegate)
: 
FOnlineAsyncTaskBasic(Subsystem), 
LocalUserNum(InLocalUserNum), 
ListName(InListName),
Delegate(InDelegate)
{}

void FOnlineAsyncTaskGooglePlayExtensionReadFriendsList::Tick()
{
	if (!bInit)
	{
		bInit = true;
		if (Subsystem && Subsystem->GPGWrapper.IsValid())
		{
			Subsystem->GPGWrapper->ReadFriendsList(this);
		}
		else
		{
			bWasSuccessful = false;
			bIsComplete = true;
		}
	}
}

void FOnlineAsyncTaskGooglePlayExtensionReadFriendsList::Finalize()
{
	if (bWasSuccessful)
	{
		IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface();
		// Cast to our concrete implementation to access cache methods
		FOnlineFriendsGooglePlayExtension* FriendsImpl = static_cast<FOnlineFriendsGooglePlayExtension*>(FriendsInterface.Get());
		if (FriendsImpl)
		{
			FriendsImpl->UpdateFriendsCache(LocalUserNum, ListName, FetchedFriends);
		}
	}
}

void FOnlineAsyncTaskGooglePlayExtensionReadFriendsList::TriggerDelegates()
{
	Delegate.ExecuteIfBound(LocalUserNum, bWasSuccessful, ListName, ErrorStr);
}
