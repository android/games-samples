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


#include "GooglePlayGamesExtensionWrapper.h"

#include "OnlineAsyncTaskGooglePlayExtensionGetPlayerStats.h"
#include "OnlineAsyncTaskGooglePlayExtensionShowProfileUI.h"
#include "OnlineAsyncTaskGooglePlayExtensionReadFriendsList.h"
#include "OnlineAsyncTaskGooglePlayExtensionGetRecallSessionID.h"
#include "OnlineAsyncTaskGooglePlayExtensionReadSave.h"
#include "OnlineAsyncTaskGooglePlayExtensionWriteSave.h"
#include "Async/Async.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidJavaEnv.h"
#endif

//Implementation for FGooglePlayGamesExtensionWrapper

FOnSaveGameUIClosed FGooglePlayGamesExtensionWrapper::SaveUIClosedDelegate;

void FGooglePlayGamesExtensionWrapper::Init()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
        ShowSavedGamesUIId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_ShowSavedGamesUI", "(Ljava/lang/String;ZZI)V", false);
        SetSnapshotMetadataId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_SetSnapshotMetadata", "([BLjava/lang/String;JJ)V", false);
		SaveId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GPGSaveSnapshot", "(JLjava/lang/String;[B)V", false);
		LoadId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GPGLoadSnapshot", "(JLjava/lang/String;)V", false);	
	    GetRecallId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetRecallSessionId", "(J)V", false);
        SetFriendsConfigId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_SetFriendsConfig", "(IZ)V", false);
	    ReadFriendsListId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_ReadFriendsList", "(J)V", false);
	    ShowProfileUIId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_ShowProfileUI", "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", false);
	    GetPlayerStatsId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetPlayerStats", "(JZ)V", false);
	    IncrementEventId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IncrementEvent", "(Ljava/lang/String;I)V", false);
	}
#endif
}

void FGooglePlayGamesExtensionWrapper::Reset()
{
#if PLATFORM_ANDROID
	// Cleanup not strictly necessary for GameActivity methods, but good practice
    ShowSavedGamesUIId = nullptr;
    SetSnapshotMetadataId = nullptr;
    SaveId = nullptr;
	LoadId = nullptr;
    GetRecallId = nullptr;
    SetFriendsConfigId = nullptr;
    ReadFriendsListId = nullptr;
    ShowProfileUIId = nullptr;
    GetPlayerStatsId = nullptr;
    IncrementEventId = nullptr;
#endif
}

void FGooglePlayGamesExtensionWrapper::ShowSavedGamesUI(const FString& Title, bool bAllowAdd, bool bAllowDelete, int32 MaxSaves, FOnSaveGameUIClosed OnCloseDelegate)
{
    SaveUIClosedDelegate = OnCloseDelegate;

#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (ShowSavedGamesUIId)
        {
            jstring jTitle = Env->NewStringUTF(TCHAR_TO_UTF8(*Title));
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ShowSavedGamesUIId, jTitle, bAllowAdd, bAllowDelete, MaxSaves);
            Env->DeleteLocalRef(jTitle);
        }
    }
#endif
}

void FGooglePlayGamesExtensionWrapper::SetSnapshotMetadata(UTexture2D* CoverImage, const FString& Description, int64 PlayedTimeMillis, int64 ProgressValue)
{
    TArray64<uint8> CompressedData;

    if (CoverImage && CoverImage->GetPlatformData() && CoverImage->GetPlatformData()->Mips.Num() > 0)
    {
        // 1. Get raw pixel data from the first Mip
        FTexture2DMipMap& Mip = CoverImage->GetPlatformData()->Mips[0];
        int32 Width = Mip.SizeX;
        int32 Height = Mip.SizeY;
        
        uint8* Data = (uint8*)Mip.BulkData.Lock(LOCK_READ_ONLY);
        if (Data)
        {
            TArray<uint8> RawImageData;
            RawImageData.Append(Data, Mip.BulkData.GetBulkDataSize());
            Mip.BulkData.Unlock();

            // 2. Compress raw pixels to PNG so Java BitmapFactory can read it
            IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
            TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

            // Note: Most Unreal textures are BGRA8
            if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(RawImageData.GetData(), RawImageData.Num(), Width, Height, ERGBFormat::BGRA, 8))
            {
                CompressedData = ImageWrapper->GetCompressed();
            }
        }
    }

    // 3. JNI Call (Scoping fixed: CompressedData is now accessible here)
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (SetSnapshotMetadataId)
        {
            jbyteArray jCover = nullptr;
            if (CompressedData.Num() > 0)
            {
                jCover = Env->NewByteArray(CompressedData.Num());
                Env->SetByteArrayRegion(jCover, 0, CompressedData.Num(), (const jbyte*)CompressedData.GetData());
            }

            jstring jDesc = Env->NewStringUTF(TCHAR_TO_UTF8(*Description));
            
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, SetSnapshotMetadataId,
                jCover, jDesc, (jlong)PlayedTimeMillis, (jlong)ProgressValue);
            
            if (jCover) Env->DeleteLocalRef(jCover);
            Env->DeleteLocalRef(jDesc);
        }
    }
