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

#include "OnlineStatsGooglePlayExtension.h"

#include "OnlineAsyncTaskGooglePlayExtensionGetPlayerStats.h"
#include "OnlineError.h"
#include "OnlineSubsystemGooglePlayExtension.h"

//Implementation for FOnlineStatsGooglePlayExtension

FOnlineStatsGooglePlayExtension::FOnlineStatsGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem)
: Subsystem(InSubsystem)
{}

void FOnlineStatsGooglePlayExtension::QueryStats(const FUniqueNetIdRef LocalUserId, const FUniqueNetIdRef StatsUser,
	const FOnlineStatsQueryUserStatsComplete& Delegate)
{
	// Google Play Stats only works for the currently logged-in user
	if (*LocalUserId != *StatsUser)
	{
		Delegate.ExecuteIfBound(FOnlineError(EOnlineErrorResult::InvalidParams), nullptr);
		return;
	}

	// Queue the task to talk to Java
	Subsystem->QueueAsyncTask(new FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats(Subsystem, StatsUser, Delegate));
}

void FOnlineStatsGooglePlayExtension::QueryStats(const FUniqueNetIdRef LocalUserId,
	const TArray<FUniqueNetIdRef>& StatUsers, const TArray<FString>& StatNames,
	const FOnlineStatsQueryUsersStatsComplete& Delegate)
{
	// Bulk querying multiple users is not supported by Google's ML Stats API
	Delegate.ExecuteIfBound(FOnlineError(EOnlineErrorResult::NotImplemented), TArray<TSharedRef<const FOnlineStatsUserStats>>());
}

TSharedPtr<const FOnlineStatsUserStats> FOnlineStatsGooglePlayExtension::GetStats(
	const FUniqueNetIdRef StatsUserId) const
{
	if (const TSharedRef<const FOnlineStatsUserStats>* FoundStats = StatsCache.Find(StatsUserId))
	{
		return *FoundStats;
	}
	return nullptr;
}

void FOnlineStatsGooglePlayExtension::UpdateStats(const FUniqueNetIdRef LocalUserId,
	const TArray<FOnlineStatsUserUpdatedStats>& UpdatedUserStats, const FOnlineStatsUpdateStatsComplete& Delegate)
{
	// You cannot write to Google's ML Player Stats API
	Delegate.ExecuteIfBound(FOnlineError(EOnlineErrorResult::NotImplemented));
}

#if !UE_BUILD_SHIPPING
void FOnlineStatsGooglePlayExtension::ResetStats(const FUniqueNetIdRef StatsUserId)
{
	StatsCache.Remove(StatsUserId);
}
#endif

void FOnlineStatsGooglePlayExtension::CacheQueryResults(const FUniqueNetIdRef& StatsUserId,
	const TSharedRef<const FOnlineStatsUserStats>& InStats)
{
	StatsCache.Add(StatsUserId, InStats);
}
