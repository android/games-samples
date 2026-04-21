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
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"

class FOnlineFriendGooglePlay : public FOnlineFriend
{
public:
	FOnlineFriendGooglePlay(const FUniqueNetIdRef& InUserId, const FString& InDisplayName)
		: UserId(InUserId), DisplayName(InDisplayName)
	{}

	// FOnlineUser Interface
	virtual FUniqueNetIdRef GetUserId() const override { return UserId; }
	virtual FString GetRealName() const override { return FString(); }
	virtual FString GetDisplayName(const FString& Platform = FString()) const override { return DisplayName; }
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override { return false; }

	// FOnlineFriend Interface
	virtual EInviteStatus::Type GetInviteStatus() const override { return EInviteStatus::Accepted; }
	virtual const FOnlineUserPresence& GetPresence() const override { return Presence; }

	// Data
	FUniqueNetIdRef UserId;
	FString DisplayName;
	FOnlineUserPresence Presence;
};

struct FParsedFriendData;
class FOnlineSubsystemGooglePlayExtension;
/**
* 
* Implementation of Online Friends Interface for handling friend lists.
*/
class FOnlineFriendsGooglePlayExtension : public IOnlineFriends
{
public:
	FOnlineFriendsGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem);
	
	//~ Begin IOnlineIdentity Interface
	virtual bool ReadFriendsList(int32 LocalUserNum, const FString& ListName, const FOnReadFriendsListComplete& Delegate = FOnReadFriendsListComplete()) override;
	virtual bool DeleteFriendsList(int32 LocalUserNum, const FString& ListName, const FOnDeleteFriendsListComplete& Delegate = FOnDeleteFriendsListComplete()) override;
	virtual bool SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnSendInviteComplete& Delegate = FOnSendInviteComplete()) override;
	virtual bool AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnAcceptInviteComplete& Delegate = FOnAcceptInviteComplete()) override;
	virtual bool RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual void SetFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FString& Alias, const FOnSetFriendAliasComplete& Delegate = FOnSetFriendAliasComplete()) override;
	virtual void DeleteFriendAlias(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName, const FOnDeleteFriendAliasComplete& Delegate = FOnDeleteFriendAliasComplete()) override;
	virtual bool DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual bool GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray< TSharedRef<FOnlineFriend> >& OutFriends) override;
	virtual TSharedPtr<FOnlineFriend> GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual bool IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) override;
	virtual void AddRecentPlayers(const FUniqueNetId& UserId, const TArray<FReportPlayedWithUser>& InRecentPlayers, const FString& ListName, const FOnAddRecentPlayersComplete& InCompletionDelegate) override;
	virtual bool QueryRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace) override;
	virtual bool GetRecentPlayers(const FUniqueNetId& UserId, const FString& Namespace, TArray< TSharedRef<FOnlineRecentPlayer> >& OutRecentPlayers) override;
	virtual void DumpRecentPlayers() const override;
	virtual bool BlockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId) override;
	virtual bool UnblockPlayer(int32 LocalUserNum, const FUniqueNetId& PlayerId) override;
	virtual bool QueryBlockedPlayers(const FUniqueNetId& UserId) override;
	virtual bool GetBlockedPlayers(const FUniqueNetId& UserId, TArray< TSharedRef<FOnlineBlockedPlayer> >& OutBlockedPlayers) override;
	virtual void DumpBlockedPlayers() const override;
	virtual void QueryFriendSettings(const FUniqueNetId& LocalUserId, FOnSettingsOperationComplete Delegate) override;
	virtual void UpdateFriendInvitePolicySettings(const FUniqueNetId& LocalUserId, const EFriendInvitePolicy FriendInvitePolicy, const bool bAffectsExistingInvites, FOnSettingsOperationComplete Delegate) override;
	virtual bool QueryFriendSettings(const FUniqueNetId& UserId, const FString& Source, const FOnQueryFriendSettingsComplete& Delegate = FOnQueryFriendSettingsComplete()) override;
	virtual bool GetFriendSettings(const FUniqueNetId& UserId, TMap<FString, TSharedRef<FOnlineFriendSettingsSourceData> >& OutSettings) override;
	virtual EFriendInvitePolicy GetFriendInvitePolicy(const FUniqueNetId& UserId) const override;
	virtual bool SetFriendSettings(const FUniqueNetId& UserId, const FString& Source, bool bNeverShowAgain, const FOnSetFriendSettingsComplete& Delegate = FOnSetFriendSettingsComplete()) override;
	//~ End IOnlineIdentity Interface
	
	void UpdateFriendsCache(int32 LocalUserNum, const FString& ListName, const TArray<FParsedFriendData>& InData);
private:
	
	FOnlineSubsystemGooglePlayExtension* Subsystem;
	TMap<FString, TArray<TSharedRef<FOnlineFriend>>> FriendsCache;
};
