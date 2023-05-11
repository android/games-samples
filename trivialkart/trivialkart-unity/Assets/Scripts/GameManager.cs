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
#if PLAY_GAMES_PC
using Google.Android.Libraries.Play.Games.Inputmapping;
using Google.Android.Libraries.Play.Games.Inputmapping.ExternalType.Android.Content;
using Google.LibraryWrapper.Java;
#endif
/// <summary>
/// GameManager initializes the game on startup and controls the 'play' canvas.
/// GameManager initializes constant data and requests the load of savegame data.
/// Canvas switches between the 'play' 'store' 'garage' and 'pgs' canvases are
/// controlled through GameManager.
/// </summary>
public class GameManager : MonoBehaviour
{
    public GameObject playPageCanvas;
    public GameObject storePageCanvas;
    public GameObject garagePageCanvas;
    public GameObject pgsPageCanvas;
    public GameObject waitCanvas;
    public GameObject gamePlayCanvasController;
    public GameObject cloudLoadCanvas;
    public GameObject cloudLoadTextGameObject;
    public GameObject storeItemFiveCoinGameObject;
    public GameObject storeItemTenCoinGameObject;
    public GameObject storeItemTwentyCoinGameObject;
    public GameObject storeItemFiftyCoinGameObject;
    public GameObject storeItemCarSedanGameObj;
    public GameObject storeItemCarTruckGameObj;
    public GameObject storeItemCarOffroadGameObj;
    public GameObject storeItemCarKartGameObj;
    public GameObject garageItemCarSedanGameObj;
    public GameObject garageItemCarTruckGameObj;
    public GameObject garageItemCarOffroadGameObj;
    public GameObject garageItemCarKartGameObj;
    public GameObject playCarSedanGameObj;
    public GameObject playCarOffroadGameObj;
    public GameObject playCarTruckGameObj;
    public GameObject playCarKartGameObj;
    public GameObject blueGrassBackgroundGarageItemGameObj;
    public GameObject mushroomBackGroundGarageItemGameObj;
    public GameObject silverVipSubscribeButtonGameObj;
    public GameObject goldenVipSubscribeButtonGameObj;

    private List<GameObject> _canvasPagesList;
    private bool _cloudLoadMessageActive;
    private bool _waitMessageActive;

#if PLAY_GAMES_SERVICES
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
#endif

#if PLAY_GAMES_PC
    private readonly InputSDKMappingProvider _inputMapProvider = new InputSDKMappingProvider();
    private InputMappingClient _inputMappingClient;
#endif

    // Init the game.
    public void Awake()
    {
        InitConstantData();
        GameDataController.LoadGameData();
        _cloudLoadMessageActive = false;
#if PLAY_GAMES_SERVICES
        _pgsController = FindObjectOfType<PGSController>();
        _pgsController.SetLocalSaveDataReady();
        _cloudStatus = CloudStatus.CloudStatus_WaitingForInit;
        // Display a please wait message while PGS signin and cloud save
        // resolution happens
        SetWaitMessageActive(true);
#else
        SetWaitMessageActive(false);
#endif
#if PLAY_GAMES_PC
        // Set the key used for menu buttons
        GameObject.Find("GarageButton").GetComponentInChildren<Text>().text
            = string.Format("Garage ({0})", "G");
        GameObject.Find("PGSButton").GetComponentInChildren<Text>().text
            = string.Format("PGS({0})", "P");
        GameObject.Find("StoreButton").GetComponentInChildren<Text>().text
            = string.Format("Store({0})", "S");

        Context context = (Context)Utils.GetUnityActivity().GetRawObject();
        _inputMappingClient =
                Google.Android.Libraries.Play.Games.Inputmapping.Input.GetInputMappingClient(context);
        // Register InputRemappingListener before registering the InputMappingProvider
        _inputMappingClient.RegisterRemappingListener(new InputSDKRemappingListener());
        // Register the InputMappingProvider before setting any context
        _inputMappingClient.SetInputMappingProvider(_inputMapProvider);
#endif

#if PLAY_GAMES_PC
        // On a PC, some Android features available on a mobile phone or tablet
        // will be inaccessible. If your game requests access to an unsupported
        // permission, the request automatically fails or has unknown behavior.
#elif PLATFORM_ANDROID
       if (!Permission.HasUserAuthorizedPermission(Permission.Camera)) {
            Permission.RequestUserPermission(Permission.Camera);
       }
#endif
        SetCanvas(playPageCanvas);
    }

#if PLAY_GAMES_SERVICES
    // Only used if PGS is active to monitor cloud save operations
    public void Update()
    {
        if (_cloudStatus == CloudStatus.CloudStatus_WaitingForInit)
        {
            UpdateCloudWaitingForInit();
        }
        else if (_cloudStatus == CloudStatus.CloudStatus_WaitingForLoad)
        {
            UpdateCloudWaitingForLoad();
        }
        else if (_cloudStatus == CloudStatus.CloudStatus_Idle)
        {
            CheckForUpdatedCloudSave();
        }
    }

