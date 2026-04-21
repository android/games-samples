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

#include "OnlineAsyncTaskGooglePlayExtensionShowProfileUI.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineExternalUIGooglePlayExtension.h"

//Implementation for FOnlineAsyncTaskGooglePlayExtensionShowProfileUI

FOnlineAsyncTaskGooglePlayExtensionShowProfileUI::FOnlineAsyncTaskGooglePlayExtensionShowProfileUI(
	FOnlineSubsystemGooglePlayExtension* Subsystem,
	const FString& InPlayerId,
	const FString& InCurrentPlayerName,
	const FString& InOtherPlayerName,
	const FOnProfileUIClosedDelegate& InDelegate)
	: FOnlineAsyncTaskBasic(Subsystem)
	, PlayerId(InPlayerId)
	, CurrentPlayerName(InCurrentPlayerName)
	, OtherPlayerName(InOtherPlayerName)
	, Delegate(InDelegate)
{
}

void FOnlineAsyncTaskGooglePlayExtensionShowProfileUI::Tick()
{
	if (!bInit)
	{
		bInit = true;
		if (Subsystem && Subsystem->GPGWrapper.IsValid())
		{
			// Call the wrapper, passing 'this' as the task pointer
			Subsystem->GPGWrapper->ShowProfileUI(this, PlayerId, CurrentPlayerName, OtherPlayerName);
		}
		else
		{
			bWasSuccessful = false;
			bIsComplete = true;
		}
	}
}

void FOnlineAsyncTaskGooglePlayExtensionShowProfileUI::TriggerDelegates()
{
	// 1. Tell the External UI interface to broadcast that the UI has closed
	if (IOnlineExternalUIPtr ExternalUI = Subsystem->GetExternalUIInterface())
	{
		ExternalUI->TriggerOnExternalUIChangeDelegates(false);
	}

	// 2. Fire the specific delegate the game passed into the function
	Delegate.ExecuteIfBound();
}
