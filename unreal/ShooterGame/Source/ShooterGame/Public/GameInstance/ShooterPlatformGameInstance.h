// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "ShooterPlatformGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UShooterPlatformGameInstance : public UPlatformGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	bool IsLoggedInToOnlineSubsystem();

	UFUNCTION(BlueprintCallable)
	FString GetPlayerName();

protected:
	UPROPERTY(EditAnywhere)
	FString LoginAchievementName;
	UPROPERTY(EditAnywhere)
	FString LoginAchievementID;
	
private:
	void OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
	                      const FString& Error);
	
	void OnQueryAchievementsCompleted(const FUniqueNetId& UniqueNetId, bool bWasSuccessful);

public:
	void AddAchievementProgress(const float Progress, const FString& AchievementName, const FString& AchievementID);
};
