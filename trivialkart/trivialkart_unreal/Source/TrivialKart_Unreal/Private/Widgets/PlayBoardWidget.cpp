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


#include "Widgets/PlayBoardWidget.h"

#include "Actors/TrivialKartHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Infos/TrivialKartPlayerState.h"
#include "Styling/SlateTypes.h"
#include "Widgets/TrivialKartUIStyle.h"

TSharedRef<SWidget> UPlayBoardWidget::RebuildWidget()
{
	if (!IsDesignTime())
	{
		EnsureInputGuideWidget();
		EnsureMenuButtonHighlightFrames();
	}
	return Super::RebuildWidget();
}

void UPlayBoardWidget::EnsureMenuButtonHighlightFrames()
{
	if (IsDesignTime() || !WidgetTree)
	{
		return;
	}

	UButton* Buttons[3] = { GarageButton.Get(), PGSButton.Get(), StoreButton.Get() };
	for (int32 i = 0; i < 3; ++i)
	{
		UButton* Btn = Buttons[i];
		if (!Btn)
		{
			continue;
		}
		if (MenuButtonHighlightBorders[i].IsValid() && Btn->GetContent() == MenuButtonHighlightBorders[i].Get())
		{
			continue;
		}

		UWidget* ExistingContent = Btn->GetContent();
		if (!ExistingContent)
		{
			continue;
		}

		FMargin OrigPad(0.0f, 50.0f, 0.0f, 50.0f);
		if (const UButtonSlot* OldBtnSlot = Cast<UButtonSlot>(Btn->GetContentSlot()))
		{
			OrigPad = OldBtnSlot->GetPadding();
		}

		UBorder* HighlightFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		if (!HighlightFrame)
		{
			continue;
		}

		// Use symmetric outer/inner vertical insets and generous horizontal insets so every button's
		// label/icon stays dead-center inside its highlight box while clearing rounded phone screen corners.
		constexpr float OuterHorizontal = 34.0f;
		constexpr float OuterVertical = 18.0f;
		constexpr float InnerHorizontal = 12.0f;

		const float OrigVerticalAvg = 0.5f * (OrigPad.Top + OrigPad.Bottom);
		float InnerVertical = FMath::Max(10.0f, OrigVerticalAvg - OuterVertical);
		if (UImage* IconImage = Cast<UImage>(ExistingContent))
		{
			const FVector2D OrigSize = IconImage->GetBrush().ImageSize;
			if (OrigSize.Y > 88.0f)
			{
				const float Aspect = (OrigSize.Y > KINDA_SMALL_NUMBER) ? (OrigSize.X / OrigSize.Y) : 1.0f;
				IconImage->SetDesiredSizeOverride(FVector2D(88.0f * Aspect, 88.0f));
			}
			InnerVertical = 10.0f;
		}
		else if (UTextBlock* LabelText = Cast<UTextBlock>(ExistingContent))
		{
			LabelText->SetJustification(ETextJustify::Center);
		}

		const FMargin InnerPadding(InnerHorizontal, InnerVertical, InnerHorizontal, InnerVertical);

		ExistingContent->RemoveFromParent();

		HighlightFrame->SetBrush(TrivialKartUIStyle::GetEmptyBrush());
		HighlightFrame->SetBrushColor(FLinearColor::Transparent);
		HighlightFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		HighlightFrame->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		HighlightFrame->SetHorizontalAlignment(HAlign_Center);
		HighlightFrame->SetVerticalAlignment(VAlign_Center);
		HighlightFrame->SetPadding(InnerPadding);

		if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(HighlightFrame->SetContent(ExistingContent)))
		{
			BorderSlot->SetHorizontalAlignment(HAlign_Center);
			BorderSlot->SetVerticalAlignment(VAlign_Center);
			BorderSlot->SetPadding(InnerPadding);
		}

		if (UButtonSlot* NewBtnSlot = Cast<UButtonSlot>(Btn->SetContent(HighlightFrame)))
		{
			NewBtnSlot->SetHorizontalAlignment(HAlign_Fill);
			NewBtnSlot->SetVerticalAlignment(VAlign_Fill);
			NewBtnSlot->SetPadding(FMargin(OuterHorizontal, OuterVertical, OuterHorizontal, OuterVertical));
		}

		FButtonStyle SteadyBarStyle = Btn->GetStyle();
		SteadyBarStyle.SetHovered(SteadyBarStyle.Normal);
		SteadyBarStyle.SetPressed(SteadyBarStyle.Normal);
		SteadyBarStyle.SetNormalPadding(FMargin(12.0f, 0.0f, 12.0f, 0.0f));
		SteadyBarStyle.SetPressedPadding(FMargin(12.0f, 0.0f, 12.0f, 0.0f));
		Btn->SetStyle(SteadyBarStyle);

		MenuButtonHighlightBorders[i] = HighlightFrame;
	}
}

