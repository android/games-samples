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

#include "OnlineSubsystemGooglePlayExtensionModule.h"

#include "OnlineSubsystemGooglePlayExtension.h"
#include "GooglePlayGamesExtensionWrapper.h"

#define LOCTEXT_NAMESPACE "FOnlineSubsystemGooglePlayExtensionModule"

IMPLEMENT_MODULE( FOnlineSubsystemGooglePlayExtensionModule, OnlineSubsystemGooglePlayExtension );

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryGooglePlayExtension : public IOnlineFactory
{

private:

	/** Single instantiation of the Online Subsystem interface */
	static FOnlineSubsystemGooglePlayExtensionPtr GooglePlayExtensionSingleton;

	virtual void DestroySubsystem()
	{
		if (GooglePlayExtensionSingleton.IsValid())
		{
			GooglePlayExtensionSingleton->Shutdown();
			GooglePlayExtensionSingleton = NULL;
		}
	}

public:

	FOnlineFactoryGooglePlayExtension() {}
	virtual ~FOnlineFactoryGooglePlayExtension() 
	{
		DestroySubsystem();
	}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		if (!GooglePlayExtensionSingleton.IsValid())
		{
			GooglePlayExtensionSingleton = MakeShared<FOnlineSubsystemGooglePlayExtension, ESPMode::ThreadSafe>(InstanceName);
			if (GooglePlayExtensionSingleton->IsEnabled())
			{
				if(!GooglePlayExtensionSingleton->Init())
				{
					UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemGooglePlayExtensionModule failed to initialize!"));
					DestroySubsystem();
				}
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemGooglePlayExtensionModule was disabled"));
				DestroySubsystem();
			}

			return GooglePlayExtensionSingleton;
		}

		UE_LOG_ONLINE(Warning, TEXT("Can't create more than one instance of a Google Play Extension online subsystem!"));
		return NULL;
	}
};

//Implementation for FOnlineSubsystemGooglePlayExtensionModule

void FOnlineSubsystemGooglePlayExtensionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGooglePlayModule::StartupModule()"));

	GooglePlayExtensionFactory = new FOnlineFactoryGooglePlayExtension();

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(GooglePlayExtensionSubsystemName, GooglePlayExtensionFactory);
}

void FOnlineSubsystemGooglePlayExtensionModule::ShutdownModule()
{
	FOnlineSubsystemModule* OSS = FModuleManager::GetModulePtr<FOnlineSubsystemModule>("OnlineSubsystem");
        if (OSS) OSS->UnregisterPlatformService(GooglePlayExtensionSubsystemName);
        
        if (GooglePlayExtensionFactory)
        {
            delete GooglePlayExtensionFactory;
            GooglePlayExtensionFactory = nullptr;
        }
}

FOnlineSubsystemGooglePlayExtensionPtr FOnlineFactoryGooglePlayExtension::GooglePlayExtensionSingleton = nullptr;

#undef LOCTEXT_NAMESPACE
