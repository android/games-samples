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
			|| Key == EKeys::Up
			|| Key == EKeys::Down
			|| Key == EKeys::Left
			|| Key == EKeys::Right
			|| Key == EKeys::Tab
			|| Key == EKeys::SpaceBar
			|| Key == EKeys::Enter;
	}

	bool IsGamepadTriggerAxis(const FKey& Key)
	{
		return Key == EKeys::Gamepad_LeftTriggerAxis
			|| Key == EKeys::Gamepad_RightTriggerAxis;
	}

	bool IsGenuineKeyboardKey(const FKey& Key)
	{
		if (!Key.IsValid() || Key.IsGamepadKey() || Key.IsTouch() || Key.IsMouseButton() || Key.IsAxis1D() || Key.IsAxis2D())
		{
			return false;
		}
		const FString KeyName = Key.GetFName().ToString();
		if (KeyName.StartsWith(TEXT("Android_")) || KeyName.StartsWith(TEXT("Steam_")) || KeyName.StartsWith(TEXT("Gesture_")))
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
	CachedLeftX = 0.0f;
	CachedLeftY = 0.0f;
	CachedLeftTrigger = 0.0f;
	CachedRightTrigger = 0.0f;
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
	ResetAnalogState();
}

void FTrivialKartGamepadInputProcessor::LatchStickUntilNeutral()
{
	ActiveStickDir = ETrivialKartStickDir::None;
	StickRepeatRemaining = 0.0f;
	LatchedRepeatKeys = HeldPhysicalKeys;
	if (FMath::Abs(CachedLeftX) >= StickReleaseThreshold || FMath::Abs(CachedLeftY) >= StickReleaseThreshold)
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
		if (ActiveStickDir != ETrivialKartStickDir::None || ConsumedPressedKeys.Num() > 0)
		{
			ResetState();
		}
		return;
	}
