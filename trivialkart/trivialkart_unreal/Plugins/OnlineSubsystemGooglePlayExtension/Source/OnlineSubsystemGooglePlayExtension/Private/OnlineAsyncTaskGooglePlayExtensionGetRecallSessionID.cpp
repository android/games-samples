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


#include "OnlineAsyncTaskGooglePlayExtensionGetRecallSessionID.h"

#include "GooglePlayGamesExtensionWrapper.h"

//Implementation for FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID

FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID::FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID(
	FOnlineSubsystemGooglePlayExtension* Subsystem, const FOnRecallSessionIDReceived& InDelegate): 
FOnlineAsyncTaskBasic(Subsystem), 
Delegate(InDelegate)
{}

void FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID::Tick()
{
	if (!bInit)
	{
		bInit = true;
		bool bCallDispatched = false;

		if (Subsystem && Subsystem->GPGWrapper.IsValid())
		{
			// Now checks if the call was actually made
			bCallDispatched = Subsystem->GPGWrapper->GetRecallSessionId(this);
		}

		if (!bCallDispatched)
		{
			// If JNI failed, fail the task immediately so we don't hang
			bWasSuccessful = false;
			ErrorStr = TEXT("JNI Call Failed - Wrapper or Method ID invalid");
			bIsComplete = true; 
		}
	}
}

void FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID::TriggerDelegates()
{
	Delegate.ExecuteIfBound(bWasSuccessful, SessionId, ErrorStr);
}

void FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID::OnRecallIdReceived(bool bSuccess,
	const FString& InSessionId, const FString& InError)
{
	bWasSuccessful = bSuccess;
	SessionId = InSessionId;
	ErrorStr = InError;
	
	if (!bSuccess)
	{
		UE_LOG(LogOnline, Warning, TEXT("Recall API Error: %s"), *ErrorStr);
	}

	// This tells the AsyncTaskManager to move this task to the "Done" queue
	bIsComplete = true;
}
