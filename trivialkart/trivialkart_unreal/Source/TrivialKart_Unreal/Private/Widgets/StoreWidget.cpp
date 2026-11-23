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

#include "Components/CheckBox.h"

void UStoreWidget::RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes)
{
	OutCheckBoxes.Add(FuelCheckBox);
	OutCheckBoxes.Add(CoinCheckBox);
	OutCheckBoxes.Add(CarCheckBox);
	OutCheckBoxes.Add(SubscriptionCheckBox);
}

void UStoreWidget::BindTabCheckBoxEvents()
{
	if (FuelCheckBox)
	{
		FuelCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnFuelBoxStateChanged);
	}
	if (CoinCheckBox)
	{
		CoinCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnCoinBoxStateChange);
	}
	if (CarCheckBox)
	{
		CarCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnCarBoxStateChange);
	}
	if (SubscriptionCheckBox)
	{
		SubscriptionCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UStoreWidget::OnSubscriptionBoxStateChange);
	}
}

void UStoreWidget::UnbindTabCheckBoxEvents()
{
	if (FuelCheckBox)
	{
		FuelCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnFuelBoxStateChanged);
	}
	if (CoinCheckBox)
	{
		CoinCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnCoinBoxStateChange);
	}
	if (CarCheckBox)
	{
		CarCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnCarBoxStateChange);
	}
	if (SubscriptionCheckBox)
	{
		SubscriptionCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UStoreWidget::OnSubscriptionBoxStateChange);
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
