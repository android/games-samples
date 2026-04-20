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
#include "OnlineIdentityGooglePlayExtension.h"
#include "OnlineSubsystemGooglePlayExtension.h"

/**
 * Async Task class for getting the PGS Recall Session ID
 */
class FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID : public FOnlineAsyncTaskBasic<FOnlineSubsystemGooglePlayExtension>
{
public:
	// This delegate matches the one defined in your Identity Interface
	FOnRecallSessionIDReceived Delegate;
	
	FString SessionId;
	FString ErrorStr;
	bool bInit = false;

	FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID(
		FOnlineSubsystemGooglePlayExtension* Subsystem, 
		const FOnRecallSessionIDReceived& InDelegate
	);

	virtual void Tick() override;
	virtual void TriggerDelegates() override;
	virtual FString ToString() const override { return TEXT("GooglePlayExtension Get Recall ID"); }

private:
	// Internal callback from the JNI Wrapper
	void OnRecallIdReceived(bool bSuccess, const FString& InSessionId, const FString& InError);
};
