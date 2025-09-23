// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/Widgets/ShooterUI.h"

#include "Components/TextBlock.h"

void UShooterUI::UpdateScore(const uint8 TeamByte, const int32 Score) const
{
	switch (TeamByte)
	{
	case 0:
		Team1Score->SetText(FText::FromString(FString::FromInt(Score)));
		break;
	case 1:
		Team2Score->SetText(FText::FromString(FString::FromInt(Score)));
		break;
	default:
		break;
	}
}
