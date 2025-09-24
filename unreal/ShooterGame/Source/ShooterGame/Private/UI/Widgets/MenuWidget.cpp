// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/MenuWidget.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameInstance/ShooterPlatformGameInstance.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "UI/Widgets/StoreListItemUserWidget.h"

void UMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	LogInButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnLogInButtonClicked);
	StartGameButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnStartGameButtonClicked);
	StoreButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnStoreButtonClicked);
	BackButton->OnClicked.AddUniqueDynamic(this, &UMenuWidget::OnBackButtonClicked);

	SetPlayerData();
}

void UMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();
	LogInButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnLogInButtonClicked);
	StartGameButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnStartGameButtonClicked);
	StoreButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnStoreButtonClicked);
	BackButton->OnClicked.RemoveDynamic(this, &UMenuWidget::OnBackButtonClicked);
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
				if (Error.GetErrorResult() == EOnlineErrorResult::Success)
				{
					UE_LOG(LogTemp, Log, TEXT("External Login Success for User: %s"), *UniqueId->ToString());
					SetPlayerData();
				}
				else
				{
					const FString ErrorMessage = Error.ToLogString();
					UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
					ErrorText->SetText(FText::FromString(ErrorMessage));
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

void UMenuWidget::OnBackButtonClicked()
{
	PanelSwitcher->SetActiveWidgetIndex(1);
}

void UMenuWidget::SetPlayerData() const
{
	if (const TWeakObjectPtr GameInstance = Cast<UShooterPlatformGameInstance>(GetWorld()->GetGameInstance()); GameInstance.IsValid())
	{
		if (GameInstance->IsLoggedInToOnlineSubsystem())
		{
			PanelSwitcher->SetActiveWidgetIndex(1);
			const FString NickName = GameInstance->GetPlayerName();
			if (NickName.IsEmpty())
				return;
			PlayerNameText->SetText(FText::FromString(NickName));
			TArray<UStoreItemData*> StoreItems;
			for (const FOnlineStoreOfferRef Offer : GameInstance->StoreOffers)
			{
				UStoreItemData Data = UStoreItemData();
				Data.Item = Offer;
				StoreItems.Add(&Data);
			}
			if (StoreItems.IsEmpty())
				return;
			StoreList->SetListItems(StoreItems);
		}
	}
}
