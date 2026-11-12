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

#include "Widgets/PGSWidget.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Actors/TrivialKartHUD.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Widgets/PGSLayoutText.h"

static UBorder* EnsurePGSTabHighlightBorder(UWidgetTree* WidgetTree, UCheckBox* CB)
{
	UOverlay* ParentOverlay = CB ? Cast<UOverlay>(CB->GetParent()) : nullptr;
	if (!WidgetTree || !ParentOverlay)
	{
		return nullptr;
	}

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Border->SetVisibility(ESlateVisibility::Collapsed);
	Border->SetPadding(FMargin(0.0f));
	if (UOverlaySlot* OverlaySlot = ParentOverlay->AddChildToOverlay(Border))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	return Border;
}

void UPGSWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Keep the modal root hit-testable so empty modal space blocks clicks from piercing into FSceneViewport
	SetVisibility(ESlateVisibility::Visible);

	if (WidgetTree)
	{
		UWidget* TreeRoot = WidgetTree->RootWidget;
		WidgetTree->ForEachWidget([TreeRoot](UWidget* Widget)
		{
			if (!Widget || Widget == TreeRoot || Widget->GetVisibility() != ESlateVisibility::Visible)
			{
				return;
			}
			if (Cast<UImage>(Widget) || Cast<UTextBlock>(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else if (Cast<UOverlay>(Widget) || Cast<UHorizontalBox>(Widget) || Cast<UVerticalBox>(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		});
	}

	if (BackButton)
	{
		if (!bBackStyleCached)
		{
			OriginalBackButtonStyle = BackButton->GetStyle();
			OriginalBackButtonBgColor = BackButton->GetBackgroundColor();
			bBackStyleCached = true;
		}
		BackButton->OnClicked.AddUniqueDynamic(this, &UPGSWidget::OnBackButtonClicked);
		BackButton->OnReleased.AddUniqueDynamic(this, &UPGSWidget::OnBackButtonReleased);
	}

	CheckBoxes.Empty();
	CheckBoxes.Add(AuthenticationCheckBox);
	CheckBoxes.Add(AchievementsCheckBox);
	CheckBoxes.Add(LeaderboardCheckBox);

	if (TabHighlightBorders.Num() != CheckBoxes.Num())
	{
		TabHighlightBorders.Empty();
		for (UCheckBox* CB : CheckBoxes)
		{
			TabHighlightBorders.Add(EnsurePGSTabHighlightBorder(WidgetTree, CB));
		}
	}

	if (AuthenticationCheckBox)
	{
		AuthenticationCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPGSWidget::OnAuthCheckBoxStateChanged);
	}
	if (AchievementsCheckBox)
	{
		AchievementsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPGSWidget::OnAchievementBoxStateChange);
	}
	if (LeaderboardCheckBox)
	{
		LeaderboardCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPGSWidget::OnLeaderboardBoxStateChange);
	}

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->OnInputModeUpdated.RemoveAll(this);
		PC->OnInputModeUpdated.AddUObject(this, &UPGSWidget::OnInputModeChanged);
	}

	if (TWeakObjectPtr<UTrivialKartGameInstance> Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
		Instance.IsValid() && AuthBox && PGSTextTemplate)
	{
		const FString AuthStatus = Instance->GetLoginStatus() ? FString("Logged In") : FString("Logged Out");
		if (const auto PgsAuthStatusText = Cast<UPGSLayoutText>(
							CreateWidget(this, PGSTextTemplate)))
		{
			PgsAuthStatusText->SetPGSText("Auth Status", AuthStatus);
			AuthBox->AddChildToVerticalBox(PgsAuthStatusText);
		}
		if (const auto PgsUsernameText = Cast<UPGSLayoutText>(
							CreateWidget(this, PGSTextTemplate)))
		{
			PgsUsernameText->SetPGSText("Username", Instance->GetPlayerName());
			AuthBox->AddChildToVerticalBox(PgsUsernameText);
		}
	}
	if (const IOnlineIdentityPtr IdentityInterface = Online::GetIdentityInterface(GetWorld()); 
		IdentityInterface.IsValid() && AchievementsBox && PGSTextTemplate)
	{
		if (const TSharedPtr<const FUniqueNetId> UniquePlayerId = IdentityInterface->GetUniquePlayerId(0); UniquePlayerId.IsValid())
		{
			if (const IOnlineAchievementsPtr AchievementsInterface = 
				Online::GetAchievementsInterface(GetWorld()); AchievementsInterface.IsValid())
			{
				TArray<FOnlineAchievement> Achievements;
				if (AchievementsInterface->GetCachedAchievements(*UniquePlayerId, Achievements) 
					== EOnlineCachedResult::Success)
				{
					for (auto Achievement : Achievements)
					{
						if (auto PGSText = Cast<UPGSLayoutText>(
								CreateWidget(this, PGSTextTemplate)))
						{
							PGSText->SetPGSText(Achievement.Id, FString::Printf(TEXT("%.1f"), Achievement.Progress));
							AchievementsBox->AddChild(PGSText);
						}
					}
				}
			}
		}
	}

	bBackButtonFocused = false;
	bBackButtonPressed = false;
	ActiveTabIndex = ContentSwitcher ? ContentSwitcher->GetActiveWidgetIndex() : 0;
	UpdateTabFocusVisuals();
}

void UPGSWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UPGSWidget::OnBackButtonClicked);
		BackButton->OnReleased.RemoveDynamic(this, &UPGSWidget::OnBackButtonReleased);
	}
	if (AuthenticationCheckBox)
	{
		AuthenticationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPGSWidget::OnAuthCheckBoxStateChanged);
	}
	if (AchievementsCheckBox)
	{
		AchievementsCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPGSWidget::OnAchievementBoxStateChange);
	}
	if (LeaderboardCheckBox)
	{
		LeaderboardCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPGSWidget::OnLeaderboardBoxStateChange);
	}

	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->OnInputModeUpdated.RemoveAll(this);
	}

	CheckBoxes.Empty();
	Super::NativeDestruct();
}

bool UPGSWidget::IsNavigationVisualActive() const
{
	if (const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		return PC->IsKeyNavigationActive();
	}
	return false;
}

void UPGSWidget::RestoreViewportFocus() const
{
	if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
	{
		PC->RestoreFocusToGameViewport();
	}
}

void UPGSWidget::OnInputModeChanged(ETrivialKartInputMode NewInputMode)
{
	if (!IsNavigationVisualActive())
	{
		CancelPendingPress();
	}
	UpdateTabFocusVisuals();
}

void UPGSWidget::CancelPendingPress()
{
	if (bBackButtonPressed)
	{
		bBackButtonPressed = false;
		UpdateTabFocusVisuals();
	}
}

void UPGSWidget::OnBackButtonReleased()
{
	RestoreViewportFocus();
}

void UPGSWidget::OnBackButtonClicked()
{
	if (TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr<ATrivialKartHUD> CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
			CurrentHUD.IsValid())
		{
			CurrentHUD->AddWidgetToScreen(EWidgetType::PlayBoard);
			CurrentHUD->RemoveWidgetFromScreen(EWidgetType::PGS);
		}
	}
}

void UPGSWidget::SetCheckBoxState(bool bIsChecked, UCheckBox* CheckBox, int ActiveWidgetIndex)
{
	if (CheckBox == nullptr)
	{
		return;
	}
	CancelPendingPress();
	bBackButtonFocused = false;
	if (bIsChecked)
	{
		ActiveTabIndex = ActiveWidgetIndex;
		if (ContentSwitcher)
		{
			ContentSwitcher->SetActiveWidgetIndex(ActiveWidgetIndex);
		}
		for (auto CurrentCheckBox : CheckBoxes)
		{
			if (CurrentCheckBox && CurrentCheckBox != CheckBox)
			{
				CurrentCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
			}
		}
	}
	else
	{
		CheckBox->SetCheckedState(ECheckBoxState::Checked);
	}
	UpdateTabFocusVisuals();
	RestoreViewportFocus();
}

