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
#include "Actors/TrivialKartHUD.h"
#include "Blueprint/UserWidget.h"
#include "Controllers/TrivialKartPlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/SlateTypes.h"
#include "TrivialKartTabbedModalWidget.generated.h"

class UBorder;
class UButton;
class UCheckBox;
class UScrollBox;
class USelectableItem;
class UWidget;
class UWidgetSwitcher;

/**
 * Shared base class for TrivialKart modal submenus (Store, Garage, PGS) providing
 * unified controller, keyboard, mouse, and touch tab/item/close-button navigation.
 */
UCLASS(Abstract)
class TRIVIALKART_UNREAL_API UTrivialKartTabbedModalWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY()
	TArray<UCheckBox*> CheckBoxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> TabHighlightBorders;

	TArray<TArray<TWeakObjectPtr<USelectableItem>>> CachedItemsByTab;
	TArray<TWeakObjectPtr<USelectableItem>> CachedAllItems;
	TWeakObjectPtr<USelectableItem> GamepadPressedItem;
	TWeakObjectPtr<USelectableItem> HoveredItem;

	int32 ActiveTabIndex = 0;
	int32 FocusedItemIndex = -1;
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

protected:
	virtual EWidgetType GetModalWidgetType() const { return EWidgetType::None; }
	virtual void RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes) {}
	virtual void BindTabCheckBoxEvents() {}
	virtual void UnbindTabCheckBoxEvents() {}
	virtual void InitializeModalContent() {}
	virtual int32 GetDefaultItemIndexForTab(int32 TabIndex, const TArray<USelectableItem*>& ActiveItems) const { return 0; }
	virtual UScrollBox* GetActiveReadOnlyScrollBox() const { return nullptr; }

	void SetCheckBoxState(bool bIsChecked, UCheckBox* CheckBox, int32 ActiveWidgetIndex);
	void SelectTabByIndex(int32 NewTabIndex);
	void GatherSelectableItems(UWidget* RootWidget, TArray<USelectableItem*>& OutItems, bool bOnlySelectable) const;
	void GetActiveSelectableItems(TArray<USelectableItem*>& OutItems) const;
	bool IsNavigationVisualActive() const;
	void RestoreViewportFocus() const;

private:
	UFUNCTION()
	void OnBackButtonClicked();

	UFUNCTION()
	void OnBackButtonReleased();

	void ConfigureModalHitTesting();
	void CacheAndBindSelectableItems();
	void UnbindSelectableItems();
	void RefreshAllItemsOwnership() const;
	void OnSelectableItemInteracted(USelectableItem* InteractedItem);
	void OnSelectableItemUnhovered(USelectableItem* UnhoveredItem);
	void OnSelectableItemClicked(USelectableItem* ClickedItem);
	void OnInputModeChanged(ETrivialKartInputMode NewInputMode);
};
