/*
*	Copyright 2026 The Android Open Source Project
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*		https://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/
#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"
#include "Interfaces/OnlineStatsInterface.h"

// Raw struct to hold data coming from JNI
struct FRawGooglePlayerStats
{
	float AverageSessionLength = -1.0f;
	int32 DaysSinceLastPlayed = -1;
	int32 NumberOfPurchases = -1;
	int32 NumberOfSessions = -1;
	float SessionPercentile = -1.0f;
	float SpendPercentile = -1.0f;
};

class FOnlineSubsystemGooglePlayExtension;
/**
 * Async Task class for receiving and populating the Player Stats
 */
class FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats : public FOnlineAsyncTaskBasic<FOnlineSubsystemGooglePlayExtension>
{
public:
	FUniqueNetIdRef TargetUserId;
	FOnlineStatsQueryUserStatsComplete Delegate;
    
	bool bInit = false;
	FRawGooglePlayerStats RawStats;
	FString ErrorStr;
    
	TSharedPtr<FOnlineStatsUserStats> ResultStats;

	FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats(
		FOnlineSubsystemGooglePlayExtension* Subsystem, 
		const FUniqueNetIdRef& InTargetUserId,
		const FOnlineStatsQueryUserStatsComplete& InDelegate);

	virtual void Tick() override;
	virtual void Finalize() override;
	virtual void TriggerDelegates() override;
	virtual FString ToString() const override { return TEXT("Get Player Stats"); }
};
