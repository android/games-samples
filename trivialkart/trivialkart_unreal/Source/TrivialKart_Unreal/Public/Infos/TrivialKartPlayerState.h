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
#include "GameFramework/PlayerState.h"
#include "TrivialKartPlayerState.generated.h"

DECLARE_DELEGATE_OneParam(FDistanceUpdated, const float);
DECLARE_DELEGATE_OneParam(FFuelUpdated, const float);
DECLARE_DELEGATE_OneParam(FCoinUpdated, const int);

/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API ATrivialKartPlayerState : public APlayerState
{
	GENERATED_BODY()
	
	float Fuel;
	
	float Distance;
	
	int CoinCount;
	
	UPROPERTY(EditAnywhere)
	TArray<FString> CarsOwned;
	
	UPROPERTY(EditAnywhere)
	FString FuelAchievementName;
	
	UPROPERTY(EditAnywhere)
	FString DistanceAchievementName;
	
	UPROPERTY(EditAnywhere)
	FString DistanceAchievementID;
	
	UPROPERTY(EditAnywhere)
	float DistanceAchievementThreshold;
	
	UPROPERTY(EditAnywhere)
	TArray<FString> CoinOfferIDs;
	
	FDelegateHandle CoinPurchaseHandle;
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void ConsumeFuel(const float FuelConsumption);
	void AddDistance(const float DistanceTravelled);
	void ConsumeCoins(const int CoinsUsed);
	void AddCarToInventory(const FString& CarID);
	
	float GetFuel() const;
	float GetDistance() const;
	int GetCoinCount() const;
	
	void Refuel();
	
	FDistanceUpdated OnDistanceUpdated;
	FFuelUpdated OnFuelUpdated;
	FCoinUpdated OnCoinUpdated;
	
#if UE_EDITOR
	void AddCoins(int CoinQuantity);
#endif
	
private:
	void UpdateFuel();
	void UpdateDistance();
	void UpdateCoin();
	
	UFUNCTION()
	void OnPurchaseReceived(const FString& PurchaseItemID, int Quantity);
};