    private void UpdateCloudWaitingForInit()
    {
        var cloudSaveManager = _pgsController.CloudSaveManager;
        if (cloudSaveManager.CloudSaveStatus == PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudDisabled)
        {
            Debug.Log("Cloud save not available");
            _cloudStatus = CloudStatus.CloudStatus_Disabled;
            SetWaitMessageActive(false);
        }
        else if (cloudSaveManager.CloudSaveStatus == PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudReady)
        {
            if (cloudSaveManager.HasCloudSave)
            {
                cloudSaveManager.LoadCloudSave();
                _cloudStatus = CloudStatus.CloudStatus_WaitingForLoad;
            }
            else
            {
                // If no cloud save exists, create one using the current local game data
                DoSaveGame();
                SetWaitMessageActive(false);
                _cloudStatus = CloudStatus.CloudStatus_Idle;
            }
        }
    }

    private void UpdateCloudWaitingForLoad()
    {
        var cloudSaveManager = _pgsController.CloudSaveManager;
        if (cloudSaveManager.CloudSaveStatus ==
            PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudDisabled)
        {
            Debug.Log("Cloud load not available");
            _cloudStatus = CloudStatus.CloudStatus_Disabled;
            SetWaitMessageActive(false);
        }
        else if (cloudSaveManager.CloudSaveStatus ==
                 PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudReady)
        {
            Debug.Log("Updating local save file from cloud");
            string saveData = cloudSaveManager.CloudSaveData;
            GameDataController.LoadGameDataFromJson(saveData);
            SetWaitMessageActive(false);
            _cloudStatus = CloudStatus.CloudStatus_Idle;
            // Refresh the coin counter on screen
            var playController = gamePlayCanvasController.GetComponent<GamePlayCanvasController>();
            playController.RefreshPage();
        }
    }

    // Check to see if the cloud save manager is reporting a later save, this can happen
    // when we resume gameplay after the user was playing on a different device
    // Also check if game data is marked as updated (i.e. we purchased something from
    // the store) and trigger a save if so.
    private void CheckForUpdatedCloudSave()
    {
        var cloudSaveManager = _pgsController.CloudSaveManager;
        if (cloudSaveManager.CloudSaveStatus ==
            PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudLaterSaveAvailable)
        {
            Debug.Log("Later cloud save available");
            _cloudStatus = CloudStatus.CloudStatus_UserPrompt;
            // Convert the timespan 'time played' to its original distance traveled
            // value and use it to prompt the user whether they would like to load
            // the cloud save
            var laterTimespan = cloudSaveManager.GetLaterCloudSaveTimespan();
            var laterDistance = GameDataController.ConvertTimespanToDistance(laterTimespan);
            SetCloudLoadMessageText(laterDistance);
            SetCloudLoadMessageActive(true);
        }
        else if (cloudSaveManager.CloudSaveStatus ==
                 PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudReady &&
                 GameDataController.GetGameDataUpdated())
        {
            DoSaveGame();
        }
    }
#endif

    public void OnConfirmCloudLoadButtonClicked()
    {
#if PLAY_GAMES_SERVICES
        _pgsController.CloudSaveManager.LoadLaterCloudSave();
        SetCloudLoadMessageActive(false);
        _cloudStatus = CloudStatus.CloudStatus_WaitingForLoad;
#endif
    }

    public void OnDeclineCloudLoadButtonClicked()
    {
#if PLAY_GAMES_SERVICES
        _pgsController.CloudSaveManager.IgnoreLaterCloudSave();
        SetCloudLoadMessageActive(false);
        _cloudStatus = CloudStatus.CloudStatus_Idle;
#endif
    }

    public bool GetCloudLoadMessageActive()
    {
        return _cloudLoadMessageActive;
    }

    public bool GetWaitMessageActive()
    {
        return _waitMessageActive;
    }

    public void SetCloudLoadMessageActive(bool active)
    {
        if (active != _cloudLoadMessageActive)
        {
            _cloudLoadMessageActive = active;
            cloudLoadCanvas.SetActive(active);
        }
    }

