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

#include "Widgets/SelectableItem.h"

#include "Actors/KartPawn.h"
#include "Components/Button.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Infos/TrivialKartPlayerState.h"
#include "Objects/TrivialKartSaveGame.h"

void USelectableItem::NativePreConstruct()
{
	Super::NativePreConstruct();
	SelectionButton->SetStyle(SelectionButtonStyle);
}

void USelectableItem::NativeConstruct()
{
	Super::NativeConstruct();
	SelectionButton->OnClicked.AddUniqueDynamic(this, &USelectableItem::OnSelectionButtonClicked);
	if (const TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
				Instance.IsValid())
	{
		if (TWeakObjectPtr Save = Instance->LoadGame(); Save.IsValid())
		{
			bool bIsOwned = Save->CarsOwned.Contains(CarType);
			switch (SelectionOperationType)
			{
			case ESelectableOperationType::CoinPurchase:
				{
					SetIsEnabled(!bIsOwned);
				}
				break;
			case ESelectableOperationType::ObjectSelection:
				{
					SetVisibility(bIsOwned ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
				}
				break;
			}
		}
	}
}

void USelectableItem::NativeDestruct()
{
	SelectionButton->OnClicked.RemoveDynamic(this, &USelectableItem::OnSelectionButtonClicked);
	Super::NativeDestruct();
}

void USelectableItem::OnSelectionButtonClicked()
{
	switch (SelectionOperationType)
	{
	case ESelectableOperationType::InAppPurchase:
		{
#if UE_EDITOR
			switch (PurchaseObject)
			{
			case EPurchaseObject::Coin:
				if (PurchaseObject == EPurchaseObject::Coin)
				{
					if (const TWeakObjectPtr PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); 
					PlayerState.IsValid())
					{
						PlayerState->AddCoins(PurchaseQuantity);
					}
				}
				break;
			}
#endif
			if (const TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
				Instance.IsValid())
			{
				Instance->StartPurchasing(PurchaseID, PurchaseQuantity, bIsConsumable);
			}
		}
		break;
	case ESelectableOperationType::CoinPurchase:
		{
			if (const TWeakObjectPtr PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); 
				PlayerState.IsValid())
			{
				if (PlayerState->GetCoinCount() >= CoinQuantity)
				{
					switch (PurchaseObject)
					{
					case EPurchaseObject::Car:
						{
							if (CarType.IsEmpty())
								return;
							PlayerState->AddCarToInventory(CarType);
							SetIsEnabled(false);
						}
						break;
					case EPurchaseObject::Fuel:
						{
							PlayerState->Refuel();
						}
						break;
					}
					PlayerState->ConsumeCoins(CoinQuantity);
				}
			}
		}
		break;
	case ESelectableOperationType::ObjectSelection:
		{
			if (IsValid(GetOwningPlayer()))
			{
				if (TWeakObjectPtr Pawn = Cast<AKartPawn>(GetOwningPlayer()->GetPawn()); 
					Pawn.IsValid())
				{
					Pawn->SwitchCar(CarType);
					if (const TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
						Instance.IsValid())
					{
						if (TWeakObjectPtr Save = Instance->LoadGame(); Save.IsValid())
						{
							Save->ActiveCarType = CarType;
							Instance->SaveGame(Save.Get());
						}
					}
				}
			}
		}
		break;
	}
}
