/*
 * Copyright 2022 Google LLC
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

using System;

// Class mapping for the integrity verdict Json payload,
// To be serialized using the UnityEngine.JsonUtility class
// See the Play Integrity documentation for details about
// the integrity verdict fields and values
[Serializable]
public class RequestDetails
{
    public string requestPackageName;
    public string timestampMillis;
    public string nonce;
}

[Serializable]
public class AppIntegrity
{
    public string appRecognitionVerdict;
    public string packageName;
    public string[] certificateSha256Digest;
    public string versionCode;
}

[Serializable]
public class DeviceIntegrity
{
    public string[] deviceRecognitionVerdict;
}

[Serializable]
public class AccountDetails
{
    public string appLicensingVerdict;
}

[Serializable]
public class IntegrityVerdict
{
    public RequestDetails requestDetails;
    public AppIntegrity appIntegrity;
    public DeviceIntegrity deviceIntegrity;
    public AccountDetails accountDetails;
}
