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


#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "TrivialKartGameInstance.generated.h"

class USaveGame;
class UTrivialKartSaveGame;
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPurchase, const FUniqueOfferId&, const int);
class FPurchaseReceipt;
DECLARE_LOG_CATEGORY_EXTERN(LogTemplateGameInstance, Log, All);
/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API UTrivialKartGameInstance : public UPlatformGameInstance
{
	GENERATED_BODY()
	
public:
	FOnPurchase OnPurchaseReceived;
	
	virtual void Init() override;

	virtual void Shutdown();
	
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Authentication")
	void InitiateAutoLogin();
	
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Authentication")
	bool GetLoginStatus() const;
	
	UFUNCTION(BlueprintCallable, Category = "GameInstance|Authentication")
	FString GetPlayerName() const;
	
	void AddAchievementProgress(const float Progress, const FString& AchievementName);
	void StartPurchasing(const FUniqueOfferId& OfferID, const int32 Quantity, bool bIsConsumable);
	
	UTrivialKartSaveGame* LoadGame();
	void SaveGame(UTrivialKartSaveGame* SaveData);
	
protected:
	UPROPERTY(EditAnywhere)
	TArray<FString> StoreListItemIDs;
	
	TArray<FOnlineStoreOfferRef> StoreOffers;
	
	UPROPERTY(EditAnywhere)
	FString LogInAchievementName;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<USaveGame> SaveGameTemplate;
	
	UPROPERTY()
	UTrivialKartSaveGame* SaveGameInstance;

	FDelegateHandle LoginHandle;
	FDelegateHandle ReadSaveHandle;
	FDelegateHandle WriteSaveHandle;
	
private:
	void OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
						  const FString& Error);
	
	void OnQueryAchievementsCompleted(const FUniqueNetId& UniqueNetId, bool bWasSuccessful);
	
	void OnQueryOnlineStoreOfferCompleted(bool bWasSuccessful, const TArray<FUniqueOfferId>& OfferIds, const FString& Error);

	void OnCloudReadComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName);

	void OnCloudWriteComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& FileName);
	
};
