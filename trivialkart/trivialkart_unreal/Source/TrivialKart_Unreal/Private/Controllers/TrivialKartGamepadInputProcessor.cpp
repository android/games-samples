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

#include "Controllers/TrivialKartGamepadInputProcessor.h"

#include "Controllers/TrivialKartPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"

namespace
{
	constexpr float StickActivateThreshold = 0.60f;
	constexpr float StickReleaseThreshold = 0.35f;
	constexpr float StickDominanceRatio = 1.25f;
	constexpr float GamepadModeStickThreshold = 0.45f;
	constexpr float GamepadTriggerActiveThreshold = 0.65f;
	constexpr float StickInitialRepeatDelay = 0.35f;
	constexpr float StickRepeatInterval = 0.11f;
	constexpr float MouseWakeTravelDefaultSq = 16.0f;
	constexpr float MouseWakeTravelFromGamepadSq = 4096.0f;
	constexpr double GamepadTouchpadGuardSeconds = 0.25;

	bool IsAndroidPassthroughSystemKey(const FKey& Key)
	{
		return Key == EKeys::Android_Volume_Up
			|| Key == EKeys::Android_Volume_Down
			|| Key == EKeys::Android_Menu;
	}

	bool IsSlateNavigationLeakKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_LeftStick_Up
			|| Key == EKeys::Gamepad_LeftStick_Down
			|| Key == EKeys::Gamepad_LeftStick_Left
			|| Key == EKeys::Gamepad_LeftStick_Right
			|| Key == EKeys::Gamepad_DPad_Up
			|| Key == EKeys::Gamepad_DPad_Down
			|| Key == EKeys::Gamepad_DPad_Left
			|| Key == EKeys::Gamepad_DPad_Right
			|| Key == EKeys::Gamepad_LeftTrigger
			|| Key == EKeys::Gamepad_RightTrigger
			|| Key == EKeys::Up
			|| Key == EKeys::Down
			|| Key == EKeys::Left
			|| Key == EKeys::Right
			|| Key == EKeys::Tab
			|| Key == EKeys::SpaceBar
			|| Key == EKeys::Enter;
	}

	bool IsExplicitGamepadPromptSwitchKey(const FKey& Key)
	{
		if (!Key.IsGamepadKey() || Key.IsAxis1D() || Key.IsAxis2D())
		{
			return false;
		}
		// Ignore right-stick synthesized digital keys: TrivialKart does not use the right stick,
		// and Android DualSense uncalibrated Z/RZ/RX/RY axes can synthesize right-stick events while idle.
		if (Key == EKeys::Gamepad_RightStick_Up ||
			Key == EKeys::Gamepad_RightStick_Down ||
			Key == EKeys::Gamepad_RightStick_Left ||
			Key == EKeys::Gamepad_RightStick_Right ||
			Key == EKeys::Gamepad_RightThumbstick)
		{
			return false;
		}
		return true;
	}

	bool IsGamepadTriggerAxis(const FKey& Key)
	{
		return Key == EKeys::Gamepad_LeftTriggerAxis
			|| Key == EKeys::Gamepad_RightTriggerAxis;
	}

	bool TouchPosIsCornerPhantom(const FVector2D& Pos)
	{
		return Pos.X <= 1.0f && Pos.Y <= 1.0f;
	}

	bool IsGenuineKeyboardKey(const FKey& Key)
	{
		if (!Key.IsValid() || Key.IsGamepadKey() || Key.IsTouch() || Key.IsMouseButton() || Key.IsAxis1D() || Key.IsAxis2D())
		{
			return false;
		}
		TCHAR KeyName[NAME_SIZE];
		Key.GetFName().GetPlainNameString(KeyName);
		if (FCString::Strnicmp(KeyName, TEXT("Android_"), 8) == 0 ||
			FCString::Strnicmp(KeyName, TEXT("Steam_"), 6) == 0 ||
			FCString::Strnicmp(KeyName, TEXT("Gesture_"), 8) == 0 ||
			FCString::Strnicmp(KeyName, TEXT("Touch"), 5) == 0)
		{
			return false;
		}
		return true;
	}
}

FTrivialKartGamepadInputProcessor::FTrivialKartGamepadInputProcessor(ATrivialKartPlayerController* InOwner)
	: OwnerController(InOwner)
{
	if (FSlateApplication::IsInitialized())
	{
		AppActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddRaw(
			this, &FTrivialKartGamepadInputProcessor::OnApplicationActivationChanged);
	}
}

