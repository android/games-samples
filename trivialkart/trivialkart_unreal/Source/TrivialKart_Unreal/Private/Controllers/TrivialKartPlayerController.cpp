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

#include "Controllers/TrivialKartPlayerController.h"

#include "Actors/TrivialKartHUD.h"
#include "Async/Async.h"
#include "Controllers/TrivialKartGamepadInputProcessor.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/PlatformTime.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/PlayBoardWidget.h"
#include "Widgets/SViewport.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace
{
#if PLATFORM_ANDROID
	bool ClearPendingJniException(JNIEnv* Env)
	{
		if (Env && Env->ExceptionCheck())
		{
			Env->ExceptionClear();
			return true;
		}
		return false;
	}

	struct FAndroidInputDeviceJniCache
	{
		jclass InputDeviceClass = nullptr;
		jmethodID GetDeviceIdsMethod = nullptr;
		jmethodID GetDeviceMethod = nullptr;
		jmethodID GetSourcesMethod = nullptr;
		jmethodID GetVendorIdMethod = nullptr;
		jmethodID GetControllerNumberMethod = nullptr;
		jmethodID IsVirtualMethod = nullptr;
		jmethodID GetNameMethod = nullptr;
		bool bInitialized = false;
		bool bValid = false;

		bool EnsureInitialized(JNIEnv* Env)
		{
			if (bInitialized)
			{
				return bValid;
			}
			bInitialized = true;
			if (!Env)
			{
				return false;
			}

			jclass LocalClass = Env->FindClass("android/view/InputDevice");
			if (ClearPendingJniException(Env) || !LocalClass)
			{
				return false;
			}

			InputDeviceClass = static_cast<jclass>(Env->NewGlobalRef(LocalClass));
			Env->DeleteLocalRef(LocalClass);
			if (ClearPendingJniException(Env) || !InputDeviceClass)
			{
				InputDeviceClass = nullptr;
				return false;
			}

			GetDeviceIdsMethod = Env->GetStaticMethodID(InputDeviceClass, "getDeviceIds", "()[I");
			if (ClearPendingJniException(Env) || !GetDeviceIdsMethod) return false;

			GetDeviceMethod = Env->GetStaticMethodID(InputDeviceClass, "getDevice", "(I)Landroid/view/InputDevice;");
			if (ClearPendingJniException(Env) || !GetDeviceMethod) return false;

			GetSourcesMethod = Env->GetMethodID(InputDeviceClass, "getSources", "()I");
			if (ClearPendingJniException(Env) || !GetSourcesMethod) return false;

			GetVendorIdMethod = Env->GetMethodID(InputDeviceClass, "getVendorId", "()I");
			if (ClearPendingJniException(Env) || !GetVendorIdMethod) return false;

			GetControllerNumberMethod = Env->GetMethodID(InputDeviceClass, "getControllerNumber", "()I");
			if (ClearPendingJniException(Env))
			{
				GetControllerNumberMethod = nullptr;
			}

			IsVirtualMethod = Env->GetMethodID(InputDeviceClass, "isVirtual", "()Z");
			if (ClearPendingJniException(Env) || !IsVirtualMethod) return false;

			GetNameMethod = Env->GetMethodID(InputDeviceClass, "getName", "()Ljava/lang/String;");
			if (ClearPendingJniException(Env) || !GetNameMethod) return false;

			bValid = true;
			return true;
		}
	};

	FAndroidInputDeviceJniCache GAndroidInputDeviceJni;
#endif
}

FInputActionValue UTrivialKartDriveInputModifier::ModifyRaw_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue CurrentValue,
	float DeltaTime)
{
	const FInputActionValue ZeroValue(CurrentValue.GetValueType(), FVector::ZeroVector);
	if (!PlayerInput)
	{
		return ZeroValue;
	}

	const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(PlayerInput->GetOuterAPlayerController());
	if (!PC || !PC->IsGameplayDrivingAllowed())
	{
		return ZeroValue;
	}

	switch (GateRule)
	{
	case ETrivialKartDriveGateRule::StandardKey:
		return (CurrentValue.Get<float>() > 0.0f) ? CurrentValue : ZeroValue;

	case ETrivialKartDriveGateRule::AnalogTriggerAxis:
		{
			float RawAxis = PC->GetTriggerAnalogValue(TargetKey);
			if (CurrentValue.GetValueType() != EInputActionValueType::Boolean)
			{
				RawAxis = FMath::Max(RawAxis, CurrentValue.Get<float>());
			}
			if (RawAxis >= ActiveThreshold)
			{
				return (CurrentValue.GetValueType() == EInputActionValueType::Boolean)
					? FInputActionValue(true)
					: FInputActionValue(RawAxis);
			}
			return ZeroValue;
		}

	case ETrivialKartDriveGateRule::DigitalTriggerKey:
		if (PC->IsDigitalTriggerHeld(TargetKey))
		{
			return (CurrentValue.GetValueType() == EInputActionValueType::Boolean)
				? FInputActionValue(true)
				: FInputActionValue(1.0f);
		}
		return ZeroValue;

	case ETrivialKartDriveGateRule::TouchHold:
		return (CurrentValue.Get<float>() > 0.0f &&
				PC->GetActiveInputMode() == ETrivialKartInputMode::Touch &&
				PC->IsPrimaryTouchHeld())
			? CurrentValue
			: ZeroValue;

	case ETrivialKartDriveGateRule::MouseHold:
		return (CurrentValue.Get<float>() > 0.0f &&
				PC->GetActiveInputMode() == ETrivialKartInputMode::KeyboardMouse &&
				PC->IsPrimaryMouseHeld())
			? CurrentValue
			: ZeroValue;
	}

	return ZeroValue;
}

ATrivialKartPlayerController::ATrivialKartPlayerController()
{
	bShowMouseCursor = (ActiveInputMode == ETrivialKartInputMode::KeyboardMouse);
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bEnableTouchEvents = true;
	bEnableTouchOverEvents = true;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultMCFinder(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (DefaultMCFinder.Succeeded())
	{
		DefaultMappingContext = DefaultMCFinder.Object;
	}
}

void ATrivialKartPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	RemapAllDevicesToPrimaryUser();

	if (FSlateApplication::IsInitialized())
	{
		GamepadInputProcessor = MakeShared<FTrivialKartGamepadInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(GamepadInputProcessor, 0);
	}

	RefreshAndroidGamepadSnapshot();

	bWasGamepadAttached = QueryAnyGamepadAttached();
	if (bWasGamepadAttached)
	{
		RefreshGamepadStyleForDevice(INPUTDEVICEID_NONE, false);
		SetActiveInputMode(ETrivialKartInputMode::Gamepad);
	}

	DeviceConnectionChangeHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddUObject(
		this, &ATrivialKartPlayerController::OnInputDeviceConnectionChange);

	PeriodicDevicePollHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ATrivialKartPlayerController::PollDeviceConnectionState),
		0.75f);
}

void ATrivialKartPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PeriodicDevicePollHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PeriodicDevicePollHandle);
		PeriodicDevicePollHandle.Reset();
	}
	if (DeviceCheckTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DeviceCheckTickerHandle);
		DeviceCheckTickerHandle.Reset();
	}
	if (DeviceConnectionChangeHandle.IsValid())
	{
		IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(DeviceConnectionChangeHandle);
		DeviceConnectionChangeHandle.Reset();
	}
	if (GamepadInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(GamepadInputProcessor);
	}
	GamepadInputProcessor.Reset();
	Super::EndPlay(EndPlayReason);
}

