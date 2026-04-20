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

#include "OnlineEventsGooglePlayExtension.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineSubsystemGooglePlayExtension.h"

//Implementation for FOnlineEventsGooglePlayExtension

FOnlineEventsGooglePlayExtension::FOnlineEventsGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem)
: Subsystem(InSubsystem)
{
}

bool FOnlineEventsGooglePlayExtension::TriggerEvent(const FUniqueNetId& PlayerId, const TCHAR* EventName,
                                                    const FOnlineEventParms& Parms)
{
	if (!Subsystem || !Subsystem->GPGWrapper.IsValid())
	{
		return false;
	}

	// Default increment is 1 if not specified
	int32 IncrementAmount = 1;

	// Look for an explicit increment amount passed from the game
	if (const FVariantData* AmountParam = Parms.Find(TEXT("IncrementAmount")))
	{
		if (AmountParam->GetType() == EOnlineKeyValuePairDataType::Int32)
		{
			AmountParam->GetValue(IncrementAmount);
		}
	}

	// Call Java to register the increment
	Subsystem->GPGWrapper->IncrementEvent(FString(EventName), IncrementAmount);

	return true;
}

void FOnlineEventsGooglePlayExtension::SetPlayerSessionId(const FUniqueNetId& PlayerId, const FGuid& PlayerSessionId)
{
}
