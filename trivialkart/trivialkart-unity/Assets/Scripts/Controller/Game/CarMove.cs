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

/// <summary>
/// A component script for a specific car game object.
/// It controls movement of a specific car in the play mode.
/// CarMove script is added as a component to every car game object in the play mode.
/// </summary>
public class CarMove : MonoBehaviour
{
    public GameObject tapToDriveText;
    public CarName carName;

    private Rigidbody2D _rigidbody2D;
    private GameManager _gameManger;
    private CarList.Car _carObj;
    private Gas _gas;
    private const float NoVelocity = 0.01f;
    private const float TurboVelocity = 1.5f;

    private int _clickCounter;
    private float _firstClickTime;
    private const float ClickDelay = 0.15f;

    private const int NORMAL_SPEED = 1;
    private const int TURBO_SPEED = 2;

    private void Start()
    {
        _clickCounter = 0;
        _firstClickTime = 0f;
        _gameManger = FindObjectOfType<GameManager>();
        // Get the carObj corresponding to the car game object the script attached to.
        _carObj = CarList.GetCarByName(carName);
        _rigidbody2D = GetComponent<Rigidbody2D>();
        _gas = transform.parent.gameObject.GetComponent<Gas>();
    }

    private void Update()
    {
        // Use keys to control menu/gameplay
        if (Input.GetKeyUp(KeyCode.G))
        {
            _gameManger.OnEnterGaragePageButtonClicked();
        }
        if (Input.GetKeyUp(KeyCode.S))
        {
            _gameManger.OnEnterStoreButtonClicked();
        }
        if (Input.GetKeyUp(KeyCode.Escape))
        {
            _gameManger.OnEnterPlayPageButtonClicked();
        }

        // Check for keyboard controls
        if (Input.GetKeyUp(KeyCode.Space))
        {
            if (Input.GetKey(KeyCode.LeftShift))
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
        else
        {
            // Check for fresh touches and see if they touched the car.
            for (int i = 0; i < Input.touchCount; ++i)
            {
                var touch = Input.GetTouch(i);
                if (touch.phase == TouchPhase.Began)
                {
                    ProcessTouch(touch.position);
                }
            }
        }

        // left mouse button
#if UNITY_EDITOR
        if (Input.GetMouseButtonDown(0))
        {
            ProcessTouch(Input.mousePosition);
        }
#endif
        // Check for pending car movements
        if (_clickCounter > 0 && Time.time - _firstClickTime > ClickDelay)
        {
            ProcessClickAction(_clickCounter);
        }
    }

    // Moves the car at a velocity given by the number of clicks
    private void ProcessClickAction(int clicks)
    {
        if (_rigidbody2D.velocity.magnitude < NoVelocity &&
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

    private void ProcessTouch(Vector2 touchPosition)
    {
        Ray touchRay = Camera.main.ScreenPointToRay(touchPosition);
        RaycastHit2D touchHit = Physics2D.Raycast(touchRay.origin, touchRay.direction);
        if (touchHit.rigidbody == _rigidbody2D)
        {
            if (_clickCounter == 0)
            {
                _firstClickTime = Time.time;
            }
            _clickCounter++;
        }
    }

    private void Drive()
    {
        tapToDriveText.SetActive(false);
        _rigidbody2D.AddForce(new Vector2(_carObj.Speed, 0));
    }

    private void Turbo()
    {
        tapToDriveText.SetActive(false);
        _rigidbody2D.AddForce(new Vector2(_carObj.Speed * TurboVelocity, 0));
    }
}