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
#include "Interfaces/OnlineFriendsInterface.h"

struct FParsedFriendData
{
	FString Id;
	FString DisplayName;
	FString AvatarUrl;
};

class FOnlineSubsystemGooglePlayExtension;
/**
 * Async Task class for getting the friend list from PGS
 */
class FOnlineAsyncTaskGooglePlayExtensionReadFriendsList: public FOnlineAsyncTaskBasic<FOnlineSubsystemGooglePlayExtension>
{
public:
	int32 LocalUserNum;
	FString ListName;
	FString ErrorStr;
	bool bInit = false;
	FOnReadFriendsListComplete Delegate;
    
	// Data populated by JNI
	TArray<FParsedFriendData> FetchedFriends;

	FOnlineAsyncTaskGooglePlayExtensionReadFriendsList(FOnlineSubsystemGooglePlayExtension* Subsystem, int32 InLocalUserNum, const FString& InListName, const FOnReadFriendsListComplete& InDelegate);

	virtual void Tick() override;
	virtual void Finalize() override; // Used to update the Friends Cache on the Game Thread
	virtual void TriggerDelegates() override;
	virtual FString ToString() const override { return TEXT("Read Friends List"); }
};
