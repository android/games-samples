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


#include "Infos/TrivialKartPlayerState.h"

#include "GameInstances/TrivialKartGameInstance.h"
#include "Objects/TrivialKartSaveGame.h"

void ATrivialKartPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	Refuel();
	Distance = 0.0f;
	
	UpdateDistance();
	
	if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
			Instance.IsValid())
	{
		if (TWeakObjectPtr Save = Instance->LoadGame(); Save.IsValid())
		{
			CarsOwned = Save->CarsOwned;
			CoinCount = Save->CurrentCoins;
		}
		CoinPurchaseHandle = Instance->OnPurchaseReceived.AddUObject(this, &ATrivialKartPlayerState::OnPurchaseReceived);
	}
}

void ATrivialKartPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
			Instance.IsValid())
	{
		Instance->OnPurchaseReceived.Remove(CoinPurchaseHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ATrivialKartPlayerState::ConsumeFuel(const float FuelConsumption)
{
	Fuel -= FuelConsumption;
	UpdateFuel();
	if (Fuel <= 0.0f)
	{
		if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
			Instance.IsValid())
		{
			Instance->AddAchievementProgress(100, FuelAchievementName);
		}
	}
}

void ATrivialKartPlayerState::AddDistance(const float DistanceTravelled)
{
	Distance += DistanceTravelled;
	UpdateDistance();
	if (Distance >= DistanceAchievementThreshold)
	{
		if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
			Instance.IsValid())
		{
			Instance->AddAchievementProgress(100, DistanceAchievementName);
		}
	}
}

void ATrivialKartPlayerState::ConsumeCoins(const int CoinsUsed)
{
	CoinCount -= CoinsUsed;
	if (CoinCount <= 0)
	{
		CoinCount = 0;
	}
	UpdateCoin();
}

void ATrivialKartPlayerState::AddCarToInventory(const FString& CarID)
{
	if (CarsOwned.Contains(CarID))
		return;
	CarsOwned.Add(CarID);
	if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
			Instance.IsValid())
	{
		if (TWeakObjectPtr Save = Instance->LoadGame(); Save.IsValid())
		{
			Save->CarsOwned = CarsOwned;
			Instance->SaveGame(Save.Get());
		}
	}
}

float ATrivialKartPlayerState::GetFuel() const
{
	return Fuel;
}

float ATrivialKartPlayerState::GetDistance() const
{
	return Distance;
}

int ATrivialKartPlayerState::GetCoinCount() const
{
	return CoinCount;
}

void ATrivialKartPlayerState::Refuel()
{
	Fuel = 100.0f;
	UpdateFuel();
}

#if UE_EDITOR
void ATrivialKartPlayerState::AddCoins(int CoinQuantity)
{
	CoinCount += CoinQuantity;
	UpdateCoin();
}
#endif

void ATrivialKartPlayerState::UpdateFuel()
{
	const float FuelPercentage = Fuel / 100.0f;
	OnFuelUpdated.ExecuteIfBound(FuelPercentage);
}

void ATrivialKartPlayerState::UpdateDistance()
{
	OnDistanceUpdated.ExecuteIfBound(Distance);
}

void ATrivialKartPlayerState::UpdateCoin()
{
	OnCoinUpdated.ExecuteIfBound(CoinCount);
	if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
			Instance.IsValid())
	{
		if (TWeakObjectPtr Save = Instance->LoadGame(); Save.IsValid())
		{
			Save->CurrentCoins = CoinCount;
			Instance->SaveGame(Save.Get());
		}
	}
}

void ATrivialKartPlayerState::OnPurchaseReceived(const FString& PurchaseItemID, int Quantity)
{
	if (CoinOfferIDs.Contains(PurchaseItemID))
	{
		CoinCount += Quantity;
		UpdateCoin();
	}
}
