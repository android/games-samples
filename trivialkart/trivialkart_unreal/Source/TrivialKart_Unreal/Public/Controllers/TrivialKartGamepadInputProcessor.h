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
#include "Framework/Application/IInputProcessor.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputCoreTypes.h"
#include "UObject/WeakObjectPtr.h"

class ATrivialKartPlayerController;

enum class ETrivialKartStickDir : uint8
{
	None,
	Up,
	Down,
	Left,
	Right
};

enum class ETrivialKartDeviceTriggerSignature : uint8
{
	Unknown,
	PlayStationHalfRest,
	AnalogTriggerCapable,
	SwitchDigitalOnly
};

class TRIVIALKART_UNREAL_API FTrivialKartGamepadInputProcessor : public IInputProcessor
{
public:
	explicit FTrivialKartGamepadInputProcessor(ATrivialKartPlayerController* InOwner);
	virtual ~FTrivialKartGamepadInputProcessor() override;

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

	void ResetState();
	void ResetDeviceCapabilities();
	void RemoveDeviceState(FInputDeviceId DeviceId);
	float GetMaxTriggerAnalog(bool bRightTrigger) const;
	bool IsDigitalTriggerHeld(bool bRightTrigger) const;
	ETrivialKartDeviceTriggerSignature GetDeviceTriggerSignature(FInputDeviceId DeviceId) const;
	bool IsPrimaryTouchHeld() const { return bIsPrimaryTouchHeld; }
	bool IsPrimaryMouseHeld() const { return bIsPrimaryMouseHeld; }
	void ClearPrimaryTouchHeld() { bIsPrimaryTouchHeld = false; }
	void ClearPointerDriveHeld()
	{
		bIsPrimaryTouchHeld = false;
		bIsPrimaryMouseHeld = false;
	}
	void ResetAnalogState();
	void LatchStickUntilNeutral();

private:
	struct FTrivialKartDeviceKey
	{
		FInputDeviceId DeviceId = INPUTDEVICEID_NONE;
		FKey Key;

		bool operator==(const FTrivialKartDeviceKey& Other) const
		{
			return DeviceId == Other.DeviceId && Key == Other.Key;
		}

		friend uint32 GetTypeHash(const FTrivialKartDeviceKey& Entry)
		{
			return HashCombine(GetTypeHash(Entry.DeviceId.GetId()), GetTypeHash(Entry.Key));
		}
	};

	struct FTrivialKartDeviceInputState
	{
		float LeftX = 0.0f;
		float LeftY = 0.0f;
		float LeftTriggerAnalog = 0.0f;
		float RightTriggerAnalog = 0.0f;
		bool bHasLeftTriggerAnalog = false;
		bool bHasRightTriggerAnalog = false;
		bool bSawSubHalfTrigger = false;
		bool bDigitalLeftTriggerHeld = false;
		bool bDigitalRightTriggerHeld = false;
		ETrivialKartDeviceTriggerSignature TriggerSignature = ETrivialKartDeviceTriggerSignature::Unknown;
	};

	bool ProcessKeyEvent(const FKeyEvent& InKeyEvent, bool bIsPressed);
	void GetDominantStickAxes(float& OutLeftX, float& OutLeftY) const;
	ETrivialKartStickDir ComputeDesiredStickDir() const;
	static FKey StickDirToKey(ETrivialKartStickDir Dir);
	void OnApplicationActivationChanged(const bool bIsActive);

	TWeakObjectPtr<ATrivialKartPlayerController> OwnerController;
	FDelegateHandle AppActivationHandle;

	TSet<FTrivialKartDeviceKey> ConsumedPressedKeys;
	TSet<FTrivialKartDeviceKey> HeldPhysicalKeys;
	TSet<FTrivialKartDeviceKey> LatchedRepeatKeys;
	TMap<FInputDeviceId, FTrivialKartDeviceInputState> DeviceStates;
	FVector2D AccumulatedMouseDelta = FVector2D::ZeroVector;
	double LastGamepadActivityTimeSeconds = -100.0;

	bool bIsPrimaryTouchHeld = false;
	bool bIsPrimaryMouseHeld = false;
	ETrivialKartStickDir ActiveStickDir = ETrivialKartStickDir::None;
	float StickRepeatRemaining = 0.0f;
	bool bLatchUntilNeutral = false;
};
