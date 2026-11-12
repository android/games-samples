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

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputModifiers.h"
#include "TrivialKartPlayerController.generated.h"

class FTrivialKartGamepadInputProcessor;
class UInputAction;
class UInputMappingContext;

enum class ETrivialKartInputMode : uint8
{
	Touch,
	KeyboardMouse,
	Gamepad
};

DECLARE_MULTICAST_DELEGATE_OneParam(FInputModeUpdated, ETrivialKartInputMode);

/**
 * Clamps trigger axis input so negative HID resting values (-1.0f) and
 * uncalibrated Android DualSense resting values (+0.50f) do not actuate driving.
 */
UCLASS()
class TRIVIALKART_UNREAL_API UTrivialKartPositiveTriggerModifier : public UInputModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	float ActiveThreshold = 0.65f;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(
		const UEnhancedPlayerInput* PlayerInput,
		FInputActionValue CurrentValue,
		float DeltaTime) override;
};

/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API ATrivialKartPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TObjectPtr<UInputMappingContext> UIMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeGameplayMappingContext;

	TSharedPtr<FTrivialKartGamepadInputProcessor> GamepadInputProcessor;
	FDelegateHandle DeviceConnectionChangeHandle;
	FTSTicker::FDelegateHandle DeviceCheckTickerHandle;
	FTSTicker::FDelegateHandle PeriodicDevicePollHandle;

#if PLATFORM_ANDROID
	ETrivialKartInputMode ActiveInputMode = ETrivialKartInputMode::Touch;
#else
	ETrivialKartInputMode ActiveInputMode = ETrivialKartInputMode::KeyboardMouse;
#endif
	ETrivialKartInputMode PreviousNonGamepadMode = ActiveInputMode;
	bool bGameplayInputEnabled = true;
	bool bKeyNavigationActive = false;
	bool bWasGamepadAttached = false;
	double LastNonGamepadInputTimeSeconds = -100.0;
	
public:
	ATrivialKartPlayerController();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	bool HandleGamepadKey(const FKey& InKey, bool bIsPressed = true, bool bIsRepeat = false, bool bIsShiftDown = false);
	bool HandleUICancelKey(bool bIsPressed);

	bool IsSubMenuOpen() const;
	bool IsUIBlockingInput() const;
	bool IsStickNavigationEnabled() const;
	bool IsGamepadAttached() const { return bWasGamepadAttached; }
	void SetGameplayInputEnabled(bool bEnabled);

	void SetActiveInputMode(ETrivialKartInputMode NewMode);
	ETrivialKartInputMode GetActiveInputMode() const { return ActiveInputMode; }

	void NotifyExplicitDeviceInput(ETrivialKartInputMode DetectedMode);
	void NotifyPointerDeviceInput(ETrivialKartInputMode PointerMode);
	void SetKeyNavigationActive(bool bActive);
	bool IsKeyNavigationActive() const { return ActiveInputMode == ETrivialKartInputMode::Gamepad || bKeyNavigationActive; }

	void ClearPlayBoardMenuFocus();
	void NotifyViewportPointerDown(const FVector2D& ScreenSpacePosition);
	void CancelPendingPresses();
	void LatchStickUntilNeutral();
	void RestoreFocusToGameViewport();

	int32 GetLocalPlayerUserIndex() const;
	bool ShouldAcceptInputFromUser(int32 EventUserIndex, bool bIsGamepadEvent) const;
	bool IsGameViewportFocused() const;
	bool IsScreenPositionInGameViewport(const FVector2D& ScreenSpacePosition) const;

	FInputModeUpdated OnInputModeUpdated;

private:
	UInputMappingContext* GetOrCreateGameplayMappingContext();
	static FKey ResolveCanonicalUIKey(const UInputAction* Action, const TArray<struct FEnhancedActionKeyMapping>& UIMappings);
	static FKey ResolveFallbackUIKey(const FKey& InKey, bool bIsShiftDown);
	static bool IsDirectionalUIKey(const FKey& Key);
	static bool QueryAnyGamepadAttached();
	void RemapAllDevicesToPrimaryUser() const;
	void SyncGamepadAttachmentState();
	void OnInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);
	bool EvaluateDeferredDeviceConnectionState(float DeltaTime);
	bool PollDeviceConnectionState(float DeltaTime);
};
