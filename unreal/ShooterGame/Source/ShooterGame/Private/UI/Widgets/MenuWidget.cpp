// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/MenuWidget.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineExternalUIInterface.h"

void UMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	LogInButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnLogInButtonClicked);
	StartGameButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnStartGameButtonClicked);
	StoreButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnStoreButtonClicked);

	CheckIfLoginStatusCompleted();
}

void UMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();
	LogInButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnLogInButtonClicked);
	StartGameButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnStartGameButtonClicked);
	StoreButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnStoreButtonClicked);
}

void UMenuWidget::OnLogInButtonClicked()
{
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineExternalUIPtr ExternalUI = Subsystem->GetExternalUIInterface(); ExternalUI.IsValid())
		{
			FOnLoginUIClosedDelegate LoginDelegate;
			LoginDelegate.BindWeakLambda(this, [this](FUniqueNetIdPtr UniqueId, const int ControllerIndex, const FOnlineError& Error)
			{
				switch (Error.GetErrorResult())
				{
					case EOnlineErrorResult::Success:
						{
							UE_LOG(LogTemp, Log, TEXT("External Login Success for User: %s"), *UniqueId->ToString());
							CheckIfLoginStatusCompleted();
						}
					break;
					default:
						{
							const FString ErrorMessage = Error.ToLogString();
							UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
							ErrorText->SetText(FText::FromString(ErrorMessage));
						}
					break;
				}
			});
			if (const bool bStarted = ExternalUI->ShowLoginUI(0, true, false, LoginDelegate); !bStarted)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to start External Login UI."));
				ErrorText->SetText(FText::FromString("Error: Failed to Start External Login"));
			}
		}
	}
}

void UMenuWidget::OnStartGameButtonClicked()
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), LevelToLoad);
}

void UMenuWidget::OnStoreButtonClicked()
{
	PanelSwitcher->SetActiveWidgetIndex(2);
}

void UMenuWidget::CheckIfLoginStatusCompleted() const
{
	if (const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface())
		{
			if (IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn)
			{
				PanelSwitcher->SetActiveWidgetIndex(1);
				const FString NickName = IdentityInterface->GetPlayerNickname(0);
				if (NickName.IsEmpty())
					return;
				PlayerNameText->SetText(FText::FromString(NickName));
			}
		}
	}
}
