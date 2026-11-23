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

#include "Widgets/TrivialKartTabbedModalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "Widgets/SelectableItem.h"
#include "Widgets/TrivialKartUIStyle.h"

namespace
{
	UBorder* EnsureModalTabHighlightBorder(UWidgetTree* WidgetTree, UCheckBox* CB)
	{
		UOverlay* ParentOverlay = CB ? Cast<UOverlay>(CB->GetParent()) : nullptr;
		if (!WidgetTree || !ParentOverlay)
		{
			return nullptr;
		}

		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Border->SetVisibility(ESlateVisibility::Collapsed);
		Border->SetPadding(FMargin(0.0f));
		if (UOverlaySlot* OverlaySlot = ParentOverlay->AddChildToOverlay(Border))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
		return Border;
	}
}

void UTrivialKartTabbedModalWidget::ConfigureModalHitTesting()
{
	if (WidgetTree)
	{
		TrivialKartUIStyle::NormalizeWidgetTreeHitTesting(WidgetTree, WidgetTree->RootWidget);
	}
}

void UTrivialKartTabbedModalWidget::CacheAndBindSelectableItems()
{
	UnbindSelectableItems();
	if (!ContentSwitcher)
	{
		return;
	}

	const int32 NumTabs = ContentSwitcher->GetChildrenCount();
	CachedItemsByTab.SetNum(NumTabs);
	for (int32 TabIdx = 0; TabIdx < NumTabs; ++TabIdx)
	{
		TArray<USelectableItem*> TabItems;
		GatherSelectableItems(ContentSwitcher->GetChildAt(TabIdx), TabItems, false);
		for (USelectableItem* Item : TabItems)
		{
			if (Item)
			{
				Item->OnItemInteracted.BindUObject(this, &UTrivialKartTabbedModalWidget::OnSelectableItemInteracted);
				Item->OnItemUnhovered.BindUObject(this, &UTrivialKartTabbedModalWidget::OnSelectableItemUnhovered);
				Item->OnItemClicked.BindUObject(this, &UTrivialKartTabbedModalWidget::OnSelectableItemClicked);
				CachedItemsByTab[TabIdx].Add(Item);
				CachedAllItems.Add(Item);
			}
		}
	}
}

void UTrivialKartTabbedModalWidget::UnbindSelectableItems()
{
	for (const TWeakObjectPtr<USelectableItem>& WeakItem : CachedAllItems)
	{
		if (USelectableItem* Item = WeakItem.Get())
		{
			Item->OnItemInteracted.Unbind();
			Item->OnItemUnhovered.Unbind();
			Item->OnItemClicked.Unbind();
		}
	}
	CachedItemsByTab.Empty();
	CachedAllItems.Empty();
}

void UTrivialKartTabbedModalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Keep the modal root hit-testable so empty modal space blocks clicks from piercing into FSceneViewport
	SetVisibility(ESlateVisibility::Visible);
	ConfigureModalHitTesting();

	if (BackButton)
	{
		if (!bBackStyleCached)
		{
			OriginalBackButtonStyle = BackButton->GetStyle();
			OriginalBackButtonBgColor = BackButton->GetBackgroundColor();
			bBackStyleCached = true;
		}
		BackButton->OnClicked.AddUniqueDynamic(this, &UTrivialKartTabbedModalWidget::OnBackButtonClicked);
		BackButton->OnReleased.AddUniqueDynamic(this, &UTrivialKartTabbedModalWidget::OnBackButtonReleased);
	}

	CheckBoxes.Empty();
	RegisterTabCheckBoxes(CheckBoxes);

	if (TabHighlightBorders.Num() != CheckBoxes.Num())
	{
		TabHighlightBorders.Empty();
		for (UCheckBox* CB : CheckBoxes)
		{
			TabHighlightBorders.Add(EnsureModalTabHighlightBorder(WidgetTree, CB));
		}
	}

	BindTabCheckBoxEvents();
	InitializeModalContent();
	CacheAndBindSelectableItems();

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->OnInputModeUpdated.RemoveAll(this);
		PC->OnInputModeUpdated.AddUObject(this, &UTrivialKartTabbedModalWidget::OnInputModeChanged);
	}

	GamepadPressedItem.Reset();
	HoveredItem.Reset();
	bBackButtonPressed = false;
	RefreshAllItemsOwnership();
	ActiveTabIndex = ContentSwitcher ? ContentSwitcher->GetActiveWidgetIndex() : 0;

	TArray<USelectableItem*> ActiveItems;
	GetActiveSelectableItems(ActiveItems);
	FocusedItemIndex = (ActiveItems.Num() > 0)
		? GetDefaultItemIndexForTab(ActiveTabIndex, ActiveItems)
		: -1;
	RefreshGamepadFocus(IsNavigationVisualActive());
}

void UTrivialKartTabbedModalWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UTrivialKartTabbedModalWidget::OnBackButtonClicked);
		BackButton->OnReleased.RemoveDynamic(this, &UTrivialKartTabbedModalWidget::OnBackButtonReleased);
	}

	UnbindTabCheckBoxEvents();
	UnbindSelectableItems();

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->OnInputModeUpdated.RemoveAll(this);
	}

	CheckBoxes.Empty();
	Super::NativeDestruct();
}

bool UTrivialKartTabbedModalWidget::IsNavigationVisualActive() const
{
	if (const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		return PC->IsKeyNavigationActive();
	}
	return false;
}

void UTrivialKartTabbedModalWidget::RestoreViewportFocus() const
{
	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->RestoreFocusToGameViewport();
	}
}

void UTrivialKartTabbedModalWidget::OnInputModeChanged(ETrivialKartInputMode NewInputMode)
{
	if (!IsNavigationVisualActive())
	{
		CancelPendingPress();
	}
	RefreshGamepadFocus(false);
}

void UTrivialKartTabbedModalWidget::OnBackButtonReleased()
{
	RestoreViewportFocus();
}

void UTrivialKartTabbedModalWidget::OnBackButtonClicked()
{
	if (TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr<ATrivialKartHUD> CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD());
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::PlayBoard);
			const EWidgetType ModalType = GetModalWidgetType();
			if (ModalType != EWidgetType::None)
			{
				CurrentHUD->RemoveWidgetFromScreen(ModalType);
			}
		}
	}
}

void UTrivialKartTabbedModalWidget::SetCheckBoxState(bool bIsChecked, UCheckBox* CheckBox, int32 ActiveWidgetIndex)
{
	if (CheckBox == nullptr)
	{
		return;
	}
	CancelPendingPress();
	HoveredItem.Reset();
	FocusedItemIndex = -1;
	if (bIsChecked)
	{
		ActiveTabIndex = ActiveWidgetIndex;
		if (ContentSwitcher)
		{
			ContentSwitcher->SetActiveWidgetIndex(ActiveWidgetIndex);
		}
		for (UCheckBox* CurrentCheckBox : CheckBoxes)
		{
			if (CurrentCheckBox && CurrentCheckBox != CheckBox)
			{
				CurrentCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
			}
		}
	}
	else
	{
		CheckBox->SetCheckedState(ECheckBoxState::Checked);
	}
	RefreshAllItemsOwnership();
	RefreshGamepadFocus(false);
	RestoreViewportFocus();
}

void UTrivialKartTabbedModalWidget::SelectTabByIndex(int32 NewTabIndex)
{
	if (CheckBoxes.Num() == 0)
	{
		return;
	}
	ActiveTabIndex = (NewTabIndex + CheckBoxes.Num()) % CheckBoxes.Num();
	if (UCheckBox* TargetCheckBox = CheckBoxes[ActiveTabIndex])
	{
		TargetCheckBox->SetCheckedState(ECheckBoxState::Checked);
		SetCheckBoxState(true, TargetCheckBox, ActiveTabIndex);
	}
}

void UTrivialKartTabbedModalWidget::RefreshAllItemsOwnership() const
{
	for (const TWeakObjectPtr<USelectableItem>& WeakItem : CachedAllItems)
	{
		if (USelectableItem* Item = WeakItem.Get())
		{
			Item->RefreshOwnershipState();
		}
	}
}

void UTrivialKartTabbedModalWidget::GetActiveSelectableItems(TArray<USelectableItem*>& OutItems) const
{
	OutItems.Reset();
	if (!ContentSwitcher)
	{
		return;
	}
	const int32 TabIdx = ContentSwitcher->GetActiveWidgetIndex();
	if (!CachedItemsByTab.IsValidIndex(TabIdx))
	{
		return;
	}
	for (const TWeakObjectPtr<USelectableItem>& WeakItem : CachedItemsByTab[TabIdx])
	{
		if (USelectableItem* Item = WeakItem.Get(); Item && Item->IsItemSelectable())
		{
			OutItems.Add(Item);
		}
	}
}