FTrivialKartGamepadInputProcessor::~FTrivialKartGamepadInputProcessor()
{
	if (AppActivationHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(AppActivationHandle);
		AppActivationHandle.Reset();
	}
}

void FTrivialKartGamepadInputProcessor::ResetAnalogState()
{
	for (TPair<FInputDeviceId, FTrivialKartDeviceInputState>& Pair : DeviceStates)
	{
		Pair.Value.LeftX = 0.0f;
		Pair.Value.LeftY = 0.0f;
		Pair.Value.LeftTriggerAnalog = 0.0f;
		Pair.Value.RightTriggerAnalog = 0.0f;
		Pair.Value.bDigitalLeftTriggerHeld = false;
		Pair.Value.bDigitalRightTriggerHeld = false;
	}
	ActiveStickDir = ETrivialKartStickDir::None;
	StickRepeatRemaining = 0.0f;
	bLatchUntilNeutral = false;

	if (OwnerController.IsValid())
	{
		OwnerController->CancelPendingPresses();
	}
}

void FTrivialKartGamepadInputProcessor::ResetState()
{
	ConsumedPressedKeys.Empty();
	HeldPhysicalKeys.Empty();
	LatchedRepeatKeys.Empty();
	AccumulatedMouseDelta = FVector2D::ZeroVector;
	bIsPrimaryTouchHeld = false;
	bIsPrimaryMouseHeld = false;
	ResetAnalogState();
}

void FTrivialKartGamepadInputProcessor::ResetDeviceCapabilities()
{
	DeviceStates.Empty();
}

void FTrivialKartGamepadInputProcessor::RemoveDeviceState(FInputDeviceId DeviceId)
{
	DeviceStates.Remove(DeviceId);
	for (auto It = ConsumedPressedKeys.CreateIterator(); It; ++It)
	{
		if (It->DeviceId == DeviceId)
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = HeldPhysicalKeys.CreateIterator(); It; ++It)
	{
		if (It->DeviceId == DeviceId)
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = LatchedRepeatKeys.CreateIterator(); It; ++It)
	{
		if (It->DeviceId == DeviceId)
		{
			It.RemoveCurrent();
		}
	}
}

float FTrivialKartGamepadInputProcessor::GetMaxTriggerAnalog(bool bRightTrigger) const
{
	float MaxValue = 0.0f;
	for (const TPair<FInputDeviceId, FTrivialKartDeviceInputState>& Pair : DeviceStates)
	{
		const float Candidate = bRightTrigger ? Pair.Value.RightTriggerAnalog : Pair.Value.LeftTriggerAnalog;
		MaxValue = FMath::Max(MaxValue, Candidate);
	}
	return MaxValue;
}

bool FTrivialKartGamepadInputProcessor::IsDigitalTriggerHeld(bool bRightTrigger) const
{
	for (const TPair<FInputDeviceId, FTrivialKartDeviceInputState>& Pair : DeviceStates)
	{
		const bool bHasAnalog = bRightTrigger ? Pair.Value.bHasRightTriggerAnalog : Pair.Value.bHasLeftTriggerAnalog;
		const bool bDigitalHeld = bRightTrigger ? Pair.Value.bDigitalRightTriggerHeld : Pair.Value.bDigitalLeftTriggerHeld;
		if (!bHasAnalog && bDigitalHeld)
		{
			return true;
		}
	}
	return false;
}

ETrivialKartDeviceTriggerSignature FTrivialKartGamepadInputProcessor::GetDeviceTriggerSignature(FInputDeviceId DeviceId) const
{
	if (const FTrivialKartDeviceInputState* Found = DeviceStates.Find(DeviceId))
	{
		return Found->TriggerSignature;
	}
	return ETrivialKartDeviceTriggerSignature::Unknown;
}

void FTrivialKartGamepadInputProcessor::GetDominantStickAxes(float& OutLeftX, float& OutLeftY) const
{
	OutLeftX = 0.0f;
	OutLeftY = 0.0f;
	float BestMagSq = 0.0f;
	for (const TPair<FInputDeviceId, FTrivialKartDeviceInputState>& Pair : DeviceStates)
	{
		const float MagSq = Pair.Value.LeftX * Pair.Value.LeftX + Pair.Value.LeftY * Pair.Value.LeftY;
		if (MagSq > BestMagSq)
		{
			BestMagSq = MagSq;
			OutLeftX = Pair.Value.LeftX;
			OutLeftY = Pair.Value.LeftY;
		}
	}
}

