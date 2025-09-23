// Copyright Epic Games, Inc. All Rights Reserved.


#include "StateTree/ShooterStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "Actors/ShooterNPC.h"
#include "Camera/CameraComponent.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Controllers/ShooterAIController.h"
#include "StateTreeAsyncExecutionContext.h"

bool FStateTreeLineOfSightToTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// ensure the target is valid
	if (!IsValid(InstanceData.Target))
	{
		return !InstanceData.bMustHaveLineOfSight;
	}
	
	// check if the character is facing towards the target
	const FVector TargetDir = (InstanceData.Target->GetActorLocation() - InstanceData.Character->GetActorLocation()).GetSafeNormal();

	const float FacingDot = FVector::DotProduct(TargetDir, InstanceData.Character->GetActorForwardVector());
	const float MaxDot = FMath::Cos(FMath::DegreesToRadians(InstanceData.LineOfSightConeAngle));

	// is the facing outside of our cone half angle?
	if (FacingDot <= MaxDot)
	{
		return !InstanceData.bMustHaveLineOfSight;
	}

	// get the target's bounding box
	FVector CenterOfMass, Extent;
	InstanceData.Target->GetActorBounds(true, CenterOfMass, Extent, false);

	// divide the vertical extent by the number of line of sight checks we'll do
	const float ExtentZOffset = Extent.Z * 2.0f / InstanceData.NumberOfVerticalLineOfSightChecks;

	// get the character's camera location as the source for the line checks
	const FVector Start = InstanceData.Character->GetFirstPersonCameraComponent()->GetComponentLocation();

	// ignore the character and target. We want to ensure there's an unobstructed trace not counting them
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(InstanceData.Character);
	QueryParams.AddIgnoredActor(InstanceData.Target);

	FHitResult OutHit;

	// run a number of vertically offset line traces to the target location
	for (int32 i = 0; i < InstanceData.NumberOfVerticalLineOfSightChecks - 1; ++i)
	{
		// calculate the endpoint for the trace
		const FVector End = CenterOfMass + FVector(0.0f, 0.0f, Extent.Z - ExtentZOffset * i);

		InstanceData.Character->GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

		// is the trace unobstructed?
		if (!OutHit.bBlockingHit)
		{
			// we only need one unobstructed trace, so terminate early
			return InstanceData.bMustHaveLineOfSight;
		}
	}

	// no line of sight found
	return !InstanceData.bMustHaveLineOfSight;
}

#if WITH_EDITOR
FText FStateTreeLineOfSightToTargetCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Has Line of Sight</b>");
}
#endif

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeFaceActorTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// set the AI Controller's focus
		InstanceData.Controller->SetFocus(InstanceData.ActorToFaceTowards);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceActorTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// clear the AI Controller's focus
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceActorTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Face Towards Actor</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeFaceLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// set the AI Controller's focus
		InstanceData.Controller->SetFocalPoint(InstanceData.FaceLocation);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceLocationTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// clear the AI Controller's focus
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceLocationTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Face Towards Location</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeSetRandomFloatTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// calculate the output value
		InstanceData.OutValue = FMath::RandRange(InstanceData.MinValue, InstanceData.MaxValue);
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeSetRandomFloatTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Set Random Float</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeShootAtTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// tell the character to shoot the target
		InstanceData.Character->StartShooting(InstanceData.Target);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeShootAtTargetTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// tell the character to stop shooting
		InstanceData.Character->StopShooting();
	}
}

#if WITH_EDITOR
FText FStateTreeShootAtTargetTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Shoot at Target</b>");
}
#endif // WITH_EDITOR

