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
#include "Interfaces/OnlineIdentityInterface.h"

DECLARE_DELEGATE_ThreeParams(FOnRecallSessionIDReceived, bool /*bWasSuccessful*/, const FString& /*SessionId*/, const FString& /*Error*/);

class FOnlineSubsystemGooglePlayExtension;
/**
 * Implementation of Online Identity Interface for getting PGS Recall Session ID
 */
class FOnlineIdentityGooglePlayExtension : public IOnlineIdentity
{
public:
	FOnlineIdentityGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem, const IOnlineIdentityPtr& InIdentity);
	~FOnlineIdentityGooglePlayExtension();

	void Initialize();
	//~ Begin IOnlineIdentity Interface
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const override;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const override;
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) override;
	virtual bool Logout(int32 LocalUserNum) override;
	virtual bool AutoLogin(int32 LocalUserNum) override;
	virtual FUniqueNetIdPtr GetUniquePlayerId(int32 LocalUserNum) const override;
	virtual FUniqueNetIdPtr CreateUniquePlayerId(uint8* Bytes, int32 Size) override;
	virtual FUniqueNetIdPtr CreateUniquePlayerId(const FString& Str) override;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const override;
	virtual ELoginStatus::Type GetLoginStatus(const FUniqueNetId& UserId) const override;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const override;
	virtual FString GetPlayerNickname(const FUniqueNetId& UserId) const override;
	virtual FString GetAuthToken(int32 LocalUserNum) const override;
	virtual void RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate) override;
	virtual void GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate, EShowPrivilegeResolveUI ShowResolveUI=EShowPrivilegeResolveUI::Default) override;
	virtual FPlatformUserId GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& NetId) const override;
	virtual FString GetAuthType() const override;
	//~ End IOnlineIdentity Interface
	
	void GetRecallSessionId() const;
	
	
private:
	TSharedPtr<class FUserOnlineAccountGooglePlayExtension> LocalPlayerAccount;

	FOnlineSubsystemGooglePlayExtension* Subsystem;
	IOnlineIdentityPtr BaseIdentityPtr;
	
	FDelegateHandle LoginCompleteDelegateHandle;
	
	FOnRecallSessionIDReceived OnRecallSessionIdReceivedDelegate;
	
	void ClearIdentity();
	
	void OnLoginCompleted(int LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void OnRecallSessionIDReceived(bool bWasSuccessful, const FString& SessionId, const FString& Error);
};