void UPlayBoardWidget::EnsureInputGuideWidget()
{
	if (IsDesignTime() || InputGuideText.IsValid() || !WidgetTree || !WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	if (!RootOverlay)
	{
		UWidget* OldRoot = WidgetTree->RootWidget;
		RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		if (RootOverlay)
		{
			RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			WidgetTree->RootWidget = RootOverlay;
			if (UOverlaySlot* OldRootSlot = RootOverlay->AddChildToOverlay(OldRoot))
			{
				OldRootSlot->SetHorizontalAlignment(HAlign_Fill);
				OldRootSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}

	if (!RootOverlay)
	{
		return;
	}

	UBorder* GuideBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	UTextBlock* GuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!GuideBorder || !GuideText)
	{
		return;
	}

	InputGuideText = GuideText;

	GuideBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	GuideBorder->SetPadding(FMargin(24.0f, 10.0f, 24.0f, 10.0f));
	GuideBorder->SetBrush(TrivialKartUIStyle::MakeRoundedHighlightBrush(
		12.0f,
		2.5f,
		FLinearColor(0.05f, 0.08f, 0.14f, 0.78f),
		FLinearColor(1.0f, 0.85f, 0.2f, 0.9f)));
	GuideBorder->SetBrushColor(FLinearColor::White);

	GuideText->SetVisibility(ESlateVisibility::HitTestInvisible);
	FSlateFontInfo GuideFont = CoinText ? CoinText->GetFont() : GuideText->GetFont();
	GuideFont.Size = 22;
	GuideText->SetFont(GuideFont);
	GuideText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.96f, 0.82f, 1.0f)));
	GuideText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	GuideText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	GuideText->SetJustification(ETextJustify::Center);

	GuideBorder->SetContent(GuideText);

	if (UOverlaySlot* BannerSlot = RootOverlay->AddChildToOverlay(GuideBorder))
	{
		BannerSlot->SetHorizontalAlignment(HAlign_Center);
		BannerSlot->SetVerticalAlignment(VAlign_Top);
		BannerSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
	}

	USizeBox* PadSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UBorder* PadBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	UVerticalBox* PadVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UImage* HandIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	UTextBlock* PadLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

	if (PadSizeBox && PadBorder && PadVBox && HandIconImage && PadLabel)
	{
		TouchDrivePadSizeBox = PadSizeBox;
		TouchDrivePadBorder = PadBorder;

		PadSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		PadSizeBox->SetWidthOverride(246.0f);
		PadSizeBox->SetHeightOverride(246.0f);

		PadBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		PadBorder->SetHorizontalAlignment(HAlign_Center);
		PadBorder->SetVerticalAlignment(VAlign_Center);
		PadBorder->SetPadding(FMargin(18.0f));

		PadBorder->SetBrush(TrivialKartUIStyle::MakeRoundedHighlightBrush(
			123.0f,
			4.5f,
			FLinearColor(0.04f, 0.07f, 0.14f, 0.72f),
			FLinearColor(1.0f, 0.85f, 0.2f, 0.9f)));
		PadBorder->SetBrushColor(FLinearColor::White);

		HandIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (const APlayerController* PC = GetOwningPlayer())
		{
			if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(PC->GetHUD()))
			{
				if (UTexture2D* HandTex = CurrentHUD->GetOrCreateTouchHandTexture())
				{
					HandIconImage->SetBrushFromTexture(HandTex, false);
				}
			}
		}
		HandIconImage->SetDesiredSizeOverride(FVector2D(114.0f, 114.0f));

		PadLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		FSlateFontInfo PadFont = CoinText ? CoinText->GetFont() : PadLabel->GetFont();
		PadFont.Size = 20;
		PadLabel->SetFont(PadFont);
		PadLabel->SetText(NSLOCTEXT("TrivialKart", "HoldToDrive", "HOLD TO\nDRIVE"));
		PadLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.78f, 1.0f)));
		PadLabel->SetShadowOffset(FVector2D(1.5f, 1.5f));
		PadLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		PadLabel->SetJustification(ETextJustify::Center);

		if (UVerticalBoxSlot* IconSlot = PadVBox->AddChildToVerticalBox(HandIconImage))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}
		if (UVerticalBoxSlot* LabelSlot = PadVBox->AddChildToVerticalBox(PadLabel))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}

		PadBorder->SetContent(PadVBox);
		PadSizeBox->SetContent(PadBorder);

		if (UOverlaySlot* PadOverlaySlot = RootOverlay->AddChildToOverlay(PadSizeBox))
		{
			PadOverlaySlot->SetHorizontalAlignment(HAlign_Right);
			PadOverlaySlot->SetVerticalAlignment(VAlign_Center);
			PadOverlaySlot->SetPadding(FMargin(0.0f, 0.0f, 48.0f, 0.0f));
		}
	}
}

