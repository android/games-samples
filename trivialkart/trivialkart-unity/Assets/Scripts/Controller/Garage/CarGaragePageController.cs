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

using UnityEngine;
using UnityEngine.InputSystem;

/// <summary>
/// Controller for the car garage page.
/// It listens to the car switch button click events;
/// It controls availability and selected status of car garage items.
/// </summary>
public class CarGaragePageController : MonoBehaviour
{
    public InputActionAsset inputActionAsset;
    private InputAction _leftTabAction;
    private InputAction _rightTabAction;
    
    private void Awake()
    {
        _leftTabAction = inputActionAsset.FindAction("LeftTab");
        _leftTabAction.Enable();
        
        _rightTabAction = inputActionAsset.FindAction("RightTab");
        _rightTabAction.Enable();
    }
    
    private void Update()
    {
        // Use the keyboard arrows to select the car
        if (_leftTabAction.WasPressedThisFrame())
        {
            SwitchToPreviousCar();
        }
        if (_rightTabAction.WasPressedThisFrame())
        {
            SwitchToNextCar();
        }
    }

    // Refresh the page when switching to the car garage page.
    private void OnEnable()
    {
        RefreshPage();
    }

    private void RefreshPage()
    {
        SetCarAvailability();
        SetCarUsageStatus();
    }

    // Check if player owns the car.
    private void SetCarAvailability()
    {
        foreach (var car in CarList.List)
        {
            var isCarOwned = GameDataController.GetGameData().CheckCarOwnership(car);
            car.GarageItemGameObj.SetActive(isCarOwned);
        }
    }

    private void SetCarUsageStatus()
    {
        foreach (var carObj in CarList.List)
        {
            carObj.GarageItemGameObj.transform.Find("statusText").gameObject.SetActive(false);
        }

        GameDataController.GetGameData().CarInUseObj.GarageItemGameObj.transform.Find("statusText").gameObject
            .SetActive(true);
    }

    public void OnCarGarageItemClicked(int carIndex)
    {
        SwitchCarInUse(CarList.GetCarByCarIndex(carIndex));
    }

    private void SwitchCarInUse(CarList.Car targetCar)
    {
        GameDataController.GetGameData().UpdateCarInUse(targetCar);
        SetCarUsageStatus();
    }

    // Select the next unlocked car from the current index
    private void SwitchToNextCar()
    {
        // Current car index
        int carIndex = CarList
            .List.IndexOf(GameDataController.GetGameData().CarInUseObj);
        int totalCars = CarList.List.Count;

        // Search for the next unlocked car to the right
        do
        {
            carIndex = (carIndex + 1) % totalCars;
        } while (!GameDataController.GetGameData()
            .CheckCarOwnership(CarList.GetCarByCarIndex(carIndex)));
        SwitchCarInUse(CarList.GetCarByCarIndex(carIndex));
    }

    // Select the previous unlocked car from the current index
    private void SwitchToPreviousCar()
    {
        // Current car index
        int carIndex = CarList
            .List.IndexOf(GameDataController.GetGameData().CarInUseObj);
        int totalCars = CarList.List.Count;

        // Search for the next unlocked car to the left
        do
        {
            carIndex = (carIndex - 1 + totalCars) % totalCars;
        } while (!GameDataController.GetGameData()
            .CheckCarOwnership(CarList.GetCarByCarIndex(carIndex)));
        SwitchCarInUse(CarList.GetCarByCarIndex(carIndex));
    }
}