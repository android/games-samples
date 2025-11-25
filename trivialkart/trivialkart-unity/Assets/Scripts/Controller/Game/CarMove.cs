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

using System.Globalization;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.InputSystem;

/// <summary>
/// A component script for a specific car game object.
/// It controls movement of a specific car in the play mode.
/// CarMove script is added as a component to every car game object in the play mode.
/// </summary>
public class CarMove : MonoBehaviour
{
    public InputActionAsset inputActionAsset;
    public GameObject odometerLabel;
    public GameObject tapToDriveText;
    public float carSpeed;

    private Rigidbody2D _rigidbody2D;
    private GameManager _gameManger;
    private PGSController _pgsController;
    private Gas _gas;
    private Text _odometerText;
    private const float NoVelocity = 0.01f;
    private const float TurboVelocity = 1.5f;

    private int _clickCounter;
    private float _firstClickTime;
    private const float ClickDelay = 0.15f;

    private const int NORMAL_SPEED = 1;
    private const int TURBO_SPEED = 2;

    // Achievement for going this far
    private const float ACHIEVEMENT_DISTANCE = 30.0f;

    private InputAction _garageAction;
    private InputAction _pgsAction;
    private InputAction _storeAction;
    private InputAction _playPageAction;
    private InputAction _moveAction;
    private InputAction _boostAction;
    

    private void Start()
    {
        _clickCounter = 0;
        _firstClickTime = 0f;
        _gameManger = FindObjectOfType<GameManager>();
        _pgsController = FindObjectOfType<PGSController>();
        _rigidbody2D = GetComponent<Rigidbody2D>();
        _gas = transform.parent.gameObject.GetComponent<Gas>();
        _odometerText = odometerLabel.GetComponent<Text>();
        
        _garageAction = inputActionAsset.FindAction("Garage");
        _garageAction.Enable();
        
        _pgsAction = inputActionAsset.FindAction("PGS");
        _pgsAction.Enable();
        
        _storeAction = inputActionAsset.FindAction("Store");
        _storeAction.Enable();
        
        _playPageAction  = inputActionAsset.FindAction("PlayPage");
        _playPageAction.Enable();
        
        _moveAction = inputActionAsset.FindAction("Move");
        _moveAction.Enable();
        
        _boostAction = inputActionAsset.FindAction("Boost");
        _boostAction.Enable();
        
        PlayerPrefs.SetFloat("dist", 0f);
    }

    private void FixedUpdate()
    {
        float newDistance = _rigidbody2D.linearVelocity.x * Time.deltaTime;
        newDistance += PlayerPrefs.GetFloat("dist", 0f);
        PlayerPrefs.SetFloat("dist", newDistance);
        CheckDistanceAchievement(newDistance);
        _odometerText.text = newDistance.ToString("N1", CultureInfo.CurrentCulture);
    }
    
    private void CheckDistanceAchievement(float newDistance)
    {
        if (_pgsController.CurrentSignInStatus == PGSController.PgsSigninStatus.PgsSigninLoggedIn)
        {
            var achievementManager = _pgsController.AchievementManager;
            if (!achievementManager.GetAchievementUnlocked(
                    PGSAchievementManager.TrivialKartAchievements.Tk_Achievement_Distance) &&
                newDistance >= ACHIEVEMENT_DISTANCE)
            {
                achievementManager.UnlockAchievement(
                    PGSAchievementManager.TrivialKartAchievements.Tk_Achievement_Distance);
            }
        }
    }

    private void Update()
    {
        // Checks to see if we are due to update the distance traveled leaderboard
        //_pgsController.UpdateLeaderboard();

        
        if (_pgsAction.WasPressedThisFrame())
        {
            _gameManger.OnEnterPGSPageButtonClicked();
        }
        if (_playPageAction.WasPressedThisFrame())
        {
            _gameManger.OnEnterPlayPageButtonClicked();
        }

        // Check for keyboard controls
        if (_moveAction.WasPressedThisFrame())
        {
            if (_boostAction.WasPerformedThisFrame())
            {
                // left shift + space for turbo
                ProcessClickAction(TURBO_SPEED);
            }
            else
            {
                // single space to drive
                ProcessClickAction(NORMAL_SPEED);
            }
        }
    }

    // Moves the car at a velocity given by the number of clicks
    private void ProcessClickAction(int clicks)
    {
        if (_rigidbody2D.linearVelocity.magnitude < NoVelocity &&
                _gas.HasGas() && _gameManger.IsInPlayCanvas())
        {
            switch (clicks)
            {
                case NORMAL_SPEED:
                    // Single click to drive
                    Drive();
                    break;
                case TURBO_SPEED:
                default:
                    // Double click for turbo
                    Turbo();
                    break;
            }
        }
        _clickCounter = 0;
    }

    private void Drive()
    {
        tapToDriveText.SetActive(false);
        _rigidbody2D.AddForce(new Vector2(carSpeed, 0));
    }

    private void Turbo()
    {
        tapToDriveText.SetActive(false);
        _rigidbody2D.AddForce(new Vector2(carSpeed * TurboVelocity, 0));
    }
}