void UPlayBoardWidget::UpdateInputGuideText(ETrivialKartInputMode InputMode)
{
	if (InputMode == ETrivialKartInputMode::Gamepad)
	{
		if (bFocusedByHover)
		{
			ClearMenuFocus(false);
		}
	}
	else if (InputMode == ETrivialKartInputMode::Touch)
	{
		ClearMenuFocus(false);
	}
	else if (InputMode == ETrivialKartInputMode::KeyboardMouse)
	{
		const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer());
		if (PC && !PC->IsKeyNavigationActive() && !bFocusedByHover)
		{
			ClearMenuFocus(true);
		}
	}

	EnsureInputGuideWidget();
	if (!InputGuideText.IsValid())
	{
		return;
	}

	const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer());
	const ETrivialKartGamepadStyle GamepadStyle = PC ? PC->GetActiveGamepadStyle() : ETrivialKartGamepadStyle::GenericXbox;

	if (LastRenderedGuideMode.IsSet() && LastRenderedGuideMode.GetValue() == InputMode &&
		(InputMode != ETrivialKartInputMode::Gamepad ||
		 (LastRenderedGamepadStyle.IsSet() && LastRenderedGamepadStyle.GetValue() == GamepadStyle)))
	{
		return;
	}
	LastRenderedGuideMode = InputMode;
	LastRenderedGamepadStyle = GamepadStyle;

	const ESlateVisibility TouchPadVisibility = (InputMode == ETrivialKartInputMode::Touch)
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	if (TouchDrivePadSizeBox.IsValid())
	{
		TouchDrivePadSizeBox->SetVisibility(TouchPadVisibility);
	}
	if (TouchDrivePadBorder.IsValid())
	{
		TouchDrivePadBorder->SetVisibility(TouchPadVisibility);
	}

	switch (InputMode)
	{
	case ETrivialKartInputMode::Touch:
		InputGuideText->SetText(NSLOCTEXT("TrivialKart", "GuideTouch", "Touch and hold to drive"));
		break;
	case ETrivialKartInputMode::Gamepad:
		switch (GamepadStyle)
		{
		case ETrivialKartGamepadStyle::PlayStation:
			InputGuideText->SetText(NSLOCTEXT("TrivialKart", "GuideGamepadPS", "Hold Trigger, X, or D-Pad Right to drive"));
			break;
		case ETrivialKartGamepadStyle::NintendoSwitch:
			InputGuideText->SetText(NSLOCTEXT("TrivialKart", "GuideGamepadSwitch", "Hold Trigger, B, or D-Pad Right to drive"));
			break;
		default:
			InputGuideText->SetText(NSLOCTEXT("TrivialKart", "GuideGamepad", "Hold Trigger, A, or D-Pad Right to drive"));
			break;
		}
		break;
	case ETrivialKartInputMode::KeyboardMouse:
		InputGuideText->SetText(NSLOCTEXT("TrivialKart", "GuideKeyboardMouse", "Hold Spacebar or Click to drive"));
		break;
	}
}

void UPlayBoardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LastRenderedGuideMode.Reset();
	LastRenderedGamepadStyle.Reset();
	EnsureInputGuideWidget();
	EnsureMenuButtonHighlightFrames();

	// If RebuildWidget ran before HUD was ready, ensure the cached hand texture is assigned now
	if (TouchDrivePadBorder.IsValid())
	{
		if (UVerticalBox* PadVBox = Cast<UVerticalBox>(TouchDrivePadBorder->GetContent()))
		{
			if (UImage* HandIconImage = Cast<UImage>(PadVBox->GetChildAt(0)))
			{
				if (!HandIconImage->GetBrush().GetResourceObject())
				{
					if (const APlayerController* PC = GetOwningPlayer())
					{
						if (ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(PC->GetHUD()))
						{
							if (UTexture2D* HandTex = CurrentHUD->GetOrCreateTouchHandTexture())
							{
								HandIconImage->SetBrushFromTexture(HandTex, false);
							}
						}
					}
				}
			}
		}
	}

	TrivialKartUIStyle::NormalizeWidgetTreeHitTesting(WidgetTree);

	if (ATrivialKartPlayerController* PlayerController = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PlayerController->OnInputModeUpdated.RemoveAll(this);
		PlayerController->OnInputModeUpdated.AddUObject(this, &UPlayBoardWidget::UpdateInputGuideText);
		UpdateInputGuideText(PlayerController->GetActiveInputMode());
	}
#if PLATFORM_ANDROID
	else
	{
		UpdateInputGuideText(ETrivialKartInputMode::Touch);
	}
#else
	else
	{
		UpdateInputGuideText(ETrivialKartInputMode::KeyboardMouse);
	}
#endif

	if (GarageButton)
	{
		GarageButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnGarageButtonClicked);
		GarageButton->OnHovered.AddUniqueDynamic(this, &ThisClass::OnGarageButtonHovered);
		GarageButton->OnUnhovered.AddUniqueDynamic(this, &ThisClass::OnMenuButtonUnhovered);
		GarageButton->OnPressed.AddUniqueDynamic(this, &ThisClass::OnGarageButtonPressed);
		GarageButton->OnReleased.AddUniqueDynamic(this, &ThisClass::OnMenuButtonReleased);
	}
	if (PGSButton)
	{
		PGSButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnPGSButtonClicked);
		PGSButton->OnHovered.AddUniqueDynamic(this, &ThisClass::OnPGSButtonHovered);
		PGSButton->OnUnhovered.AddUniqueDynamic(this, &ThisClass::OnMenuButtonUnhovered);
		PGSButton->OnPressed.AddUniqueDynamic(this, &ThisClass::OnPGSButtonPressed);
		PGSButton->OnReleased.AddUniqueDynamic(this, &ThisClass::OnMenuButtonReleased);
	}
	if (StoreButton)
	{
		StoreButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnStoreButtonClicked);
		StoreButton->OnHovered.AddUniqueDynamic(this, &ThisClass::OnStoreButtonHovered);
		StoreButton->OnUnhovered.AddUniqueDynamic(this, &ThisClass::OnMenuButtonUnhovered);
		StoreButton->OnPressed.AddUniqueDynamic(this, &ThisClass::OnStoreButtonPressed);
		StoreButton->OnReleased.AddUniqueDynamic(this, &ThisClass::OnMenuButtonReleased);
	}

	FocusedButtonIndex = -1;
	PressedButtonIndex = -1;
	bFocusedByHover = false;
	UpdateButtonFocusVisuals();

	if (TWeakObjectPtr<ATrivialKartPlayerState> PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); PlayerState.IsValid())
	{
		PlayerState->OnFuelUpdated.BindUObject(this, &UPlayBoardWidget::UpdateFuelBar);
		PlayerState->OnDistanceUpdated.BindUObject(this, &UPlayBoardWidget::UpdateDistanceText);
		PlayerState->OnCoinUpdated.BindUObject(this, &UPlayBoardWidget::UpdateCoinText);
		const float FuelPercentage = PlayerState->GetFuel() / 100.0f;
		UpdateFuelBar(FuelPercentage);
		UpdateDistanceText(PlayerState->GetDistance());
		UpdateCoinText(PlayerState->GetCoinCount());
	}
}