void UPGSWidget::SelectTabByIndex(int32 NewTabIndex)
{
	if (CheckBoxes.Num() == 0)
	{
		return;
	}
	bBackButtonFocused = false;
	ActiveTabIndex = (NewTabIndex + CheckBoxes.Num()) % CheckBoxes.Num();
	if (UCheckBox* TargetCheckBox = CheckBoxes[ActiveTabIndex])
	{
		TargetCheckBox->SetCheckedState(ECheckBoxState::Checked);
		SetCheckBoxState(true, TargetCheckBox, ActiveTabIndex);
	}
}

void UPGSWidget::UpdateTabFocusVisuals()
{
	const bool bShowNavVisual = IsNavigationVisualActive();

	if (BackButton && bBackStyleCached)
	{
		const bool bBackActive = bShowNavVisual && bBackButtonFocused;
		BackButton->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (bBackActive)
		{
			const bool bPressed = bBackButtonPressed || BackButton->IsPressed();
			FSlateBrush HighlightBrush;
			HighlightBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			HighlightBrush.TintColor = FSlateColor(bPressed
				? FLinearColor(1.0f, 0.35f, 0.0f, 0.68f)
				: FLinearColor(1.0f, 0.45f, 0.0f, 0.48f));
			HighlightBrush.OutlineSettings.CornerRadii = FVector4(10.0f, 10.0f, 10.0f, 10.0f);
			HighlightBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			HighlightBrush.OutlineSettings.Color = FSlateColor(FLinearColor(1.0f, 0.9f, 0.0f, 1.0f));
			HighlightBrush.OutlineSettings.Width = 6.0f;

			FButtonStyle FocusedStyle = OriginalBackButtonStyle;
			FocusedStyle.SetNormal(HighlightBrush);
			FocusedStyle.SetHovered(HighlightBrush);
			FocusedStyle.SetPressed(HighlightBrush);
			BackButton->SetStyle(FocusedStyle);
			BackButton->SetBackgroundColor(FLinearColor::White);
			BackButton->SetRenderScale(FVector2D(bPressed ? 0.88f : 1.12f, bPressed ? 0.88f : 1.12f));
		}
		else
		{
			BackButton->SetStyle(OriginalBackButtonStyle);
			BackButton->SetBackgroundColor(OriginalBackButtonBgColor);
			BackButton->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	const bool bTabBarFocused = bShowNavVisual && !bBackButtonFocused;
	for (int32 i = 0; i < CheckBoxes.Num(); ++i)
	{
		UCheckBox* CB = CheckBoxes[i];
		UBorder* HighlightBorder = TabHighlightBorders.IsValidIndex(i) ? TabHighlightBorders[i].Get() : nullptr;
		if (!CB)
		{
			continue;
		}
		UWidget* TabScaleTarget = CB->GetParent()
			? static_cast<UWidget*>(CB->GetParent())
			: static_cast<UWidget*>(CB);
		TabScaleTarget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (i == ActiveTabIndex)
		{
			if (HighlightBorder)
			{
				FSlateBrush OverlayBrush;
				OverlayBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
				OverlayBrush.ImageSize = CB->GetWidgetStyle().CheckedImage.ImageSize;
				OverlayBrush.TintColor = FSlateColor(FLinearColor(1.0f, 0.45f, 0.0f, 0.48f));
				OverlayBrush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
				OverlayBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
				OverlayBrush.OutlineSettings.Color = FSlateColor(bTabBarFocused
					? FLinearColor(1.0f, 0.9f, 0.0f, 1.0f)
					: FLinearColor::Transparent);
				OverlayBrush.OutlineSettings.Width = bTabBarFocused ? 6.0f : 0.0f;
				HighlightBorder->SetBrush(OverlayBrush);
				HighlightBorder->SetBrushColor(FLinearColor::White);
				HighlightBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			const float TargetTabScale = bTabBarFocused ? 1.15f : 1.0f;
			TabScaleTarget->SetRenderScale(FVector2D(TargetTabScale, TargetTabScale));
		}
		else
		{
			if (HighlightBorder)
			{
				HighlightBorder->SetVisibility(ESlateVisibility::Collapsed);
			}
			TabScaleTarget->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}
}

bool UPGSWidget::HandleGamepadKey(const FKey& Key, bool bIsPressed)
{
	if (!bIsPressed)
	{
		if (Key == EKeys::Gamepad_FaceButton_Bottom && bBackButtonPressed)
		{
			bBackButtonPressed = false;
			UpdateTabFocusVisuals();
			if (bBackButtonFocused)
			{
				OnBackButtonClicked();
			}
			return true;
		}
		return false;
	}

	if (Key == EKeys::Gamepad_FaceButton_Right)
	{
		CancelPendingPress();
		OnBackButtonClicked();
		return true;
	}
	if (!IsNavigationVisualActive() &&
		(Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right ||
		 Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down ||
		 Key == EKeys::Gamepad_FaceButton_Bottom))
	{
		if (ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayer()))
		{
			PC->SetKeyNavigationActive(true);
		}
		UpdateTabFocusVisuals();
		return true;
	}
	if (Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (bBackButtonFocused && BackButton)
		{
			bBackButtonPressed = true;
			UpdateTabFocusVisuals();
		}
		return true;
	}
	if (Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Gamepad_DPad_Left)
	{
		if (bBackButtonFocused && Key == EKeys::Gamepad_DPad_Left)
		{
			return true;
		}
		SelectTabByIndex(ActiveTabIndex - 1);
		return true;
	}
	if (Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_DPad_Right)
	{
		if (bBackButtonFocused && Key == EKeys::Gamepad_DPad_Right)
		{
			return true;
		}
		SelectTabByIndex(ActiveTabIndex + 1);
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Up)
	{
		UScrollBox* TargetScroll = (ActiveTabIndex == 1) ? AchievementsBox.Get() : ((ActiveTabIndex == 2) ? LeaderboardBox.Get() : nullptr);
		if (!bBackButtonFocused && TargetScroll && TargetScroll->GetScrollOffset() > KINDA_SMALL_NUMBER)
		{
			const float MaxOffset = FMath::Max(0.0f, TargetScroll->GetScrollOffsetOfEnd());
			TargetScroll->SetScrollOffset(FMath::Clamp(TargetScroll->GetScrollOffset() - 60.0f, 0.0f, MaxOffset));
		}
		else if (!bBackButtonFocused && BackButton)
		{
			bBackButtonFocused = true;
			UpdateTabFocusVisuals();
		}
		return true;
	}
	if (Key == EKeys::Gamepad_DPad_Down)
	{
		if (bBackButtonFocused)
		{
			bBackButtonFocused = false;
			UpdateTabFocusVisuals();
			return true;
		}
		UScrollBox* TargetScroll = (ActiveTabIndex == 1) ? AchievementsBox.Get() : ((ActiveTabIndex == 2) ? LeaderboardBox.Get() : nullptr);
		if (TargetScroll)
		{
			const float MaxOffset = FMath::Max(0.0f, TargetScroll->GetScrollOffsetOfEnd());
			TargetScroll->SetScrollOffset(FMath::Clamp(TargetScroll->GetScrollOffset() + 60.0f, 0.0f, MaxOffset));
		}
		return true;
	}
	return false;
}

void UPGSWidget::OnAuthCheckBoxStateChanged(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, AuthenticationCheckBox, 0);
}

void UPGSWidget::OnAchievementBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, AchievementsCheckBox, 1);
}

void UPGSWidget::OnLeaderboardBoxStateChange(bool bIsChecked)
{
	SetCheckBoxState(bIsChecked, LeaderboardCheckBox, 2);
}
