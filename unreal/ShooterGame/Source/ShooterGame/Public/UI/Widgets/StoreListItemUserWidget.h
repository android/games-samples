// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "StoreListItemUserWidget.generated.h"


UCLASS()
class UStoreItemData : public UObject
{
	GENERATED_BODY()

public:
	TSharedPtr<FOnlineStoreOffer> Item;
	
};
class UImage;
class UTextBlock;
/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UStoreListItemUserWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemBackground;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemName;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemPrice;

	UPROPERTY(EditAnywhere)
	FSlateBrush SelectionBrush;
	
	UPROPERTY(EditAnywhere)
	FSlateBrush DeselectionBrush;

	UPROPERTY()
	UStoreItemData* ItemData;

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
};
