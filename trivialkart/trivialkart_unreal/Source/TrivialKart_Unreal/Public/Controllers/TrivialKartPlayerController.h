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
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
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

enum class ETrivialKartGamepadStyle : uint8
{
	GenericXbox,
	PlayStation,
	NintendoSwitch
};

UENUM()
enum class ETrivialKartDriveGateRule : uint8
{
	StandardKey,
	AnalogTriggerAxis,
	DigitalTriggerKey,
	TouchHold,
	MouseHold
};

DECLARE_MULTICAST_DELEGATE_OneParam(FInputModeUpdated, ETrivialKartInputMode);

/**
 * Unified input modifier for runtime gameplay driving mappings:
 * - Gates driving while UI overlays or bottom-bar menu focus block gameplay (without dynamic
 *   RemoveMappingContext/AddMappingContext rebuilds that lock out DualSense +0.50f resting triggers).
 * - Evaluates per-device analog trigger axes (>= 0.65f) and digital-only triggers independently so
 *   simultaneously connected controllers never overwrite or suppress each other's inputs.
 * - Guards Touch1 and LeftMouseButton against DualSense touchpad phantom touches and UI clicks.
 */
UCLASS()
class TRIVIALKART_UNREAL_API UTrivialKartDriveInputModifier : public UInputModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	ETrivialKartDriveGateRule GateRule = ETrivialKartDriveGateRule::StandardKey;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FKey TargetKey;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float ActiveThreshold = 0.65f;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(
		const UEnhancedPlayerInput* PlayerInput,
		FInputActionValue CurrentValue,
		float DeltaTime) override;
};

/**
 * PlayerController managing unified Touch, Keyboard/Mouse, and multi-Gamepad input routing.
 */
UCLASS()
class TRIVIALKART_UNREAL_API ATrivialKartPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeGameplayMappingContext;

	struct FTrivialKartAndroidGamepadInfo
	{
		int32 AndroidDeviceId = -1;
		int32 ControllerNumber = 0;
		int32 VendorId = 0;
		FString DeviceName;
		ETrivialKartGamepadStyle Style = ETrivialKartGamepadStyle::GenericXbox;
	};

	struct FTrivialKartUnrealSlotBinding
	{
		int32 SlotIndex = INDEX_NONE;
		FString SlotName;
		bool bEvaluated = false;
	};

	TSharedPtr<FTrivialKartGamepadInputProcessor> GamepadInputProcessor;
	FDelegateHandle DeviceConnectionChangeHandle;
	FTSTicker::FDelegateHandle DeviceCheckTickerHandle;
	FTSTicker::FDelegateHandle PeriodicDevicePollHandle;

	TArray<FTrivialKartAndroidGamepadInfo> CachedAndroidGamepads;
	TMap<int32, ETrivialKartGamepadStyle> KnownAndroidDeviceStyles;
	TArray<ETrivialKartGamepadStyle> PendingHotplugStyles;
	TSet<FInputDeviceId> SeenUnrealGamepadDevices;
	TMap<FInputDeviceId, ETrivialKartGamepadStyle> ConfirmedUnrealDeviceStyles;
	TMap<FInputDeviceId, FTrivialKartUnrealSlotBinding> UnrealDeviceSlotBindings;
	TSet<int32> ClaimedAndroidControllerSlots;
	FInputDeviceId LastActiveGamepadDeviceId = INPUTDEVICEID_NONE;
	bool bHasInitialAndroidSnapshot = false;

#if PLATFORM_ANDROID
	ETrivialKartInputMode ActiveInputMode = ETrivialKartInputMode::Touch;
#else
	ETrivialKartInputMode ActiveInputMode = ETrivialKartInputMode::KeyboardMouse;