void FTrivialKartGamepadInputProcessor::LatchStickUntilNeutral()
{
	ActiveStickDir = ETrivialKartStickDir::None;
	StickRepeatRemaining = 0.0f;
	LatchedRepeatKeys = HeldPhysicalKeys;
	float CurrentLeftX = 0.0f;
	float CurrentLeftY = 0.0f;
	GetDominantStickAxes(CurrentLeftX, CurrentLeftY);
	if (FMath::Abs(CurrentLeftX) >= StickReleaseThreshold || FMath::Abs(CurrentLeftY) >= StickReleaseThreshold)
	{
		bLatchUntilNeutral = true;
	}
}

void FTrivialKartGamepadInputProcessor::OnApplicationActivationChanged(const bool bIsActive)
{
	if (!bIsActive)
	{
		ResetState();
	}
}

ETrivialKartStickDir FTrivialKartGamepadInputProcessor::ComputeDesiredStickDir() const
{
	float CachedLeftX = 0.0f;
	float CachedLeftY = 0.0f;
	GetDominantStickAxes(CachedLeftX, CachedLeftY);
	const float AbsX = FMath::Abs(CachedLeftX);
	const float AbsY = FMath::Abs(CachedLeftY);

	if (ActiveStickDir == ETrivialKartStickDir::Left || ActiveStickDir == ETrivialKartStickDir::Right)
	{
		if (AbsY >= StickActivateThreshold && AbsY > AbsX * StickDominanceRatio)
		{
			return CachedLeftY > 0.0f ? ETrivialKartStickDir::Up : ETrivialKartStickDir::Down;
		}
		if (ActiveStickDir == ETrivialKartStickDir::Right && CachedLeftX <= -StickActivateThreshold)
		{
			return ETrivialKartStickDir::Left;
		}
		if (ActiveStickDir == ETrivialKartStickDir::Left && CachedLeftX >= StickActivateThreshold)
		{
			return ETrivialKartStickDir::Right;
		}
		if ((ActiveStickDir == ETrivialKartStickDir::Right && CachedLeftX >= StickReleaseThreshold) ||
			(ActiveStickDir == ETrivialKartStickDir::Left && CachedLeftX <= -StickReleaseThreshold))
		{
			return ActiveStickDir;
		}
	}
	else if (ActiveStickDir == ETrivialKartStickDir::Up || ActiveStickDir == ETrivialKartStickDir::Down)
	{
		if (AbsX >= StickActivateThreshold && AbsX > AbsY * StickDominanceRatio)
		{
			return CachedLeftX > 0.0f ? ETrivialKartStickDir::Right : ETrivialKartStickDir::Left;
		}
		if (ActiveStickDir == ETrivialKartStickDir::Up && CachedLeftY <= -StickActivateThreshold)
		{
			return ETrivialKartStickDir::Down;
		}
		if (ActiveStickDir == ETrivialKartStickDir::Down && CachedLeftY >= StickActivateThreshold)
		{
			return ETrivialKartStickDir::Up;
		}
		if ((ActiveStickDir == ETrivialKartStickDir::Up && CachedLeftY >= StickReleaseThreshold) ||
			(ActiveStickDir == ETrivialKartStickDir::Down && CachedLeftY <= -StickReleaseThreshold))
		{
			return ActiveStickDir;
		}
	}

	if (AbsX >= StickActivateThreshold && AbsX >= AbsY * StickDominanceRatio)
	{
		return CachedLeftX > 0.0f ? ETrivialKartStickDir::Right : ETrivialKartStickDir::Left;
	}
	if (AbsY >= StickActivateThreshold && AbsY >= AbsX * StickDominanceRatio)
	{
		return CachedLeftY > 0.0f ? ETrivialKartStickDir::Up : ETrivialKartStickDir::Down;
	}
	return ETrivialKartStickDir::None;
}

FKey FTrivialKartGamepadInputProcessor::StickDirToKey(ETrivialKartStickDir Dir)
{
	switch (Dir)
	{
	case ETrivialKartStickDir::Up:
		return EKeys::Gamepad_DPad_Up;
	case ETrivialKartStickDir::Down:
		return EKeys::Gamepad_DPad_Down;
	case ETrivialKartStickDir::Left:
		return EKeys::Gamepad_DPad_Left;
	case ETrivialKartStickDir::Right:
		return EKeys::Gamepad_DPad_Right;
	default:
		return EKeys::Invalid;
	}
}

void FTrivialKartGamepadInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	if (!OwnerController.IsValid())
	{
		return;
	}

#if WITH_EDITOR
	if (!OwnerController->IsGameViewportFocused())
	{
		if (ActiveStickDir != ETrivialKartStickDir::None ||
			ConsumedPressedKeys.Num() > 0 ||
			HeldPhysicalKeys.Num() > 0 ||
			bIsPrimaryMouseHeld ||
			bIsPrimaryTouchHeld ||
			GetMaxTriggerAnalog(false) > 0.0f ||
			GetMaxTriggerAnalog(true) > 0.0f ||
			IsDigitalTriggerHeld(false) ||
			IsDigitalTriggerHeld(true))
		{
			ResetState();
		}
		return;
	}
#endif

	// Confirm digital-only trigger devices (e.g. Switch Pro) once a full frame passes with digital trigger held and zero analog trigger axis
	for (TPair<FInputDeviceId, FTrivialKartDeviceInputState>& Pair : DeviceStates)
	{
		FTrivialKartDeviceInputState& DevState = Pair.Value;
		if (DevState.TriggerSignature == ETrivialKartDeviceTriggerSignature::Unknown &&
			(DevState.bDigitalLeftTriggerHeld || DevState.bDigitalRightTriggerHeld) &&
			!DevState.bHasLeftTriggerAnalog && !DevState.bHasRightTriggerAnalog)
		{
			DevState.TriggerSignature = ETrivialKartDeviceTriggerSignature::SwitchDigitalOnly;
			OwnerController->NotifyGamepadHardwareSignatureUpdated(Pair.Key);
		}
	}

	if (bLatchUntilNeutral)
	{
		float CurrentLeftX = 0.0f;
		float CurrentLeftY = 0.0f;
		GetDominantStickAxes(CurrentLeftX, CurrentLeftY);
		if (FMath::Abs(CurrentLeftX) < StickReleaseThreshold && FMath::Abs(CurrentLeftY) < StickReleaseThreshold)
		{
			bLatchUntilNeutral = false;
		}
		else
		{
			ActiveStickDir = ETrivialKartStickDir::None;
			StickRepeatRemaining = 0.0f;
			return;
		}
	}

	if (!OwnerController->IsStickNavigationEnabled())
	{
		ActiveStickDir = ETrivialKartStickDir::None;
		StickRepeatRemaining = 0.0f;
		return;
	}

	const ETrivialKartStickDir DesiredDir = ComputeDesiredStickDir();
	if (DesiredDir == ETrivialKartStickDir::None)
	{
		ActiveStickDir = ETrivialKartStickDir::None;
		StickRepeatRemaining = 0.0f;
		return;
	}

	// On PlayBoard when menu focus is inactive, allow Left Stick Up/Down to enter the bottom menu while Left Stick Right continues driving
	if (!OwnerController->IsUIBlockingInput() &&
		(DesiredDir == ETrivialKartStickDir::Left || DesiredDir == ETrivialKartStickDir::Right))
	{
		ActiveStickDir = ETrivialKartStickDir::None;
		StickRepeatRemaining = 0.0f;
		return;
	}

	if (DesiredDir != ActiveStickDir)
	{
		ActiveStickDir = DesiredDir;
		StickRepeatRemaining = StickInitialRepeatDelay;
		const FKey MappedKey = StickDirToKey(ActiveStickDir);
		if (MappedKey.IsValid())
		{
			OwnerController->HandleGamepadKey(MappedKey, true, false);
		}
		return;
	}

	StickRepeatRemaining -= DeltaTime;
	if (StickRepeatRemaining <= 0.0f)
	{
		StickRepeatRemaining = StickRepeatInterval;
		const FKey MappedKey = StickDirToKey(ActiveStickDir);
		if (MappedKey.IsValid())
		{
			OwnerController->HandleGamepadKey(MappedKey, true, true);
		}
	}
}

bool FTrivialKartGamepadInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	return ProcessKeyEvent(InKeyEvent, true);
}

bool FTrivialKartGamepadInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	return ProcessKeyEvent(InKeyEvent, false);
}

bool FTrivialKartGamepadInputProcessor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	if (!OwnerController.IsValid())
	{
		return false;
	}

	const FKey Key = InAnalogInputEvent.GetKey();
	if (!OwnerController->ShouldAcceptInputFromUser(InAnalogInputEvent.GetUserIndex(), Key.IsGamepadKey()))
	{
		return false;
	}

