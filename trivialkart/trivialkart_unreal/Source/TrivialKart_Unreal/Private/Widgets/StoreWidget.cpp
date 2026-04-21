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

#include "Widgets/StoreWidget.h"

#include "Actors/TrivialKartHUD.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"

void UStoreWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BackButton->OnClicked.AddUniqueDynamic(this, &UStoreWidget::OnBackButtonClicked);
	CheckBoxes.Empty();
	CheckBoxes.Add(FuelCheckBox);
	CheckBoxes.Add(CoinCheckBox);
	CheckBoxes.Add(CarCheckBox);
	CheckBoxes.Add(SubscriptionCheckBox);
	FuelCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnFuelBoxStateChanged);
	CoinCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnCoinBoxStateChange);
	CarCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnCarBoxStateChange);
	SubscriptionCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnSubscriptionBoxStateChange);
}

void UStoreWidget::NativeDestruct()
{
	BackButton->OnClicked.RemoveDynamic(this, &UStoreWidget::OnBackButtonClicked);
	FuelCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnFuelBoxStateChanged);
	CoinCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnCoinBoxStateChange);
	CarCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnCarBoxStateChange);
	SubscriptionCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnSubscriptionBoxStateChange);
	CheckBoxes.Empty();
	Super::NativeDestruct();
}

void UStoreWidget::OnBackButtonClicked()
{
	if (TWeakObjectPtr PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::PlayBoard);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::Store);
		}
	}
}

void UStoreWidget::SetCheckBoxState(bool bIsChecked, UCheckBox* CheckBox, int ActiveWidgetIndex)
{
	if (CheckBox == nullptr)
		return;
	if (bIsChecked)
	{
		ContentSwitcher->SetActiveWidgetIndex(ActiveWidgetIndex);
		for (auto CurrentCheckBox : CheckBoxes)
		{
			if (CurrentCheckBox != CheckBox)
			{
				CurrentCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
			}
		}
	}
	else
	{
		CheckBox->SetCheckedState(ECheckBoxState::Checked);
	}
}

void UStoreWidget::OnFuelBoxStateChanged(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, FuelCheckBox, 0);
}

void UStoreWidget::OnCoinBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, CoinCheckBox, 1);
}

void UStoreWidget::OnCarBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, CarCheckBox, 2);
}

void UStoreWidget::OnSubscriptionBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, SubscriptionCheckBox, 3);
}
