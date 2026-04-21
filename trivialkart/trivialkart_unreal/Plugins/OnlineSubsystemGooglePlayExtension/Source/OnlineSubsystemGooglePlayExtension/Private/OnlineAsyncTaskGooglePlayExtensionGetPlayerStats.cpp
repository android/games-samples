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

#include "OnlineAsyncTaskGooglePlayExtensionGetPlayerStats.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineError.h"
#include "OnlineStatsGooglePlayExtension.h"
#include "OnlineSubsystemGooglePlayExtension.h"

//Implementation for FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats

FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats::FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats(
	FOnlineSubsystemGooglePlayExtension* Subsystem, 
	const FUniqueNetIdRef& InTargetUserId,
	const FOnlineStatsQueryUserStatsComplete& InDelegate)
	: FOnlineAsyncTaskBasic(Subsystem)
	, TargetUserId(InTargetUserId)
	, Delegate(InDelegate)
{}

void FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats::Tick()
{
	if (!bInit)
	{
		bInit = true;
		bool bCallDispatched = false;

		if (Subsystem && Subsystem->GPGWrapper.IsValid())
		{
			// Note: bForceReload is passed as false here
			bCallDispatched = Subsystem->GPGWrapper->GetPlayerStats(this, false);
		}

		if (!bCallDispatched)
		{
			bWasSuccessful = false;
			ErrorStr = TEXT("JNI Call Failed");
			bIsComplete = true; 
		}
	}
}

void FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats::Finalize()
{
	if (bWasSuccessful)
    {
        // Construct the Unreal Interface object
        ResultStats = MakeShared<FOnlineStatsUserStats>(TargetUserId);

        // Google returns -1.0f or -1 for "UNSET_VALUE" if it doesn't have enough data yet.
        // We only add valid data to the dictionary.
        
        if (RawStats.AverageSessionLength != -1.0f)
            ResultStats->Stats.Add(TEXT("AverageSessionLength"), FOnlineStatValue(RawStats.AverageSessionLength));
            
        if (RawStats.DaysSinceLastPlayed != -1)
            ResultStats->Stats.Add(TEXT("DaysSinceLastPlayed"), FOnlineStatValue(RawStats.DaysSinceLastPlayed));
            
        if (RawStats.NumberOfPurchases != -1)
            ResultStats->Stats.Add(TEXT("NumberOfPurchases"), FOnlineStatValue(RawStats.NumberOfPurchases));
            
        if (RawStats.NumberOfSessions != -1)
            ResultStats->Stats.Add(TEXT("NumberOfSessions"), FOnlineStatValue(RawStats.NumberOfSessions));
            
        if (RawStats.SessionPercentile != -1.0f)
            ResultStats->Stats.Add(TEXT("SessionPercentile"), FOnlineStatValue(RawStats.SessionPercentile));
            
        if (RawStats.SpendPercentile != -1.0f)
            ResultStats->Stats.Add(TEXT("SpendPercentile"), FOnlineStatValue(RawStats.SpendPercentile));

        // Cache it in the subsystem
        IOnlineStatsPtr StatsInterface = Subsystem->GetStatsInterface();
        if (FOnlineStatsGooglePlayExtension* StatsImpl = static_cast<FOnlineStatsGooglePlayExtension*>(StatsInterface.Get()))
        {
            StatsImpl->CacheQueryResults(TargetUserId, ResultStats.ToSharedRef());
        }
    }
}

void FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats::TriggerDelegates()
{
	FOnlineError ResultState;
	ResultState.bSucceeded = bWasSuccessful;
	if (!bWasSuccessful)
	{
		ResultState.ErrorMessage = FText::FromString(ErrorStr);
	}

	// Fire the interface delegate
	Delegate.ExecuteIfBound(ResultState, ResultStats);
}