UInputMappingContext* ATrivialKartPlayerController::GetOrCreateGameplayMappingContext()
{
	if (RuntimeGameplayMappingContext)
	{
		return RuntimeGameplayMappingContext;
	}
	if (!DefaultMappingContext)
	{
		return nullptr;
	}

	const UInputAction* MoveAction = nullptr;
	for (const FEnhancedActionKeyMapping& Existing : DefaultMappingContext->GetMappings())
	{
		if (Existing.Action)
		{
			MoveAction = Existing.Action;
			break;
		}
	}

	if (!MoveAction)
	{
		return DefaultMappingContext;
	}

	// Build a clean runtime context from scratch so DefaultMappingContext's unguarded Touch1
	// (which lacks FKeyDetails::TouchKey and stays latched true on Android DualSense) cannot survive.
	RuntimeGameplayMappingContext = NewObject<UInputMappingContext>(this);
	if (!RuntimeGameplayMappingContext)
	{
		return DefaultMappingContext;
	}

	auto MapDriveKeyWithRule = [this, MoveAction](const FKey& Key, ETrivialKartDriveGateRule Rule, const FKey& RuleTargetKey = EKeys::Invalid)
	{
		if (!Key.IsValid())
		{
			return;
		}
		FEnhancedActionKeyMapping& Mapping = RuntimeGameplayMappingContext->MapKey(MoveAction, Key);
		if (UTrivialKartDriveInputModifier* Modifier = NewObject<UTrivialKartDriveInputModifier>(RuntimeGameplayMappingContext))
		{
			Modifier->GateRule = Rule;
			Modifier->TargetKey = RuleTargetKey;
			Mapping.Modifiers.Add(Modifier);
		}
	};

	MapDriveKeyWithRule(EKeys::TouchKeys[0], ETrivialKartDriveGateRule::TouchHold);
	MapDriveKeyWithRule(EKeys::LeftMouseButton, ETrivialKartDriveGateRule::MouseHold);

	UTrivialKartDriveInputModifier* SharedStandardGate = NewObject<UTrivialKartDriveInputModifier>(RuntimeGameplayMappingContext);
	if (SharedStandardGate)
	{
		SharedStandardGate->GateRule = ETrivialKartDriveGateRule::StandardKey;
	}

	const FKey StandardDriveKeys[] = {
		EKeys::Gamepad_FaceButton_Bottom,
		EKeys::Gamepad_DPad_Right,
		EKeys::Gamepad_LeftStick_Right,
		EKeys::SpaceBar,
		EKeys::Right
	};

	for (const FKey& Key : StandardDriveKeys)
	{
		if (Key.IsValid())
		{
			FEnhancedActionKeyMapping& KeyMapping = RuntimeGameplayMappingContext->MapKey(MoveAction, Key);
			if (SharedStandardGate)
			{
				KeyMapping.Modifiers.Add(SharedStandardGate);
			}
		}
	}

	MapDriveKeyWithRule(EKeys::Gamepad_LeftTrigger, ETrivialKartDriveGateRule::DigitalTriggerKey, EKeys::Gamepad_LeftTrigger);
	MapDriveKeyWithRule(EKeys::Gamepad_RightTrigger, ETrivialKartDriveGateRule::DigitalTriggerKey, EKeys::Gamepad_RightTrigger);
	MapDriveKeyWithRule(EKeys::Gamepad_LeftTriggerAxis, ETrivialKartDriveGateRule::AnalogTriggerAxis, EKeys::Gamepad_LeftTriggerAxis);
	MapDriveKeyWithRule(EKeys::Gamepad_RightTriggerAxis, ETrivialKartDriveGateRule::AnalogTriggerAxis, EKeys::Gamepad_RightTriggerAxis);

	return RuntimeGameplayMappingContext;
}

void ATrivialKartPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		if (UInputMappingContext* GameplayMC = GetOrCreateGameplayMappingContext())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
			{
				FModifyContextOptions Options;
				Options.bForceImmediately = true;
				Options.bIgnoreAllPressedKeysUntilRelease = false;
				Subsystem->AddMappingContext(GameplayMC, 0, Options);
			}
		}
	}
}

FKey ATrivialKartPlayerController::ResolveFallbackUIKey(const FKey& InKey, bool bIsShiftDown)
{
	if (InKey == EKeys::Tab)
	{
		return bIsShiftDown ? EKeys::Gamepad_LeftShoulder : EKeys::Gamepad_RightShoulder;
	}
	if (InKey == EKeys::Gamepad_DPad_Up || InKey == EKeys::Up)
	{
		return EKeys::Gamepad_DPad_Up;
	}
	if (InKey == EKeys::Gamepad_DPad_Down || InKey == EKeys::Down)
	{
		return EKeys::Gamepad_DPad_Down;
	}
	if (InKey == EKeys::Gamepad_DPad_Left || InKey == EKeys::Left)
	{
		return EKeys::Gamepad_DPad_Left;
	}
	if (InKey == EKeys::Gamepad_DPad_Right || InKey == EKeys::Right)
	{
		return EKeys::Gamepad_DPad_Right;
	}
	if (InKey == EKeys::Gamepad_FaceButton_Bottom || InKey == EKeys::Enter)
	{
		return EKeys::Gamepad_FaceButton_Bottom;
	}
	if (InKey == EKeys::Gamepad_FaceButton_Right || InKey == EKeys::Escape || InKey == EKeys::BackSpace)
	{
		return EKeys::Gamepad_FaceButton_Right;
	}
	if (InKey == EKeys::Gamepad_LeftShoulder)
	{
		return EKeys::Gamepad_LeftShoulder;
	}
	if (InKey == EKeys::Gamepad_RightShoulder)
	{
		return EKeys::Gamepad_RightShoulder;
	}
	return EKeys::Invalid;
}

bool ATrivialKartPlayerController::IsDirectionalUIKey(const FKey& Key)
{
	return Key == EKeys::Gamepad_DPad_Up
		|| Key == EKeys::Gamepad_DPad_Down
		|| Key == EKeys::Gamepad_DPad_Left
		|| Key == EKeys::Gamepad_DPad_Right;
}

bool ATrivialKartPlayerController::IsDirectGameplayDriveKey(const FKey& Key)
{
	return Key == EKeys::Gamepad_FaceButton_Bottom
		|| Key == EKeys::Gamepad_DPad_Right
		|| Key == EKeys::Gamepad_LeftStick_Right
		|| Key == EKeys::SpaceBar
		|| Key == EKeys::Right
		|| Key == EKeys::Gamepad_LeftTrigger
		|| Key == EKeys::Gamepad_RightTrigger;
}

bool ATrivialKartPlayerController::HandleGamepadKey(const FKey& InKey, bool bIsPressed, bool bIsRepeat, bool bIsShiftDown)
{
	ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD());
	if (!CurrentHUD)
	{
		return false;
	}

	if (CurrentHUD->IsLoadingOpen())
	{
		return bIsPressed;
	}

	// Dedicated primary drive keys on PlayBoard always clear bottom-bar focus and drive the kart as instructed by the HUD banner
	if (!IsSubMenuOpen() &&
		(InKey == EKeys::SpaceBar || InKey == EKeys::Gamepad_LeftTrigger || InKey == EKeys::Gamepad_RightTrigger))
	{
		if (bIsPressed)
		{
			CurrentHUD->ClearPlayBoardMenuFocus();
		}
		return false;
	}

	const FKey CanonicalKey = ResolveFallbackUIKey(InKey, bIsShiftDown);
	if (CanonicalKey.IsValid())
	{
		const bool bAllowRepeat = IsDirectionalUIKey(CanonicalKey);
		if (bIsPressed && bIsRepeat && !bAllowRepeat)
		{
			return IsUIBlockingInput();
		}

		if (CurrentHUD->HandleGamepadKey(CanonicalKey, bIsPressed, bIsRepeat))
		{
			if (bIsPressed)
			{
				SetKeyNavigationActive(true);
			}
			return true;
		}
	}

	if (bIsPressed && !IsUIBlockingInput() && IsDirectGameplayDriveKey(InKey))
	{
		CurrentHUD->ClearPlayBoardMenuFocus();
	}

	return false;
}

bool ATrivialKartPlayerController::HandleUICancelKey(bool bIsPressed)
{
	if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		return CurrentHUD->HandleGamepadKey(EKeys::Gamepad_FaceButton_Right, bIsPressed);
	}
	return false;
}