EStateTreeRunStatus FStateTreeSenseEnemiesTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// bind the perception updated delegate on the controller
		InstanceData.Controller->OnShooterPerceptionUpdated.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](AActor* SensedActor, const FAIStimulus& Stimulus)
			{
				// get the instance data inside the lambda
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();

				if (FInstanceDataType* LambdaInstanceData = StrongContext.GetInstanceDataPtr<FInstanceDataType>())
				{
					if (SensedActor->ActorHasTag(LambdaInstanceData->SenseTag))
					{
						bool bDirectLOS = false;

						// calculate the direction of the stimulus
						const FVector StimulusDir = (Stimulus.StimulusLocation - LambdaInstanceData->Character->GetActorLocation()).GetSafeNormal();

						// infer the angle from the dot product between the character facing and the stimulus direction
						const float DirDot = FVector::DotProduct(StimulusDir, LambdaInstanceData->Character->GetActorForwardVector());
						const float MaxDot = FMath::Cos(FMath::DegreesToRadians(LambdaInstanceData->DirectLineOfSightCone));

						// is the direction within our perception cone?
						if (DirDot >= MaxDot)
						{
							// run a line trace between the character and the sensed actor
							FCollisionQueryParams QueryParams;
							QueryParams.AddIgnoredActor(LambdaInstanceData->Character);
							QueryParams.AddIgnoredActor(SensedActor);

							FHitResult OutHit;

							// we have direct line of sight if this trace is unobstructed
							bDirectLOS = !LambdaInstanceData->Character->GetWorld()->LineTraceSingleByChannel(OutHit, LambdaInstanceData->Character->GetActorLocation(), SensedActor->GetActorLocation(), ECC_Visibility, QueryParams);

						}

						// check if we have a direct line of sight to the stimulus
						if (bDirectLOS)
						{
							// set the controller's target
							LambdaInstanceData->Controller->SetCurrentTarget(SensedActor);

							// set the task output
							LambdaInstanceData->TargetActor = SensedActor;

							// set the flags
							LambdaInstanceData->bHasTarget = true;
							LambdaInstanceData->bHasInvestigateLocation = false;

						// no direct line of sight to target
						} else {

							// if we already have a target, ignore the partial sense and keep on them
							if (!IsValid(LambdaInstanceData->TargetActor))
							{
								// is this stimulus stronger than the last one we had?
								if (Stimulus.Strength > LambdaInstanceData->LastStimulusStrength)
								{
									// update the stimulus strength
									LambdaInstanceData->LastStimulusStrength = Stimulus.Strength;

									// set the investigate location
									LambdaInstanceData->InvestigateLocation = Stimulus.StimulusLocation;

									// set the investigate flag
									LambdaInstanceData->bHasInvestigateLocation = true;
								}
							}
						}
					}
				}
			}
		);

		// bind the perception forgotten delegate on the controller
		InstanceData.Controller->OnShooterPerceptionForgotten.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](AActor* SensedActor)
			{
				// get the instance data inside the lambda
				FInstanceDataType* LambdaInstanceData = WeakContext.MakeStrongExecutionContext().GetInstanceDataPtr<FInstanceDataType>();

				if (!LambdaInstanceData)
				{
					return;
				}

				bool bForget = false;

				// are we forgetting the current target?
				if (SensedActor == LambdaInstanceData->TargetActor)
				{
					bForget = true;

				} else {

					// are we forgetting about a partial sense?
					if (!IsValid(LambdaInstanceData->TargetActor))
					{
						bForget = true;
					}
				}

				if (bForget)
				{
					// clear the target
					LambdaInstanceData->TargetActor = nullptr;

					// clear the flags
					LambdaInstanceData->bHasInvestigateLocation = false;
					LambdaInstanceData->bHasTarget = false;

					// reset the stimulus strength
					LambdaInstanceData->LastStimulusStrength = 0.0f;

					// clear the target on the controller
					LambdaInstanceData->Controller->ClearCurrentTarget();
					LambdaInstanceData->Controller->ClearFocus(EAIFocusPriority::Gameplay);
				}

			}
		);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeSenseEnemiesTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// unbind the perception delegates
		InstanceData.Controller->OnShooterPerceptionUpdated.Unbind();
		InstanceData.Controller->OnShooterPerceptionForgotten.Unbind();
	}
}

#if WITH_EDITOR
FText FStateTreeSenseEnemiesTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Sense Enemies</b>");
}
#endif // WITH_EDITOR