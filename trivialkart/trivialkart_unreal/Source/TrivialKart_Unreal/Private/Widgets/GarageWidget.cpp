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


#include "Widgets/GarageWidget.h"

#include "Actors/TrivialKartHUD.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"

void UGarageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BackButton->OnClicked.AddUniqueDynamic(this, &UGarageWidget::OnBackButtonClicked);
	CheckBoxes.Empty();
	CheckBoxes.Add(KartCheckBox);
	CheckBoxes.Add(LevelCheckBox);
	KartCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UGarageWidget::OnKartBoxStateChanged);
	LevelCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UGarageWidget::OnLevelBoxStateChange);
}

void UGarageWidget::NativeDestruct()
{
	BackButton->OnClicked.RemoveDynamic(this, &UGarageWidget::OnBackButtonClicked);
	KartCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UGarageWidget::OnKartBoxStateChanged);
	LevelCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UGarageWidget::OnLevelBoxStateChange);
	Super::NativeDestruct();
}

void UGarageWidget::OnBackButtonClicked()
{
	if (TWeakObjectPtr PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::PlayBoard);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::Garage);
		}
	}
}

void UGarageWidget::SetCheckBoxState(bool bIsChecked, UCheckBox* CheckBox, int ActiveWidgetIndex)
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

void UGarageWidget::OnKartBoxStateChanged(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, KartCheckBox, 0);
}

void UGarageWidget::OnLevelBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, LevelCheckBox, 1);
}
