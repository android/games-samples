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

#include "Modules/ModuleManager.h"

class FOnlineSubsystemGooglePlayExtensionModule : public IModuleInterface
{
public:
	/** Class responsible for creating instance(s) of the subsystem */
	class FOnlineFactoryGooglePlayExtension* GooglePlayExtensionFactory;

	virtual ~FOnlineSubsystemGooglePlayExtensionModule() = default;
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}
};

typedef TSharedPtr<FOnlineSubsystemGooglePlayExtensionModule> FOnlineSubsystemGooglePlayExtensionModulePtr;