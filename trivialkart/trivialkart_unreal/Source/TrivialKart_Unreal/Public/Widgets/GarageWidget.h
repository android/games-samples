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
#include "GarageWidget.generated.h"

class UCheckBox;
class USelectableItem;

/**
 * Garage modal widget managing Kart and Level selection tabs.
 */
UCLASS()
class TRIVIALKART_UNREAL_API UGarageWidget : public UTrivialKartTabbedModalWidget
{
	GENERATED_BODY()

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> KartCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> LevelCheckBox;

protected:
	virtual EWidgetType GetModalWidgetType() const override { return EWidgetType::Garage; }
	virtual void RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes) override;
	virtual void BindTabCheckBoxEvents() override;
	virtual void UnbindTabCheckBoxEvents() override;
	virtual int32 GetDefaultItemIndexForTab(int32 TabIndex, const TArray<USelectableItem*>& ActiveItems) const override;

private:
	UFUNCTION()
	void OnKartBoxStateChanged(bool bIsChecked);

	UFUNCTION()
	void OnLevelBoxStateChange(bool bIsChecked);
};
