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

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Android;

#if PLAY_GAMES_PC
using Google.Play.InputMapping;
#endif

/// <summary>
/// GameManager inits the game when the game starts and controls the play canvas.
/// It inits constant data, requests for game data load;
/// It controls canvas switches among play, store and garage;
/// </summary>
public class GameManager : MonoBehaviour
{
    public GameObject playPageCanvas;
    public GameObject storePageCanvas;
    public GameObject garagePageCanvas;
    public GameObject pgsPageCanvas;
    public GameObject waitCanvas;
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
    private bool _waitMessageActive;

#if PLAY_GAMES_PC
    private readonly InputSDKMappingProvider _inputMapProvider = new InputSDKMappingProvider();
#endif

    // Init the game.
    public void Awake()
    {
        _waitMessageActive = false;
#if USE_SERVER
        NetworkRequestController.RegisterUserDevice();
#endif
        InitConstantData();
        GameDataController.LoadGameData();
        SetCanvas(playPageCanvas);

#if PLAY_GAMES_PC
        InputMappingClient inputMappingClient =
            Google.Play.InputMapping.Input.GetInputMappingClient();
        inputMappingClient.RegisterInputMappingProvider(_inputMapProvider);
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

    }

    public bool GetWaitMessageActive()
    {
        return _waitMessageActive;
    }

    public void SetWaitMessageActive(bool active)
    {
        if (active != _waitMessageActive)
        {
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

    private void OnApplicationPause(bool pauseStatus)
    {
        GameDataController.SaveGameData();
    }

    private void OnApplicationQuit()
    {
        GameDataController.SaveGameData();

#if PLAY_GAMES_PC
        InputMappingClient inputMappingClient =
            Google.Play.InputMapping.Input.GetInputMappingClient();
        inputMappingClient.UnregisterInputMappingProvider(_inputMapProvider);
#endif
    }
}
