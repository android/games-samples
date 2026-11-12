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

#include "Widgets/SelectableItem.h"

#include "Actors/KartPawn.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Controllers/TrivialKartPlayerController.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Infos/TrivialKartPlayerState.h"
#include "Objects/TrivialKartSaveGame.h"

void USelectableItem::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (SelectionButton)
	{
		SelectionButton->SetStyle(SelectionButtonStyle);
	}
}

void USelectableItem::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectionButton)
	{
		SelectionButton->OnClicked.AddUniqueDynamic(this, &USelectableItem::OnSelectionButtonClicked);
		SelectionButton->OnHovered.AddUniqueDynamic(this, &USelectableItem::OnSelectionButtonHovered);
		SelectionButton->OnUnhovered.AddUniqueDynamic(this, &USelectableItem::OnSelectionButtonUnhovered);
		SelectionButton->OnPressed.AddUniqueDynamic(this, &USelectableItem::OnSelectionButtonPressed);
		SelectionButton->OnReleased.AddUniqueDynamic(this, &USelectableItem::OnSelectionButtonReleased);
	}

	EnsureOverlayHitTestAndBorder();
	RefreshOwnershipState();
}

void USelectableItem::RefreshOwnershipState()
{
	if (CarType.IsEmpty())
	{
		return;
	}
	if (const TWeakObjectPtr<UTrivialKartGameInstance> Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
		Instance.IsValid())
	{
		if (TWeakObjectPtr<UTrivialKartSaveGame> Save = Instance->LoadGame(); Save.IsValid())
		{
			const bool bIsOwned = Save->CarsOwned.Contains(CarType);
			switch (SelectionOperationType)
			{
			case ESelectableOperationType::CoinPurchase:
				SetIsEnabled(!bIsOwned);
				break;
			case ESelectableOperationType::ObjectSelection:
				SetVisibility(bIsOwned ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
				break;
			default:
				break;
			}
		}
	}
}

void USelectableItem::NativeDestruct()
{
	if (SelectionButton)
	{
		SelectionButton->OnClicked.RemoveDynamic(this, &USelectableItem::OnSelectionButtonClicked);
		SelectionButton->OnHovered.RemoveDynamic(this, &USelectableItem::OnSelectionButtonHovered);
		SelectionButton->OnUnhovered.RemoveDynamic(this, &USelectableItem::OnSelectionButtonUnhovered);
		SelectionButton->OnPressed.RemoveDynamic(this, &USelectableItem::OnSelectionButtonPressed);
		SelectionButton->OnReleased.RemoveDynamic(this, &USelectableItem::OnSelectionButtonReleased);
	}
	Super::NativeDestruct();
}

void USelectableItem::EnsureOverlayHitTestAndBorder()
{
	if (UOverlay* CardOverlay = Cast<UOverlay>(GetParent()))
	{
		if (CardOverlay->GetVisibility() == ESlateVisibility::Visible)
		{
			CardOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		for (int32 i = 0; i < CardOverlay->GetChildrenCount(); ++i)
		{
			UWidget* Sibling = CardOverlay->GetChildAt(i);
			if (Sibling && Sibling != this && Sibling != FocusHighlightBorder && Sibling->IsVisible())
			{
				Sibling->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}

	if (!FocusHighlightBorder && WidgetTree && SelectionButton)
	{
		if (UOverlay* ButtonOverlay = Cast<UOverlay>(SelectionButton->GetParent()))
		{
			FocusHighlightBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			if (FocusHighlightBorder)
			{
				FocusHighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
				FocusHighlightBorder->SetPadding(FMargin(0.0f));
				if (UOverlaySlot* OverlaySlot = ButtonOverlay->AddChildToOverlay(FocusHighlightBorder))
				{
					OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
					OverlaySlot->SetVerticalAlignment(VAlign_Fill);
				}
			}
		}
	}
}

UWidget* USelectableItem::GetVisualScaleTarget() const
{
	if (UOverlay* CardOverlay = Cast<UOverlay>(GetParent()))
	{
		return CardOverlay;
	}
	return const_cast<USelectableItem*>(this);
}

void USelectableItem::OnSelectionButtonHovered()
{
	if (IsItemSelectable() && (!SelectionButton || !SelectionButton->IsPressed()))
	{
		OnItemInteracted.ExecuteIfBound(this);
	}
}

void USelectableItem::OnSelectionButtonUnhovered()
{
	OnItemUnhovered.ExecuteIfBound(this);
}

void USelectableItem::OnSelectionButtonPressed()
{
	if (IsItemSelectable())
	{
		SetButtonPressed(true);
		OnItemInteracted.ExecuteIfBound(this);
	}
}

void USelectableItem::OnSelectionButtonReleased()
{
	SetButtonPressed(false);
	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->RestoreFocusToGameViewport();
	}
}

void USelectableItem::SetButtonPressed(bool bInPressed)
{
	bIsButtonPressed = bInPressed;
	UpdateHighlightVisuals();
}

void USelectableItem::OnSelectionButtonClicked()
{
	SetButtonPressed(false);

	switch (SelectionOperationType)
	{
	case ESelectableOperationType::InAppPurchase:
		{
#if UE_EDITOR
			switch (PurchaseObject)
			{
			case EPurchaseObject::Coin:
				if (const TWeakObjectPtr<ATrivialKartPlayerState> PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); 
					PlayerState.IsValid())
				{
					PlayerState->AddCoins(PurchaseQuantity);
				}
				break;
			default:
				break;
			}
#endif
			if (const TWeakObjectPtr<UTrivialKartGameInstance> Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
				Instance.IsValid())
			{
				Instance->StartPurchasing(PurchaseID, PurchaseQuantity, bIsConsumable);
			}
		}
		break;
	case ESelectableOperationType::CoinPurchase:
		{
			if (const TWeakObjectPtr<ATrivialKartPlayerState> PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState());
				PlayerState.IsValid())
			{
				if (PlayerState->GetCoinCount() >= CoinQuantity)
				{
					switch (PurchaseObject)
					{
					case EPurchaseObject::Car:
						if (!CarType.IsEmpty())
						{
							PlayerState->AddCarToInventory(CarType);
							SetIsEnabled(false);
							PlayerState->ConsumeCoins(CoinQuantity);
						}
						break;
					case EPurchaseObject::Fuel:
						PlayerState->Refuel();
						PlayerState->ConsumeCoins(CoinQuantity);
						break;
					default:
						break;
					}
				}
			}
		}
		break;
	case ESelectableOperationType::ObjectSelection:
		{
			if (IsValid(GetOwningPlayer()))
			{
				if (TWeakObjectPtr<AKartPawn> Pawn = Cast<AKartPawn>(GetOwningPlayer()->GetPawn()); 
					Pawn.IsValid())
				{
					Pawn->SwitchCar(CarType);
					if (const TWeakObjectPtr<UTrivialKartGameInstance> Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
						Instance.IsValid())
					{
						if (TWeakObjectPtr<UTrivialKartSaveGame> Save = Instance->LoadGame(); Save.IsValid())
						{
							Save->ActiveCarType = CarType;
							Instance->SaveGame(Save.Get());
						}
					}
				}
			}
		}
		break;
	default:
		break;
	}

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->RestoreFocusToGameViewport();
	}

	OnItemClicked.ExecuteIfBound(this);
}

