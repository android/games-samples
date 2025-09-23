// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterUI.generated.h"

class UTextBlock;
/**
 *  Simple scoreboard UI for a first person shooter game
 */
UCLASS(abstract)
class SHOOTERGAME_API UShooterUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Team1Score;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Team2Score;
	
public:
	void UpdateScore(const uint8 TeamByte, const int32 Score) const;
};
