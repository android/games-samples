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
#include "Blueprint/UserWidget.h"
#include "Controllers/TrivialKartPlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/SlateTypes.h"
#include "GarageWidget.generated.h"

class UBorder;
class UButton;
class UWidgetSwitcher;
class UCheckBox;
class USelectableItem;

/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API UGarageWidget : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> BackButton;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> KartCheckBox;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> LevelCheckBox;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;
	
protected:
	UPROPERTY()
	TArray<UCheckBox*> CheckBoxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> TabHighlightBorders;

	TWeakObjectPtr<USelectableItem> GamepadPressedItem;
	TWeakObjectPtr<USelectableItem> HoveredItem;

	int32 ActiveTabIndex = 0;
	int32 FocusedItemIndex = 0;
	FButtonStyle OriginalBackButtonStyle;
	FLinearColor OriginalBackButtonBgColor = FLinearColor::White;
	bool bBackStyleCached = false;
	bool bBackButtonPressed = false;

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	bool HandleGamepadKey(const FKey& Key, bool bIsPressed = true);
	void RefreshGamepadFocus(bool bScrollIntoView = true);
	void RefreshOwnershipAndFocus(bool bScrollIntoView = false);
	void CancelPendingPress();
	
private:
	UFUNCTION()
	void OnBackButtonClicked();
	UFUNCTION()
	void OnBackButtonReleased();
	void SetCheckBoxState(bool bIsChecked, UCheckBox* CheckBox, int ActiveWidgetIndex);
	void SelectTabByIndex(int32 NewTabIndex);
	void RefreshAllItemsOwnership() const;
	void GatherSelectableItems(UWidget* RootWidget, TArray<USelectableItem*>& OutItems, bool bOnlySelectable) const;
	int32 FindActiveCarItemIndex() const;
	void OnSelectableItemInteracted(USelectableItem* InteractedItem);
	void OnSelectableItemUnhovered(USelectableItem* UnhoveredItem);
	void OnSelectableItemClicked(USelectableItem* ClickedItem);
	void OnInputModeChanged(ETrivialKartInputMode NewInputMode);
	bool IsNavigationVisualActive() const;
	void RestoreViewportFocus() const;

	UFUNCTION()
	void OnKartBoxStateChanged(bool bIsChecked);
	UFUNCTION()
	void OnLevelBoxStateChange(bool bIsChecked);
};
