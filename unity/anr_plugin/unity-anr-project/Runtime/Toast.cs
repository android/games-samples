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
 */﻿

using UnityEngine;

namespace com.google.unity.anr
{
    public static class Toast
    {
#if UNITY_ANDROID && !UNITY_EDITOR

        private static AndroidJavaObject _toastJavaObject;

        public static void ShowToast(string message)
        {
            if (!PluginBinder.IsInitialized) return;
            _toastJavaObject ??= new AndroidJavaObject($"{PluginBinder.PackageName}.ToastHelper");
            _toastJavaObject.CallStatic("showToast", PluginBinder.Context, message);
        }
#endif

#if UNITY_EDITOR
        public static void ShowToast(string message)
        {
        }
#endif
    }
}