void UPlayBoardWidget::NativeDestruct()
{
	if (GarageButton)
	{
		GarageButton->OnClicked.RemoveDynamic(this, &ThisClass::OnGarageButtonClicked);
		GarageButton->OnHovered.RemoveDynamic(this, &ThisClass::OnGarageButtonHovered);
		GarageButton->OnUnhovered.RemoveDynamic(this, &ThisClass::OnMenuButtonUnhovered);
		GarageButton->OnPressed.RemoveDynamic(this, &ThisClass::OnGarageButtonPressed);
		GarageButton->OnReleased.RemoveDynamic(this, &ThisClass::OnMenuButtonReleased);
	}
	if (PGSButton)
	{
		PGSButton->OnClicked.RemoveDynamic(this, &ThisClass::OnPGSButtonClicked);
		PGSButton->OnHovered.RemoveDynamic(this, &ThisClass::OnPGSButtonHovered);
		PGSButton->OnUnhovered.RemoveDynamic(this, &ThisClass::OnMenuButtonUnhovered);
		PGSButton->OnPressed.RemoveDynamic(this, &ThisClass::OnPGSButtonPressed);
		PGSButton->OnReleased.RemoveDynamic(this, &ThisClass::OnMenuButtonReleased);
	}
	if (StoreButton)
	{
		StoreButton->OnClicked.RemoveDynamic(this, &ThisClass::OnStoreButtonClicked);
		StoreButton->OnHovered.RemoveDynamic(this, &ThisClass::OnStoreButtonHovered);
		StoreButton->OnUnhovered.RemoveDynamic(this, &ThisClass::OnMenuButtonUnhovered);
		StoreButton->OnPressed.RemoveDynamic(this, &ThisClass::OnStoreButtonPressed);
		StoreButton->OnReleased.RemoveDynamic(this, &ThisClass::OnMenuButtonReleased);
	}

	if (ATrivialKartPlayerController* PlayerController = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PlayerController->OnInputModeUpdated.RemoveAll(this);
	}
	
	if (TWeakObjectPtr<ATrivialKartPlayerState> PlayerState = Cast<ATrivialKartPlayerState>(GetOwningPlayerState()); PlayerState.IsValid())
	{
		if (PlayerState->OnFuelUpdated.IsBoundToObject(this))
		{
			PlayerState->OnFuelUpdated.Unbind();
		}
		if (PlayerState->OnDistanceUpdated.IsBoundToObject(this))
		{
			PlayerState->OnDistanceUpdated.Unbind();
		}
		if (PlayerState->OnCoinUpdated.IsBoundToObject(this))
		{
			PlayerState->OnCoinUpdated.Unbind();
		}
	}
	Super::NativeDestruct();
}

bool UPlayBoardWidget::IsAnyMenuButtonHovered() const
{
	return (GarageButton && GarageButton->IsHovered())
		|| (PGSButton && PGSButton->IsHovered())
		|| (StoreButton && StoreButton->IsHovered());
}

bool UPlayBoardWidget::IsPointOverAnyMenuButton(const FVector2D& ScreenSpacePosition) const
{
	const UButton* Buttons[3] = { GarageButton.Get(), PGSButton.Get(), StoreButton.Get() };
	for (const UButton* Btn : Buttons)
	{
		if (Btn && Btn->IsVisible() && Btn->GetCachedGeometry().IsUnderLocation(ScreenSpacePosition))
		{
			return true;
		}
	}
	return false;
}

void UPlayBoardWidget::SetHoverFocus(int32 NewButtonIndex)
{
	const bool bWasUnfocused = (FocusedButtonIndex < 0);
	FocusedButtonIndex = NewButtonIndex;
	bFocusedByHover = true;
	UpdateButtonFocusVisuals();

	if (bWasUnfocused && FocusedButtonIndex >= 0)
	{
		if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
		{
			PC->LatchStickUntilNeutral();
		}
	}
}

