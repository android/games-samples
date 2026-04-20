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

#include "OnlineFriendsGooglePlayExtension.h"

#include "OnlineAsyncTaskGooglePlayExtensionReadFriendsList.h"
#include "OnlineSubsystemGooglePlayExtension.h"
#include "Interfaces/OnlineIdentityInterface.h"

//Implementation for FOnlineFriendsGooglePlayExtension

FOnlineFriendsGooglePlayExtension::FOnlineFriendsGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem)
	: Subsystem(InSubsystem)
{}

bool FOnlineFriendsGooglePlayExtension::ReadFriendsList(int32 LocalUserNum, const FString& ListName,
	const FOnReadFriendsListComplete& Delegate)
{
	Subsystem->QueueAsyncTask(new FOnlineAsyncTaskGooglePlayExtensionReadFriendsList(Subsystem, LocalUserNum, ListName, Delegate));
	return true;
}

bool FOnlineFriendsGooglePlayExtension::DeleteFriendsList(int32 LocalUserNum, const FString& ListName,
	const FOnDeleteFriendsListComplete& Delegate)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName, const FOnSendInviteComplete& Delegate)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName, const FOnAcceptInviteComplete& Delegate)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName)
{
	return false;
}

void FOnlineFriendsGooglePlayExtension::SetFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName, const FString& Alias, const FOnSetFriendAliasComplete& Delegate)
{}

void FOnlineFriendsGooglePlayExtension::DeleteFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName, const FOnDeleteFriendAliasComplete& Delegate)
{}

bool FOnlineFriendsGooglePlayExtension::DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::GetFriendsList(int32 LocalUserNum, const FString& ListName,
	TArray<TSharedRef<FOnlineFriend>>& OutFriends)
{
	if (FriendsCache.Contains(ListName))
	{
		OutFriends = FriendsCache[ListName];
		return true;
	}
	return false;
}

TSharedPtr<FOnlineFriend> FOnlineFriendsGooglePlayExtension::GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName)
{
	if (FriendsCache.Contains(ListName))
	{
		const TArray<TSharedRef<FOnlineFriend>>& List = FriendsCache[ListName];
		for (const auto& Friend : List)
		{
			if (*Friend->GetUserId() == FriendId)
			{
				return Friend;
			}
		}
	}
	return nullptr;
}

bool FOnlineFriendsGooglePlayExtension::IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId,
	const FString& ListName)
{
	return GetFriend(LocalUserNum, FriendId, ListName).IsValid();
}

void FOnlineFriendsGooglePlayExtension::AddRecentPlayers(const FUniqueNetId& UserId,
	const TArray<FReportPlayedWithUser>& InRecentPlayers, const FString& ListName,
	const FOnAddRecentPlayersComplete& InCompletionDelegate)
{}

bool FOnlineFriendsGooglePlayExtension::QueryRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::GetRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace,
	TArray<TSharedRef<FOnlineRecentPlayer>>& OutRecentPlayers)
{
	return false;
}

void FOnlineFriendsGooglePlayExtension::DumpRecentPlayers() const
{}

bool FOnlineFriendsGooglePlayExtension::BlockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::UnblockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::QueryBlockedPlayers(const FUniqueNetId& UserId)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::GetBlockedPlayers(const FUniqueNetId& UserId,
	TArray<TSharedRef<FOnlineBlockedPlayer>>& OutBlockedPlayers)
{
	return false;
}

void FOnlineFriendsGooglePlayExtension::DumpBlockedPlayers() const
{}

void FOnlineFriendsGooglePlayExtension::QueryFriendSettings(const FUniqueNetId& LocalUserId,
	FOnSettingsOperationComplete Delegate)
{
	
}

void FOnlineFriendsGooglePlayExtension::UpdateFriendInvitePolicySettings(const FUniqueNetId& LocalUserId,
	const EFriendInvitePolicy FriendInvitePolicy, const bool bAffectsExistingInvites,
	FOnSettingsOperationComplete Delegate)
{
	
}

bool FOnlineFriendsGooglePlayExtension::QueryFriendSettings(const FUniqueNetId& UserId, const FString& Source,
	const FOnQueryFriendSettingsComplete& Delegate)
{
	return false;
}

bool FOnlineFriendsGooglePlayExtension::GetFriendSettings(const FUniqueNetId& UserId,
	TMap<FString, TSharedRef<FOnlineFriendSettingsSourceData>>& OutSettings)
{
	return false;
}

EFriendInvitePolicy FOnlineFriendsGooglePlayExtension::GetFriendInvitePolicy(const FUniqueNetId& UserId) const
{
	return EFriendInvitePolicy::Friends_of_Friends;
}

bool FOnlineFriendsGooglePlayExtension::SetFriendSettings(const FUniqueNetId& UserId, const FString& Source,
	bool bNeverShowAgain, const FOnSetFriendSettingsComplete& Delegate)
{
	return false;
}

void FOnlineFriendsGooglePlayExtension::UpdateFriendsCache(int32 LocalUserNum, const FString& ListName,
	const TArray<FParsedFriendData>& InData)
{
	TArray<TSharedRef<FOnlineFriend>>& List = FriendsCache.FindOrAdd(ListName);
	List.Empty();

	for (const auto& Data : InData)
	{
		// NOTE: We assume your Identity Interface uses a specific ID format.
		// You might need your own 'CreateUniquePlayerId' implementation if it's not standard.
		// Here we just use the raw string from Google.
		FUniqueNetIdPtr NetId = Subsystem->GetIdentityInterface()->CreateUniquePlayerId(Data.Id);
		if (NetId.IsValid())
		{
			TSharedRef<FOnlineFriendGooglePlay> NewFriend = MakeShared<FOnlineFriendGooglePlay>(NetId.ToSharedRef(), Data.DisplayName);
			// If you want to store the Avatar URL, you can extend FOnlineFriendGooglePlay to hold it.
			// NewFriend->AvatarUrl = Data.AvatarUrl; 
            
			// Set basic presence to Online so they show up
			NewFriend->Presence.bIsOnline = true; 
            
			List.Add(NewFriend);
		}
	}
}