#endif

	if (bLatchUntilNeutral)
	{
		if (FMath::Abs(CachedLeftX) < StickReleaseThreshold && FMath::Abs(CachedLeftY) < StickReleaseThreshold)
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

	const float Value = InAnalogInputEvent.GetAnalogValue();
	const float AbsValue = FMath::Abs(Value);

	if (Key == EKeys::Gamepad_LeftX)
	{
		CachedLeftX = Value;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		CachedLeftY = Value;
	}
	else if (Key == EKeys::Gamepad_LeftTriggerAxis)
	{
		CachedLeftTrigger = Value;
	}
	else if (Key == EKeys::Gamepad_RightTriggerAxis)
	{
		CachedRightTrigger = Value;
	}

	const bool bIsStickActive = (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY) && (AbsValue >= GamepadModeStickThreshold);
	// Use signed Value >= 0.65f for triggers so DualSense Android 12+ resting +0.50f and legacy HID resting -1.0f never false-trigger
	const bool bIsTriggerPulled = IsGamepadTriggerAxis(Key) && (Value >= GamepadTriggerActiveThreshold);

	if (bIsStickActive || bIsTriggerPulled)
	{
		LastGamepadActivityTimeSeconds = FPlatformTime::Seconds();
		AccumulatedMouseDelta = FVector2D::ZeroVector;
		OwnerController->NotifyExplicitDeviceInput(ETrivialKartInputMode::Gamepad);

		if (bIsTriggerPulled && !OwnerController->IsSubMenuOpen())
		{
			OwnerController->ClearPlayBoardMenuFocus();
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

#if WITH_EDITOR
	if (!OwnerController->IsScreenPositionInGameViewport(MouseEvent.GetScreenSpacePosition()))
	{
		return false;
	}
#endif

	AccumulatedMouseDelta = FVector2D::ZeroVector;
	OwnerController->NotifyPointerDeviceInput(MouseEvent.IsTouchEvent()
		? ETrivialKartInputMode::Touch
		: ETrivialKartInputMode::KeyboardMouse);
	OwnerController->NotifyViewportPointerDown(MouseEvent.GetScreenSpacePosition());
	return false;
}

bool FTrivialKartGamepadInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
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

#if WITH_EDITOR
	if (!OwnerController->IsGameViewportFocused())
	{
		if (!bIsPressed)
		{
			ConsumedPressedKeys.Remove(RawKey);
			HeldPhysicalKeys.Remove(RawKey);
			LatchedRepeatKeys.Remove(RawKey);
		}
		return false;
	}
#endif

	// Block premature digital trigger keys synthesized on Android DualSense (+0.50f resting) before real trigger travel (>= 0.65f)
	if (RawKey == EKeys::Gamepad_LeftTrigger || RawKey == EKeys::Gamepad_RightTrigger)
	{
		const float CachedAxis = (RawKey == EKeys::Gamepad_LeftTrigger) ? CachedLeftTrigger : CachedRightTrigger;
		if (bIsPressed && CachedAxis < GamepadTriggerActiveThreshold)
		{
			return true;
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

	if (bIsSynthesizedStickMenuKey || (!bIsPressed && RawKey == EKeys::Gamepad_LeftStick_Right && ConsumedPressedKeys.Contains(RawKey)))
	{
		if (bIsPressed)
		{
			LastGamepadActivityTimeSeconds = FPlatformTime::Seconds();
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			OwnerController->NotifyExplicitDeviceInput(ETrivialKartInputMode::Gamepad);
			ConsumedPressedKeys.Add(RawKey);
			return true;
		}
		const bool bWasConsumed = ConsumedPressedKeys.Remove(RawKey) > 0;
		return (RawKey != EKeys::Gamepad_LeftStick_Right) || bWasConsumed;
	}

	if (RawKey == EKeys::Android_Back)
	{
		if (bIsPressed)
		{
			if (!InKeyEvent.IsRepeat())
			{
				HeldPhysicalKeys.Add(RawKey);
				LatchedRepeatKeys.Remove(RawKey);
			}
			if (InKeyEvent.IsRepeat() && (ConsumedPressedKeys.Contains(RawKey) || LatchedRepeatKeys.Contains(RawKey)))
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
				ConsumedPressedKeys.Add(RawKey);
			}
			return bHandled;
		}

		HeldPhysicalKeys.Remove(RawKey);
		LatchedRepeatKeys.Remove(RawKey);
		const bool bWasConsumed = ConsumedPressedKeys.Remove(RawKey) > 0;
		const bool bHandled = bWasConsumed ? OwnerController->HandleUICancelKey(false) : false;
		return bHandled || bWasConsumed;
	}

	if (bIsPressed)
	{
		if (!InKeyEvent.IsRepeat())
		{
			HeldPhysicalKeys.Add(RawKey);
			LatchedRepeatKeys.Remove(RawKey);
		}

		if (RawKey.IsGamepadKey())
		{
			LastGamepadActivityTimeSeconds = FPlatformTime::Seconds();
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			OwnerController->NotifyExplicitDeviceInput(ETrivialKartInputMode::Gamepad);
		}
		else if (IsGenuineKeyboardKey(RawKey))
		{
			OwnerController->NotifyExplicitDeviceInput(ETrivialKartInputMode::KeyboardMouse);
		}

		if (InKeyEvent.IsRepeat() && LatchedRepeatKeys.Contains(RawKey))
		{
			return true;
		}

		if (InKeyEvent.IsRepeat() && ConsumedPressedKeys.Contains(RawKey) && !OwnerController->IsUIBlockingInput())
		{
			return true;
		}

		const bool bHandled = OwnerController->HandleGamepadKey(RawKey, true, InKeyEvent.IsRepeat(), InKeyEvent.IsShiftDown());
		// Prevent Enter or Tab from ever triggering an invisible Slate widget click when custom focus is inactive
		const bool bBlockUnfocusedActivationLeak = (RawKey == EKeys::Enter || RawKey == EKeys::Tab);
		const bool bBlockSlateLeak = (OwnerController->IsUIBlockingInput() && IsSlateNavigationLeakKey(RawKey)) || bBlockUnfocusedActivationLeak;
		if (bHandled || bBlockSlateLeak)
		{
			ConsumedPressedKeys.Add(RawKey);
			return true;
		}

		if (RawKey == EKeys::SpaceBar)
		{
			OwnerController->RestoreFocusToGameViewport();
		}
		return false;
	}

	HeldPhysicalKeys.Remove(RawKey);
	LatchedRepeatKeys.Remove(RawKey);
	const bool bWasConsumed = ConsumedPressedKeys.Remove(RawKey) > 0;
	const bool bHandled = OwnerController->HandleGamepadKey(RawKey, false, false, InKeyEvent.IsShiftDown());
	return bHandled || bWasConsumed;
}
