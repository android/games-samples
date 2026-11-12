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
	void ResetAnalogState();
	void LatchStickUntilNeutral();

private:
	bool ProcessKeyEvent(const FKeyEvent& InKeyEvent, bool bIsPressed);
	ETrivialKartStickDir ComputeDesiredStickDir() const;
	static FKey StickDirToKey(ETrivialKartStickDir Dir);
	void OnApplicationActivationChanged(const bool bIsActive);

	TWeakObjectPtr<ATrivialKartPlayerController> OwnerController;
	FDelegateHandle AppActivationHandle;

	TSet<FKey> ConsumedPressedKeys;
	TSet<FKey> HeldPhysicalKeys;
	TSet<FKey> LatchedRepeatKeys;
	FVector2D AccumulatedMouseDelta = FVector2D::ZeroVector;
	double LastGamepadActivityTimeSeconds = -100.0;

	float CachedLeftX = 0.0f;
	float CachedLeftY = 0.0f;
	float CachedLeftTrigger = 0.0f;
	float CachedRightTrigger = 0.0f;
	ETrivialKartStickDir ActiveStickDir = ETrivialKartStickDir::None;
	float StickRepeatRemaining = 0.0f;
	bool bLatchUntilNeutral = false;
};
