// Copyright Epic Games, Inc. All Rights Reserved.


#include "Controllers/ShooterAIController.h"
#include "Actors/ShooterNPC.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AI/Navigation/PathFollowingAgentInterface.h"

AShooterAIController::AShooterAIController()
{
	// create the StateTree component
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));

	// create the AI perception component. It will be configured in BP
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// subscribe to the AI perception delegates
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AShooterAIController::OnPerceptionUpdated);
	AIPerception->OnTargetPerceptionForgotten.AddDynamic(this, &AShooterAIController::OnPerceptionForgotten);
}

void AShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ensure we're possessing an NPC
	if (AShooterNPC* NPC = Cast<AShooterNPC>(InPawn))
	{
		// add the team tag to the pawn
		NPC->Tags.Add(TeamTag);

		// subscribe to the pawn's OnDeath delegate
		NPC->OnPawnDeath.AddDynamic(this, &AShooterAIController::OnPawnDeath);
	}
}

void AShooterAIController::OnPawnDeath()
{
	// stop movement
	GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::UserAbort);

	// stop StateTree logic
	StateTreeAI->StopLogic(FString(""));

	// unpossess the pawn
	UnPossess();

	// destroy this controller
	Destroy();
}

void AShooterAIController::SetCurrentTarget(AActor* Target)
{
	TargetEnemy = Target;
}

void AShooterAIController::ClearCurrentTarget()
{
	TargetEnemy = nullptr;
}

void AShooterAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// pass the data to the StateTree delegate hook
	OnShooterPerceptionUpdated.ExecuteIfBound(Actor, Stimulus);
}

void AShooterAIController::OnPerceptionForgotten(AActor* Actor)
{
	// pass the data to the StateTree delegate hook
	OnShooterPerceptionForgotten.ExecuteIfBound(Actor);
}
