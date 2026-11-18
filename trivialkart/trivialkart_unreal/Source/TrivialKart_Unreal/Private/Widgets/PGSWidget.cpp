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

#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Widgets/PGSLayoutText.h"

void UPGSWidget::RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes)
{
	OutCheckBoxes.Add(AuthenticationCheckBox);
	OutCheckBoxes.Add(AchievementsCheckBox);
	OutCheckBoxes.Add(LeaderboardCheckBox);
}

void UPGSWidget::BindTabCheckBoxEvents()
{
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
}

void UPGSWidget::UnbindTabCheckBoxEvents()
{
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
}

void UPGSWidget::InitializeModalContent()
{
	if (TWeakObjectPtr<UTrivialKartGameInstance> Instance = Cast<UTrivialKartGameInstance>(GetGameInstance());
		Instance.IsValid() && AuthBox && PGSTextTemplate)
	{
		const FString AuthStatus = Instance->GetLoginStatus() ? FString("Logged In") : FString("Logged Out");
		if (UPGSLayoutText* PgsAuthStatusText = Cast<UPGSLayoutText>(CreateWidget(this, PGSTextTemplate)))
		{
			PgsAuthStatusText->SetPGSText("Auth Status", AuthStatus);
			AuthBox->AddChildToVerticalBox(PgsAuthStatusText);
		}
		if (UPGSLayoutText* PgsUsernameText = Cast<UPGSLayoutText>(CreateWidget(this, PGSTextTemplate)))
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
					for (const FOnlineAchievement& Achievement : Achievements)
					{
						if (UPGSLayoutText* PGSText = Cast<UPGSLayoutText>(CreateWidget(this, PGSTextTemplate)))
						{
							PGSText->SetPGSText(Achievement.Id, FString::Printf(TEXT("%.1f"), Achievement.Progress));
							AchievementsBox->AddChild(PGSText);
						}
					}
				}
			}
		}
	}
}

UScrollBox* UPGSWidget::GetActiveReadOnlyScrollBox() const
{
	if (ActiveTabIndex == 1)
	{
		return AchievementsBox.Get();
	}
	if (ActiveTabIndex == 2)
	{
		return LeaderboardBox.Get();
	}
	return nullptr;
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