#endif
}

bool FGooglePlayGamesExtensionWrapper::SaveSnapshot(FOnlineAsyncTaskGooglePlayExtensionWriteSave* Task, const FString& FileName, const TArray<uint8>& Data)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (SaveId) {
            jstring jFile = Env->NewStringUTF(TCHAR_TO_UTF8(*FileName));
            jbyteArray jData = Env->NewByteArray(Data.Num());
            Env->SetByteArrayRegion(jData, 0, Data.Num(), (const jbyte*)Data.GetData());
            
            // Call method on GameActivity Instance
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, SaveId, (jlong)Task, jFile, jData);
            
            Env->DeleteLocalRef(jFile);
            Env->DeleteLocalRef(jData);
            return true;
        }
    }
#endif
    return false;
}

bool FGooglePlayGamesExtensionWrapper::LoadSnapshot(FOnlineAsyncTaskGooglePlayExtensionReadSave* Task, const FString& FileName)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (LoadId) {
            jstring jFile = Env->NewStringUTF(TCHAR_TO_UTF8(*FileName));
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, LoadId, (jlong)Task, jFile);
            Env->DeleteLocalRef(jFile);
            return true;
        }
    }
#endif
    return false;
}

bool FGooglePlayGamesExtensionWrapper::GetRecallSessionId(FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID* Task)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (GetRecallId)
        {
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, GetRecallId, (jlong)Task);
            return true;
        }
    }
#endif
    return false;
}

void FGooglePlayGamesExtensionWrapper::SetFriendsConfig(int32 PageSize, bool bForceReload)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (SetFriendsConfigId)
        {
            jboolean jForce = bForceReload ? JNI_TRUE : JNI_FALSE;
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, SetFriendsConfigId, (jint)PageSize, jForce);
        }
    }
#endif
}

bool FGooglePlayGamesExtensionWrapper::ReadFriendsList(FOnlineAsyncTaskGooglePlayExtensionReadFriendsList* Task)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (ReadFriendsListId)
        {
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ReadFriendsListId, (jlong)Task);
            return true;
        }
    }
#endif
    return false;
}

void FGooglePlayGamesExtensionWrapper::ShowProfileUI(FOnlineAsyncTaskGooglePlayExtensionShowProfileUI* Task, const FString& PlayerId, const FString& CurrentPlayerName, const FString& OtherPlayerName)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (ShowProfileUIId)
        {
            jstring jPlayerId = Env->NewStringUTF(TCHAR_TO_UTF8(*PlayerId));
            jstring jCurrentName = Env->NewStringUTF(TCHAR_TO_UTF8(*CurrentPlayerName));
            jstring jOtherName = Env->NewStringUTF(TCHAR_TO_UTF8(*OtherPlayerName));
            
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ShowProfileUIId, (jlong)Task, jPlayerId, jCurrentName, jOtherName);
            
            Env->DeleteLocalRef(jPlayerId);
            Env->DeleteLocalRef(jCurrentName);
            Env->DeleteLocalRef(jOtherName);
        }
    }
#endif
}

bool FGooglePlayGamesExtensionWrapper::GetPlayerStats(FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats* Task,
    bool bForceReload)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (GetPlayerStatsId)
        {
            jboolean jForceReload = bForceReload ? JNI_TRUE : JNI_FALSE;
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, GetPlayerStatsId, (jlong)Task, jForceReload);
            return true;
        }
    }
#endif
    return false;
}

void FGooglePlayGamesExtensionWrapper::IncrementEvent(const FString& EventId, int32 IncrementAmount)
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        if (IncrementEventId)
        {
            jstring jEventId = Env->NewStringUTF(TCHAR_TO_UTF8(*EventId));
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, IncrementEventId, jEventId, IncrementAmount);
            Env->DeleteLocalRef(jEventId);
        }
    }
#endif
}

