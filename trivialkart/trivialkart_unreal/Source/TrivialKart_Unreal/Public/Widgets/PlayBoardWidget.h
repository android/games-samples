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
#include "PlayBoardWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UButton;
/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API UPlayBoardWidget : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> GarageButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> PGSButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StoreButton;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DistanceText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CoinText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> FuelBar;

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	UFUNCTION()
	void OnGarageButtonClicked();
	UFUNCTION()
	void OnPGSButtonClicked();
	UFUNCTION()
	void OnStoreButtonClicked();
	
	UFUNCTION()
	void UpdateFuelBar(const float FuelBarPercentage) const;
	UFUNCTION()
	void UpdateDistanceText(const float Distance) const;
	UFUNCTION()
	void UpdateCoinText(const int Quantity) const;
};
