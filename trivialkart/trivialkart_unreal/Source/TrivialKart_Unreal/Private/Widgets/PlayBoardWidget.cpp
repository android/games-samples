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


#include "Widgets/PlayBoardWidget.h"

#include "Actors/TrivialKartHUD.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Infos/TrivialKartPlayerState.h"


void UPlayBoardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	GarageButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnGarageButtonClicked);
	PGSButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnPGSButtonClicked);
	StoreButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnStoreButtonClicked);
	
	if (TWeakObjectPtr PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); PlayerState.IsValid())
	{
		PlayerState->OnFuelUpdated.BindUObject(this, &UPlayBoardWidget::UpdateFuelBar);
		PlayerState->OnDistanceUpdated.BindUObject(this, &UPlayBoardWidget::UpdateDistanceText);
		PlayerState->OnCoinUpdated.BindUObject(this, &UPlayBoardWidget::UpdateCoinText);
		const float FuelPercentage = PlayerState->GetFuel() / 100.0f;
		UpdateFuelBar(FuelPercentage);
		UpdateDistanceText(PlayerState->GetDistance());
		UpdateCoinText(PlayerState->GetCoinCount());
	}
}

void UPlayBoardWidget::NativeDestruct()
{
	GarageButton->OnClicked.RemoveDynamic(this, &ThisClass::OnGarageButtonClicked);
	PGSButton->OnClicked.RemoveDynamic(this, &ThisClass::OnPGSButtonClicked);
	StoreButton->OnClicked.RemoveDynamic(this, &ThisClass::OnStoreButtonClicked);
	
	if (TWeakObjectPtr PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); PlayerState.IsValid())
	{
		PlayerState->OnFuelUpdated.Unbind();
		PlayerState->OnDistanceUpdated.Unbind();
		PlayerState->OnCoinUpdated.Unbind();
	}
	Super::NativeDestruct();
}

void UPlayBoardWidget::OnGarageButtonClicked()
{
	if (TWeakObjectPtr PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::Garage);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PlayBoard);
		}
	}
}

void UPlayBoardWidget::OnPGSButtonClicked()
{
	if (TWeakObjectPtr PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::PGS);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PlayBoard);
		}
	}
}

void UPlayBoardWidget::OnStoreButtonClicked()
{
	if (TWeakObjectPtr PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::Store);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PlayBoard);
		}
	}
}

void UPlayBoardWidget::UpdateFuelBar(const float FuelBarPercentage) const
{
	FuelBar->SetPercent(FuelBarPercentage);
}

void UPlayBoardWidget::UpdateDistanceText(const float Distance) const
{
	DistanceText->SetText(FText::FromString(FString::FromInt(FMath::RoundToInt(Distance))));
}

void UPlayBoardWidget::UpdateCoinText(const int Quantity) const
{
	CoinText->SetText(FText::FromString(FString::FromInt(Quantity)));
}
