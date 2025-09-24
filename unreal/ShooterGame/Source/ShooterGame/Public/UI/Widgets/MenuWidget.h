// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuWidget.generated.h"

class UTextBlock;
class UButton;
class UWidgetSwitcher;
/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta=(BindWidget))
	UWidgetSwitcher* PanelSwitcher;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ErrorText;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> LogInButton;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartGameButton;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StoreButton;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> LevelToLoad;

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnLogInButtonClicked();
	UFUNCTION()
	void OnStartGameButtonClicked();
	UFUNCTION()
	void OnStoreButtonClicked();
	void SetPlayerData() const;
	
};