void USelectableItem::SetGamepadFocused(bool bInFocused, bool bInShowVisual)
{
	bIsGamepadFocused = bInFocused;
	bShowFocusVisual = bInShowVisual;
	UpdateHighlightVisuals();
}

void USelectableItem::UpdateHighlightVisuals()
{
	if (!SelectionButton)
	{
		return;
	}

	EnsureOverlayHitTestAndBorder();

	UWidget* ScaleTarget = GetVisualScaleTarget();
	ScaleTarget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (ScaleTarget != this)
	{
		SetRenderScale(FVector2D(1.0f, 1.0f));
	}

	const bool bShowHighlight = (bIsGamepadFocused && bShowFocusVisual && IsItemSelectable()) || bIsButtonPressed;
	if (bShowHighlight)
	{
		if (FocusHighlightBorder)
		{
			FSlateBrush OverlayBrush;
			OverlayBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			OverlayBrush.TintColor = FSlateColor(bIsButtonPressed
				? FLinearColor(1.0f, 0.30f, 0.0f, 0.72f)
				: FLinearColor(1.0f, 0.45f, 0.0f, 0.48f));
			OverlayBrush.OutlineSettings.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
			OverlayBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			OverlayBrush.OutlineSettings.Color = FSlateColor(bIsButtonPressed
				? FLinearColor(1.0f, 1.0f, 0.2f, 1.0f)
				: FLinearColor(1.0f, 0.9f, 0.0f, 1.0f));
			OverlayBrush.OutlineSettings.Width = bIsButtonPressed ? 10.0f : 8.0f;
			FocusHighlightBorder->SetBrush(OverlayBrush);
			FocusHighlightBorder->SetBrushColor(FLinearColor::White);
			FocusHighlightBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		const float TargetScale = bIsButtonPressed ? 0.88f : 1.08f;
		ScaleTarget->SetRenderScale(FVector2D(TargetScale, TargetScale));
	}
	else
	{
		if (FocusHighlightBorder)
		{
			FocusHighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
		ScaleTarget->SetRenderScale(FVector2D(1.0f, 1.0f));
	}
}

bool USelectableItem::IsButtonHovered() const
{
	return SelectionButton && SelectionButton->IsHovered();
}

bool USelectableItem::IsItemSelectable() const
{
	return IsVisible() && GetIsEnabled() && SelectionButton && SelectionButton->GetIsEnabled();
}