bool ATrivialKartPlayerController::IsLoadingOpen() const
{
	if (const ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		return CurrentHUD->IsLoadingOpen();
	}
	return false;
}

bool ATrivialKartPlayerController::IsSubMenuOpen() const
{
	if (const ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		return CurrentHUD->IsSubMenuOpen();
	}
	return false;
}

bool ATrivialKartPlayerController::IsUIBlockingInput() const
{
	if (const ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		return CurrentHUD->IsUIBlockingInput();
	}
	return false;
}

bool ATrivialKartPlayerController::IsGameplayDrivingAllowed() const
{
#if WITH_EDITOR
	if (!IsGameViewportFocused())
	{
		return false;
	}
#endif
	return bGameplayInputEnabled && !IsUIBlockingInput();
}

bool ATrivialKartPlayerController::IsStickNavigationEnabled() const
{
	if (const ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		return CurrentHUD->IsStickNavigationEnabled();
	}
	return false;
}

bool ATrivialKartPlayerController::IsPointOverPlayBoardMenuButton(const FVector2D& ScreenSpacePosition) const
{
	if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		if (const UPlayBoardWidget* PlayBoard = Cast<UPlayBoardWidget>(CurrentHUD->GetWidgetOfType(EWidgetType::PlayBoard)))
		{
			return PlayBoard->IsPointOverAnyMenuButton(ScreenSpacePosition);
		}
	}
	return false;
}

void ATrivialKartPlayerController::SetGameplayInputEnabled(bool bEnabled)
{
	bGameplayInputEnabled = bEnabled;
	if (!bEnabled && GamepadInputProcessor.IsValid())
	{
		GamepadInputProcessor->ClearPointerDriveHeld();
	}
}

void ATrivialKartPlayerController::NotifyExplicitDeviceInput(ETrivialKartInputMode DetectedMode)
{
	if (DetectedMode != ETrivialKartInputMode::Gamepad)
	{
		LastNonGamepadInputTimeSeconds = FPlatformTime::Seconds();
		SetActiveInputMode(DetectedMode);
	}
}

void ATrivialKartPlayerController::NotifyExplicitGamepadInput(FInputDeviceId SourceDeviceId)
{
	if (SourceDeviceId.IsValid())
	{
		LastActiveGamepadDeviceId = SourceDeviceId;
	}

	// Zero-allocation / zero-JNI fast path for 60-240Hz stick & trigger analog events
	if (ActiveInputMode == ETrivialKartInputMode::Gamepad &&
		bKeyNavigationActive &&
		SourceDeviceId.IsValid())
	{
		if (const ETrivialKartGamepadStyle* ConfirmedStyle = ConfirmedUnrealDeviceStyles.Find(SourceDeviceId))
		{
			if (ActiveGamepadStyle != *ConfirmedStyle)
			{
				ActiveGamepadStyle = *ConfirmedStyle;
				OnInputModeUpdated.Broadcast(ActiveInputMode);
			}
			return;
		}
	}

	bKeyNavigationActive = true;
	RefreshGamepadStyleForDevice(LastActiveGamepadDeviceId, true);
	SetActiveInputMode(ETrivialKartInputMode::Gamepad);
}

void ATrivialKartPlayerController::NotifyGamepadDeviceDiscovered(FInputDeviceId SourceDeviceId)
{
	if (SourceDeviceId.IsValid() && !SeenUnrealGamepadDevices.Contains(SourceDeviceId))
	{
		RefreshAndroidGamepadSnapshot();
		InvalidatePendingUnrealSlotEvaluations();
		ResolveGamepadStyleForDevice(SourceDeviceId);
	}
}

void ATrivialKartPlayerController::NotifyGamepadHardwareSignatureUpdated(FInputDeviceId SourceDeviceId)
{
	if (SourceDeviceId.IsValid())
	{
		ResolveGamepadStyleForDevice(SourceDeviceId);
	}
	if (LastActiveGamepadDeviceId.IsValid())
	{
		RefreshGamepadStyleForDevice(LastActiveGamepadDeviceId, true);
	}
}

void ATrivialKartPlayerController::NotifyPointerDeviceInput(ETrivialKartInputMode PointerMode)
{
	LastNonGamepadInputTimeSeconds = FPlatformTime::Seconds();
	const bool bWasKeyNav = bKeyNavigationActive;
	bKeyNavigationActive = false;

	if (ActiveInputMode != PointerMode)
	{
		SetActiveInputMode(PointerMode);
	}
	else if (bWasKeyNav)
	{
		OnInputModeUpdated.Broadcast(ActiveInputMode);
	}
}

void ATrivialKartPlayerController::SetKeyNavigationActive(bool bActive)
{
	if (bKeyNavigationActive != bActive)
	{
		bKeyNavigationActive = bActive;
		OnInputModeUpdated.Broadcast(ActiveInputMode);
	}
}

void ATrivialKartPlayerController::SetActiveInputMode(ETrivialKartInputMode NewMode)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<ATrivialKartPlayerController> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, NewMode]()
		{
			if (ATrivialKartPlayerController* StrongThis = WeakThis.Get())
			{
				StrongThis->SetActiveInputMode(NewMode);
			}
		});
		return;
	}

	if (ActiveInputMode == NewMode)
	{
		return;
	}

	if (NewMode != ETrivialKartInputMode::Gamepad)
	{
		PreviousNonGamepadMode = NewMode;
		if (GamepadInputProcessor.IsValid())
		{
			GamepadInputProcessor->ResetAnalogState();
		}
	}
	else
	{
		bKeyNavigationActive = true;
	}

	if (NewMode != ETrivialKartInputMode::Touch && GamepadInputProcessor.IsValid())
	{
		GamepadInputProcessor->ClearPrimaryTouchHeld();
	}

	ActiveInputMode = NewMode;
	bShowMouseCursor = (ActiveInputMode == ETrivialKartInputMode::KeyboardMouse);
	OnInputModeUpdated.Broadcast(ActiveInputMode);
}

void ATrivialKartPlayerController::ClearPlayBoardMenuFocus()
{
	if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		CurrentHUD->ClearPlayBoardMenuFocus();
	}
}

void ATrivialKartPlayerController::NotifyViewportPointerDown(const FVector2D& ScreenSpacePosition)
{
	if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		CurrentHUD->NotifyViewportPointerDown(ScreenSpacePosition);
	}
}

void ATrivialKartPlayerController::CancelPendingPresses()
{
	if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		CurrentHUD->CancelPendingPresses();
	}
}

void ATrivialKartPlayerController::LatchStickUntilNeutral()
{
	if (GamepadInputProcessor.IsValid())
	{
		GamepadInputProcessor->LatchStickUntilNeutral();
	}
}

float ATrivialKartPlayerController::GetTriggerAnalogValue(const FKey& TriggerAxisKey) const
{
	if (!GamepadInputProcessor.IsValid())
	{
		return 0.0f;
	}
	if (TriggerAxisKey == EKeys::Gamepad_LeftTriggerAxis)
	{
		return GamepadInputProcessor->GetMaxTriggerAnalog(false);
	}
	if (TriggerAxisKey == EKeys::Gamepad_RightTriggerAxis)
	{
		return GamepadInputProcessor->GetMaxTriggerAnalog(true);
	}
	return 0.0f;
}

bool ATrivialKartPlayerController::IsDigitalTriggerHeld(const FKey& DigitalTriggerKey) const
{
	if (!GamepadInputProcessor.IsValid())
	{
		return false;
	}
	if (DigitalTriggerKey == EKeys::Gamepad_LeftTrigger)
	{
		return GamepadInputProcessor->IsDigitalTriggerHeld(false);
	}
	if (DigitalTriggerKey == EKeys::Gamepad_RightTrigger)
	{
		return GamepadInputProcessor->IsDigitalTriggerHeld(true);
	}
	return false;
}

bool ATrivialKartPlayerController::IsPrimaryTouchHeld() const
{
	return GamepadInputProcessor.IsValid() && GamepadInputProcessor->IsPrimaryTouchHeld();
}

