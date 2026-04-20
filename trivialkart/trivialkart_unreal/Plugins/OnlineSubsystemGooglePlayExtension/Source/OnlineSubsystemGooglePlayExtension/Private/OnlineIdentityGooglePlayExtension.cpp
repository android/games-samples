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

#include "OnlineIdentityGooglePlayExtension.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineAsyncTaskGooglePlayExtensionGetRecallSessionID.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemGooglePlayExtension.h"

//Utility Class to store RecallSessionID for the current player online account.

class FUserOnlineAccountGooglePlayExtension : public FUserOnlineAccount
{
public:
	FUserOnlineAccountGooglePlayExtension(const FUniqueNetIdRef& InPlayerNetId, FString InPlayerAlias, FString InAuthCode, FString InRecallSessionID)
	: UniqueNetId(InPlayerNetId)
	, PlayerAlias(MoveTemp(InPlayerAlias))
	, AuthCode(MoveTemp(InAuthCode))
	, RecallSessionID(InRecallSessionID)
	{}

	// FOnlineUser
	virtual FUniqueNetIdRef GetUserId() const override 
	{ 
		return UniqueNetId; 
	}
	
	virtual FString GetRealName() const override 
	{ 
		return FString(); 
	}
	
	virtual FString GetDisplayName(const FString& Platform = FString()) const override 
	{ 
		return PlayerAlias; 
	}

	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override 
	{ 
		if (AttrName == USER_ATTR_DISPLAYNAME)
		{
			OutAttrValue = PlayerAlias;
			return true;
		}
		else if (AttrName == USER_ATTR_ID)
		{
			OutAttrValue = UniqueNetId->ToString();
			return true;
		}
		else if (AttrName == FString("recallSessionID"))
		{
			OutAttrValue = RecallSessionID;
			return true;
		}
		return false;
	}
	
	virtual bool SetUserLocalAttribute(const FString& AttrName, const FString& InAttrValue) override 
	{ 
		return false;
	}

	// FUserOnlineAccount
	virtual FString GetAccessToken() const override 
	{ 
		return FString(); 
	}
	
	virtual bool HasAccessTokenExpired(const FDateTime& Time) const override 
	{ 
		return true; 
	}

	virtual bool GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const override
	{
		if (AttrName == AUTH_ATTR_AUTHORIZATION_CODE && !AuthCode.IsEmpty())
		{
			OutAttrValue = AuthCode;
			return true;
		}
		return false;
	}
	
	virtual bool SetUserAttribute(const FString& AttrName, const FString& AttrValue) override 
	{ 
		return false; 
	}
private:
	FUniqueNetIdRef UniqueNetId;
	FString PlayerAlias;
	FString AuthCode;
	FString RecallSessionID;
};

//Implementation for FUserOnlineAccountGooglePlayExtension

FOnlineIdentityGooglePlayExtension::FOnlineIdentityGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem, const IOnlineIdentityPtr& InIdentity)
	: Subsystem(InSubsystem), BaseIdentityPtr(InIdentity)
{
	check(Subsystem != nullptr);
	ClearIdentity();
}

FOnlineIdentityGooglePlayExtension::~FOnlineIdentityGooglePlayExtension()
{
	BaseIdentityPtr->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteDelegateHandle);
	OnRecallSessionIdReceivedDelegate.Unbind();
}

void FOnlineIdentityGooglePlayExtension::Initialize()
{
	LoginCompleteDelegateHandle = BaseIdentityPtr->AddOnLoginCompleteDelegate_Handle(0, 
		FOnLoginCompleteDelegate::CreateRaw(this, 
			&FOnlineIdentityGooglePlayExtension::OnLoginCompleted));
	OnRecallSessionIdReceivedDelegate.BindRaw(this, &FOnlineIdentityGooglePlayExtension::OnRecallSessionIDReceived);
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityGooglePlayExtension::GetUserAccount(const FUniqueNetId& UserId) const
{
	if (LocalPlayerAccount && *LocalPlayerAccount->GetUserId() == UserId)
	{
		return LocalPlayerAccount;
	}
	return BaseIdentityPtr->GetUserAccount(UserId);
}

TArray<TSharedPtr<FUserOnlineAccount>> FOnlineIdentityGooglePlayExtension::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > Result;
	if (LocalPlayerAccount)
	{
		Result.Add(LocalPlayerAccount);
	}
	else
	{
		Result = BaseIdentityPtr->GetAllUserAccounts();
	}
	return Result;
}

