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
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/PlatformTime.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWidget.h"

FInputActionValue UTrivialKartPositiveTriggerModifier::ModifyRaw_Implementation(
	const UEnhancedPlayerInput* PlayerInput,
	FInputActionValue CurrentValue,
	float DeltaTime)
{
	const float RawAxis = CurrentValue.Get<float>();
	if (RawAxis >= ActiveThreshold)
	{
		return CurrentValue;
	}
	return FInputActionValue(CurrentValue.GetValueType(), FVector::ZeroVector);
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

	bWasGamepadAttached = QueryAnyGamepadAttached();
	if (bWasGamepadAttached)
	{
		SetActiveInputMode(ETrivialKartInputMode::Gamepad);
	}

	DeviceConnectionChangeHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddUObject(
		this, &ATrivialKartPlayerController::OnInputDeviceConnectionChange);

	PeriodicDevicePollHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ATrivialKartPlayerController::PollDeviceConnectionState),
		0.75f);

	if (FSlateApplication::IsInitialized())
	{
		GamepadInputProcessor = MakeShared<FTrivialKartGamepadInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(GamepadInputProcessor, 0);
	}
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

	RuntimeGameplayMappingContext = DuplicateObject<UInputMappingContext>(DefaultMappingContext, this);
	if (!RuntimeGameplayMappingContext)
	{
		return DefaultMappingContext;
	}

	const UInputAction* MoveAction = nullptr;
	for (const FEnhancedActionKeyMapping& Existing : RuntimeGameplayMappingContext->GetMappings())
	{
		if (Existing.Action)
		{
			MoveAction = Existing.Action;
			break;
		}
	}

	if (!MoveAction)
	{
		return RuntimeGameplayMappingContext;
	}

	auto HasKeyMapped = [this, MoveAction](const FKey& Key) -> bool
	{
		for (const FEnhancedActionKeyMapping& Mapping : RuntimeGameplayMappingContext->GetMappings())
		{
			if (Mapping.Action == MoveAction && Mapping.Key == Key)
			{
				return true;
			}
		}
		return false;
	};

	const FKey StandardDriveKeys[] = {
		EKeys::Gamepad_FaceButton_Bottom,
		EKeys::Gamepad_DPad_Right,
		EKeys::Gamepad_LeftStick_Right,
		EKeys::SpaceBar,
		EKeys::Right,
		EKeys::LeftMouseButton,
		EKeys::TouchKeys[0]
	};

	for (const FKey& Key : StandardDriveKeys)
	{
		if (Key.IsValid() && !HasKeyMapped(Key))
		{
			RuntimeGameplayMappingContext->MapKey(MoveAction, Key);
		}
	}

	UTrivialKartPositiveTriggerModifier* TriggerModifier =
		NewObject<UTrivialKartPositiveTriggerModifier>(RuntimeGameplayMappingContext);

	const FKey TriggerAxisKeys[] = {
		EKeys::Gamepad_LeftTriggerAxis,
		EKeys::Gamepad_RightTriggerAxis
	};

	for (const FKey& AxisKey : TriggerAxisKeys)
	{
		if (AxisKey.IsValid() && !HasKeyMapped(AxisKey))
		{
			FEnhancedActionKeyMapping& TriggerMapping = RuntimeGameplayMappingContext->MapKey(MoveAction, AxisKey);
			if (TriggerModifier)
			{
				TriggerMapping.Modifiers.Add(TriggerModifier);
			}
		}
	}

	return RuntimeGameplayMappingContext;
}

void ATrivialKartPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController() && bGameplayInputEnabled)
	{
		if (UInputMappingContext* GameplayMC = GetOrCreateGameplayMappingContext())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(GameplayMC, 0);
			}
		}
	}
}

