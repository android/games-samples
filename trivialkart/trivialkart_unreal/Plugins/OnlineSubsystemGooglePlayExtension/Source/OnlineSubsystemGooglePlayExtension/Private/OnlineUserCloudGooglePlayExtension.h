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
#include "Interfaces/OnlineUserCloudInterface.h"

class FOnlineSubsystemGooglePlayExtension;
/**
 * User Cloud Interface implementation which will help with cloud file storage feature of PGS SDK
 */
class FOnlineUserCloudGooglePlayExtension : public IOnlineUserCloud
{
public:
	FOnlineUserCloudGooglePlayExtension(FOnlineSubsystemGooglePlayExtension* InSubsystem);
	
	virtual bool WriteUserFile(const FUniqueNetId& UserId, const FString& FileName, TArray<uint8>& FileContents, bool bCompressBeforeUpload = false) override;
	virtual bool ReadUserFile(const FUniqueNetId& UserId, const FString& FileName) override;
	virtual bool GetFileContents(const FUniqueNetId& UserId, const FString& FileName, TArray<uint8>& FileContents) override;
	
	// Boilerplate Stubs
	virtual bool ClearFile(const FUniqueNetId& UserId, const FString& FileName) override { return false; }
	virtual bool ClearFiles(const FUniqueNetId& UserId) override { return false; }
	virtual void EnumerateUserFiles(const FUniqueNetId& UserId) override {}
	virtual void GetUserFileList(const FUniqueNetId& UserId, TArray<FCloudFileHeader>& UserFiles) override {}
	virtual bool ReadUserFile(const FUniqueNetId& UserId, const FString& FileName, FOnReadUserFileCompleteDelegate Delegate) { return ReadUserFile(UserId, FileName); }
	virtual void CancelWriteUserFile(const FUniqueNetId& UserId, const FString& FileName) override {}
	virtual void DumpCloudFileState(const FUniqueNetId& UserId, const FString& FileName) override {}
	virtual void DumpCloudState(const FUniqueNetId& UserId) override {}
	virtual bool DeleteUserFile(const FUniqueNetId& UserId, const FString& FileName, bool bShouldCloudDelete, bool bShouldLocallyDelete) override { return false; }
	virtual bool RequestUsageInfo(const FUniqueNetId& UserId) override { return false; }

	// Completion Handlers
	void OnWriteComplete(bool bSuccess, const FUniqueNetId& NetId, const FString& FileName);
	void OnReadComplete(bool bSuccess, const FUniqueNetId& NetId, const FString& FileName);
	
	void UpdateCache(const FString& FileName, const TArray<uint8>& Data);

private:
	FOnlineSubsystemGooglePlayExtension* Subsystem;
	TMap<FString, TArray<uint8>> CachedFiles;
	
};