#if PLATFORM_ANDROID
extern "C" {
    // JNI Export must now match GameActivity package and class
    // Format: Java_com_epicgames_unreal_GameActivity_METHODNAME

    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnSavedGamesUIClosed(JNIEnv* jEnv, jobject thiz, jboolean bSelected, jstring jSnapshotName)
    {
        FString SnapshotName = FJavaHelper::FStringFromParam(jEnv, jSnapshotName);
        
        // Execute on Game Thread since JNI comes from the Android UI thread
        AsyncTask(ENamedThreads::GameThread, [bSelected, SnapshotName]()
        {
            FGooglePlayGamesExtensionWrapper::SaveUIClosedDelegate.ExecuteIfBound((bool)bSelected, SnapshotName);
            FGooglePlayGamesExtensionWrapper::SaveUIClosedDelegate.Unbind();
        });
    }
    
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnGPGSaveComplete(JNIEnv* jEnv, jobject thiz, jlong TaskPtr, jboolean bSuccess, jstring jMsg)
    {
        auto* Task = reinterpret_cast<FOnlineAsyncTaskGooglePlayExtensionWriteSave*>(TaskPtr);
        if (Task) {
            Task->bWasSuccessful = (bool)bSuccess;
            Task->bIsComplete = true;
        }
    }

    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnGPGLoadComplete(JNIEnv* jEnv, jobject thiz, jlong TaskPtr, jboolean bSuccess, jbyteArray jData, jstring jMsg)
    {
        auto* Task = reinterpret_cast<FOnlineAsyncTaskGooglePlayExtensionReadSave*>(TaskPtr);
        if (Task) {
            if (bSuccess && jData) {
                jsize Len = jEnv->GetArrayLength(jData);
                TArray<uint8> OutData;
                OutData.AddUninitialized(Len);
                jEnv->GetByteArrayRegion(jData, 0, Len, (jbyte*)OutData.GetData());
                Task->ReadData = OutData;
            }
            Task->bWasSuccessful = (bool)bSuccess;
            Task->bIsComplete = true;
        }
    }
    
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnRecallIdReceived(JNIEnv* jEnv, jobject thiz, jlong TaskPtr, jboolean bSuccess, jstring jSessionId, jstring jError)
    {
        auto* Task = reinterpret_cast<FOnlineAsyncTaskGooglePlayExtensionGetRecallSessionID*>(TaskPtr);
        if (Task) {
            if (bSuccess)
            {
                Task->SessionId = FJavaHelper::FStringFromParam(jEnv, jSessionId);
            }
            else
            {
                Task->ErrorStr = FJavaHelper::FStringFromParam(jEnv, jError);
            }
            Task->bWasSuccessful = (bool)bSuccess;
            Task->bIsComplete = true;
        }
    }
    
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnReadFriendsListComplete(JNIEnv* jEnv, jobject thiz, jlong TaskPtr, jboolean bSuccess, jstring jError, jobjectArray jIds, jobjectArray jNames, jobjectArray jAvatars)
    {
        auto* Task = reinterpret_cast<FOnlineAsyncTaskGooglePlayExtensionReadFriendsList*>(TaskPtr);
        if (Task)
        {
            if (bSuccess)
            {
                // Parse Arrays using FJavaHelper or manual looping
                // Simplified manual loop for clarity:
                int Count = jEnv->GetArrayLength(jIds);
                for(int i=0; i<Count; i++)
                {
                    jstring SId = (jstring)jEnv->GetObjectArrayElement(jIds, i);
                    jstring SName = (jstring)jEnv->GetObjectArrayElement(jNames, i);
                    jstring SAvatar = (jstring)jEnv->GetObjectArrayElement(jAvatars, i);
                    
                    FString Id = FJavaHelper::FStringFromParam(jEnv, SId);
                    FString Name = FJavaHelper::FStringFromParam(jEnv, SName);
                    FString Avatar = FJavaHelper::FStringFromParam(jEnv, SAvatar);
                    
                    Task->FetchedFriends.Add({Id, Name, Avatar});
                }
            }
            else
            {
                Task->ErrorStr = FJavaHelper::FStringFromParam(jEnv, jError);
            }
            
            Task->bWasSuccessful = (bool)bSuccess;
            Task->bIsComplete = true;
        }
    }
    
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnProfileUIClosed(JNIEnv* jEnv, jobject thiz, jlong TaskPtr)
    {
        auto* Task = reinterpret_cast<FOnlineAsyncTaskGooglePlayExtensionShowProfileUI*>(TaskPtr);
        if (Task)
        {
            Task->bWasSuccessful = true;
            
            // Marking this true tells the Subsystem to move it to the Game Thread queue
            Task->bIsComplete = true; 
        }
    }
    
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_GameActivity_nativeOnPlayerStatsReceived(JNIEnv* jEnv, jobject thiz, jlong TaskPtr, jboolean bSuccess, jstring jErrorMsg, jfloat avgSessionLen, jint daysSinceLast, jint numPurchases, jint numSessions, jfloat sessionPct, jfloat spendPct)
    {
        auto* Task = reinterpret_cast<FOnlineAsyncTaskGooglePlayExtensionGetPlayerStats*>(TaskPtr);
        if (Task)
        {
            Task->bWasSuccessful = (bool)bSuccess;
            if (bSuccess)
            {
                Task->RawStats.AverageSessionLength = (float)avgSessionLen;
                Task->RawStats.DaysSinceLastPlayed = (int32)daysSinceLast;
                Task->RawStats.NumberOfPurchases = (int32)numPurchases;
                Task->RawStats.NumberOfSessions = (int32)numSessions;
                Task->RawStats.SessionPercentile = (float)sessionPct;
                Task->RawStats.SpendPercentile = (float)spendPct;
            }
            else
            {
                Task->ErrorStr = FJavaHelper::FStringFromParam(jEnv, jErrorMsg);
            }
            
            Task->bIsComplete = true;
        }
    }
}
#endif
