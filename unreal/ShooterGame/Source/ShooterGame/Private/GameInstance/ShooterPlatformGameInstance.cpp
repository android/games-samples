// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/ShooterPlatformGameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"

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
				if (const IOnlineStoreV2Ptr StoreInterface = Subsystem->GetStoreV2Interface())
				{
					TArray<FString> StoreListItemKeys;
					StoreListItemIDs.GetKeys(StoreListItemKeys);
					StoreInterface->QueryOffersById(*IdentityInterface->GetUniquePlayerId(0), StoreListItemKeys, FOnQueryOnlineStoreOffersComplete::CreateUObject(this,
							&UShooterPlatformGameInstance::OnQueryOnlineStoreOfferCompleted));
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

void UShooterPlatformGameInstance::OnQueryOnlineStoreOfferCompleted(bool bWasSuccessful,
	const TArray<FUniqueOfferId>& OfferIds, const FString& Error)
{
	if (bWasSuccessful)
	{
		if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
		{
			if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
			{
				if (const IOnlineStoreV2Ptr StoreInterface = Subsystem->GetStoreV2Interface())
				{
					StoreInterface->GetOffers(StoreOffers);
				}
			}
		}
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

void UShooterPlatformGameInstance::OnCheckoutComplete(const FOnlineError& OnlineError,
	const TSharedRef<FPurchaseReceipt>& PurchaseReceipt)
{
	if (!OnlineError.WasSuccessful())
		return;
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			// The Purchase Token is passed as the ReceiptId to tell the platform which purchase to finalize (consume/acknowledge).
			Subsystem->GetPurchaseInterface()->FinalizePurchase(*IdentityInterface->GetUniquePlayerId(0), PurchaseReceipt->TransactionId);
		}
	}
}

void UShooterPlatformGameInstance::StartPurchasing(FOnlineStoreOfferRef PurchaseItem, int32 Quantity)
{
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			if (const IOnlinePurchasePtr PurchaseInterface =
				Subsystem->GetPurchaseInterface(); PurchaseInterface.IsValid())
			{
				if (PurchaseInterface->IsAllowedToPurchase(*IdentityInterface->GetUniquePlayerId(0)))
				{
					FPurchaseCheckoutRequest CheckoutRequest;
	
					// Use the product ID from the Google Play Console (OfferId).
					// Quantity is 1 for consumables, or can be 0 or 1 for non-consumables depending on platform and use.
					CheckoutRequest.AddPurchaseOffer(
						"", 
						PurchaseItem->OfferId,
						Quantity,
						StoreListItemIDs[PurchaseItem->OfferId]
					);
					PurchaseInterface->Checkout(*IdentityInterface->GetUniquePlayerId(0),
						CheckoutRequest,
						FOnPurchaseCheckoutComplete::CreateUObject(this, &UShooterPlatformGameInstance::OnCheckoutComplete));
				}
			}
		}
	}
}
