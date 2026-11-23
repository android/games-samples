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

#include "Components/CheckBox.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Objects/TrivialKartSaveGame.h"
#include "Widgets/SelectableItem.h"

void UGarageWidget::RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes)
{
	OutCheckBoxes.Add(KartCheckBox);
	OutCheckBoxes.Add(LevelCheckBox);
}

void UGarageWidget::BindTabCheckBoxEvents()
{
	if (KartCheckBox)
	{
		KartCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UGarageWidget::OnKartBoxStateChanged);
	}
	if (LevelCheckBox)
	{
		LevelCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UGarageWidget::OnLevelBoxStateChange);
	}
}

void UGarageWidget::UnbindTabCheckBoxEvents()
{
	if (KartCheckBox)
	{
		KartCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UGarageWidget::OnKartBoxStateChanged);
	}
	if (LevelCheckBox)
	{
		LevelCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UGarageWidget::OnLevelBoxStateChange);
	}
}

int32 UGarageWidget::GetDefaultItemIndexForTab(int32 TabIndex, const TArray<USelectableItem*>& ActiveItems) const
{
	if (TabIndex == 0)
	{
		if (UTrivialKartGameInstance* Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()))
		{
			if (UTrivialKartSaveGame* Save = Instance->LoadGame())
			{
				if (!Save->ActiveCarType.IsEmpty())
				{
					for (int32 i = 0; i < ActiveItems.Num(); ++i)
					{
						if (ActiveItems[i] && ActiveItems[i]->GetCarType().Equals(Save->ActiveCarType, ESearchCase::IgnoreCase))
						{
							return i;
						}
					}
				}
			}
		}
	}
	return 0;
}

void UGarageWidget::OnKartBoxStateChanged(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, KartCheckBox, 0);
}

void UGarageWidget::OnLevelBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, LevelCheckBox, 1);
}