FKey ATrivialKartPlayerController::ResolveCanonicalUIKey(
	const UInputAction* Action,
	const TArray<FEnhancedActionKeyMapping>& UIMappings)
{
	if (!Action)
	{
		return EKeys::Invalid;
	}

	const FName ActionName = Action->GetFName();
	if (ActionName == TEXT("IA_UI_MoveUp"))
	{
		return EKeys::Gamepad_DPad_Up;
	}
	if (ActionName == TEXT("IA_UI_MoveDown"))
	{
		return EKeys::Gamepad_DPad_Down;
	}
	if (ActionName == TEXT("IA_UI_MoveLeft"))
	{
		return EKeys::Gamepad_DPad_Left;
	}
	if (ActionName == TEXT("IA_UI_MoveRight"))
	{
		return EKeys::Gamepad_DPad_Right;
	}
	if (ActionName == TEXT("IA_UI_Confirm"))
	{
		return EKeys::Gamepad_FaceButton_Bottom;
	}
	if (ActionName == TEXT("IA_UI_Cancel"))
	{
		return EKeys::Gamepad_FaceButton_Right;
	}
	if (ActionName == TEXT("IA_UI_PrevTab"))
	{
		return EKeys::Gamepad_LeftShoulder;
	}
	if (ActionName == TEXT("IA_UI_NextTab"))
	{
		return EKeys::Gamepad_RightShoulder;
	}

	for (const FEnhancedActionKeyMapping& Candidate : UIMappings)
	{
		if (Candidate.Action == Action && Candidate.Key.IsGamepadKey())
		{
			return Candidate.Key;
		}
	}
	return EKeys::Invalid;
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

bool ATrivialKartPlayerController::HandleGamepadKey(const FKey& InKey, bool bIsPressed, bool bIsRepeat, bool bIsShiftDown)
{
	ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD());
	if (!CurrentHUD)
	{
		return false;
	}

	// SpaceBar on PlayBoard always drives the kart as instructed by the HUD banner, even if mouse hovers a bottom menu button
	if (!IsSubMenuOpen() && InKey == EKeys::SpaceBar)
	{
		if (bIsPressed)
		{
			CurrentHUD->ClearPlayBoardMenuFocus();
		}
		return false;
	}

	FKey CanonicalKey = EKeys::Invalid;
	bool bAllowRepeat = false;

	if (UIMappingContext && InKey != EKeys::Tab)
	{
		const TArray<FEnhancedActionKeyMapping>& UIMappings = UIMappingContext->GetMappings();
		for (const FEnhancedActionKeyMapping& Mapping : UIMappings)
		{
			if (Mapping.Key == InKey && Mapping.Action)
			{
				CanonicalKey = ResolveCanonicalUIKey(Mapping.Action, UIMappings);
				bAllowRepeat = (Mapping.Action->ValueType != EInputActionValueType::Boolean) || IsDirectionalUIKey(CanonicalKey);
				break;
			}
		}
	}

	if (!CanonicalKey.IsValid())
	{
		CanonicalKey = ResolveFallbackUIKey(InKey, bIsShiftDown);
		bAllowRepeat = IsDirectionalUIKey(CanonicalKey);
	}

	if (CanonicalKey.IsValid())
	{
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

	UInputMappingContext* GameplayMC = GetOrCreateGameplayMappingContext();
	if (bIsPressed && GameplayMC && !IsUIBlockingInput())
	{
		for (const FEnhancedActionKeyMapping& Mapping : GameplayMC->GetMappings())
		{
			if (Mapping.Key == InKey)
			{
				CurrentHUD->ClearPlayBoardMenuFocus();
				break;
			}
		}
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

bool ATrivialKartPlayerController::IsStickNavigationEnabled() const
{
	if (const ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(GetHUD()))
	{
		return CurrentHUD->IsStickNavigationEnabled();
	}
	return false;
}

void ATrivialKartPlayerController::SetGameplayInputEnabled(bool bEnabled)
{
	UInputMappingContext* GameplayMC = GetOrCreateGameplayMappingContext();
	if (bGameplayInputEnabled == bEnabled || !GameplayMC)
	{
		return;
	}
	bGameplayInputEnabled = bEnabled;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		FModifyContextOptions Options;
		Options.bForceImmediately = true;
		if (bGameplayInputEnabled)
		{
			Subsystem->AddMappingContext(GameplayMC, 0, Options);
		}
		else
		{
			Subsystem->RemoveMappingContext(GameplayMC, Options);
		}
	}
}

void ATrivialKartPlayerController::NotifyExplicitDeviceInput(ETrivialKartInputMode DetectedMode)
{
	if (DetectedMode != ETrivialKartInputMode::Gamepad)
	{
		LastNonGamepadInputTimeSeconds = FPlatformTime::Seconds();
	}
	else
	{
		bKeyNavigationActive = true;
	}
	SetActiveInputMode(DetectedMode);
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

bool ATrivialKartPlayerController::QueryAnyGamepadAttached()
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	const TSharedPtr<GenericApplication> PlatformApp = FSlateApplication::Get().GetPlatformApplication();
	return PlatformApp.IsValid() && PlatformApp->IsGamepadAttached();
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

	const bool bNowAttached = QueryAnyGamepadAttached();
	const bool bJustAttachedFirstGamepad = (!bWasGamepadAttached && bNowAttached);
	bWasGamepadAttached = bNowAttached;

	if (bJustAttachedFirstGamepad)
	{
		if ((FPlatformTime::Seconds() - LastNonGamepadInputTimeSeconds) > 0.25)
		{
			SetActiveInputMode(ETrivialKartInputMode::Gamepad);
		}
	}
	else if (!bNowAttached && ActiveInputMode == ETrivialKartInputMode::Gamepad)
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
