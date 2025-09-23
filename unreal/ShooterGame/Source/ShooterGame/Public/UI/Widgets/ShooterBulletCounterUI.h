// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterBulletCounterUI.generated.h"

class UProgressBar;
class UImage;
/**
 *  Simple bullet counter UI widget for a first person shooter game
 */
UCLASS(abstract)
class SHOOTERGAME_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInstance> BulletCounterMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BulletCounterDynamicMaterial;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BulletCountImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LifeBar;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> Damage;

private:
	int32 CurrentMagSize;
	int32 CurrentBulletCount;
	UPROPERTY(EditAnywhere)
	float BulletThickness;
	
public:

	virtual void NativePreConstruct() override;

	void UpdateBulletCounter(const int32 MagazineSize, const int32 BulletCount);

	void Damaged(const float LifePercent);
};