#if WITH_EDITOR
	if (!OwnerController->IsGameViewportFocused())
	{
		return false;
	}
#endif

	const FInputDeviceId SourceDeviceId = InAnalogInputEvent.GetInputDeviceId();
	const bool bIsNewlyDiscoveredGamepad = Key.IsGamepadKey() && SourceDeviceId.IsValid() && !DeviceStates.Contains(SourceDeviceId);
	FTrivialKartDeviceInputState& DevState = DeviceStates.FindOrAdd(SourceDeviceId);
	if (bIsNewlyDiscoveredGamepad)
	{
		OwnerController->NotifyGamepadDeviceDiscovered(SourceDeviceId);
	}

	const float Value = InAnalogInputEvent.GetAnalogValue();
	const float AbsValue = FMath::Abs(Value);

	if (Key == EKeys::Gamepad_LeftX)
	{
		DevState.LeftX = Value;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		DevState.LeftY = Value;
	}
	else if (Key == EKeys::Gamepad_LeftTriggerAxis || Key == EKeys::Gamepad_RightTriggerAxis)
	{
		const bool bIsLeft = (Key == EKeys::Gamepad_LeftTriggerAxis);
		if (bIsLeft)
		{
			DevState.LeftTriggerAnalog = Value;
		}
		else
		{
			DevState.RightTriggerAnalog = Value;
		}

		if (AbsValue > KINDA_SMALL_NUMBER)
		{
			if (bIsLeft)
			{
				DevState.bHasLeftTriggerAnalog = true;
				DevState.bDigitalLeftTriggerHeld = false;
			}
			else
			{
				DevState.bHasRightTriggerAnalog = true;
				DevState.bDigitalRightTriggerHeld = false;
			}

			const FTrivialKartDeviceKey DigitalDevKey{ SourceDeviceId, bIsLeft ? EKeys::Gamepad_LeftTrigger : EKeys::Gamepad_RightTrigger };
			if (HeldPhysicalKeys.Remove(DigitalDevKey) > 0)
			{
				LatchedRepeatKeys.Remove(DigitalDevKey);
				ConsumedPressedKeys.Add(DigitalDevKey);
			}
		}

		if ((DevState.bHasLeftTriggerAnalog || DevState.bHasRightTriggerAnalog) && Value >= 0.0f && Value < 0.30f)
		{
			DevState.bSawSubHalfTrigger = true;
			if (DevState.TriggerSignature != ETrivialKartDeviceTriggerSignature::AnalogTriggerCapable)
			{
				DevState.TriggerSignature = ETrivialKartDeviceTriggerSignature::AnalogTriggerCapable;
				OwnerController->NotifyGamepadHardwareSignatureUpdated(SourceDeviceId);
			}
		}
		else if (!DevState.bSawSubHalfTrigger &&
			DevState.bHasLeftTriggerAnalog &&
			DevState.bHasRightTriggerAnalog &&
			FMath::IsWithinInclusive(DevState.LeftTriggerAnalog, 0.46f, 0.54f) &&
			FMath::IsWithinInclusive(DevState.RightTriggerAnalog, 0.46f, 0.54f))
		{
			if (DevState.TriggerSignature != ETrivialKartDeviceTriggerSignature::PlayStationHalfRest)
			{
				DevState.TriggerSignature = ETrivialKartDeviceTriggerSignature::PlayStationHalfRest;
				OwnerController->NotifyGamepadHardwareSignatureUpdated(SourceDeviceId);
			}
		}
		else if (DevState.TriggerSignature == ETrivialKartDeviceTriggerSignature::Unknown &&
			AbsValue > KINDA_SMALL_NUMBER &&
			!(!DevState.bSawSubHalfTrigger && FMath::IsWithinInclusive(Value, 0.46f, 0.54f)))
		{
			DevState.TriggerSignature = ETrivialKartDeviceTriggerSignature::AnalogTriggerCapable;
			OwnerController->NotifyGamepadHardwareSignatureUpdated(SourceDeviceId);
		}
	}

	const bool bIsStickActive = (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY) && (AbsValue >= GamepadModeStickThreshold);
	// Use signed Value >= 0.65f for triggers so DualSense Android 12+ resting +0.50f and legacy HID resting -1.0f never false-trigger
	const bool bIsTriggerPulled = IsGamepadTriggerAxis(Key) && (Value >= GamepadTriggerActiveThreshold);

	if (bIsStickActive || bIsTriggerPulled)
	{
		LastGamepadActivityTimeSeconds = FPlatformTime::Seconds();
		AccumulatedMouseDelta = FVector2D::ZeroVector;
		OwnerController->NotifyExplicitGamepadInput(SourceDeviceId);

		if (bIsTriggerPulled && !OwnerController->IsSubMenuOpen() && !OwnerController->IsLoadingOpen())
		{
			OwnerController->ClearPlayBoardMenuFocus();
			OwnerController->RestoreFocusToGameViewport();
		}
	}

	if (OwnerController->IsUIBlockingInput() && (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY) && AbsValue >= 0.20f)
	{
		return true;
	}

	return false;
}

