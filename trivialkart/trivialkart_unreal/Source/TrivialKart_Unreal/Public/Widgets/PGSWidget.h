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
#include "Widgets/TrivialKartTabbedModalWidget.h"
#include "PGSWidget.generated.h"

class UCheckBox;
class UPGSLayoutText;
class UScrollBox;
class UVerticalBox;

/**
 * Play Games Services modal widget managing Authentication, Achievements, and Leaderboard tabs.
 */
UCLASS()
class TRIVIALKART_UNREAL_API UPGSWidget : public UTrivialKartTabbedModalWidget
{
	GENERATED_BODY()

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> AuthenticationCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> AchievementsCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCheckBox> LeaderboardCheckBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> AuthBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> AchievementsBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> LeaderboardBox;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UPGSLayoutText> PGSTextTemplate;

protected:
	virtual EWidgetType GetModalWidgetType() const override { return EWidgetType::PGS; }
	virtual void RegisterTabCheckBoxes(TArray<UCheckBox*>& OutCheckBoxes) override;
	virtual void BindTabCheckBoxEvents() override;
	virtual void UnbindTabCheckBoxEvents() override;
	virtual void InitializeModalContent() override;
	virtual UScrollBox* GetActiveReadOnlyScrollBox() const override;

private:
	UFUNCTION()
	void OnAuthCheckBoxStateChanged(bool bIsChecked);

	UFUNCTION()
	void OnAchievementBoxStateChange(bool bIsChecked);

	UFUNCTION()
	void OnLeaderboardBoxStateChange(bool bIsChecked);
};