#endif
	ETrivialKartInputMode PreviousNonGamepadMode = ActiveInputMode;
	ETrivialKartGamepadStyle ActiveGamepadStyle = ETrivialKartGamepadStyle::GenericXbox;
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

	bool IsLoadingOpen() const;
	bool IsSubMenuOpen() const;
	bool IsUIBlockingInput() const;
	bool IsGameplayDrivingAllowed() const;
	bool IsStickNavigationEnabled() const;
	bool IsPointOverPlayBoardMenuButton(const FVector2D& ScreenSpacePosition) const;
	bool IsGamepadAttached() const { return bWasGamepadAttached; }
	void SetGameplayInputEnabled(bool bEnabled);

	void SetActiveInputMode(ETrivialKartInputMode NewMode);
	ETrivialKartInputMode GetActiveInputMode() const { return ActiveInputMode; }
	ETrivialKartGamepadStyle GetActiveGamepadStyle() const { return ActiveGamepadStyle; }

	void NotifyExplicitDeviceInput(ETrivialKartInputMode DetectedMode);
	void NotifyExplicitGamepadInput(FInputDeviceId SourceDeviceId);
	void NotifyGamepadDeviceDiscovered(FInputDeviceId SourceDeviceId);
	void NotifyGamepadHardwareSignatureUpdated(FInputDeviceId SourceDeviceId);
	void NotifyPointerDeviceInput(ETrivialKartInputMode PointerMode);
	void SetKeyNavigationActive(bool bActive);
	bool IsKeyNavigationActive() const { return ActiveInputMode == ETrivialKartInputMode::Gamepad || bKeyNavigationActive; }

	void ClearPlayBoardMenuFocus();
	void NotifyViewportPointerDown(const FVector2D& ScreenSpacePosition);
	void CancelPendingPresses();
	void LatchStickUntilNeutral();
	float GetTriggerAnalogValue(const FKey& TriggerAxisKey) const;
	bool IsDigitalTriggerHeld(const FKey& DigitalTriggerKey) const;
	bool IsPrimaryTouchHeld() const;
	bool IsPrimaryMouseHeld() const;
	void RestoreFocusToGameViewport();

	int32 GetLocalPlayerUserIndex() const;
	bool ShouldAcceptInputFromUser(int32 EventUserIndex, bool bIsGamepadEvent) const;
	bool IsGameViewportFocused() const;
	bool IsScreenPositionInGameViewport(const FVector2D& ScreenSpacePosition) const;

	FInputModeUpdated OnInputModeUpdated;

private:
	UInputMappingContext* GetOrCreateGameplayMappingContext();
	static FKey ResolveFallbackUIKey(const FKey& InKey, bool bIsShiftDown);
	static bool IsDirectionalUIKey(const FKey& Key);
	static bool IsDirectGameplayDriveKey(const FKey& Key);
	bool QueryAnyGamepadAttached() const;
	void RefreshAndroidGamepadSnapshot();
	void InvalidatePendingUnrealSlotEvaluations();
	void EvictUnrealGamepadDevice(FInputDeviceId DeviceId);
	int32 CountConnectedAndroidStyle(ETrivialKartGamepadStyle Style) const;
	bool HasConnectedAndroidStyle(ETrivialKartGamepadStyle Style) const { return CountConnectedAndroidStyle(Style) > 0; }
	bool TryPopUniformPendingHotplugStyle(ETrivialKartGamepadStyle& OutStyle);
	bool TryClassifyGamepadName(const FString& ControllerName, ETrivialKartGamepadStyle& OutStyle) const;
	bool TryResolveStyleFromUnrealControllerSlot(FInputDeviceId SourceDeviceId, ETrivialKartGamepadStyle& OutStyle);
	ETrivialKartGamepadStyle ResolveGamepadStyleForDevice(FInputDeviceId SourceDeviceId);
	void RefreshGamepadStyleForDevice(FInputDeviceId SourceDeviceId, bool bBroadcastIfChanged);
	void RemapAllDevicesToPrimaryUser() const;
	void SyncGamepadAttachmentState();
	void OnInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);
	bool EvaluateDeferredDeviceConnectionState(float DeltaTime);
	bool PollDeviceConnectionState(float DeltaTime);
};
