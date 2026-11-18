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
#include "Widgets/TrivialKartTabbedModalWidget.h"
#include "StoreWidget.generated.h"

class UCheckBox;

/**
 * Store modal widget managing Fuel, Coin, Car, and Subscription tabs.
 */
UCLASS()
class TRIVIALKART_UNREAL_API UStoreWidget : public UTrivialKartTabbedModalWidget
{
	GENERATED_BODY()

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> FuelCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> CoinCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> CarCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> SubscriptionCheckBox;

protected:
	virtual EWidgetType GetModalWidgetType() const override { return EWidgetType::Store; }
	virtual void RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes) override;
	virtual void BindTabCheckBoxEvents() override;
	virtual void UnbindTabCheckBoxEvents() override;

private:
	UFUNCTION()
	void OnFuelBoxStateChanged(bool bIsChecked);

	UFUNCTION()
	void OnCoinBoxStateChange(bool bIsChecked);

	UFUNCTION()
	void OnCarBoxStateChange(bool bIsChecked);

	UFUNCTION()
	void OnSubscriptionBoxStateChange(bool bIsChecked);
};
