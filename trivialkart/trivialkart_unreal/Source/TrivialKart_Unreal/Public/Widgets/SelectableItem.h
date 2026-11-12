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
#include "SelectableItem.generated.h"


UENUM()
enum class ESelectableOperationType : uint8
{
	None,
	InAppPurchase,
	CoinPurchase,
	ObjectSelection
};

UENUM()
enum class EPurchaseObject : uint8
{
	None,
	Fuel,
	Car,
	Coin,
	Subscription
};
class UButtonWidgetStyle;
class UButton;
class UBorder;
class SButton;
/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API USelectableItem : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SelectionButton;
	
	UPROPERTY(EditAnywhere)
	FButtonStyle SelectionButtonStyle;
	
	UPROPERTY(EditAnywhere)
	ESelectableOperationType SelectionOperationType = ESelectableOperationType::None;
	
	UPROPERTY(EditAnywhere)
	EPurchaseObject PurchaseObject = EPurchaseObject::None;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::InAppPurchase", EditConditionHides))
	FString PurchaseID;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::InAppPurchase", EditConditionHides))
	bool bIsConsumable = true;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::InAppPurchase", EditConditionHides))
	int PurchaseQuantity = 1;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::CoinPurchase", EditConditionHides))
	int CoinQuantity = 10;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "PurchaseObject == EPurchaseObject::Car", EditConditionHides))
	FString CarType;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FocusHighlightBorder;

	bool bIsGamepadFocused = false;
	bool bShowFocusVisual = false;
	bool bIsButtonPressed = false;

	void EnsureOverlayHitTestAndBorder();
	UWidget* GetVisualScaleTarget() const;
	void UpdateHighlightVisuals();
	
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UFUNCTION()
	void OnSelectionButtonClicked();

	UFUNCTION()
	void OnSelectionButtonHovered();

	UFUNCTION()
	void OnSelectionButtonUnhovered();

	UFUNCTION()
	void OnSelectionButtonPressed();

	UFUNCTION()
	void OnSelectionButtonReleased();

	void SetButtonPressed(bool bInPressed);

	DECLARE_DELEGATE_OneParam(FOnSelectableItemInteracted, USelectableItem*);
	FOnSelectableItemInteracted OnItemInteracted;
	FOnSelectableItemInteracted OnItemUnhovered;
	FOnSelectableItemInteracted OnItemClicked;

	void SetGamepadFocused(bool bInFocused, bool bInShowVisual = true);
	bool IsGamepadFocused() const { return bIsGamepadFocused; }
	bool IsButtonHovered() const;
	bool IsItemSelectable() const;
	void RefreshOwnershipState();
	EPurchaseObject GetPurchaseObject() const { return PurchaseObject; }
	const FString& GetCarType() const { return CarType; }
};
