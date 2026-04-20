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
	ObjectSelection,
};

UENUM()
enum class EPurchaseObject : uint8
{
	None,
	Fuel,
	Car,
	Coin
};
class UButtonWidgetStyle;
class UButton;
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
	ESelectableOperationType SelectionOperationType;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::InAppPurchase", EditConditionHides))
	FString PurchaseID;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::InAppPurchase", EditConditionHides))
	int PurchaseQuantity;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::InAppPurchase", EditConditionHides))
	bool bIsConsumable;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "SelectionOperationType == ESelectableOperationType::CoinPurchase", EditConditionHides))
	int CoinQuantity;
	
	UPROPERTY(EditAnywhere)
	EPurchaseObject PurchaseObject;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "PurchaseObject == EPurchaseObject::Car", EditConditionHides))
	FString CarType;
	
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UFUNCTION()
	void OnSelectionButtonClicked();
};
