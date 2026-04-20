/*
* Copyright 2026 The Android Open Source Project
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */


#include "Actors/KartPawn.h"

#include "EnhancedInputComponent.h"
#include "PaperFlipbookComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "Infos/TrivialKartPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Objects/TrivialKartSaveGame.h"

// Sets default values
AKartPawn::AKartPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CollisionComponent->InitBoxExtent(FVector(100.0f, 32.0f, 50.0f));
	
	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("SpriteComponent"));
	if (Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Sprite->SetupAttachment(CollisionComponent);
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->SetGenerateOverlapEvents(false);
	}
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.0f;
	SpringArm->SocketOffset = FVector(400.0f, 400.0f, 0.0f);
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	Camera->bUsePawnControlRotation = false;
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(400.0f);

}

// Called when the game starts or when spawned
void AKartPawn::BeginPlay()
{
	Super::BeginPlay();
	if (const TWeakObjectPtr Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()); 
						Instance.IsValid())
	{
		if (TWeakObjectPtr Save = Instance->LoadGame(); Save.IsValid())
		{
			SwitchCar(Save->ActiveCarType);
		}
	}
}

void AKartPawn::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if (OtherActor->ActorHasTag("Finish"))
	{
		if (TWeakObjectPtr CurrentActor = 
			UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()); 
			CurrentActor.IsValid())
		{
			
			SetActorLocation(CurrentActor->GetActorLocation());
		}
	}
}

// Called to bind functionality to input
void AKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKartPawn::Move);
	}
}

void AKartPawn::SwitchCar(const FString& CarName)
{
	if (CarTypes.Contains(CarName))
	{
		Sprite->SetFlipbook(CarTypes[CarName]);
	}
}

void AKartPawn::Move()
{
	if (GetController() != nullptr)
	{
		if (TWeakObjectPtr CurrentPlayerState = 
			Cast<ATrivialKartPlayerState>(GetPlayerState()); CurrentPlayerState.IsValid())
		{
			if (CurrentPlayerState->GetFuel() <= 0.0f)
				return;
			AddActorLocalOffset(FVector(MoveSpeed * GetWorld()->DeltaTimeSeconds, 0.0f, 0.0f));
		
			CurrentPlayerState->ConsumeFuel(FuelConsumptionRate);
			const float Distance = FMath::Abs(MoveSpeed * GetWorld()->DeltaTimeSeconds)/100;
			CurrentPlayerState->AddDistance(Distance);
		}
	}
}

