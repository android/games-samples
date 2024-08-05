/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
using UnityEngine;

public static class GameModeUtil
{
	public const int GameModeUnsupported = 0;
 	public const int GameModeStandard = 1;
 	public const int GameModePerformance = 2;
 	public const int GameModeBattery = 3;
 	public const int GameModeCustom = 4;

	private const int MinimumGameModeApiLevel = 31;

 	public static int GetGameMode()
 	{
  		int gameMode = GameModeUnsupported;
  		AndroidJNI.AttachCurrentThread();

	    using var version = new AndroidJavaClass("android.os.Build$VERSION");
	    var sdkLevel = version.GetStatic<int>("SDK_INT");	    
  		if (sdkLevel < MinimumGameModeApiLevel) return gameMode;

  		using var unityPlayerClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
  		using var context = unityPlayerClass.GetStatic<AndroidJavaObject>("currentActivity");
  		using var gameManager = context.Call<AndroidJavaObject>("getSystemService", "game");
  		gameMode = gameManager.Call<int>("getGameMode");

  		return gameMode;
 	}

 	public static string GetGameModeString(int gameMode)
 	{
  		string gameModeString;
  		switch (gameMode)
  		{
   			case GameModeUtil.GameModeUnsupported:
    			gameModeString = "GameModeUnsupported";
    			break;
   			case GameModeUtil.GameModeStandard:
    			gameModeString = "GameModeStandard";
    			break;
   			case GameModeUtil.GameModePerformance:
    			gameModeString = "GameModePerformance";
    			break;
   			case GameModeUtil.GameModeBattery:
    			gameModeString = "GameModeBattery";
    			break;
   			case GameModeUtil.GameModeCustom:
    			gameModeString = "GameModeCustom";
    			break;
   			default:
    			gameModeString = "Unknown game mode";
    			break;
  		}
  		return gameModeString;
 	}
}