bool ATrivialKartPlayerController::IsPrimaryMouseHeld() const
{
	return GamepadInputProcessor.IsValid() && GamepadInputProcessor->IsPrimaryMouseHeld();
}

void ATrivialKartPlayerController::RestoreFocusToGameViewport()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
		{
			if (TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget())
			{
#if !WITH_EDITOR
				const UGameInstance* GI = GetGameInstance();
				if (!GI || GI->GetNumLocalPlayers() <= 1)
				{
					FSlateApplication::Get().SetAllUserFocus(ViewportWidget, EFocusCause::SetDirectly);
					return;
				}
#endif
				FSlateApplication::Get().SetUserFocus(GetLocalPlayerUserIndex(), ViewportWidget, EFocusCause::SetDirectly);
			}
		}
	}
}

int32 ATrivialKartPlayerController::GetLocalPlayerUserIndex() const
{
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		return LocalPlayer->GetControllerId();
	}
	return 0;
}

bool ATrivialKartPlayerController::ShouldAcceptInputFromUser(int32 EventUserIndex, bool bIsGamepadEvent) const
{
	if (EventUserIndex == GetLocalPlayerUserIndex())
	{
		return true;
	}
	if (bIsGamepadEvent)
	{
		if (const UGameInstance* GI = GetGameInstance())
		{
			if (GI->GetNumLocalPlayers() <= 1)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool ATrivialKartPlayerController::IsGameViewportFocused() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
		{
			if (TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget())
			{
				return ViewportWidget->HasAnyUserFocusOrFocusedDescendants();
			}
		}
	}
	return false;
}

bool ATrivialKartPlayerController::IsScreenPositionInGameViewport(const FVector2D& ScreenSpacePosition) const
{
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
		{
			if (TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget())
			{
				return ViewportWidget->GetTickSpaceGeometry().IsUnderLocation(ScreenSpacePosition);
			}
		}
	}
	return false;
}

bool ATrivialKartPlayerController::QueryAnyGamepadAttached() const
{
#if PLATFORM_ANDROID
	if (bHasInitialAndroidSnapshot && GAndroidInputDeviceJni.bValid)
	{
		return !CachedAndroidGamepads.IsEmpty();
	}
#endif
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	const TSharedPtr<GenericApplication> PlatformApp = FSlateApplication::Get().GetPlatformApplication();
	return PlatformApp.IsValid() && PlatformApp->IsGamepadAttached();
}

void ATrivialKartPlayerController::InvalidatePendingUnrealSlotEvaluations()
{
	for (TPair<FInputDeviceId, FTrivialKartUnrealSlotBinding>& Pair : UnrealDeviceSlotBindings)
	{
		if (Pair.Value.SlotIndex == INDEX_NONE || !ConfirmedUnrealDeviceStyles.Contains(Pair.Key))
		{
			Pair.Value.bEvaluated = false;
		}
	}
}

void ATrivialKartPlayerController::EvictUnrealGamepadDevice(FInputDeviceId DeviceId)
{
	if (!DeviceId.IsValid())
	{
		return;
	}
	if (const FTrivialKartUnrealSlotBinding* Bound = UnrealDeviceSlotBindings.Find(DeviceId))
	{
		if (Bound->SlotIndex != INDEX_NONE)
		{
			ClaimedAndroidControllerSlots.Remove(Bound->SlotIndex);
		}
		UnrealDeviceSlotBindings.Remove(DeviceId);
	}
	if (LastActiveGamepadDeviceId == DeviceId)
	{
		LastActiveGamepadDeviceId = INPUTDEVICEID_NONE;
	}
	if (GamepadInputProcessor.IsValid())
	{
		GamepadInputProcessor->RemoveDeviceState(DeviceId);
	}
	SeenUnrealGamepadDevices.Remove(DeviceId);
	ConfirmedUnrealDeviceStyles.Remove(DeviceId);
}

int32 ATrivialKartPlayerController::CountConnectedAndroidStyle(ETrivialKartGamepadStyle Style) const
{
	int32 Total = 0;
	for (const FTrivialKartAndroidGamepadInfo& Info : CachedAndroidGamepads)
	{
		if (Info.Style == Style)
		{
			++Total;
		}
	}
	return Total;
}

bool ATrivialKartPlayerController::TryPopUniformPendingHotplugStyle(ETrivialKartGamepadStyle& OutStyle)
{
	if (PendingHotplugStyles.IsEmpty())
	{
		return false;
	}
	const ETrivialKartGamepadStyle CandidateStyle = PendingHotplugStyles[0];
	for (int32 i = 1; i < PendingHotplugStyles.Num(); ++i)
	{
		if (PendingHotplugStyles[i] != CandidateStyle)
		{
			return false;
		}
	}
	PendingHotplugStyles.RemoveAt(0);
	OutStyle = CandidateStyle;
	return true;
}

