// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using System;
using System.Collections.Generic;
using System.Globalization;
using UnityEngine;
using UnityEngine.Android;
using UnityEngine.UI;

/// <summary>
/// GameManager initializes the game on startup and controls the 'play' canvas.
/// GameManager initializes constant data and requests the load of savegame data.
/// Canvas switches between the 'play' 'store' 'garage' and 'pgs' canvases are
/// controlled through GameManager.
/// </summary>
public class GameManager : MonoBehaviour
{
    public GameObject playPageCanvas;
    public GameObject pgsPageCanvas;
    public GameObject waitCanvas;

    private List<GameObject> _canvasPagesList;
    private bool _cloudLoadMessageActive;
    private bool _waitMessageActive;


    private enum CloudStatus
    {
        // Waiting for initialization of cloud save to finish
        CloudStatus_WaitingForInit = 0,
        // Cloud save is disabled
        CloudStatus_Disabled,
        // Waiting for a load from a cloud save to complete
        CloudStatus_WaitingForLoad,
        // Cloud save is initialized and idle
        CloudStatus_Idle,
        // Prompting if the user wishes to load a later cloud
        // save following the game returning to the foreground
        CloudStatus_UserPrompt,
    }

    private CloudStatus _cloudStatus;
    private PGSController _pgsController = null;



    // Init the game.
    public void Awake()
    {
        _cloudLoadMessageActive = false;
        _pgsController = FindObjectOfType<PGSController>();
        _pgsController.SetLocalSaveDataReady();
        _cloudStatus = CloudStatus.CloudStatus_WaitingForInit;
        _canvasPagesList = new List<GameObject>() { playPageCanvas, pgsPageCanvas, waitCanvas };
        // Display a please wait message while PGS signin and cloud save
        // resolution happens
        SetWaitMessageActive(true);

        SetCanvas(playPageCanvas);
    }

    public bool GetCloudLoadMessageActive()
    {
        return _cloudLoadMessageActive;
    }

    public bool GetWaitMessageActive()
    {
        return _waitMessageActive;
    }

    public void SetWaitMessageActive(bool active)
    {
        if (active != _waitMessageActive)
        {
            Debug.Log("SetWaitMessageActive: " + active);
            _waitMessageActive = active;
            waitCanvas.SetActive(active);
        }
    }
    
    public void OnEnterPlayPageButtonClicked()
    {
        SetCanvas(playPageCanvas);
    }

    // Switch pages when entering the play games services page
    public void OnEnterPGSPageButtonClicked()
    {
        SetCanvas(pgsPageCanvas);
    }

    private void SetCanvas(GameObject targetCanvasPage)
    {
        // Set all canvas pages to be inactive.
        foreach (var canvasPage in _canvasPagesList)
        {
            canvasPage.SetActive(false);
        }

        // Set the target canvas page to be active.
        targetCanvasPage.SetActive(true);
    }

    // Check if the player is in play mode (page).
    public bool IsInPlayCanvas()
    {
        return playPageCanvas.activeInHierarchy;
    }

    private void OnApplicationFocus(bool hasFocus)
    {
        if (!hasFocus)
        {
            // Save when focus is lost
            // DoSaveGame();
        }
    }

    private void OnApplicationPause(bool pauseStatus)
    {
        if (!pauseStatus && _pgsController != null)
        {
            Debug.Log("OnApplicationPause, resuming : " + _pgsController.CurrentSignInStatus);
        }
    }

    private void OnApplicationQuit()
    {
        // Save before exiting
        // DoSaveGame();
    }

    private void OnDestroy()
    {

    }
}
