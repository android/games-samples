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
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Widgets/SelectableItem.h"

static UBorder* EnsureTabHighlightBorder(UWidgetTree* WidgetTree, UCheckBox* CB)
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

void UStoreWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Keep the modal root hit-testable so empty modal space blocks clicks from piercing into FSceneViewport
	SetVisibility(ESlateVisibility::Visible);

	if (WidgetTree)
	{
		UWidget* TreeRoot = WidgetTree->RootWidget;
		WidgetTree->ForEachWidget([TreeRoot](UWidget* Widget)
		{
			if (!Widget || Widget == TreeRoot || Widget->GetVisibility() != ESlateVisibility::Visible)
			{
				return;
			}
			if (Cast<UImage>(Widget) || Cast<UTextBlock>(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else if (Cast<UOverlay>(Widget) || Cast<UHorizontalBox>(Widget) || Cast<UVerticalBox>(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		});
	}
	
	if (BackButton)
	{
		if (!bBackStyleCached)
		{
			OriginalBackButtonStyle = BackButton->GetStyle();
			OriginalBackButtonBgColor = BackButton->GetBackgroundColor();
			bBackStyleCached = true;
		}
		BackButton->OnClicked.AddUniqueDynamic(this, &UStoreWidget::OnBackButtonClicked);
		BackButton->OnReleased.AddUniqueDynamic(this, &UStoreWidget::OnBackButtonReleased);
	}

	CheckBoxes.Empty();
	CheckBoxes.Add(FuelCheckBox);
	CheckBoxes.Add(CoinCheckBox);
	CheckBoxes.Add(CarCheckBox);
	CheckBoxes.Add(SubscriptionCheckBox);

	if (TabHighlightBorders.Num() != CheckBoxes.Num())
	{
		TabHighlightBorders.Empty();
		for (UCheckBox* CB : CheckBoxes)
		{
			TabHighlightBorders.Add(EnsureTabHighlightBorder(WidgetTree, CB));
		}
	}

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

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->OnInputModeUpdated.RemoveAll(this);
		PC->OnInputModeUpdated.AddUObject(this, &UStoreWidget::OnInputModeChanged);
	}

	GamepadPressedItem.Reset();
	HoveredItem.Reset();
	bBackButtonPressed = false;
	RefreshAllItemsOwnership();
	ActiveTabIndex = ContentSwitcher ? ContentSwitcher->GetActiveWidgetIndex() : 0;
	FocusedItemIndex = 0;
	RefreshGamepadFocus(false);
}

void UStoreWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UStoreWidget::OnBackButtonClicked);
		BackButton->OnReleased.RemoveDynamic(this, &UStoreWidget::OnBackButtonReleased);
	}
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

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->OnInputModeUpdated.RemoveAll(this);
	}

	CheckBoxes.Empty();
	Super::NativeDestruct();
}

bool UStoreWidget::IsNavigationVisualActive() const
{
	if (const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		return PC->IsKeyNavigationActive();
	}
	return false;
}

void UStoreWidget::RestoreViewportFocus() const
{
	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->RestoreFocusToGameViewport();
	}
}

void UStoreWidget::OnInputModeChanged(ETrivialKartInputMode NewInputMode)
{
	if (!IsNavigationVisualActive())
	{
		CancelPendingPress();
	}
	RefreshGamepadFocus(false);
}

void UStoreWidget::OnBackButtonReleased()
{
	RestoreViewportFocus();
}

