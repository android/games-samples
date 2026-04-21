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


#include "OnlineAsyncTaskGooglePlayExtensionReadSave.h"

#include "GooglePlayGamesExtensionWrapper.h"
#include "OnlineUserCloudGooglePlayExtension.h"
#include "Interfaces/OnlineIdentityInterface.h"

//Implementation for FOnlineAsyncTaskGooglePlayExtensionReadSave

FOnlineAsyncTaskGooglePlayExtensionReadSave::FOnlineAsyncTaskGooglePlayExtensionReadSave(
	FOnlineSubsystemGooglePlayExtension* Subsystem, const FString& Name, 
	FOnReadUserFileCompleteDelegate InDel) : FOnlineAsyncTaskBasic(Subsystem), FileName(Name), 
Delegate(InDel) {}

void FOnlineAsyncTaskGooglePlayExtensionReadSave::Tick()
{
	if (!bInit)
	{
		bInit = true;
		if (Subsystem->	GPGWrapper.IsValid())
		{
			Subsystem->GPGWrapper->LoadSnapshot(this, FileName);
		}
		else
		{
			bIsComplete = true;
		}
	}
}

void FOnlineAsyncTaskGooglePlayExtensionReadSave::TriggerDelegates()
{
	if (bWasSuccessful)
	{
		IOnlineUserCloudPtr CloudPtr = Subsystem->GetUserCloudInterface();
		FOnlineUserCloudGooglePlayExtension* CloudImpl = static_cast<FOnlineUserCloudGooglePlayExtension*>(CloudPtr.Get());
        
		if (CloudImpl)
		{
			CloudImpl->UpdateCache(FileName, ReadData);
		}
	}
	if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
		IdentityInterface.IsValid())
	{
		Delegate.ExecuteIfBound(bWasSuccessful, *IdentityInterface->GetUniquePlayerId(0), FileName);
	}
}