void UPlayBoardWidget::SetKeyboardOrGamepadFocus(int32 NewButtonIndex)
{
	const bool bNeedStickLatch = (FocusedButtonIndex < 0) || bFocusedByHover;
	PressedButtonIndex = -1;
	FocusedButtonIndex = NewButtonIndex;
	bFocusedByHover = false;
	UpdateButtonFocusVisuals();

	if (bNeedStickLatch && FocusedButtonIndex >= 0)
	{
		if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
		{
			PC->LatchStickUntilNeutral();
		}
	}
}

void UPlayBoardWidget::OnGarageButtonHovered()
{
	SetHoverFocus(0);
}

void UPlayBoardWidget::OnPGSButtonHovered()
{
	SetHoverFocus(1);
}

void UPlayBoardWidget::OnStoreButtonHovered()
{
	SetHoverFocus(2);
}

void UPlayBoardWidget::OnMenuButtonUnhovered()
{
	if (!IsAnyMenuButtonHovered() && bFocusedByHover && PressedButtonIndex < 0)
	{
		ClearMenuFocus(false);
	}
}

void UPlayBoardWidget::OnGarageButtonPressed()
{
	FocusedButtonIndex = 0;
	PressedButtonIndex = 0;
	bFocusedByHover = true;
	UpdateButtonFocusVisuals();
}

void UPlayBoardWidget::OnPGSButtonPressed()
{
	FocusedButtonIndex = 1;
	PressedButtonIndex = 1;
	bFocusedByHover = true;
	UpdateButtonFocusVisuals();
}

void UPlayBoardWidget::OnStoreButtonPressed()
{
	FocusedButtonIndex = 2;
	PressedButtonIndex = 2;
	bFocusedByHover = true;
	UpdateButtonFocusVisuals();
}

void UPlayBoardWidget::OnMenuButtonReleased()
{
	PressedButtonIndex = -1;
	if (bFocusedByHover && !IsAnyMenuButtonHovered())
	{
		ClearMenuFocus(false);
	}
	else
	{
		UpdateButtonFocusVisuals();
	}

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->RestoreFocusToGameViewport();
	}
}

void UPlayBoardWidget::ClearMenuFocus(bool bOnlyIfNotHovered)
{
	if (bOnlyIfNotHovered && IsAnyMenuButtonHovered())
	{
		return;
	}
	const bool bHadFocus = (FocusedButtonIndex >= 0 || PressedButtonIndex >= 0);
	if (bHadFocus)
	{
		FocusedButtonIndex = -1;
		PressedButtonIndex = -1;
		bFocusedByHover = false;
		UpdateButtonFocusVisuals();
	}
	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		if (bHadFocus)
		{
			PC->LatchStickUntilNeutral();
		}
		PC->RestoreFocusToGameViewport();
	}
}

void UPlayBoardWidget::CancelPendingPress()
{
	if (PressedButtonIndex >= 0)
	{
		PressedButtonIndex = -1;
		UpdateButtonFocusVisuals();
	}
}

void UPlayBoardWidget::OnViewportPointerDown(const FVector2D& ScreenSpacePosition)
{
	if (!IsPointOverAnyMenuButton(ScreenSpacePosition))
	{
		ClearMenuFocus(false);
	}
}