void UStoreWidget::OnBackButtonClicked()
{
	if (TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr<ATrivialKartHUD> CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
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
		for (auto CurrentCheckBox : CheckBoxes)
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

void UStoreWidget::SelectTabByIndex(int32 NewTabIndex)
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

void UStoreWidget::RefreshAllItemsOwnership() const
{
	TArray<USelectableItem*> AllItems;
	GatherSelectableItems(ContentSwitcher, AllItems, false);
	for (USelectableItem* Item : AllItems)
	{
		if (Item)
		{
			Item->RefreshOwnershipState();
		}
	}
}

void UStoreWidget::GatherSelectableItems(UWidget* RootWidget, TArray<USelectableItem*>& OutItems, bool bOnlySelectable) const
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

void UStoreWidget::OnSelectableItemInteracted(USelectableItem* InteractedItem)
{
	HoveredItem = InteractedItem;
	TArray<USelectableItem*> ActiveItems;
	if (ContentSwitcher)
	{
		GatherSelectableItems(ContentSwitcher->GetActiveWidget(), ActiveItems, true);
	}
	const int32 FoundIndex = ActiveItems.IndexOfByKey(InteractedItem);
	if (FoundIndex != INDEX_NONE)
	{
		FocusedItemIndex = FoundIndex;
	}
	RefreshGamepadFocus(false);
}

void UStoreWidget::OnSelectableItemUnhovered(USelectableItem* UnhoveredItem)
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

void UStoreWidget::OnSelectableItemClicked(USelectableItem* ClickedItem)
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

void UStoreWidget::RefreshOwnershipAndFocus(bool bScrollIntoView)
{
	RefreshAllItemsOwnership();
	RefreshGamepadFocus(bScrollIntoView);
}

void UStoreWidget::CancelPendingPress()
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

void UStoreWidget::RefreshGamepadFocus(bool bScrollIntoView)
{
	const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer());
	const bool bShowNavVisual = IsNavigationVisualActive();
	const bool bAllowMouseHoverVisual = (!bShowNavVisual && PC && PC->GetActiveInputMode() == ETrivialKartInputMode::KeyboardMouse);

	TArray<USelectableItem*> ActiveItems;
	if (ContentSwitcher)
	{
		GatherSelectableItems(ContentSwitcher->GetActiveWidget(), ActiveItems, true);
	}

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

	TArray<USelectableItem*> AllItems;
	GatherSelectableItems(ContentSwitcher, AllItems, false);
	for (USelectableItem* Item : AllItems)
	{
		if (Item)
		{
			const bool bIsInActiveTab = ActiveItems.Contains(Item);
			Item->OnItemInteracted.BindUObject(this, &UStoreWidget::OnSelectableItemInteracted);
			Item->OnItemUnhovered.BindUObject(this, &UStoreWidget::OnSelectableItemUnhovered);
			Item->OnItemClicked.BindUObject(this, &UStoreWidget::OnSelectableItemClicked);
			if (!bIsInActiveTab || (Item != GamepadPressedItem.Get() && !Item->IsButtonHovered()))
			{
				Item->SetButtonPressed(false);
			}
			const bool bIsHoveredByMouse = (bAllowMouseHoverVisual && bIsInActiveTab && HoveredItem.Get() == Item && Item->IsButtonHovered());
			const bool bIsFocusedTarget = bShowNavVisual ? (Item == FocusedItem) : bIsHoveredByMouse;
			Item->SetGamepadFocused(bIsFocusedTarget, bShowNavVisual || bIsHoveredByMouse);
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
			FSlateBrush HighlightBrush;
			HighlightBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			HighlightBrush.TintColor = FSlateColor(bPressed
				? FLinearColor(1.0f, 0.35f, 0.0f, 0.68f)
				: FLinearColor(1.0f, 0.45f, 0.0f, 0.48f));
			HighlightBrush.OutlineSettings.CornerRadii = FVector4(10.0f, 10.0f, 10.0f, 10.0f);
			HighlightBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			HighlightBrush.OutlineSettings.Color = FSlateColor(FLinearColor(1.0f, 0.9f, 0.0f, 1.0f));
			HighlightBrush.OutlineSettings.Width = 6.0f;

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
				FSlateBrush OverlayBrush;
				OverlayBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
				OverlayBrush.ImageSize = CB->GetWidgetStyle().CheckedImage.ImageSize;
				OverlayBrush.TintColor = FSlateColor(FLinearColor(1.0f, 0.45f, 0.0f, 0.48f));
				OverlayBrush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
				OverlayBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
				OverlayBrush.OutlineSettings.Color = FSlateColor(bTabBarFocused
					? FLinearColor(1.0f, 0.9f, 0.0f, 1.0f)
					: FLinearColor::Transparent);
				OverlayBrush.OutlineSettings.Width = bTabBarFocused ? 6.0f : 0.0f;
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

bool UStoreWidget::HandleGamepadKey(const FKey& Key, bool bIsPressed)
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
			if (ContentSwitcher)
			{
				GatherSelectableItems(ContentSwitcher->GetActiveWidget(), ActiveItems, true);
			}
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
	if (ContentSwitcher)
	{
		GatherSelectableItems(ContentSwitcher->GetActiveWidget(), ActiveItems, true);
	}

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
				FocusedItemIndex = 0;
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
		}
		else if (FocusedItemIndex == -1 && BackButton)
		{
			FocusedItemIndex = -2;
		}
		RefreshGamepadFocus(false);
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Down)
	{
		CancelPendingPress();
		if (FocusedItemIndex == -2)
		{
			FocusedItemIndex = -1;
		}
		else if (FocusedItemIndex == -1 && ActiveItems.Num() > 0)
		{
			FocusedItemIndex = 0;
		}
		RefreshGamepadFocus();
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
			FocusedItemIndex = 0;
			RefreshGamepadFocus();
		}
		return true;
	}

	return false;
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
