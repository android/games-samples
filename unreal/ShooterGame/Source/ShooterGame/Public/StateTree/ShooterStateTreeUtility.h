// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"

#include "ShooterStateTreeUtility.generated.h"

class AShooterNPC;
class AAIController;
class AShooterAIController;

/**
 *  Instance data struct for the FStateTreeLineOfSightToTargetCondition condition
 */
USTRUCT()
struct FStateTreeLineOfSightToTargetConditionInstanceData
{
	GENERATED_BODY()
	
	/** Targeting character */
	UPROPERTY(EditAnywhere, Category = "Context")
	AShooterNPC* Character;

	/** Target to check line of sight for */
	UPROPERTY(EditAnywhere, Category = "Condition")
	AActor* Target;

	/** Max allowed line of sight cone angle, in degrees */
	UPROPERTY(EditAnywhere, Category = "Condition")
	float LineOfSightConeAngle = 35.0f;

	/** Number of vertical line of sight checks to run to try and get around low obstacles */
	UPROPERTY(EditAnywhere, Category = "Condition")
	int32 NumberOfVerticalLineOfSightChecks = 5;

	/** If true, the condition passes if the character has line of sight */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bMustHaveLineOfSight = true;
};
STATETREE_POD_INSTANCEDATA(FStateTreeLineOfSightToTargetConditionInstanceData);

/**
 *  StateTree condition to check if the character is grounded
 */
USTRUCT(DisplayName = "Has Line of Sight to Target", Category="Shooter")
struct FStateTreeLineOfSightToTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	/** Set the instance data type */
	using FInstanceDataType = FStateTreeLineOfSightToTargetConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Default constructor */
	FStateTreeLineOfSightToTargetCondition() = default;
	
	/** Tests the StateTree condition */
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	/** Provides the description string */
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data struct for the Face Towards Actor StateTree task
 */
USTRUCT()
struct FStateTreeFaceActorInstanceData
{
	GENERATED_BODY()

	/** AI Controller that will determine the focused actor */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller;

	/** Actor that will be faced towards */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> ActorToFaceTowards;
};

/**
 *  StateTree task to face an AI-Controlled Pawn towards an Actor
 */
USTRUCT(meta=(DisplayName="Face Towards Actor", Category="Shooter"))
struct FStateTreeFaceActorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeFaceActorInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data struct for the Face Towards Location StateTree task
 */
USTRUCT()
struct FStateTreeFaceLocationInstanceData
{
	GENERATED_BODY()

	/** AI Controller that will determine the focused location */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller;

	/** Location that will be faced towards */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FVector FaceLocation = FVector::ZeroVector;
};

/**
 *  StateTree task to face an AI-Controlled Pawn towards a world location
 */
USTRUCT(meta=(DisplayName="Face Towards Location", Category="Shooter"))
struct FStateTreeFaceLocationTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeFaceLocationInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data struct for the Set Random Float StateTree task
 */
USTRUCT()
struct FStateTreeSetRandomFloatData
{
	GENERATED_BODY()

	/** Minimum random value */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float MinValue = 0.0f;

	/** Maximum random value */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float MaxValue = 0.0f;

	/** Output calculated value */
	UPROPERTY(EditAnywhere, Category = Output)
	float OutValue = 0.0f;
};

/**
 *  StateTree task to calculate a random float value within the specified range
 */
USTRUCT(meta=(DisplayName="Set Random Float", Category="Shooter"))
struct FStateTreeSetRandomFloatTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeSetRandomFloatData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data struct for the Shoot At Target StateTree task
 */
USTRUCT()
struct FStateTreeShootAtTargetInstanceData
{
	GENERATED_BODY()

	/** NPC that will do the shooting */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AShooterNPC> Character;

	/** Target to shoot at */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> Target;
};

/**
 *  StateTree task to have an NPC shoot at an actor
 */
USTRUCT(meta=(DisplayName="Shoot at Target", Category="Shooter"))
struct FStateTreeShootAtTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeShootAtTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Instance data struct for the Sense Enemies StateTree task
 */
USTRUCT()
struct FStateTreeSenseEnemiesInstanceData
{
	GENERATED_BODY()

	/** Sensing AI Controller */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AShooterAIController> Controller;

	/** Sensing NPC */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AShooterNPC> Character;

	/** Sensed actor to target */
	UPROPERTY(EditAnywhere, Category = Output)
	TObjectPtr<AActor> TargetActor;

	/** Sensed location to investigate */
	UPROPERTY(EditAnywhere, Category = Output)
	FVector InvestigateLocation = FVector::ZeroVector;

	/** True if a target was successfully sensed */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasTarget = false;

	/** True if an investigate location was successfully sensed */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasInvestigateLocation = false;

	/** Tag required on sensed actors */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FName SenseTag = FName("Player");

	/** Line of sight cone half angle to consider a full sense */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float DirectLineOfSightCone = 85.0f;

	/** Strength of the last processed stimulus */
	UPROPERTY(EditAnywhere)
	float LastStimulusStrength = 0.0f;
};

/**
 *  StateTree task to have an NPC process AI Perceptions and sense nearby enemies
 */
USTRUCT(meta=(DisplayName="Sense Enemies", Category="Shooter"))
struct FStateTreeSenseEnemiesTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeSenseEnemiesInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////