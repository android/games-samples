// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class ACharacter;
class UPrimitiveComponent;

/**
 *  Simple projectile class for a first person shooter game
 */
UCLASS(abstract)
class SHOOTERGAME_API AShooterProjectile : public AActor
{
	GENERATED_BODY()
	
	/** Provides collision detection for the projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** Handles movement for the projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

protected:

	/** Loudness of the AI perception noise done by this projectile on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Noise", meta = (ClampMin = 0, ClampMax = 100))
	float NoiseLoudness = 3.0f;

	/** Range of the AI perception noise done by this projectile on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Noise", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float NoiseRange = 3000.0f;

	/** Tag of the AI perception noise done by this projectile on hit */
	UPROPERTY(EditAnywhere, Category="Noise")
	FName NoiseTag = FName("Projectile");

	/** Physics force to apply on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit", meta = (ClampMin = 0, ClampMax = 50000))
	float PhysicsForce = 100.0f;

	/** Damage to apply on hit */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit", meta = (ClampMin = 0, ClampMax = 100))
	float HitDamage = 25.0f;

	/** Type of damage to apply. Can be used to represent specific types of damage such as fire, explosion, etc. */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit")
	TSubclassOf<UDamageType> HitDamageType;

	/** If true, the projectile can damage the character that shot it */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit")
	bool bDamageOwner = false;

	/** If true, the projectile will explode and apply radial damage to all actors in range */
	UPROPERTY(EditAnywhere, Category="Projectile|Explosion")
	bool bExplodeOnHit = false;

	/** Max distance for actors to be affected by explosion damage */
	UPROPERTY(EditAnywhere, Category="Projectile|Explosion", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float ExplosionRadius = 500.0f;	

	/** If true, this projectile has already hit another surface */
	bool bHit = false;

	/** How long to wait after a hit before destroying this projectile */
	UPROPERTY(EditAnywhere, Category="Projectile|Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float DeferredDestructionTime = 5.0f;

	/** Timer to handle deferred destruction of this projectile */
	FTimerHandle DestructionTimer;

public:	

	/** Constructor */
	AShooterProjectile();

protected:
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Handles collision */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

protected:

	/** Looks up actors within the explosion radius and damages them */
	void ExplosionCheck(const FVector& ExplosionCenter);

	/** Processes a projectile hit for the given actor */
	void ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection);

	/** Passes control to Blueprint to implement any effects on hit. */
	UFUNCTION(BlueprintImplementableEvent, Category="Projectile", meta = (DisplayName = "On Projectile Hit"))
	void BP_OnProjectileHit(const FHitResult& Hit);

	/** Called from the destruction timer to destroy this projectile */
	void OnDeferredDestruction();

};
