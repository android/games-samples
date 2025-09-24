// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/ShooterPlatformGameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineAchievementsInterface.h"

void UShooterPlatformGameInstance::Init()
{
	Super::Init();
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			IdentityInterface->AutoLogin(0);
			IdentityInterface->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &UShooterPlatformGameInstance::OnLoginCompleted));
		}
	}
}

bool UShooterPlatformGameInstance::IsLoggedInToOnlineSubsystem()
{
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			return IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn;
		}
	}
	return false;
}

FString UShooterPlatformGameInstance::GetPlayerName()
{
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			return IdentityInterface->GetPlayerNickname(0);
		}
	}
	return FString();
}

void UShooterPlatformGameInstance::OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful,
	const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
		{
			if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
			{
				if (const IOnlineAchievementsPtr AchievementsInterface =
					Subsystem->GetAchievementsInterface())
				{
					AchievementsInterface->QueryAchievements(*IdentityInterface->GetUniquePlayerId(0),
						FOnQueryAchievementsCompleteDelegate::CreateUObject(this,
							&UShooterPlatformGameInstance::OnQueryAchievementsCompleted));
				}
			}
		}
	}
}

void UShooterPlatformGameInstance::OnQueryAchievementsCompleted(const FUniqueNetId& UniqueNetId, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Achievements Cached for User: %s"), *UniqueNetId.ToString());
		AddAchievementProgress(100.0f, LoginAchievementName, LoginAchievementID);
	}
}

void UShooterPlatformGameInstance::AddAchievementProgress(const float Progress,
	const FString& AchievementName, const FString& AchievementID)
{
	if (AchievementName.IsEmpty() && AchievementID.IsEmpty())
		return;
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			if (const IOnlineAchievementsPtr AchievementsInterface =
				Subsystem->GetAchievementsInterface(); AchievementsInterface.IsValid())
			{
				FOnlineAchievement LoginAchievement;
				AchievementsInterface->GetCachedAchievement(*IdentityInterface->GetUniquePlayerId(0),LoginAchievementID, LoginAchievement);
				if (LoginAchievement.Progress < 100.0)
				{
					const float CurrentProgress = LoginAchievement.Progress + Progress;
					const FOnlineAchievementsWritePtr AchievementPtr = MakeShareable(new FOnlineAchievementsWrite());
					AchievementPtr->SetFloatStat(LoginAchievementName, CurrentProgress);
					FOnlineAchievementsWriteRef AchievementRef = AchievementPtr.ToSharedRef();
					AchievementsInterface->WriteAchievements(*IdentityInterface->GetUniquePlayerId(0),
						AchievementRef);
				}
			}
		}
	}
}
