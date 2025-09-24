// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "ShooterPlatformGameInstance.generated.h"

class FPurchaseReceipt;
/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UShooterPlatformGameInstance : public UPlatformGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	bool IsLoggedInToOnlineSubsystem();

	UFUNCTION(BlueprintCallable)
	FString GetPlayerName();

	TArray<FOnlineStoreOfferRef> StoreOffers;
	
protected:
	UPROPERTY(EditAnywhere)
	FString LoginAchievementName;
	UPROPERTY(EditAnywhere)
	FString LoginAchievementID;
	UPROPERTY(EditAnywhere)
	TMap<FString, bool> StoreListItemIDs;
	
private:
	void OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
	                      const FString& Error);
	
	void OnQueryAchievementsCompleted(const FUniqueNetId& UniqueNetId, bool bWasSuccessful);
	
	void OnQueryOnlineStoreOfferCompleted(bool bWasSuccessful, const TArray<FUniqueOfferId>& OfferIds, const FString& Error);

public:
	void AddAchievementProgress(const float Progress, const FString& AchievementName, const FString& AchievementID);
	void OnCheckoutComplete(const FOnlineError& OnlineError, const TSharedRef<FPurchaseReceipt>& PurchaseReceipt);
	void StartPurchasing(FOnlineStoreOfferRef PurchaseItem, int32 Quantity);
};
