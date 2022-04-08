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
using System.Collections;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

#if PLAY_INTEGRITY
using Google.Play.Integrity;
#endif

public class PlayIntegrityController : MonoBehaviour
{
    public GameObject verdictLabel;

#if PLAY_INTEGRITY
    private const string URL_GETNONCE = "https://your-play-integrity-server.com/getNonce";
    private const string URL_PROCESSTOKEN = "https://your-play-integrity-server.com/processToken";
    private const string CONTENT_TYPE = "Content-Type";
    private const string JSON_CONTENT = "application/json";

    // Verdict values returned from Play Integrity
    private const string MEETS_BASIC_INTEGRITY = "MEETS_BASIC_INTEGRITY";
    private const string MEETS_DEVICE_INTEGRITY = "MEETS_DEVICE_INTEGRITY";

    private const string MEETS_STRONG_INTEGRITY = "MEETS_STRONG_INTEGRITY";
    // Virtual integrity is for trusted emulators, like Google Play Games for PC
    private const string MEETS_VIRTUAL_INTEGRITY = "MEETS_VIRTUAL_INTEGRITY";
    private const string APP_VERSION_RECOGNIZED = "PLAY_RECOGNIZED";
    private const string APP_VERSION_UNRECOGNIZED = "UNRECOGNIZED_VERSION";
    private const string ACCOUNT_LICENSED = "LICENSED";
    private const string ACCOUNT_UNLICENSED = "UNLICENSED";
    private const string UNEVALUATED_VALUE = "UNEVALUATED";

    // The nonce returned from the server to associate with an integrity request
    private string _nonceString = "";

    // We only perform an integrity check once per application session
    private bool _performedIntegrityCheck = false;

    // Class to serialize the getNonce response Json, using
    // the UnityEngine.JsonUtility class.
    [Serializable]
    public class NonceResult
    {
        public string nonce;
    }

    public void StartIntegrityCheck()
    {
        Debug.Log("StartIntegrityCheck called");
        if (!_performedIntegrityCheck)
        {
            StartCoroutine(RunIntegrityCheck());
        }
    }

    IEnumerator RunIntegrityCheck()
    {
        // Call our server to retrieve a nonce.
        yield return GetNonceRequest();
        if (!String.IsNullOrEmpty(_nonceString))
        {
            // Create an instance of an integrity manager.
            var integrityManager = new IntegrityManager();

            // Request the integrity token by providing a nonce.
            var tokenRequest = new IntegrityTokenRequest(_nonceString);
            var requestIntegrityTokenOperation =
                integrityManager.RequestIntegrityToken(tokenRequest);

            // Wait for PlayAsyncOperation to complete.
            yield return requestIntegrityTokenOperation;

            // Check the resulting error code.
            if (requestIntegrityTokenOperation.Error != IntegrityErrorCode.NoError)
            {
                Debug.Log("IntegrityAsyncOperation failed with error: " +
                                requestIntegrityTokenOperation.Error);
                yield break;
            }

            // Get the response.
            var tokenResponse = requestIntegrityTokenOperation.GetResult();

            // Send the response to our server with a POST request to
            // decrypt and verify, then process the resulting integrity result.
            yield return PostTokenReponse(tokenResponse.Token);
        }
    }

    IEnumerator PostTokenReponse(string tokenResponse)
    {
        // Start a HTTP POST request to the processToken URL, sending it the
        // integrity token data provided by Play Integrity.
        var tokenRequest = new UnityWebRequest(URL_PROCESSTOKEN, "POST");
        string tokenJson = "{ \"tokenString\" : \"" + tokenResponse + "\" }";
        byte[] jsonBuffer = new System.Text.UTF8Encoding().GetBytes(tokenJson);
        tokenRequest.uploadHandler = (UploadHandler)new UploadHandlerRaw(jsonBuffer);
        tokenRequest.downloadHandler = (DownloadHandler) new DownloadHandlerBuffer();
        tokenRequest.SetRequestHeader(CONTENT_TYPE, JSON_CONTENT);
        yield return tokenRequest.SendWebRequest();

        if (tokenRequest.result == UnityWebRequest.Result.Success)
        {
            // Parse the integrity verdict response Json
            ProcessVerdict(tokenRequest.downloadHandler.text);
        }
        else
        {
            Debug.Log("Web request error on processToken: " + tokenRequest.error);
        }
    }

