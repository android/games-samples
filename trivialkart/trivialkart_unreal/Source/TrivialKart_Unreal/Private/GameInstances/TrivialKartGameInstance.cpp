/*
* Copyright 2026 The Android Open Source Project
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */


#include "GameInstances/TrivialKartGameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Objects/TrivialKartSaveGame.h"
#include "Actors/TrivialKartHUD.h"
#include "GameFramework/PlayerState.h"
#include "Actors/KartPawn.h"

DEFINE_LOG_CATEGORY(LogTemplateGameInstance);

void UTrivialKartGameInstance::Init()
{
	Super::Init();

	// Authentication
	if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
	{
		LoginHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(0,
			FOnLoginCompleteDelegate::CreateUObject(this,
				&UTrivialKartGameInstance::OnLoginCompleted));
	}

	// Cloud Save
	if (const IOnlineUserCloudPtr CloudInterface = Online::GetUserCloudInterface(GetWorld()))
	{
		ReadSaveHandle = CloudInterface->AddOnReadUserFileCompleteDelegate_Handle(
			FOnReadUserFileCompleteDelegate::CreateUObject(this, &UTrivialKartGameInstance::OnCloudReadComplete));
		WriteSaveHandle = CloudInterface->AddOnWriteUserFileCompleteDelegate_Handle(
			FOnReadUserFileCompleteDelegate::CreateUObject(this, &UTrivialKartGameInstance::OnCloudWriteComplete));
	}
	InitiateAutoLogin();
}

void UTrivialKartGameInstance::Shutdown()
{
	// Authentication
	if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
	}

	// Cloud Save
	if (const IOnlineUserCloudPtr CloudInterface = Online::GetUserCloudInterface(GetWorld()))
	{
		CloudInterface->ClearOnReadUserFileCompleteDelegate_Handle(ReadSaveHandle);
		CloudInterface->ClearOnWriteUserFileCompleteDelegate_Handle(WriteSaveHandle);
	}
}

void UTrivialKartGameInstance::InitiateAutoLogin()
{
	/*if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
	{
		IdentityInterface->AutoLogin(0);
	}*/
}

bool UTrivialKartGameInstance::GetLoginStatus() const
{
	if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
	{
		return IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn;
	}
	return false;
}

FString UTrivialKartGameInstance::GetPlayerName() const
{
	if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
	{
		return IdentityInterface->GetPlayerNickname(0);
	}
	return FString();
}

void UTrivialKartGameInstance::AddAchievementProgress(const float Progress, const FString& AchievementName)
{
	// Achievements
	/*if (AchievementName.IsEmpty())
		return;
	if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()); 
		IdentityInterface.IsValid())
	{
		if (const IOnlineAchievementsPtr AchievementsInterface = 
			Online::GetAchievementsInterface(GetWorld()); AchievementsInterface.IsValid())
		{
			FOnlineAchievement CurrentAchievement;
			AchievementsInterface->GetCachedAchievement(*IdentityInterface->GetUniquePlayerId(0), AchievementName, CurrentAchievement);
			if (CurrentAchievement.Progress < 100.0)
			{
				const float CurrentProgress = CurrentAchievement.Progress + Progress;
				const FOnlineAchievementsWritePtr AchievementPtr = MakeShareable(new FOnlineAchievementsWrite());
				AchievementPtr->SetFloatStat(AchievementName, CurrentProgress);
				FOnlineAchievementsWriteRef AchievementRef = AchievementPtr.ToSharedRef();
				AchievementsInterface->WriteAchievements(*IdentityInterface->GetUniquePlayerId(0),
					AchievementRef);
			}
		}
	}*/
}