bool FOnlineIdentityGooglePlayExtension::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	return BaseIdentityPtr->Login(LocalUserNum, AccountCredentials);
}

bool FOnlineIdentityGooglePlayExtension::Logout(int32 LocalUserNum)
{
	if (LocalUserNum == 0)
	{
		bool bWasLoggedIn = LocalPlayerAccount.IsValid();
		ClearIdentity();
		if(bWasLoggedIn)
		{
			TriggerOnLoginStatusChangedDelegates(0, ELoginStatus::LoggedIn, ELoginStatus::NotLoggedIn, *BaseIdentityPtr->GetUniquePlayerId(0));
			TriggerOnLoginChangedDelegates(0);
		}
	}
	return BaseIdentityPtr->Logout(LocalUserNum);
}

bool FOnlineIdentityGooglePlayExtension::AutoLogin(int32 LocalUserNum)
{
	return BaseIdentityPtr->AutoLogin(LocalUserNum);
}

FUniqueNetIdPtr FOnlineIdentityGooglePlayExtension::GetUniquePlayerId(int32 LocalUserNum) const
{
	return BaseIdentityPtr->GetUniquePlayerId(LocalUserNum);
}

FUniqueNetIdPtr FOnlineIdentityGooglePlayExtension::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	return BaseIdentityPtr->CreateUniquePlayerId(Bytes, Size);
}

FUniqueNetIdPtr FOnlineIdentityGooglePlayExtension::CreateUniquePlayerId(const FString& Str)
{
	return BaseIdentityPtr->CreateUniquePlayerId(Str);
}

ELoginStatus::Type FOnlineIdentityGooglePlayExtension::GetLoginStatus(int32 LocalUserNum) const
{
	return BaseIdentityPtr->GetLoginStatus(LocalUserNum);
}

ELoginStatus::Type FOnlineIdentityGooglePlayExtension::GetLoginStatus(const FUniqueNetId& UserId) const
{
	return BaseIdentityPtr->GetLoginStatus(UserId);
}

FString FOnlineIdentityGooglePlayExtension::GetPlayerNickname(int32 LocalUserNum) const
{
	return BaseIdentityPtr->GetPlayerNickname(LocalUserNum);
}

FString FOnlineIdentityGooglePlayExtension::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	return BaseIdentityPtr->GetPlayerNickname(UserId);
}

FString FOnlineIdentityGooglePlayExtension::GetAuthToken(int32 LocalUserNum) const
{
	return BaseIdentityPtr->GetAuthToken(LocalUserNum);
}

void FOnlineIdentityGooglePlayExtension::RevokeAuthToken(const FUniqueNetId& UserId,
	const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	BaseIdentityPtr->RevokeAuthToken(UserId, Delegate);
}

void FOnlineIdentityGooglePlayExtension::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege,
	const FOnGetUserPrivilegeCompleteDelegate& Delegate, EShowPrivilegeResolveUI ShowResolveUI)
{
	BaseIdentityPtr->GetUserPrivilege(UserId, Privilege, Delegate, ShowResolveUI);
}

FPlatformUserId FOnlineIdentityGooglePlayExtension::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& NetId) const
{
	return BaseIdentityPtr->GetPlatformUserIdFromUniqueNetId(NetId);
}

FString FOnlineIdentityGooglePlayExtension::GetAuthType() const
{
	return BaseIdentityPtr->GetAuthType();
}

void FOnlineIdentityGooglePlayExtension::GetRecallSessionId() const
{
	Subsystem->QueueAsyncTask(new FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID(Subsystem, OnRecallSessionIdReceivedDelegate));
}

void FOnlineIdentityGooglePlayExtension::ClearIdentity()
{
	LocalPlayerAccount.Reset();
}

void FOnlineIdentityGooglePlayExtension::OnLoginCompleted(int LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
                                                          const FString& Error)
{
	if (bWasSuccessful)
	{
		GetRecallSessionId();
	}
	else
	{
		TriggerOnLoginCompleteDelegates(LocalUserNum, bWasSuccessful, UserId, Error);
	}
}

void FOnlineIdentityGooglePlayExtension::OnRecallSessionIDReceived(bool bWasSuccessful, const FString& SessionId, 
	const FString& Error)
{
	LocalPlayerAccount = MakeShared<FUserOnlineAccountGooglePlayExtension>(GetUniquePlayerId(0).ToSharedRef(), GetPlayerNickname(0), GetAuthToken(0), SessionId);
	TriggerOnLoginCompleteDelegates(0, true, *GetUniquePlayerId(0), Error);
}