bool FTrivialKartGamepadInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (!OwnerController.IsValid())
	{
		return false;
	}

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
#if WITH_EDITOR
	if (!OwnerController->IsScreenPositionInGameViewport(ScreenPos))
	{
		return false;
	}
#endif

	AccumulatedMouseDelta = FVector2D::ZeroVector;
	if (MouseEvent.IsTouchEvent())
	{
		if (OwnerController->IsGamepadAttached() && TouchPosIsCornerPhantom(ScreenPos))
		{
			return false;
		}
	}

	OwnerController->NotifyPointerDeviceInput(MouseEvent.IsTouchEvent()
		? ETrivialKartInputMode::Touch
		: ETrivialKartInputMode::KeyboardMouse);
	if (!OwnerController->IsLoadingOpen())
	{
		OwnerController->NotifyViewportPointerDown(ScreenPos);
	}

	const bool bBlockedByUI = OwnerController->IsUIBlockingInput() || OwnerController->IsPointOverPlayBoardMenuButton(ScreenPos);
	if (MouseEvent.IsTouchEvent())
	{
		bIsPrimaryMouseHeld = false;
		if (MouseEvent.GetPointerIndex() >= 3 || bBlockedByUI)
		{
			bIsPrimaryTouchHeld = false;
		}
		else if (MouseEvent.GetPointerIndex() == 0)
		{
			bIsPrimaryTouchHeld = true;
		}
	}
	else
	{
		bIsPrimaryTouchHeld = false;
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			bIsPrimaryMouseHeld = !bBlockedByUI;
		}
	}
	return false;
}

bool FTrivialKartGamepadInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsTouchEvent() && MouseEvent.GetPointerIndex() == 0)
	{
		bIsPrimaryTouchHeld = false;
	}
	else if (!MouseEvent.IsTouchEvent() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPrimaryMouseHeld = false;
	}
	return false;
}

bool FTrivialKartGamepadInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (OwnerController.IsValid() && !MouseEvent.IsTouchEvent())
	{
#if WITH_EDITOR
		if (!OwnerController->IsScreenPositionInGameViewport(MouseEvent.GetScreenSpacePosition()))
		{
			return false;
		}
#endif
#if PLATFORM_ANDROID
		// DualSense / DualShock 4 controllers on Android expose their touchpad as a cursor device; require a click rather than cursor drift while a gamepad is attached
		if (OwnerController->IsGamepadAttached() && OwnerController->GetActiveInputMode() == ETrivialKartInputMode::Gamepad)
		{
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			return false;
		}
#endif
		// Guard against accidental thumb grazes on DualSense / DualShock 4 built-in touchpads while actively using gamepad
		if ((FPlatformTime::Seconds() - LastGamepadActivityTimeSeconds) < GamepadTouchpadGuardSeconds)
		{
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			return false;
		}

		const float RequiredTravelSq = (OwnerController->GetActiveInputMode() == ETrivialKartInputMode::Gamepad)
			? MouseWakeTravelFromGamepadSq
			: MouseWakeTravelDefaultSq;

		AccumulatedMouseDelta += MouseEvent.GetCursorDelta();
		if (AccumulatedMouseDelta.SizeSquared() >= RequiredTravelSq)
		{
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			OwnerController->NotifyPointerDeviceInput(ETrivialKartInputMode::KeyboardMouse);
		}
	}
	return false;
}