    public void SetCloudLoadMessageText(float distance)
    {
        string distanceString = distance.ToString("N1", CultureInfo.CurrentCulture);
        string cloudString = String.Format("Load the cloud save with {0} distance traveled?",
            distanceString);
        Text cloudText = cloudLoadTextGameObject.GetComponent<Text>();
        cloudText.text = cloudString;
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

    // Switch pages when entering the store.
    public void OnEnterStoreButtonClicked()
    {
        SetCanvas(storePageCanvas);
    }

    // Switch pages when returning to play mode.
    public void OnEnterPlayPageButtonClicked()
    {
        SetCanvas(playPageCanvas);
    }

    // Switch pages when entering the garage.
    public void OnEnterGaragePageButtonClicked()
    {
        SetCanvas(garagePageCanvas);
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
#if PLAY_GAMES_PC
        // Update InputContext. Make sure to have registered your
        // InputMappingProvider before setting any context.
        if (IsInPlayCanvas())
        {
            _inputMappingClient
                .SetInputContext(InputSDKMappingProvider.roadControlsContext);
        }
        else
        {
            _inputMappingClient
                .SetInputContext(InputSDKMappingProvider.menuControlsContext);
        }
#endif
    }

    // Check if the player is in play mode (page).
    public bool IsInPlayCanvas()
    {
        return playPageCanvas.activeInHierarchy;
    }

    // Init constant game data before the game starts.
    private void InitConstantData()
    {
        InitCoinList();
        InitCarList();
        InitBackGroundList();
        InitSubscriptionList();
        _canvasPagesList = new List<GameObject>() {playPageCanvas,
            storePageCanvas, garagePageCanvas, pgsPageCanvas};
    }

    // Link car game object to the car object in carList.
    private void InitCarList()
    {
        // TODO: Improve it.
        CarList.CarSedan.GarageItemGameObj = garageItemCarSedanGameObj;
        CarList.CarSedan.PlayCarGameObj = playCarSedanGameObj;
        CarList.CarSedan.StoreItemCarGameObj = storeItemCarSedanGameObj;
        CarList.CarTruck.GarageItemGameObj = garageItemCarTruckGameObj;
        CarList.CarTruck.PlayCarGameObj = playCarTruckGameObj;
        CarList.CarTruck.StoreItemCarGameObj = storeItemCarTruckGameObj;
        CarList.CarOffroad.GarageItemGameObj = garageItemCarOffroadGameObj;
        CarList.CarOffroad.PlayCarGameObj = playCarOffroadGameObj;
        CarList.CarOffroad.StoreItemCarGameObj = storeItemCarOffroadGameObj;
        CarList.CarKart.GarageItemGameObj = garageItemCarKartGameObj;
        CarList.CarKart.PlayCarGameObj = playCarKartGameObj;
        CarList.CarKart.StoreItemCarGameObj = storeItemCarKartGameObj;
    }

    // Link background game object to the background object in backgroundList.
    private void InitBackGroundList()
    {
        BackgroundList.BlueGrassBackground.GarageItemGameObj = blueGrassBackgroundGarageItemGameObj;
        BackgroundList.BlueGrassBackground.ImageSprite = Resources.Load<Sprite>("background/blueGrass");
        BackgroundList.MushroomBackground.GarageItemGameObj = mushroomBackGroundGarageItemGameObj;
        BackgroundList.MushroomBackground.ImageSprite = Resources.Load<Sprite>("background/coloredShroom");
    }

    private void InitSubscriptionList()
    {
        SubscriptionList.SilverSubscription.SubscribeButtonGameObj = silverVipSubscribeButtonGameObj;
        SubscriptionList.GoldenSubscription.SubscribeButtonGameObj = goldenVipSubscribeButtonGameObj;
        SubscriptionList.NoSubscription.SubscribeButtonGameObj = new GameObject();
    }

    private void InitCoinList()
    {
        CoinList.FiveCoins.StoreItemCoinGameObj = storeItemFiveCoinGameObject;
        CoinList.TenCoins.StoreItemCoinGameObj = storeItemTenCoinGameObject;
        CoinList.TwentyCoins.StoreItemCoinGameObj = storeItemTwentyCoinGameObject;
        CoinList.FiftyCoins.StoreItemCoinGameObj = storeItemFiftyCoinGameObject;
    }

    private void DoSaveGame()
    {
        GameDataController.SaveGameData();
#if PLAY_GAMES_SERVICES
        if (_pgsController != null)
        {
            if (_pgsController.CurrentSignInStatus ==
                PGSController.PgsSigninStatus.PgsSigninLoggedIn)
            {
                var cloudSaveManager = _pgsController.CloudSaveManager;
                if (cloudSaveManager.CloudSaveStatus ==
                    PGSCloudSaveManager.PgsCloudSaveStatus.PgsCloudReady &&
                    _cloudStatus == CloudStatus.CloudStatus_Idle)
                {
                    // Convert 'distance traveled' to be our 'time spent playing' metric
                    // and pass it in the expected TimeSpan format
                    cloudSaveManager.SaveToCloud(GameDataController.ExportGameDataJson(),
                        GameDataController.GetGameData().DistanceTraveledAsTimespan());
                }
            }
        }
#endif
    }

    private void OnApplicationFocus(bool hasFocus)
    {
        if (!hasFocus)
        {
            // Save when focus is lost
            DoSaveGame();
        }
    }

#if PLAY_GAMES_SERVICES
    private void OnApplicationPause(bool pauseStatus)
    {
        if (!pauseStatus && _pgsController != null)
        {
            Debug.Log("OnApplicationPause, resuming : " + _pgsController.CurrentSignInStatus);
            if (_pgsController.CurrentSignInStatus ==
                PGSController.PgsSigninStatus.PgsSigninLoggedIn)
            {
                // Check for cloud save updates when we resume
                var cloudSaveManager = _pgsController.CloudSaveManager;
                cloudSaveManager.CheckForCloudUpdates();
            }
        }
    }
#endif

    private void OnApplicationQuit()
    {
        // Save before exiting
        DoSaveGame();
    }

    private void OnDestroy()
    {
#if PLAY_GAMES_PC
        _inputMappingClient.ClearInputMappingProvider();
        _inputMappingClient.ClearRemappingListener();
#endif
    }
}