void UTrivialKartGameInstance::StartPurchasing(const FUniqueOfferId& OfferID, const int32 Quantity, bool bIsConsumable)
{
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
		{
			if (const IOnlineStoreV2Ptr StoreV2Interface = Subsystem->GetStoreV2Interface())
			{
				if (const TSharedPtr<FOnlineStoreOffer> CurrentOrder = StoreV2Interface->GetOffer(OfferID);
					CurrentOrder.IsValid())
				{
					if (const IOnlinePurchasePtr PurchaseInterface =
						Online::GetPurchaseInterface(GetWorld()); PurchaseInterface.IsValid())
					{
						if (PurchaseInterface->IsAllowedToPurchase(*IdentityInterface->GetUniquePlayerId(0)))
						{
							FPurchaseCheckoutRequest CheckoutRequest;

							// Use the product ID from the Google Play Console (OfferId).
							CheckoutRequest.AddPurchaseOffer(
								"", 
								CurrentOrder->OfferId,
								Quantity,
								bIsConsumable
							);
							PurchaseInterface->Checkout(*IdentityInterface->GetUniquePlayerId(0),
								CheckoutRequest,
								FOnPurchaseCheckoutComplete::CreateWeakLambda(this, 
									[this, IdentityInterface, OfferID, Quantity]
									(const FOnlineError& OnlineError, const TSharedRef<FPurchaseReceipt>& PurchaseReceipt)
								{
									if (!OnlineError.WasSuccessful())
										return;
									if (OnPurchaseReceived.IsBound())
									{
										OnPurchaseReceived.Broadcast(OfferID, Quantity);
									}
									// The Purchase Token is passed as the ReceiptId to tell the platform which purchase to finalize (consume/acknowledge).
									Online::GetPurchaseInterface(GetWorld())->FinalizePurchase(*IdentityInterface->GetUniquePlayerId(0), PurchaseReceipt->TransactionId);
								}));
						}
					}
				}
			}
		}
	}
}

UTrivialKartSaveGame* UTrivialKartGameInstance::LoadGame()
{
#if WITH_EDITOR
	SaveGameInstance = Cast<UTrivialKartSaveGame>(UGameplayStatics::LoadGameFromSlot("TrivialKartLocalSave", 0));
#endif
	if (!IsValid(SaveGameInstance))
	{
		SaveGameInstance = Cast<UTrivialKartSaveGame>(UGameplayStatics::CreateSaveGameObject(SaveGameTemplate));
	}
	return SaveGameInstance;
}

void UTrivialKartGameInstance::SaveGame(UTrivialKartSaveGame* SaveData)
{
	if (!IsValid(SaveData))
		return;

#if WITH_EDITOR
	UGameplayStatics::SaveGameToSlot(SaveData, "TrivialKartLocalSave", 0);
#endif

	// Cloud Save
	/*if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
	{
		if (const IOnlineUserCloudPtr CloudInterface = Online::GetUserCloudInterface(GetWorld()))
		{
			if (TWeakObjectPtr PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0); PlayerController.IsValid())
			{
				if (TWeakObjectPtr PlayerHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); PlayerHUD.IsValid())
				{
					PlayerHUD->AddWidgetToScreen(EWidgetType::Loading, 10);
				}
			}
			TArray<uint8> ObjectBytes;
			UGameplayStatics::SaveGameToMemory(SaveData, ObjectBytes);

			CloudInterface->WriteUserFile(*IdentityInterface->GetUniquePlayerId(0), "TrivialKartCloudSave", ObjectBytes);
		}
	}*/
}

void UTrivialKartGameInstance::OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
	const FString& Error)
{
	UE_LOG(LogTemplateGameInstance, Log, TEXT("Local User: %d of Unique ID: %s has logged in with Status: %s"),
		LocalUserNum, *UserId.ToDebugString(), bWasSuccessful ? TEXT("Success") : *Error);
	if (bWasSuccessful)
	{
		if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
		{
			// Achievements
			/*if (const IOnlineAchievementsPtr AchievementsInterface =
				Online::GetAchievementsInterface(GetWorld()))
			{
				AchievementsInterface->QueryAchievements(*IdentityInterface->GetUniquePlayerId(0),
					FOnQueryAchievementsCompleteDelegate::CreateUObject(this,
						&UTrivialKartGameInstance::OnQueryAchievementsCompleted));
			}*/

			if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
			{
				if (const IOnlineStoreV2Ptr StoreInterface = Subsystem->GetStoreV2Interface())
				{
					StoreInterface->QueryOffersById(*IdentityInterface->GetUniquePlayerId(0), StoreListItemIDs,
						FOnQueryOnlineStoreOffersComplete::CreateUObject(this, &UTrivialKartGameInstance::OnQueryOnlineStoreOfferCompleted));
				}
			}

			//Cloud Save
			/*if (const IOnlineUserCloudPtr CloudInterface = Online::GetUserCloudInterface(GetWorld()))
			{
				CloudInterface->ReadUserFile(*IdentityInterface->GetUniquePlayerId(0), "TrivialKartCloudSave");
			}
			else*/
			{
				if (TWeakObjectPtr PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0); PlayerController.IsValid())
				{
					if (TWeakObjectPtr PlayerHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); PlayerHUD.IsValid())
					{
						PlayerHUD->RemoveWidgetFromScreen(EWidgetType::Loading);
					}
				}

			}
		}
	}
}