bool FTrivialKartGamepadInputProcessor::ProcessKeyEvent(const FKeyEvent& InKeyEvent, bool bIsPressed)
{
	if (!OwnerController.IsValid())
	{
		return false;
	}

	const FKey RawKey = InKeyEvent.GetKey();
	if (IsAndroidPassthroughSystemKey(RawKey))
	{
		return false;
	}

	if (!OwnerController->ShouldAcceptInputFromUser(InKeyEvent.GetUserIndex(), RawKey.IsGamepadKey()))
	{
		return false;
	}

	const FInputDeviceId SourceDeviceId = InKeyEvent.GetInputDeviceId();
	const FTrivialKartDeviceKey DevKey{ SourceDeviceId, RawKey };

#if WITH_EDITOR
	if (!OwnerController->IsGameViewportFocused())
	{
		if (!bIsPressed)
		{
			ConsumedPressedKeys.Remove(DevKey);
			HeldPhysicalKeys.Remove(DevKey);
			LatchedRepeatKeys.Remove(DevKey);
			if (FTrivialKartDeviceInputState* DevState = DeviceStates.Find(SourceDeviceId))
			{
				if (RawKey == EKeys::Gamepad_LeftTrigger)
				{
					DevState->bDigitalLeftTriggerHeld = false;
				}
				else if (RawKey == EKeys::Gamepad_RightTrigger)
				{
					DevState->bDigitalRightTriggerHeld = false;
				}
			}
		}
		return false;
	}
#endif

	if (RawKey.IsGamepadKey() && SourceDeviceId.IsValid() && !DeviceStates.Contains(SourceDeviceId))
	{
		DeviceStates.Add(SourceDeviceId);
		OwnerController->NotifyGamepadDeviceDiscovered(SourceDeviceId);
	}

	// Track trigger capabilities per FInputDeviceId so analog-trigger devices (DualSense +0.50f, Xbox) suppress
	// their own redundant digital trigger keys without suppressing digital-only Switch Pro / Joy-Con triggers.
	if (RawKey == EKeys::Gamepad_LeftTrigger || RawKey == EKeys::Gamepad_RightTrigger)
	{
		FTrivialKartDeviceInputState& DevState = DeviceStates.FindOrAdd(SourceDeviceId);
		const bool bIsLeft = (RawKey == EKeys::Gamepad_LeftTrigger);
		const bool bHasDeviceAnalogAxis = bIsLeft ? DevState.bHasLeftTriggerAnalog : DevState.bHasRightTriggerAnalog;

		if (bIsPressed)
		{
			if (bHasDeviceAnalogAxis || (InKeyEvent.IsRepeat() && !HeldPhysicalKeys.Contains(DevKey)))
			{
				HeldPhysicalKeys.Remove(DevKey);
				LatchedRepeatKeys.Remove(DevKey);
				ConsumedPressedKeys.Add(DevKey);
				return true;
			}

			if (!InKeyEvent.IsRepeat())
			{
				if (bIsLeft)
				{
					DevState.bDigitalLeftTriggerHeld = true;
				}
				else
				{
					DevState.bDigitalRightTriggerHeld = true;
				}
			}
		}
		else
		{
			const bool bWasDigitalHeld = bIsLeft ? DevState.bDigitalLeftTriggerHeld : DevState.bDigitalRightTriggerHeld;
			if (bIsLeft)
			{
				DevState.bDigitalLeftTriggerHeld = false;
			}
			else
			{
				DevState.bDigitalRightTriggerHeld = false;
			}

			if (bWasDigitalHeld &&
				!DevState.bHasLeftTriggerAnalog &&
				!DevState.bHasRightTriggerAnalog &&
				DevState.TriggerSignature == ETrivialKartDeviceTriggerSignature::Unknown)
			{
				DevState.TriggerSignature = ETrivialKartDeviceTriggerSignature::SwitchDigitalOnly;
				OwnerController->NotifyGamepadHardwareSignatureUpdated(SourceDeviceId);
			}

			if (bHasDeviceAnalogAxis && ConsumedPressedKeys.Remove(DevKey) > 0)
			{
				HeldPhysicalKeys.Remove(DevKey);
				LatchedRepeatKeys.Remove(DevKey);
				return true;
			}
		}
	}

	// Analog thumbstick UI navigation (with hysteresis, axis dominance, and neutral latch) is driven from Tick().
	// Swallow Slate's low-threshold synthesized digital stick keys during UI navigation so they never double-fire
	// or trigger gameplay ClearPlayBoardMenuFocus() when moving Left/Right across HUD menu buttons.
	const bool bIsSynthesizedStickMenuKey =
		RawKey == EKeys::Gamepad_LeftStick_Up
		|| RawKey == EKeys::Gamepad_LeftStick_Down
		|| RawKey == EKeys::Gamepad_LeftStick_Left
		|| (RawKey == EKeys::Gamepad_LeftStick_Right && OwnerController->IsUIBlockingInput());

	if (bIsSynthesizedStickMenuKey || (!bIsPressed && RawKey == EKeys::Gamepad_LeftStick_Right && ConsumedPressedKeys.Contains(DevKey)))
	{
		if (bIsPressed)
		{
			if (!InKeyEvent.IsRepeat())
			{
				LastGamepadActivityTimeSeconds = FPlatformTime::Seconds();
				AccumulatedMouseDelta = FVector2D::ZeroVector;
				OwnerController->NotifyExplicitGamepadInput(SourceDeviceId);
			}
			ConsumedPressedKeys.Add(DevKey);
			return true;
		}
		const bool bWasConsumed = ConsumedPressedKeys.Remove(DevKey) > 0;
		return (RawKey != EKeys::Gamepad_LeftStick_Right) || bWasConsumed;
	}

	if (RawKey == EKeys::Android_Back)
	{
		if (bIsPressed)
		{
			if (!InKeyEvent.IsRepeat())
			{
				HeldPhysicalKeys.Add(DevKey);
				LatchedRepeatKeys.Remove(DevKey);
			}
			if (InKeyEvent.IsRepeat() && (ConsumedPressedKeys.Contains(DevKey) || LatchedRepeatKeys.Contains(DevKey)))
			{
				return true;
			}
			if (!OwnerController->IsUIBlockingInput())
			{
				return false;
			}
			if (InKeyEvent.IsRepeat())
			{
				return true;
			}
			const bool bHandled = OwnerController->HandleUICancelKey(true);
			if (bHandled)
			{
				ConsumedPressedKeys.Add(DevKey);
			}
			return bHandled;
		}

		HeldPhysicalKeys.Remove(DevKey);
		LatchedRepeatKeys.Remove(DevKey);
		const bool bWasConsumed = ConsumedPressedKeys.Remove(DevKey) > 0;
		const bool bHandled = bWasConsumed ? OwnerController->HandleUICancelKey(false) : false;
		return bHandled || bWasConsumed;
	}

	if (bIsPressed)
	{
		if (!InKeyEvent.IsRepeat())
		{
			HeldPhysicalKeys.Add(DevKey);
			LatchedRepeatKeys.Remove(DevKey);
		}

		if (!InKeyEvent.IsRepeat() && IsExplicitGamepadPromptSwitchKey(RawKey))
		{
			LastGamepadActivityTimeSeconds = FPlatformTime::Seconds();
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			OwnerController->NotifyExplicitGamepadInput(SourceDeviceId);
		}
		else if (IsGenuineKeyboardKey(RawKey))
		{
			OwnerController->NotifyExplicitDeviceInput(ETrivialKartInputMode::KeyboardMouse);
		}

		if (InKeyEvent.IsRepeat() && LatchedRepeatKeys.Contains(DevKey))
		{
			return true;
		}

		if (InKeyEvent.IsRepeat() && ConsumedPressedKeys.Contains(DevKey) && !OwnerController->IsUIBlockingInput())
		{
			return true;
		}

		const bool bHandled = OwnerController->HandleGamepadKey(RawKey, true, InKeyEvent.IsRepeat(), InKeyEvent.IsShiftDown());
		// Prevent Enter or Tab from ever triggering an invisible Slate widget click when custom focus is inactive
		const bool bBlockUnfocusedActivationLeak = (RawKey == EKeys::Enter || RawKey == EKeys::Tab);
		const bool bBlockSlateLeak = (OwnerController->IsUIBlockingInput() && IsSlateNavigationLeakKey(RawKey)) || bBlockUnfocusedActivationLeak;
		if (bHandled || bBlockSlateLeak)
		{
			ConsumedPressedKeys.Add(DevKey);
			return true;
		}

		if (!OwnerController->IsUIBlockingInput() &&
			(RawKey == EKeys::SpaceBar || RawKey == EKeys::Gamepad_LeftTrigger || RawKey == EKeys::Gamepad_RightTrigger))
		{
			OwnerController->RestoreFocusToGameViewport();
		}
		return false;
	}

	HeldPhysicalKeys.Remove(DevKey);
	LatchedRepeatKeys.Remove(DevKey);
	const bool bWasConsumed = ConsumedPressedKeys.Remove(DevKey) > 0;
	const bool bHandled = OwnerController->HandleGamepadKey(RawKey, false, false, InKeyEvent.IsShiftDown());
	return bHandled || bWasConsumed;
}
