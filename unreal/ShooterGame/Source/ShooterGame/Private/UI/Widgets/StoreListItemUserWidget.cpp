// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/StoreListItemUserWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameInstance/ShooterPlatformGameInstance.h"

void UStoreListItemUserWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (ItemData = Cast<UStoreItemData>(ListItemObject); IsValid(ItemData))
	{
		ItemBackground->SetBrush(DeselectionBrush);
		ItemName->SetText(ItemData->Item->Title);
		ItemPrice->SetText(ItemData->Item->GetDisplayPrice());
	}
}

void UStoreListItemUserWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	if (bIsSelected && ItemData->Item->IsPurchaseable())
	{
		ItemBackground->SetBrush(SelectionBrush);
		if (const TWeakObjectPtr GameInstance = Cast<UShooterPlatformGameInstance>(GetWorld()->GetGameInstance()); GameInstance.IsValid())
		{
			GameInstance->StartPurchasing(ItemData->Item.ToSharedRef(), 1);
		}
	}
}