bool UPlayBoardWidget::HandleGamepadKey(const FKey& Key, bool bIsPressed, bool bIsRepeat)
{
	if (!bIsPressed)
	{
		if (Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			if (PressedButtonIndex >= 0 && PressedButtonIndex == FocusedButtonIndex && FocusedButtonIndex <= 2)
			{
				const int32 TargetButton = FocusedButtonIndex;
				PressedButtonIndex = -1;
				UpdateButtonFocusVisuals();
				if (TargetButton == 0)
				{
					OnGarageButtonClicked();
				}
				else if (TargetButton == 1)
				{
					OnPGSButtonClicked();
				}
				else if (TargetButton == 2)
				{
					OnStoreButtonClicked();
				}
				return true;
			}
			CancelPendingPress();
		}
		return false;
	}

	if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_DPad_Up)
	{
		if (!bIsRepeat)
		{
			if (FocusedButtonIndex < 0 || bFocusedByHover)
			{
				SetKeyboardOrGamepadFocus(FocusedButtonIndex >= 0 ? FocusedButtonIndex : 0);
			}
			else
			{
				ClearMenuFocus(false);
			}
		}
		return true;
	}

	if (Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (FocusedButtonIndex >= 0)
		{
			ClearMenuFocus(false);
			return true;
		}
		return false;
	}

	if (Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (FocusedButtonIndex >= 0 && FocusedButtonIndex <= 2 && !bFocusedByHover)
		{
			PressedButtonIndex = FocusedButtonIndex;
			UpdateButtonFocusVisuals();
			return true;
		}
		return false;
	}

	if (Key == EKeys::Gamepad_LeftShoulder)
	{
		SetKeyboardOrGamepadFocus((FocusedButtonIndex < 0 || bFocusedByHover) ? 0 : (FocusedButtonIndex + 2) % 3);
		return true;
	}
	if (Key == EKeys::Gamepad_RightShoulder)
	{
		SetKeyboardOrGamepadFocus((FocusedButtonIndex < 0 || bFocusedByHover) ? 0 : (FocusedButtonIndex + 1) % 3);
		return true;
	}

	if (Key == EKeys::Gamepad_DPad_Left)
	{
		if (FocusedButtonIndex >= 0 && !bFocusedByHover)
		{
			SetKeyboardOrGamepadFocus((FocusedButtonIndex + 2) % 3);
		}
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Right)
	{
		if (FocusedButtonIndex >= 0 && !bFocusedByHover)
		{
			SetKeyboardOrGamepadFocus((FocusedButtonIndex + 1) % 3);
			return true;
		}
		if (bFocusedByHover)
		{
			ClearMenuFocus(false);
		}
		return false;
	}

	return false;
}

void UPlayBoardWidget::UpdateButtonFocusVisuals()
{
	UButton* Buttons[3] = { GarageButton.Get(), PGSButton.Get(), StoreButton.Get() };

	for (int32 i = 0; i < 3; ++i)
	{
		UButton* Btn = Buttons[i];
		UBorder* HighlightFrame = MenuButtonHighlightBorders[i].Get();
		if (!Btn || !HighlightFrame)
		{
			continue;
		}

		if (i == FocusedButtonIndex)
		{
			const bool bIsPressed = (i == PressedButtonIndex) || Btn->IsPressed();
			HighlightFrame->SetBrush(TrivialKartUIStyle::MakeButtonFocusBrush(bIsPressed, 16.0f));
			HighlightFrame->SetBrushColor(FLinearColor::White);
			const float TargetScale = bIsPressed ? 0.95f : 1.02f;
			HighlightFrame->SetRenderScale(FVector2D(TargetScale, TargetScale));
		}
		else
		{
			HighlightFrame->SetBrush(TrivialKartUIStyle::GetEmptyBrush());
			HighlightFrame->SetBrushColor(FLinearColor::Transparent);
			HighlightFrame->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATrivialKartHUD* CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()))
		{
			CurrentHUD->SyncGameplayInputContext();
		}
	}
}

void UPlayBoardWidget::OnGarageButtonClicked()
{
	if (TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr<ATrivialKartHUD> CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::Garage);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PlayBoard);
		}
	}
}

void UPlayBoardWidget::OnPGSButtonClicked()
{
	if (TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr<ATrivialKartHUD> CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::PGS);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PlayBoard);
		}
	}
}

void UPlayBoardWidget::OnStoreButtonClicked()
{
	if (TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr<ATrivialKartHUD> CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::Store);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PlayBoard);
		}
	}
}

void UPlayBoardWidget::UpdateFuelBar(const float FuelBarPercentage) const
{
	if (FuelBar)
	{
		FuelBar->SetPercent(FuelBarPercentage);
	}
}

void UPlayBoardWidget::UpdateDistanceText(const float Distance) const
{
	if (DistanceText)
	{
		DistanceText->SetText(FText::FromString(FString::FromInt(FMath::RoundToInt(Distance))));
	}
}

void UPlayBoardWidget::UpdateCoinText(const int Quantity) const
{
	if (CoinText)
	{
		CoinText->SetText(FText::FromString(FString::FromInt(Quantity)));
	}
}
