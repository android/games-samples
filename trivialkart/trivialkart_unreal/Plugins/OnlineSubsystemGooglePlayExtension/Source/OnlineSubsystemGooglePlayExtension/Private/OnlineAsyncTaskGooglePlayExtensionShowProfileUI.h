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
#include "OnlineSubsystemGooglePlayExtension.h"
#include "Interfaces/OnlineExternalUIInterface.h"

/**
 * Async Task class for showing the player Profile UI via PGS
 */
class FOnlineAsyncTaskGooglePlayExtensionShowProfileUI : public FOnlineAsyncTaskBasic<FOnlineSubsystemGooglePlayExtension>
{
public:
	FString PlayerId;
	FString CurrentPlayerName;
	FString OtherPlayerName;
	FOnProfileUIClosedDelegate Delegate;
	bool bInit = false;

	FOnlineAsyncTaskGooglePlayExtensionShowProfileUI(
		FOnlineSubsystemGooglePlayExtension* Subsystem,
		const FString& InPlayerId,
		const FString& InCurrentPlayerName,
		const FString& InOtherPlayerName,
		const FOnProfileUIClosedDelegate& InDelegate
	);

	virtual void Tick() override;
	virtual void TriggerDelegates() override;
	virtual FString ToString() const override { return TEXT("Show Google Play Profile UI"); }
};