void UTrivialKartTabbedModalWidget::GatherSelectableItems(UWidget* RootWidget, TArray<USelectableItem*>& OutItems, bool bOnlySelectable) const
{
	if (!RootWidget)
	{
		return;
	}
	if (USelectableItem* Item = Cast<USelectableItem>(RootWidget))
	{
		if (!bOnlySelectable || Item->IsItemSelectable())
		{
			OutItems.Add(Item);
		}
		return;
	}
	if (bOnlySelectable && !RootWidget->IsVisible())
	{
		return;
	}
	if (UPanelWidget* Panel = Cast<UPanelWidget>(RootWidget))
	{
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			GatherSelectableItems(Panel->GetChildAt(i), OutItems, bOnlySelectable);
		}
	}
}

void UTrivialKartTabbedModalWidget::OnSelectableItemInteracted(USelectableItem* InteractedItem)
{
	HoveredItem = InteractedItem;
	TArray<USelectableItem*> ActiveItems;
	GetActiveSelectableItems(ActiveItems);
	const int32 FoundIndex = ActiveItems.IndexOfByKey(InteractedItem);
	if (FoundIndex != INDEX_NONE)
	{
		FocusedItemIndex = FoundIndex;
	}
	RefreshGamepadFocus(false);
}

void UTrivialKartTabbedModalWidget::OnSelectableItemUnhovered(USelectableItem* UnhoveredItem)
{
	if (HoveredItem.Get() == UnhoveredItem)
	{
		HoveredItem.Reset();
		if (!IsNavigationVisualActive())
		{
			RefreshGamepadFocus(false);
		}
	}
}

void UTrivialKartTabbedModalWidget::OnSelectableItemClicked(USelectableItem* ClickedItem)
{
	RefreshAllItemsOwnership();
	if (ClickedItem && !ClickedItem->IsItemSelectable())
	{
		HoveredItem.Reset();
		RefreshGamepadFocus(false);
	}
	else
	{
		OnSelectableItemInteracted(ClickedItem);
	}
}

void UTrivialKartTabbedModalWidget::RefreshOwnershipAndFocus(bool bScrollIntoView)
{
	RefreshAllItemsOwnership();
	RefreshGamepadFocus(bScrollIntoView);
}

void UTrivialKartTabbedModalWidget::CancelPendingPress()
{
	if (USelectableItem* PressedItem = GamepadPressedItem.Get())
	{
		PressedItem->SetButtonPressed(false);
	}
	GamepadPressedItem.Reset();
	if (bBackButtonPressed)
	{
		bBackButtonPressed = false;
		RefreshGamepadFocus(false);
	}
}

