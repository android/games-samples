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
public static class DisplayRateUtil
{
 	public static void SetDisplayRefreshRate(int refreshRate)
 	{
		// This routine is dependent on a custom Activity overriding the standard
		// UnityPlayerActivity that implements a SetDisplayRefreshRate function. For this
		// project it is located in the Assets/Plugins/Android/VkQualityTestActivity.java file
  		using var unityPlayerClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
  		using var activity = unityPlayerClass.GetStatic<AndroidJavaObject>("currentActivity");
  		activity.Call("SetDisplayRefreshRate", refreshRate);
 	}    
}