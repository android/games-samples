// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/Widgets/ShooterBulletCounterUI.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Kismet/KismetMaterialLibrary.h"

void UShooterBulletCounterUI::NativePreConstruct()
{
	Super::NativePreConstruct();
	BulletCounterDynamicMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), BulletCounterMaterial);
	if (BulletCounterDynamicMaterial)
	{
		BulletCountImage->SetBrushFromMaterial(BulletCounterDynamicMaterial);
	}
}

void UShooterBulletCounterUI::UpdateBulletCounter(const int32 MagazineSize, const int32 BulletCount)
{
	CurrentMagSize = MagazineSize;
	CurrentBulletCount = BulletCount;
	ESlateVisibility Visibility = CurrentMagSize > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	BulletCountImage->SetVisibility(ESlateVisibility::Visible);
	BulletCountImage->SetDesiredSizeOverride(FVector2D(CurrentMagSize * BulletThickness, BulletThickness));
	if (BulletCounterDynamicMaterial)
	{
		BulletCounterDynamicMaterial->SetScalarParameterValue("Bullet Max", CurrentMagSize);
		BulletCounterDynamicMaterial->SetScalarParameterValue("Bullet Count", CurrentBulletCount);
	}
}

void UShooterBulletCounterUI::Damaged(const float LifePercent)
{
	LifeBar->SetPercent(LifePercent);
	if (LifePercent < 1.0f && Damage)
	{
		PlayAnimation(Damage);
	}
}