void UTrivialKartGameInstance::OnQueryAchievementsCompleted(const FUniqueNetId& UniqueNetId, bool bWasSuccessful)
{
	UE_LOG(LogTemplateGameInstance, Log, TEXT("Achievements Cached for User: %s has %s"), *UniqueNetId.ToString(), bWasSuccessful ? TEXT("Succeeded") : TEXT("Failed"));
	if (bWasSuccessful)
	{
		AddAchievementProgress(20, LogInAchievementName);
	}
}

void UTrivialKartGameInstance::OnQueryOnlineStoreOfferCompleted(bool bWasSuccessful,
	const TArray<FUniqueOfferId>& OfferIds, const FString& Error)
{
	if (bWasSuccessful)
	{
		if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
		{
			if (const IOnlineStoreV2Ptr StoreInterface = Subsystem->GetStoreV2Interface())
			{
				StoreInterface->GetOffers(StoreOffers);
			}
		}
	}
}

void UTrivialKartGameInstance::OnCloudReadComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName)
{
	// Cloud Save
	/*if (bWasSuccessful)
	{
		if (const IOnlineUserCloudPtr CloudInterface = Online::GetUserCloudInterface(GetWorld()))
		{
			TArray<uint8> FileContents;
			if (CloudInterface->GetFileContents(UserId, FileName, FileContents))
			{
				UTrivialKartSaveGame* CloudData = Cast<UTrivialKartSaveGame>(UGameplayStatics::LoadGameFromMemory(FileContents));

				if (IsValid(CloudData))
				{
					SaveGameInstance = CloudData;
				}
			}
		}
	}*/
	if (TWeakObjectPtr PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0); PlayerController.IsValid())
	{
		if (TWeakObjectPtr CurrentPawn = Cast<AKartPawn>(PlayerController->GetPawn()); CurrentPawn.IsValid() && IsValid(SaveGameInstance))
		{
			CurrentPawn->SwitchCar(SaveGameInstance->ActiveCarType);
		}
		if (TWeakObjectPtr PlayerHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); PlayerHUD.IsValid())
		{
			PlayerHUD->RemoveWidgetFromScreen(EWidgetType::Loading);
		}
	}
	UE_LOG(LogTemplateGameInstance, Log, TEXT("Cloud Save %s for file: %s"), bWasSuccessful ? *FString("was successful") : *FString("has failed"), *FileName);
}

void UTrivialKartGameInstance::OnCloudWriteComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName)
{
	// Cloud Save
	/*if (bWasSuccessful)
	{
		if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()))
		{
			if (const IOnlineUserCloudPtr CloudInterface = Online::GetUserCloudInterface(GetWorld()))
			{
				CloudInterface->ReadUserFile(*IdentityInterface->GetUniquePlayerId(0), "TrivialKartCloudSave");
			}
		}
	}
	else*/
	{
		if (TWeakObjectPtr PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0); PlayerController.IsValid())
		{
			if (TWeakObjectPtr PlayerHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); PlayerHUD.IsValid())
			{
				PlayerHUD->RemoveWidgetFromScreen(EWidgetType::Loading);
			}
		}
	}
}