    IEnumerator GetNonceRequest()
    {
        // Start a HTTP GET request to the getNonce URL to retrieve a new
        // nonce to associate with our integrity check.
        var nonceRequest = new UnityWebRequest(URL_GETNONCE, "GET");
        nonceRequest.downloadHandler = (DownloadHandler) new DownloadHandlerBuffer();
        nonceRequest.SetRequestHeader(CONTENT_TYPE, JSON_CONTENT);
        yield return nonceRequest.SendWebRequest();

        if (nonceRequest.result == UnityWebRequest.Result.Success)
        {
            var result = JsonUtility.FromJson<NonceResult>(nonceRequest.downloadHandler.text);
            if (result != null)
            {
                _nonceString = result.nonce;
            }
            else
            {
                Debug.Log("Invalid nonce json");
                _nonceString = "";
            }
        }
        else
        {
            Debug.Log("Web request error on getNonce: " + nonceRequest.error);
            _nonceString = "";
        }
    }

    void ProcessVerdict(string verdictJson)
    {
        _performedIntegrityCheck = true;
        Debug.Log("ProcessVerdict");
        Debug.Log(verdictJson);

        var label = verdictLabel.GetComponent<Text>();
        // Serialize the verdict Json into our IntegrityVerdict data class.
        var verdict = JsonUtility.FromJson<IntegrityVerdict>(verdictJson);
        if (verdict == null)
        {
            Debug.Log("Invalid IntegrityVerdict json");
            label.text = "Invalid verdict json";
            return;
        }

        // The server already checked the nonce, look at the integrity signals
        // in the verdict
        bool foundDeviceIntegrity = false;
        string verdictSummary = "Device integrity: ";
        foreach(var deviceVerdict in verdict.deviceIntegrity.deviceRecognitionVerdict)
        {
            switch (deviceVerdict)
            {
                case MEETS_BASIC_INTEGRITY:
                    foundDeviceIntegrity = true;
                    verdictSummary += "Basic ";
                    break;
                case MEETS_DEVICE_INTEGRITY:
                    foundDeviceIntegrity = true;
                    verdictSummary += "Device ";
                    break;
                case MEETS_STRONG_INTEGRITY:
                    foundDeviceIntegrity = true;
                    verdictSummary += "Strong ";
                    break;
                case MEETS_VIRTUAL_INTEGRITY:
                    foundDeviceIntegrity = true;
                    verdictSummary += "Virtual ";
                    break;
                default:
                    Debug.Log("Unknown device integrity signal: " + deviceVerdict);
                    break;
            }
        }

        if (!foundDeviceIntegrity)
        {
            // Didn't find any of the device integrity signals, say the
            // device failed integrity
            verdictSummary += "failed integrity ";
        }

        switch (verdict.appIntegrity.appRecognitionVerdict)
        {
            case APP_VERSION_RECOGNIZED:
                verdictSummary += "- App version recognized";
                break;
            case APP_VERSION_UNRECOGNIZED:
                verdictSummary += "- App version unrecognized";
                break;
            case UNEVALUATED_VALUE:
                verdictSummary += "- App version unevaluated";
                break;
            default:
                Debug.Log("Unrecognized app verdict: " +
                          verdict.appIntegrity.appRecognitionVerdict);
                break;
        }

        switch (verdict.accountDetails.appLicensingVerdict)
        {
            case ACCOUNT_LICENSED:
                verdictSummary += "- Account licensed";
                break;
            case ACCOUNT_UNLICENSED:
                verdictSummary += "- Account unlicensed";
                break;
            case UNEVALUATED_VALUE:
                verdictSummary += "- Account unevaluated";
                break;
            default:
                Debug.Log("Unrecognized account verdict: " +
                          verdict.accountDetails.appLicensingVerdict);
                break;
        }

        Debug.Log("Verdict summary string: " + verdictSummary);
        label.text = verdictSummary;
    }
#endif
}