void ATrivialKartPlayerController::RefreshAndroidGamepadSnapshot()
{
#if PLATFORM_ANDROID
	CachedAndroidGamepads.Reset();

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		if (!GAndroidInputDeviceJni.EnsureInitialized(Env))
		{
			return;
		}

		jintArray DeviceIdsArray = static_cast<jintArray>(
			Env->CallStaticObjectMethod(GAndroidInputDeviceJni.InputDeviceClass, GAndroidInputDeviceJni.GetDeviceIdsMethod));
		if (ClearPendingJniException(Env))
		{
			DeviceIdsArray = nullptr;
		}

		if (DeviceIdsArray)
		{
			const jsize Count = Env->GetArrayLength(DeviceIdsArray);
			if (!ClearPendingJniException(Env) && Count > 0)
			{
				if (jint* DeviceIds = Env->GetIntArrayElements(DeviceIdsArray, nullptr))
				{
					auto CollectMatchingDevices = [&](jint RequiredSourceMask)
					{
						for (jsize i = 0; i < Count; ++i)
						{
							const int32 AndroidDevId = static_cast<int32>(DeviceIds[i]);
							jobject DevObj = Env->CallStaticObjectMethod(
								GAndroidInputDeviceJni.InputDeviceClass,
								GAndroidInputDeviceJni.GetDeviceMethod,
								DeviceIds[i]);
							if (ClearPendingJniException(Env) || !DevObj)
							{
								continue;
							}

							const jboolean bIsVirtual = Env->CallBooleanMethod(DevObj, GAndroidInputDeviceJni.IsVirtualMethod);
							if (ClearPendingJniException(Env) || bIsVirtual)
							{
								Env->DeleteLocalRef(DevObj);
								continue;
							}

							const jint Sources = Env->CallIntMethod(DevObj, GAndroidInputDeviceJni.GetSourcesMethod);
							if (ClearPendingJniException(Env) || (Sources & RequiredSourceMask) != RequiredSourceMask)
							{
								Env->DeleteLocalRef(DevObj);
								continue;
							}

							const jint VendorId = Env->CallIntMethod(DevObj, GAndroidInputDeviceJni.GetVendorIdMethod);
							if (ClearPendingJniException(Env))
							{
								Env->DeleteLocalRef(DevObj);
								continue;
							}

							int32 ControllerNumber = 0;
							if (GAndroidInputDeviceJni.GetControllerNumberMethod)
							{
								ControllerNumber = static_cast<int32>(
									Env->CallIntMethod(DevObj, GAndroidInputDeviceJni.GetControllerNumberMethod));
								if (ClearPendingJniException(Env))
								{
									ControllerNumber = 0;
								}
							}

							auto ReadJavaStringField = [&](jmethodID MethodId) -> FString
							{
								FString Result;
								if (!MethodId)
								{
									return Result;
								}
								jstring JStr = static_cast<jstring>(Env->CallObjectMethod(DevObj, MethodId));
								if (!ClearPendingJniException(Env) && JStr)
								{
									if (const char* UTFChars = Env->GetStringUTFChars(JStr, nullptr))
									{
										Result = UTF8_TO_TCHAR(UTFChars);
										Env->ReleaseStringUTFChars(JStr, UTFChars);
									}
									ClearPendingJniException(Env);
									Env->DeleteLocalRef(JStr);
								}
								return Result;
							};

							FString DevName = ReadJavaStringField(GAndroidInputDeviceJni.GetNameMethod);
							DevName.TrimStartAndEndInline();

							// Skip auxiliary sensor / touchpad sub-devices registered alongside DualSense and Switch Pro
							if (DevName.Contains(TEXT("Motion Sensors"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("Touchpad"), ESearchCase::IgnoreCase) ||
								DevName.EndsWith(TEXT(" IMU"), ESearchCase::IgnoreCase))
							{
								Env->DeleteLocalRef(DevObj);
								continue;
							}


							ETrivialKartGamepadStyle DevStyle = ETrivialKartGamepadStyle::GenericXbox;
							if (VendorId == 0x054c ||
								DevName.Contains(TEXT("DualSense"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("DualShock"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("PlayStation"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("Sony"), ESearchCase::IgnoreCase))
							{
								DevStyle = ETrivialKartGamepadStyle::PlayStation;
							}
							else if (VendorId == 0x057e ||
								DevName.Contains(TEXT("Nintendo"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("Switch"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("Joy-Con"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("JoyCon"), ESearchCase::IgnoreCase) ||
								DevName.Contains(TEXT("Pro Controller"), ESearchCase::IgnoreCase))
							{
								DevStyle = ETrivialKartGamepadStyle::NintendoSwitch;
							}

							FTrivialKartAndroidGamepadInfo Info;
							Info.AndroidDeviceId = AndroidDevId;
							Info.ControllerNumber = ControllerNumber;
							Info.VendorId = static_cast<int32>(VendorId);
							Info.DeviceName = MoveTemp(DevName);
							Info.Style = DevStyle;
							CachedAndroidGamepads.Add(MoveTemp(Info));

							Env->DeleteLocalRef(DevObj);
						}
					};

					// Prefer strict SOURCE_GAMEPAD (0x00000401) so IMU/Motion-Sensor SOURCE_JOYSTICK sub-devices
					// never duplicate DualSense or Switch Pro entries; fall back to SOURCE_JOYSTICK only if needed.
					constexpr jint SourceGamepad = 0x00000401;
					constexpr jint SourceJoystick = 0x01000010;
					CollectMatchingDevices(SourceGamepad);
					if (CachedAndroidGamepads.IsEmpty())
					{
						CollectMatchingDevices(SourceJoystick);
					}

					Env->ReleaseIntArrayElements(DeviceIdsArray, DeviceIds, JNI_ABORT);
				}
				else
				{
					ClearPendingJniException(Env);
				}
			}
			Env->DeleteLocalRef(DeviceIdsArray);
		}
	}

	CachedAndroidGamepads.Sort([](const FTrivialKartAndroidGamepadInfo& A, const FTrivialKartAndroidGamepadInfo& B)
	{
		const int32 OrderA = (A.ControllerNumber > 0) ? A.ControllerNumber : MAX_int32;
		const int32 OrderB = (B.ControllerNumber > 0) ? B.ControllerNumber : MAX_int32;
		if (OrderA != OrderB)
		{
			return OrderA < OrderB;
		}
		return A.AndroidDeviceId < B.AndroidDeviceId;
	});

	// Revalidate cached Unreal controller-slot bindings (evict released or reassigned slots, and allow re-evaluation)
	TArray<FInputDeviceId, TInlineAllocator<8>> ReleasedSlotDevices;
	for (TPair<FInputDeviceId, FTrivialKartUnrealSlotBinding>& Pair : UnrealDeviceSlotBindings)
	{
		if (Pair.Value.SlotIndex == INDEX_NONE)
		{
			Pair.Value.bEvaluated = false;
			continue;
		}

		FString CurrentSlotName = UKismetSystemLibrary::GetGamepadControllerName(Pair.Value.SlotIndex);
		CurrentSlotName.TrimStartAndEndInline();
		if (CurrentSlotName.IsEmpty() ||
			CurrentSlotName.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
			(!Pair.Value.SlotName.IsEmpty() && !CurrentSlotName.Equals(Pair.Value.SlotName, ESearchCase::CaseSensitive)))
		{
			ReleasedSlotDevices.Add(Pair.Key);
		}
		else if (!ConfirmedUnrealDeviceStyles.Contains(Pair.Key))
		{
			Pair.Value.bEvaluated = false;
		}
	}
	for (FInputDeviceId ReleasedDeviceId : ReleasedSlotDevices)
	{
		EvictUnrealGamepadDevice(ReleasedDeviceId);
	}

	if (!bHasInitialAndroidSnapshot)
	{
		bHasInitialAndroidSnapshot = true;
		for (const FTrivialKartAndroidGamepadInfo& Info : CachedAndroidGamepads)
		{
			KnownAndroidDeviceStyles.Add(Info.AndroidDeviceId, Info.Style);
		}
		return;
	}

	for (auto It = KnownAndroidDeviceStyles.CreateIterator(); It; ++It)
	{
		const int32 KnownId = It->Key;
		const bool bStillPresent = CachedAndroidGamepads.ContainsByPredicate([KnownId](const FTrivialKartAndroidGamepadInfo& Info)
		{
			return Info.AndroidDeviceId == KnownId;
		});
		if (!bStillPresent)
		{
			PendingHotplugStyles.RemoveSingle(It->Value);
			It.RemoveCurrent();
		}
	}

	for (const FTrivialKartAndroidGamepadInfo& Info : CachedAndroidGamepads)
	{
		if (!KnownAndroidDeviceStyles.Contains(Info.AndroidDeviceId))
		{
			KnownAndroidDeviceStyles.Add(Info.AndroidDeviceId, Info.Style);
			PendingHotplugStyles.Add(Info.Style);
		}
	}

	const ETrivialKartGamepadStyle AllStyles[] = {
		ETrivialKartGamepadStyle::PlayStation,
		ETrivialKartGamepadStyle::NintendoSwitch,
		ETrivialKartGamepadStyle::GenericXbox
	};
	for (ETrivialKartGamepadStyle Style : AllStyles)
	{
		const int32 ConnectedCount = CountConnectedAndroidStyle(Style);
		int32 ConfirmedCount = 0;
		for (const TPair<FInputDeviceId, ETrivialKartGamepadStyle>& Pair : ConfirmedUnrealDeviceStyles)
		{
			if (Pair.Value == Style)
			{
				++ConfirmedCount;
			}
		}
		if (ConfirmedCount > ConnectedCount)
		{
			TArray<FInputDeviceId, TInlineAllocator<8>> ExcessDevices;
			for (int32 Pass = 0; Pass < 2 && ExcessDevices.Num() < (ConfirmedCount - ConnectedCount); ++Pass)
			{
				for (const TPair<FInputDeviceId, ETrivialKartGamepadStyle>& Pair : ConfirmedUnrealDeviceStyles)
				{
					if (Pair.Value != Style)
					{
						continue;
					}
					// Pass 0 preserves LastActiveGamepadDeviceId so turning off an idle 2nd identical controller never resets the active one
					if (Pass == 0 && Pair.Key == LastActiveGamepadDeviceId)
					{
						continue;
					}
					ExcessDevices.AddUnique(Pair.Key);
					if (ExcessDevices.Num() >= (ConfirmedCount - ConnectedCount))
					{
						break;
					}
				}
			}
			for (FInputDeviceId ExcessDeviceId : ExcessDevices)
			{
				EvictUnrealGamepadDevice(ExcessDeviceId);
			}
		}
	}
#endif
}

bool ATrivialKartPlayerController::TryClassifyGamepadName(const FString& ControllerName, ETrivialKartGamepadStyle& OutStyle) const
{
	FString Trimmed = ControllerName;
	Trimmed.TrimStartAndEndInline();
	if (Trimmed.IsEmpty() || Trimmed.Equals(TEXT("None"), ESearchCase::IgnoreCase))
	{
		return false;
	}

#if PLATFORM_ANDROID
	// In UE5 FAndroidInputInterface, Microsoft (0x045e) and Sony (0x054c) map to explicit names while
	// Nintendo Switch Pro / Joy-Con (0x057e) frequently reports "Generic". Cross-reference non-MS/Sony VendorIds.
	if (Trimmed.Equals(TEXT("Generic"), ESearchCase::IgnoreCase))
	{
		bool bFoundNonMsSony = false;
		bool bUniformNonMsSony = true;
		ETrivialKartGamepadStyle CandidateStyle = ETrivialKartGamepadStyle::GenericXbox;
		for (const FTrivialKartAndroidGamepadInfo& Info : CachedAndroidGamepads)
		{
			if (Info.VendorId != 0x045e && Info.VendorId != 0x054c)
			{
				if (!bFoundNonMsSony)
				{
					bFoundNonMsSony = true;
					CandidateStyle = Info.Style;
				}
				else if (Info.Style != CandidateStyle)
				{
					bUniformNonMsSony = false;
					break;
				}
			}
		}
		if (bFoundNonMsSony && bUniformNonMsSony)
		{
			OutStyle = CandidateStyle;
			return true;
		}
		return false;
	}

	// 1. Exact case-insensitive match against JNI InputDevice.getName() entries (backed by hardware VendorId)
	bool bFoundExactName = false;
	bool bUniformExactStyle = true;
	ETrivialKartGamepadStyle ExactStyle = ETrivialKartGamepadStyle::GenericXbox;
	for (const FTrivialKartAndroidGamepadInfo& Info : CachedAndroidGamepads)
	{
		if (!Info.DeviceName.IsEmpty() && Info.DeviceName.Equals(Trimmed, ESearchCase::IgnoreCase))
		{
			if (!bFoundExactName)
			{
				bFoundExactName = true;
				ExactStyle = Info.Style;
			}
			else if (Info.Style != ExactStyle)
			{
				bUniformExactStyle = false;
				break;
			}
		}
	}
	if (bFoundExactName)
	{
		if (bUniformExactStyle)
		{
			OutStyle = ExactStyle;
			return true;
		}
		return false;
	}
#else
	if (Trimmed.Equals(TEXT("Generic"), ESearchCase::IgnoreCase))
	{
		return false;
	}
#endif

	// 2. Explicit controller brand / model tokens (shared across Android & Desktop)
	if (Trimmed.Contains(TEXT("Xbox"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("X-Box"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("XInput"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("Microsoft"), ESearchCase::IgnoreCase))
	{
		OutStyle = ETrivialKartGamepadStyle::GenericXbox;
		return true;
	}
	if (Trimmed.Contains(TEXT("DualSense"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("DualShock"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("PlayStation"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("Sony"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("PS4"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("PS5"), ESearchCase::IgnoreCase))
	{
		OutStyle = ETrivialKartGamepadStyle::PlayStation;
		return true;
	}
	if (Trimmed.Contains(TEXT("Nintendo"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("Switch"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("Joy-Con"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("JoyCon"), ESearchCase::IgnoreCase) ||
		Trimmed.Contains(TEXT("Pro Controller"), ESearchCase::IgnoreCase))
	{
		OutStyle = ETrivialKartGamepadStyle::NintendoSwitch;
		return true;
	}

#if PLATFORM_ANDROID
	if (Trimmed.Equals(TEXT("Wireless Controller"), ESearchCase::IgnoreCase) &&
		HasConnectedAndroidStyle(ETrivialKartGamepadStyle::PlayStation))
	{
		OutStyle = ETrivialKartGamepadStyle::PlayStation;
		return true;
	}

	// 3. Substring containment against JNI InputDevice.getName() entries
	if (Trimmed.Len() >= 4)
	{
		bool bFoundSub = false;
		bool bUniformSubStyle = true;
		ETrivialKartGamepadStyle SubStyle = ETrivialKartGamepadStyle::GenericXbox;
		for (const FTrivialKartAndroidGamepadInfo& Info : CachedAndroidGamepads)
		{
			if (Info.DeviceName.Len() >= 4 &&
				(Info.DeviceName.Contains(Trimmed, ESearchCase::IgnoreCase) ||
				 Trimmed.Contains(Info.DeviceName, ESearchCase::IgnoreCase)))
			{
				if (!bFoundSub)
				{
					bFoundSub = true;
					SubStyle = Info.Style;
				}
				else if (Info.Style != SubStyle)
				{
					bUniformSubStyle = false;
					break;
				}
			}
		}
		if (bFoundSub && bUniformSubStyle)
		{
			OutStyle = SubStyle;
			return true;
		}
	}
#endif
	return false;
}

bool ATrivialKartPlayerController::TryResolveStyleFromUnrealControllerSlot(
	FInputDeviceId SourceDeviceId,
	ETrivialKartGamepadStyle& OutStyle)
{
#if PLATFORM_ANDROID
	if (!SourceDeviceId.IsValid())
	{
		return false;
	}

	auto IsValidSlotName = [](const FString& Name) -> bool
	{
		return !Name.IsEmpty() && !Name.Equals(TEXT("None"), ESearchCase::IgnoreCase);
	};

	FTrivialKartUnrealSlotBinding& Binding = UnrealDeviceSlotBindings.FindOrAdd(SourceDeviceId);
	if (Binding.bEvaluated)
	{
		if (Binding.SlotIndex != INDEX_NONE && !Binding.SlotName.IsEmpty())
		{
			ETrivialKartGamepadStyle CachedNamedStyle = ETrivialKartGamepadStyle::GenericXbox;
			if (TryClassifyGamepadName(Binding.SlotName, CachedNamedStyle) && HasConnectedAndroidStyle(CachedNamedStyle))
			{
				OutStyle = CachedNamedStyle;
				return true;
			}
		}
		return false;
	}

	Binding.bEvaluated = true;

	// Query FAndroidInputInterface::DeviceMapping[0..7] names directly via GetGamepadControllerName.
	// Note: Do NOT call IPlatformInputDeviceMapper::RemapControllerIdToPlatformUserAndDevice under
	// input.DeviceMappingPolicy=3 (MapAllDevicesToPrimaryUser), which maps Idx=0 to PrimaryUser's primary
	// device regardless of which physical controller occupies FAndroidInputInterface::DeviceMapping[0].
	// Instead, FAndroidInputInterface assigns DeviceMapping[0..7] and dispatches SendControllerEvents() in
	// ascending slot order, and NotifyGamepadDeviceDiscovered runs on each controller's very first packet,
	// so UnclaimedPopulatedSlots[0] pairs 1-to-1 with SourceDeviceId in exact arrival order.
	FString SlotNames[8];
	for (int32 Idx = 0; Idx < 8; ++Idx)
	{
		SlotNames[Idx] = UKismetSystemLibrary::GetGamepadControllerName(Idx);
		SlotNames[Idx].TrimStartAndEndInline();
	}

	if (Binding.SlotIndex != INDEX_NONE)
	{
		if (!IsValidSlotName(SlotNames[Binding.SlotIndex]) ||
			(!Binding.SlotName.IsEmpty() && !SlotNames[Binding.SlotIndex].Equals(Binding.SlotName, ESearchCase::CaseSensitive)))
		{
			ClaimedAndroidControllerSlots.Remove(Binding.SlotIndex);
			Binding.SlotIndex = INDEX_NONE;
			Binding.SlotName.Reset();
		}
	}

	if (Binding.SlotIndex == INDEX_NONE)
	{
		TArray<int32, TInlineAllocator<8>> UnclaimedPopulatedSlots;
		for (int32 Idx = 0; Idx < 8; ++Idx)
		{
			if (IsValidSlotName(SlotNames[Idx]) && !ClaimedAndroidControllerSlots.Contains(Idx))
			{
				UnclaimedPopulatedSlots.Add(Idx);
			}
		}

		if (UnclaimedPopulatedSlots.Num() == 1)
		{
			const int32 MatchedSlot = UnclaimedPopulatedSlots[0];
			Binding.SlotIndex = MatchedSlot;
			Binding.SlotName = SlotNames[MatchedSlot];
			ClaimedAndroidControllerSlots.Add(MatchedSlot);
		}
		else if (UnclaimedPopulatedSlots.Num() > 1)
		{
			bool bAnyClassified = false;
			bool bAllSameSlotStyle = true;
			ETrivialKartGamepadStyle SharedStyle = ETrivialKartGamepadStyle::GenericXbox;
			for (int32 CandidateSlot : UnclaimedPopulatedSlots)
			{
				ETrivialKartGamepadStyle SlotStyle = ETrivialKartGamepadStyle::GenericXbox;
				if (!TryClassifyGamepadName(SlotNames[CandidateSlot], SlotStyle))
				{
					bAllSameSlotStyle = false;
					break;
				}
				if (!bAnyClassified)
				{
					bAnyClassified = true;
					SharedStyle = SlotStyle;
				}
				else if (SlotStyle != SharedStyle)
				{
					bAllSameSlotStyle = false;
					break;
				}
			}
			if (bAnyClassified && bAllSameSlotStyle && HasConnectedAndroidStyle(SharedStyle))
			{
				const int32 MatchedSlot = UnclaimedPopulatedSlots[0];
				Binding.SlotIndex = MatchedSlot;
				Binding.SlotName = SlotNames[MatchedSlot];
				ClaimedAndroidControllerSlots.Add(MatchedSlot);
				OutStyle = SharedStyle;
				return true;
			}
		}
	}

	if (Binding.SlotIndex != INDEX_NONE && !Binding.SlotName.IsEmpty())
	{
		ETrivialKartGamepadStyle NamedStyle = ETrivialKartGamepadStyle::GenericXbox;
		if (TryClassifyGamepadName(Binding.SlotName, NamedStyle) && HasConnectedAndroidStyle(NamedStyle))
		{
			OutStyle = NamedStyle;
			return true;
		}
	}
	return false;
#else
	(void)SourceDeviceId;
	(void)OutStyle;
	return false;
#endif
}

ETrivialKartGamepadStyle ATrivialKartPlayerController::ResolveGamepadStyleForDevice(FInputDeviceId SourceDeviceId)
{
#if PLATFORM_ANDROID
	if (!bHasInitialAndroidSnapshot || CachedAndroidGamepads.IsEmpty())
	{
		RefreshAndroidGamepadSnapshot();
	}
	if (CachedAndroidGamepads.IsEmpty())
	{
		return ETrivialKartGamepadStyle::GenericXbox;
	}

	// Fast path: all currently connected gamepads share the same controller family
	const ETrivialKartGamepadStyle FirstStyle = CachedAndroidGamepads[0].Style;
	bool bAllSameStyle = true;
	for (int32 i = 1; i < CachedAndroidGamepads.Num(); ++i)
	{
		if (CachedAndroidGamepads[i].Style != FirstStyle)
		{
			bAllSameStyle = false;
			break;
		}
	}
	if (bAllSameStyle)
	{
		if (SourceDeviceId.IsValid())
		{
			ETrivialKartGamepadStyle IgnoredSlotStyle = FirstStyle;
			TryResolveStyleFromUnrealControllerSlot(SourceDeviceId, IgnoredSlotStyle);
			SeenUnrealGamepadDevices.Add(SourceDeviceId);
			ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, FirstStyle);
		}
		PendingHotplugStyles.Empty();
		return FirstStyle;
	}

	if (!SourceDeviceId.IsValid())
	{
		return FirstStyle;
	}

	const bool bIsNewlySeenUnrealDevice = !SeenUnrealGamepadDevices.Contains(SourceDeviceId);
	SeenUnrealGamepadDevices.Add(SourceDeviceId);

	const ETrivialKartDeviceTriggerSignature Sig = GamepadInputProcessor.IsValid()
		? GamepadInputProcessor->GetDeviceTriggerSignature(SourceDeviceId)
		: ETrivialKartDeviceTriggerSignature::Unknown;

	// Ensure SourceDeviceId claims its newly populated FAndroidInputInterface slot even if confirmed via trigger signature
	ETrivialKartGamepadStyle PreSlotStyle = ETrivialKartGamepadStyle::GenericXbox;
	const bool bHasDirectSlotStyle = TryResolveStyleFromUnrealControllerSlot(SourceDeviceId, PreSlotStyle);

	// 1. Unambiguous physical hardware trigger signatures (evaluated first so hardware signatures always self-heal)
	if (Sig == ETrivialKartDeviceTriggerSignature::PlayStationHalfRest && HasConnectedAndroidStyle(ETrivialKartGamepadStyle::PlayStation))
	{
		if (bIsNewlySeenUnrealDevice)
		{
			PendingHotplugStyles.RemoveSingle(ETrivialKartGamepadStyle::PlayStation);
		}
		ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, ETrivialKartGamepadStyle::PlayStation);
		return ETrivialKartGamepadStyle::PlayStation;
	}
	if (Sig == ETrivialKartDeviceTriggerSignature::SwitchDigitalOnly && HasConnectedAndroidStyle(ETrivialKartGamepadStyle::NintendoSwitch))
	{
		if (bIsNewlySeenUnrealDevice)
		{
			PendingHotplugStyles.RemoveSingle(ETrivialKartGamepadStyle::NintendoSwitch);
		}
		ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, ETrivialKartGamepadStyle::NintendoSwitch);
		return ETrivialKartGamepadStyle::NintendoSwitch;
	}

	// 2. Previously confirmed hardware style (preserved across calibrated 0.0..1.0 trigger travel)
	if (const ETrivialKartGamepadStyle* ExistingConfirmed = ConfirmedUnrealDeviceStyles.Find(SourceDeviceId))
	{
		return *ExistingConfirmed;
	}

	// 3. Direct per-slot hardware name lookup via FAndroidInputInterface (UKismetSystemLibrary::GetGamepadControllerName)
	// cross-referenced against JNI VendorId (0x054c PlayStation, 0x057e Switch, 0x045e Xbox)
	if (bHasDirectSlotStyle)
	{
		if (bIsNewlySeenUnrealDevice)
		{
			PendingHotplugStyles.RemoveSingle(PreSlotStyle);
		}
		ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, PreSlotStyle);
		return PreSlotStyle;
	}

	// 4. Hot-plugged device correlation (supports single-device or uniform multi-subdevice pairs like Joy-Con L+R)
	if (bIsNewlySeenUnrealDevice)
	{
		ETrivialKartGamepadStyle HotplugStyle = ETrivialKartGamepadStyle::GenericXbox;
		if (TryPopUniformPendingHotplugStyle(HotplugStyle))
		{
			ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, HotplugStyle);
			return HotplugStyle;
		}
	}

	// 5. Multiset elimination against already-confirmed FInputDeviceIds + AnalogTriggerCapable (which rules out Switch)
	auto CountRemainingStyle = [this, SourceDeviceId](ETrivialKartGamepadStyle Style) -> int32
	{
		int32 Count = CountConnectedAndroidStyle(Style);
		for (const TPair<FInputDeviceId, ETrivialKartGamepadStyle>& Pair : ConfirmedUnrealDeviceStyles)
		{
			if (Pair.Key != SourceDeviceId && Pair.Value == Style && Count > 0)
			{
				--Count;
			}
		}
		return Count;
	};

	const int32 RemPS = CountRemainingStyle(ETrivialKartGamepadStyle::PlayStation);
	const int32 RemSwitch = (Sig == ETrivialKartDeviceTriggerSignature::AnalogTriggerCapable)
		? 0
		: CountRemainingStyle(ETrivialKartGamepadStyle::NintendoSwitch);
	const int32 RemXbox = CountRemainingStyle(ETrivialKartGamepadStyle::GenericXbox);
	const int32 DistinctRemaining = (RemPS > 0 ? 1 : 0) + (RemSwitch > 0 ? 1 : 0) + (RemXbox > 0 ? 1 : 0);
	if (DistinctRemaining == 1)
	{
		const ETrivialKartGamepadStyle DeducedStyle = (RemPS > 0)
			? ETrivialKartGamepadStyle::PlayStation
			: ((RemSwitch > 0) ? ETrivialKartGamepadStyle::NintendoSwitch : ETrivialKartGamepadStyle::GenericXbox);
		ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, DeducedStyle);
		return DeducedStyle;
	}

	// 6. Provisional fallback restricted strictly to unclaimed connected styles (never returning a style already
	// confirmed on another FInputDeviceId).
	if (RemXbox > 0)
	{
		return ETrivialKartGamepadStyle::GenericXbox;
	}
	if (RemSwitch > 0)
	{
		return ETrivialKartGamepadStyle::NintendoSwitch;
	}
	if (RemPS > 0)
	{
		return ETrivialKartGamepadStyle::PlayStation;
	}
	return ETrivialKartGamepadStyle::GenericXbox;
#else
	if (SourceDeviceId.IsValid())
	{
		if (const ETrivialKartGamepadStyle* CachedStyle = ConfirmedUnrealDeviceStyles.Find(SourceDeviceId))
		{
			return *CachedStyle;
		}
	}

	if (UInputDeviceSubsystem* InputDeviceSubsystem = UInputDeviceSubsystem::Get())
	{
		FHardwareDeviceIdentifier HardwareDevice = SourceDeviceId.IsValid()
			? InputDeviceSubsystem->GetInputDeviceHardwareIdentifier(SourceDeviceId)
			: FHardwareDeviceIdentifier();
		const bool bResolvedSpecificDevice = !HardwareDevice.HardwareDeviceIdentifier.IsNone();
		if (!bResolvedSpecificDevice)
		{
			HardwareDevice = InputDeviceSubsystem->GetMostRecentlyUsedHardwareDevice(GetPlatformUserId());
		}

		ETrivialKartGamepadStyle ResolvedStyle = ETrivialKartGamepadStyle::GenericXbox;
		TryClassifyGamepadName(HardwareDevice.HardwareDeviceIdentifier.ToString(), ResolvedStyle);

		if (SourceDeviceId.IsValid() && bResolvedSpecificDevice)
		{
			ConfirmedUnrealDeviceStyles.Add(SourceDeviceId, ResolvedStyle);
		}
		return ResolvedStyle;
	}
	return ETrivialKartGamepadStyle::GenericXbox;
#endif
}

void ATrivialKartPlayerController::RefreshGamepadStyleForDevice(FInputDeviceId SourceDeviceId, bool bBroadcastIfChanged)
{
	const ETrivialKartGamepadStyle NewStyle = ResolveGamepadStyleForDevice(SourceDeviceId);
	if (ActiveGamepadStyle != NewStyle)
	{
		ActiveGamepadStyle = NewStyle;
		if (bBroadcastIfChanged && ActiveInputMode == ETrivialKartInputMode::Gamepad)
		{
			OnInputModeUpdated.Broadcast(ActiveInputMode);
		}
	}
}

void ATrivialKartPlayerController::RemapAllDevicesToPrimaryUser() const
{
#if !WITH_EDITOR
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (GI->GetNumLocalPlayers() > 1)
		{
			return;
		}
	}

	IPlatformInputDeviceMapper& Mapper = IPlatformInputDeviceMapper::Get();
	const FPlatformUserId PrimaryUser = Mapper.GetPrimaryPlatformUser();
	if (!PrimaryUser.IsValid())
	{
		return;
	}

	TArray<FPlatformUserId> ActiveUsers;
	Mapper.GetAllActiveUsers(ActiveUsers);
	for (const FPlatformUserId& UserId : ActiveUsers)
	{
		if (UserId.IsValid() && UserId != PrimaryUser)
		{
			TArray<FInputDeviceId> UserDevices;
			Mapper.GetAllConnectedInputDevicesForUser(UserId, UserDevices);
			for (const FInputDeviceId& DeviceId : UserDevices)
			{
				if (DeviceId.IsValid())
				{
					Mapper.Internal_ChangeInputDeviceUserMapping(DeviceId, PrimaryUser, UserId);
				}
			}
		}
	}
#endif
}

void ATrivialKartPlayerController::SyncGamepadAttachmentState()
{
	RemapAllDevicesToPrimaryUser();
	RefreshAndroidGamepadSnapshot();

	const bool bNowAttached = QueryAnyGamepadAttached();
	const bool bJustAttachedFirstGamepad = (!bWasGamepadAttached && bNowAttached);
	bWasGamepadAttached = bNowAttached;

	if (bNowAttached)
	{
		if (LastActiveGamepadDeviceId.IsValid())
		{
			RefreshGamepadStyleForDevice(LastActiveGamepadDeviceId, true);
		}
		else if (bJustAttachedFirstGamepad || !HasConnectedAndroidStyle(ActiveGamepadStyle))
		{
			RefreshGamepadStyleForDevice(INPUTDEVICEID_NONE, true);
		}
	}

	if (bJustAttachedFirstGamepad)
	{
		if ((FPlatformTime::Seconds() - LastNonGamepadInputTimeSeconds) > 0.25)
		{
			SetActiveInputMode(ETrivialKartInputMode::Gamepad);
		}
	}
	else if (!bNowAttached)
	{
		LastActiveGamepadDeviceId = INPUTDEVICEID_NONE;
		SeenUnrealGamepadDevices.Empty();
		ConfirmedUnrealDeviceStyles.Empty();
		UnrealDeviceSlotBindings.Empty();
		ClaimedAndroidControllerSlots.Empty();
		PendingHotplugStyles.Empty();
		KnownAndroidDeviceStyles.Empty();
		CachedAndroidGamepads.Empty();
		ActiveGamepadStyle = ETrivialKartGamepadStyle::GenericXbox;
		if (GamepadInputProcessor.IsValid())
		{
			GamepadInputProcessor->ResetDeviceCapabilities();
		}
		if (ActiveInputMode == ETrivialKartInputMode::Gamepad)
		{
			if (GamepadInputProcessor.IsValid())
			{
				GamepadInputProcessor->ResetState();
			}
#if PLATFORM_ANDROID
			SetActiveInputMode(ETrivialKartInputMode::Touch);
#else
			SetActiveInputMode(PreviousNonGamepadMode);
#endif
		}
	}
}

void ATrivialKartPlayerController::OnInputDeviceConnectionChange(
	EInputDeviceConnectionState NewConnectionState,
	FPlatformUserId PlatformUserId,
	FInputDeviceId InputDeviceId)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<ATrivialKartPlayerController> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, NewConnectionState, PlatformUserId, InputDeviceId]()
		{
			if (ATrivialKartPlayerController* StrongThis = WeakThis.Get())
			{
				StrongThis->OnInputDeviceConnectionChange(NewConnectionState, PlatformUserId, InputDeviceId);
			}
		});
		return;
	}

	if (NewConnectionState == EInputDeviceConnectionState::Disconnected && InputDeviceId.IsValid())
	{
		EvictUnrealGamepadDevice(InputDeviceId);
	}
	else if (NewConnectionState == EInputDeviceConnectionState::Connected && InputDeviceId.IsValid())
	{
		RefreshAndroidGamepadSnapshot();
		InvalidatePendingUnrealSlotEvaluations();
	}

	RemapAllDevicesToPrimaryUser();

	if (!DeviceCheckTickerHandle.IsValid())
	{
		DeviceCheckTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ATrivialKartPlayerController::EvaluateDeferredDeviceConnectionState),
			0.0f);
	}
}

bool ATrivialKartPlayerController::EvaluateDeferredDeviceConnectionState(float DeltaTime)
{
	DeviceCheckTickerHandle.Reset();
	SyncGamepadAttachmentState();
	return false;
}

bool ATrivialKartPlayerController::PollDeviceConnectionState(float DeltaTime)
{
	SyncGamepadAttachmentState();
	return true;
}
