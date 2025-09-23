// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterWeaponHolder.generated.h"

class AShooterWeapon;
class UAnimMontage;


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UShooterWeaponHolder : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Common interface for Shooter Game weapon holder classes
 */
class SHOOTERGAME_API IShooterWeaponHolder
{
	GENERATED_BODY()

public:

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) = 0;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) = 0;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) = 0;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) = 0;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() = 0;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) = 0;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) = 0;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) = 0;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() = 0;
};