void UTrivialKartTabbedModalWidget::RefreshGamepadFocus(bool bScrollIntoView)
{
	const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer());
	const bool bShowNavVisual = IsNavigationVisualActive();
	const bool bAllowMouseHoverVisual = (!bShowNavVisual && PC && PC->GetActiveInputMode() == ETrivialKartInputMode::KeyboardMouse);

	TArray<USelectableItem*> ActiveItems;
	GetActiveSelectableItems(ActiveItems);

	USelectableItem* FocusedItem = nullptr;
	if (ActiveItems.Num() == 0 && FocusedItemIndex >= 0)
	{
		FocusedItemIndex = -1;
	}
	else if (FocusedItemIndex >= 0)
	{
		FocusedItemIndex = FMath::Clamp(FocusedItemIndex, 0, ActiveItems.Num() - 1);
		FocusedItem = ActiveItems[FocusedItemIndex];
	}

	const int32 CurrentTabIdx = ContentSwitcher ? ContentSwitcher->GetActiveWidgetIndex() : INDEX_NONE;
	for (int32 TabIdx = 0; TabIdx < CachedItemsByTab.Num(); ++TabIdx)
	{
		const bool bIsInActiveTab = (TabIdx == CurrentTabIdx);
		for (const TWeakObjectPtr<USelectableItem>& WeakItem : CachedItemsByTab[TabIdx])
		{
			if (USelectableItem* Item = WeakItem.Get())
			{
				if (!bIsInActiveTab || (Item != GamepadPressedItem.Get() && !Item->IsButtonHovered()))
				{
					Item->SetButtonPressed(false);
				}
				const bool bIsHoveredByMouse = (bAllowMouseHoverVisual && bIsInActiveTab && HoveredItem.Get() == Item && Item->IsButtonHovered());
				const bool bIsFocusedTarget = bShowNavVisual ? (Item == FocusedItem) : bIsHoveredByMouse;
				Item->SetGamepadFocused(bIsFocusedTarget, bShowNavVisual || bIsHoveredByMouse);
			}
		}
	}

	if (FocusedItem && bScrollIntoView && bShowNavVisual)
	{
		for (UPanelWidget* Parent = FocusedItem->GetParent(); Parent != nullptr; Parent = Parent->GetParent())
		{
			if (UScrollBox* ScrollBox = Cast<UScrollBox>(Parent))
			{
				ScrollBox->ScrollWidgetIntoView(FocusedItem, true, EDescendantScrollDestination::IntoView);
				break;
			}
		}
	}

	if (BackButton && bBackStyleCached)
	{
		const bool bBackFocused = bShowNavVisual && (FocusedItemIndex == -2);
		BackButton->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (bBackFocused)
		{
			const bool bPressed = bBackButtonPressed || BackButton->IsPressed();
			const FSlateBrush HighlightBrush = TrivialKartUIStyle::MakeButtonFocusBrush(bPressed, 10.0f);

			FButtonStyle FocusedStyle = OriginalBackButtonStyle;
			FocusedStyle.SetNormal(HighlightBrush);
			FocusedStyle.SetHovered(HighlightBrush);
			FocusedStyle.SetPressed(HighlightBrush);
			BackButton->SetStyle(FocusedStyle);
			BackButton->SetBackgroundColor(FLinearColor::White);
			BackButton->SetRenderScale(FVector2D(bPressed ? 0.88f : 1.12f, bPressed ? 0.88f : 1.12f));
		}
		else
		{
			BackButton->SetStyle(OriginalBackButtonStyle);
			BackButton->SetBackgroundColor(OriginalBackButtonBgColor);
			BackButton->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	const bool bTabBarFocused = bShowNavVisual && (FocusedItemIndex == -1);
	for (int32 i = 0; i < CheckBoxes.Num(); ++i)
	{
		UCheckBox* CB = CheckBoxes[i];
		UBorder* HighlightBorder = TabHighlightBorders.IsValidIndex(i) ? TabHighlightBorders[i].Get() : nullptr;
		if (!CB)
		{
			continue;
		}
		UWidget* TabScaleTarget = CB->GetParent()
			? static_cast<UWidget*>(CB->GetParent())
			: static_cast<UWidget*>(CB);
		TabScaleTarget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (i == ActiveTabIndex)
		{
			if (HighlightBorder)
			{
				const FSlateBrush OverlayBrush = TrivialKartUIStyle::MakeRoundedHighlightBrush(
					8.0f,
					bTabBarFocused ? 6.0f : 0.0f,
					FLinearColor(1.0f, 0.45f, 0.0f, 0.48f),
					bTabBarFocused ? FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) : FLinearColor::Transparent,
					CB->GetWidgetStyle().CheckedImage.ImageSize);
				HighlightBorder->SetBrush(OverlayBrush);
				HighlightBorder->SetBrushColor(FLinearColor::White);
				HighlightBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			const float TargetTabScale = bTabBarFocused ? 1.15f : 1.0f;
			TabScaleTarget->SetRenderScale(FVector2D(TargetTabScale, TargetTabScale));
		}
		else
		{
			if (HighlightBorder)
			{
				HighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
			}
			TabScaleTarget->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}
}

bool UTrivialKartTabbedModalWidget::HandleGamepadKey(const FKey& Key, bool bIsPressed)
{
	if (!bIsPressed)
	{
		if (Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			if (bBackButtonPressed)
			{
				bBackButtonPressed = false;
				RefreshGamepadFocus(false);
				if (FocusedItemIndex == -2)
				{
					OnBackButtonClicked();
				}
				return true;
			}

			USelectableItem* PreviouslyPressed = GamepadPressedItem.Get();
			GamepadPressedItem.Reset();

			TArray<USelectableItem*> ActiveItems;
			GetActiveSelectableItems(ActiveItems);
			if (PreviouslyPressed && FocusedItemIndex >= 0 && FocusedItemIndex < ActiveItems.Num() &&
				ActiveItems[FocusedItemIndex] == PreviouslyPressed)
			{
				PreviouslyPressed->SetButtonPressed(false);
				PreviouslyPressed->OnSelectionButtonClicked();
			}
			else if (PreviouslyPressed)
			{
				PreviouslyPressed->SetButtonPressed(false);
			}
			return true;
		}
		return false;
	}

	if (Key == EKeys::Gamepad_FaceButton_Right)
	{
		CancelPendingPress();
		OnBackButtonClicked();
		return true;
	}
	if (Key == EKeys::Gamepad_LeftShoulder)
	{
		SelectTabByIndex(ActiveTabIndex - 1);
		return true;
	}
	if (Key == EKeys::Gamepad_RightShoulder)
	{
		SelectTabByIndex(ActiveTabIndex + 1);
		return true;
	}

	TArray<USelectableItem*> ActiveItems;
	GetActiveSelectableItems(ActiveItems);

	// If opened via touch/mouse with navigation focus hidden, reveal focus on the first navigation/confirm press
	if (!IsNavigationVisualActive())
	{
		if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right ||
			Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down ||
			Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			CancelPendingPress();
			if (Key == EKeys::Gamepad_DPad_Up || ActiveItems.Num() == 0)
			{
				FocusedItemIndex = -1;
			}
			else if (FocusedItemIndex < 0)
			{
				FocusedItemIndex = GetDefaultItemIndexForTab(ActiveTabIndex, ActiveItems);
			}
			if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
			{
				PC->SetKeyNavigationActive(true);
			}
			RefreshGamepadFocus(true);
			return true;
		}
	}

	if (Key == EKeys::Gamepad_DPad_Left)
	{
		CancelPendingPress();
		if (FocusedItemIndex == -2)
		{
			RefreshGamepadFocus(false);
		}
		else if (FocusedItemIndex == -1)
		{
			SelectTabByIndex(ActiveTabIndex - 1);
		}
		else if (ActiveItems.Num() > 1)
		{
			FocusedItemIndex = (FocusedItemIndex + ActiveItems.Num() - 1) % ActiveItems.Num();
			RefreshGamepadFocus();
		}
		else
		{
			RefreshGamepadFocus();
		}
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Right)
	{
		CancelPendingPress();
		if (FocusedItemIndex == -2)
		{
			RefreshGamepadFocus(false);
		}
		else if (FocusedItemIndex == -1)
		{
			SelectTabByIndex(ActiveTabIndex + 1);
		}
		else if (ActiveItems.Num() > 1)
		{
			FocusedItemIndex = (FocusedItemIndex + 1) % ActiveItems.Num();
			RefreshGamepadFocus();
		}
		else
		{
			RefreshGamepadFocus();
		}
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Up)
	{
		CancelPendingPress();
		if (FocusedItemIndex >= 0)
		{
			FocusedItemIndex = -1;
			RefreshGamepadFocus(false);
		}
		else if (FocusedItemIndex == -1)
		{
			UScrollBox* TargetScroll = GetActiveReadOnlyScrollBox();
			if (TargetScroll && TargetScroll->GetScrollOffset() > KINDA_SMALL_NUMBER)
			{
				const float MaxOffset = FMath::Max(0.0f, TargetScroll->GetScrollOffsetOfEnd());
				TargetScroll->SetScrollOffset(FMath::Clamp(TargetScroll->GetScrollOffset() - 60.0f, 0.0f, MaxOffset));
			}
			else if (BackButton)
			{
				FocusedItemIndex = -2;
				RefreshGamepadFocus(false);
			}
		}
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Down)
	{
		CancelPendingPress();
		if (FocusedItemIndex == -2)
		{
			FocusedItemIndex = -1;
			RefreshGamepadFocus(false);
		}
		else if (FocusedItemIndex == -1)
		{
			if (ActiveItems.Num() > 0)
			{
				FocusedItemIndex = GetDefaultItemIndexForTab(ActiveTabIndex, ActiveItems);
				RefreshGamepadFocus(true);
			}
			else if (UScrollBox* TargetScroll = GetActiveReadOnlyScrollBox())
			{
				const float MaxOffset = FMath::Max(0.0f, TargetScroll->GetScrollOffsetOfEnd());
				TargetScroll->SetScrollOffset(FMath::Clamp(TargetScroll->GetScrollOffset() + 60.0f, 0.0f, MaxOffset));
			}
		}
		return true;
	}
	if (Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (FocusedItemIndex == -2 && BackButton)
		{
			bBackButtonPressed = true;
			RefreshGamepadFocus(false);
		}
		else if (FocusedItemIndex >= 0 && FocusedItemIndex < ActiveItems.Num())
		{
			GamepadPressedItem = ActiveItems[FocusedItemIndex];
			GamepadPressedItem->SetButtonPressed(true);
		}
		else if (FocusedItemIndex == -1 && ActiveItems.Num() > 0)
		{
			GamepadPressedItem.Reset();
			FocusedItemIndex = GetDefaultItemIndexForTab(ActiveTabIndex, ActiveItems);
			RefreshGamepadFocus(true);
		}
		return true;
	}

	return false;
}
