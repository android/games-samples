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


#include "OnlineUserCloudGooglePlayExtension.h"
#include "OnlineSubsystemGooglePlayExtension.h"
#include "OnlineAsyncTaskGooglePlayExtensionWriteSave.h"
#include "OnlineAsyncTaskGooglePlayExtensionReadSave.h"
#include "Interfaces/OnlineIdentityInterface.h"

//Implementation for FOnlineUserCloudGooglePlayExtension

FOnlineUserCloudGooglePlayExtension::FOnlineUserCloudGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem)
	: Subsystem(InSubsystem)
{}

bool FOnlineUserCloudGooglePlayExtension::WriteUserFile(const FUniqueNetId& UserId, const FString& FileName, TArray<uint8>& FileContents, bool bCompressBeforeUpload)
{
	FOnWriteUserFileCompleteDelegate Delegate = FOnWriteUserFileCompleteDelegate::CreateRaw(this, &FOnlineUserCloudGooglePlayExtension::OnWriteComplete);
	Subsystem->QueueAsyncTask(new FOnlineAsyncTaskGooglePlayExtensionWriteSave(Subsystem, FileName, FileContents, Delegate));
	return true;
}

bool FOnlineUserCloudGooglePlayExtension::ReadUserFile(const FUniqueNetId& UserId, const FString& FileName)
{
	FOnReadUserFileCompleteDelegate Delegate = FOnReadUserFileCompleteDelegate::CreateRaw(this, &FOnlineUserCloudGooglePlayExtension::OnReadComplete);
	Subsystem->QueueAsyncTask(new FOnlineAsyncTaskGooglePlayExtensionReadSave(Subsystem, FileName, Delegate));
	return true;
}

bool FOnlineUserCloudGooglePlayExtension::GetFileContents(const FUniqueNetId& UserId, const FString& FileName, TArray<uint8>& FileContents)
{
	if (CachedFiles.Contains(FileName)) {
		FileContents = CachedFiles[FileName];
		return true;
	}
	return false;
}

void FOnlineUserCloudGooglePlayExtension::OnWriteComplete(bool bSuccess, const FUniqueNetId& NetId, const FString& FileName) 
{
	if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
		IdentityInterface.IsValid())
	{
		TriggerOnWriteUserFileCompleteDelegates(bSuccess, *IdentityInterface->GetUniquePlayerId(0), FileName);
	}
}

void FOnlineUserCloudGooglePlayExtension::OnReadComplete(bool bSuccess, const FUniqueNetId& NetId, const FString& FileName) 
{
	if (const IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
		IdentityInterface.IsValid())
	{
		TriggerOnReadUserFileCompleteDelegates(bSuccess, *IdentityInterface->GetUniquePlayerId(0), FileName);
	}
}

void FOnlineUserCloudGooglePlayExtension::UpdateCache(const FString& FileName, const TArray<uint8>& Data)
{
	CachedFiles.Add(FileName, Data);
}
