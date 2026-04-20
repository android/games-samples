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
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Widgets/PGSLayoutText.h"

void UPGSWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BackButton->OnClicked.AddUniqueDynamic(this, &UPGSWidget::OnBackButtonClicked);
	CheckBoxes.Empty();
	CheckBoxes.Add(AuthenticationCheckBox);
	CheckBoxes.Add(AchievementsCheckBox);
	CheckBoxes.Add(LeaderboardCheckBox);
	AuthenticationCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPGSWidget::OnAuthCheckBoxStateChanged);
	AchievementsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPGSWidget::OnAchievementBoxStateChange);
	LeaderboardCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UPGSWidget::OnLeaderboardBoxStateChange);
	if (TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
		Instance.IsValid())
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
		IdentityInterface.IsValid())
	{
		if (const IOnlineAchievementsPtr AchievementsInterface = 
			Online::GetAchievementsInterface(GetWorld()); AchievementsInterface.IsValid())
		{
			TArray<FOnlineAchievement> Achievements;
			if (AchievementsInterface->GetCachedAchievements(
				*IdentityInterface->GetUniquePlayerId(0), Achievements) 
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

void UPGSWidget::NativeDestruct()
{
	BackButton->OnClicked.RemoveDynamic(this, &UPGSWidget::OnBackButtonClicked);
	AuthenticationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPGSWidget::OnAuthCheckBoxStateChanged);
	AchievementsCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPGSWidget::OnAchievementBoxStateChange);
	LeaderboardCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UPGSWidget::OnLeaderboardBoxStateChange);
	Super::NativeDestruct();
}

void UPGSWidget::OnBackButtonClicked()
{
	if (TWeakObjectPtr PlayerController = GetOwningPlayer(); PlayerController.IsValid())
	{
		if (const TWeakObjectPtr CurrentHUD = Cast<ATrivialKartHUD>(PlayerController->GetHUD()); 
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
		return;
	if (bIsChecked)
	{
		ContentSwitcher->SetActiveWidgetIndex(ActiveWidgetIndex);
		for (auto CurrentCheckBox : CheckBoxes)
		{
			if (CurrentCheckBox != CheckBox)
			{
				CurrentCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
			}
		}
	}
	else
	{
		CheckBox->SetCheckedState(ECheckBoxState::Checked);
	}
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
