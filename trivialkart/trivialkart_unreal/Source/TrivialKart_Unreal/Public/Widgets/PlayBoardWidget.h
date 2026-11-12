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
#include "Misc/Optional.h"
#include "Styling/SlateTypes.h"
#include "PlayBoardWidget.generated.h"

class UBorder;
class UProgressBar;
class USizeBox;
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

	TWeakObjectPtr<UBorder> InputGuideBorder;
	TWeakObjectPtr<UTextBlock> InputGuideText;
	TWeakObjectPtr<USizeBox> TouchDrivePadSizeBox;
	TWeakObjectPtr<UBorder> TouchDrivePadBorder;

	int32 FocusedButtonIndex = -1;
	int32 PressedButtonIndex = -1;
	bool bFocusedByHover = false;
	TOptional<ETrivialKartInputMode> LastRenderedGuideMode;
	FButtonStyle OriginalButtonStyles[3];
	FLinearColor OriginalBackgroundColors[3];
	bool bStylesCached = false;

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	bool HandleGamepadKey(const FKey& Key, bool bIsPressed = true, bool bIsRepeat = false);
	bool HasMenuFocus() const { return FocusedButtonIndex >= 0; }
	bool HasBlockingMenuFocus() const { return FocusedButtonIndex >= 0 && !bFocusedByHover; }
	void ClearMenuFocus(bool bOnlyIfNotHovered = false);
	void CancelPendingPress();
	void OnViewportPointerDown(const FVector2D& ScreenSpacePosition);
	void UpdateInputGuideText(ETrivialKartInputMode InputMode);
	
private:
	void EnsureInputGuideWidget();
	bool IsAnyMenuButtonHovered() const;
	bool IsPointOverAnyMenuButton(const FVector2D& ScreenSpacePosition) const;
	void SetHoverFocus(int32 NewButtonIndex);
	void SetKeyboardOrGamepadFocus(int32 NewButtonIndex);

	UFUNCTION()
	void OnGarageButtonClicked();
	UFUNCTION()
	void OnPGSButtonClicked();
	UFUNCTION()
	void OnStoreButtonClicked();

	UFUNCTION()
	void OnGarageButtonHovered();
	UFUNCTION()
	void OnPGSButtonHovered();
	UFUNCTION()
	void OnStoreButtonHovered();
	UFUNCTION()
	void OnMenuButtonUnhovered();

	UFUNCTION()
	void OnGarageButtonPressed();
	UFUNCTION()
	void OnPGSButtonPressed();
	UFUNCTION()
	void OnStoreButtonPressed();
	UFUNCTION()
	void OnMenuButtonReleased();
	
	UFUNCTION()
	void UpdateFuelBar(const float FuelBarPercentage) const;
	UFUNCTION()
	void UpdateDistanceText(const float Distance) const;
	UFUNCTION()
	void UpdateCoinText(const int Quantity) const;

	void UpdateButtonFocusVisuals();
};
