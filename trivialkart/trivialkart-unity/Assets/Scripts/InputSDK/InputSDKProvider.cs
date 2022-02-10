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

#if PLAY_GAMES_PC
using Google.Play.InputMapping;
using System.Collections.Generic;

public class InputSDKMappingProvider : InputMappingProvider
{
    public enum InputEventIds
    {
        DRIVE,
        TURBO,
        OPEN_GARAGE,
        OPEN_STORE,
        CLOSE_GARAGE_OR_STORE,
        CHANGE_TAB,
        SELECT_NEXT_CAR_OR_BACKGROUND,
        SELECT_PREVIOUS_CAR_OR_BACKGROUND,
    }

    public InputMap OnProvideInputMap()
    {
        var driveAction = new InputAction
        {
            ActionLabel = "Drive",
            UniqueId = (int)InputEventIds.DRIVE,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_SPACE }
            }
        };

        var turboAction = new InputAction
        {
            ActionLabel = "Turbo",
            UniqueId = (int)InputEventIds.TURBO,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[]
                {
                    AndroidKeyCode.KEYCODE_SHIFT_LEFT,
                    AndroidKeyCode.KEYCODE_SPACE
                }
            }
        };

        var openGarageAction = new InputAction
        {
            ActionLabel = "Open Garage",
            UniqueId = (int)InputEventIds.OPEN_GARAGE,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_G }
            }
        };

        var openStoreAction = new InputAction
        {
            ActionLabel = "Open Store",
            UniqueId = (int)InputEventIds.OPEN_STORE,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_S }
            }
        };

        var closeGarageOrStoreAction = new InputAction
        {
            ActionLabel = "Close garage or store",
            UniqueId = (int)InputEventIds.CLOSE_GARAGE_OR_STORE,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_ESCAPE }
            }
        };

        var changeTabAction = new InputAction
        {
            ActionLabel = "Change tab",
            UniqueId = (int)InputEventIds.CHANGE_TAB,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_TAB }
            }
        };

        var nextItemAction = new InputAction
        {
            ActionLabel = "Select next car/background",
            UniqueId = (int)InputEventIds.CHANGE_TAB,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_DPAD_RIGHT }
            }
        };

        var previousItemAction = new InputAction
        {
            ActionLabel = "Select previous car/background",
            UniqueId = (int)InputEventIds.CHANGE_TAB,
            InputControls = new InputControls
            {
                AndroidKeycodes = new[] { AndroidKeyCode.KEYCODE_DPAD_LEFT }
            }
        };

        var gameInputGroup = new InputGroup
        {
            GroupLabel = "Game controls",
            InputActions = new List<InputAction>
            {
                driveAction,
                turboAction,
                openGarageAction,
                openStoreAction
            }
        };

        var menuInputGroup = new InputGroup
        {
            GroupLabel = "Garage and store controls",
            InputActions = new List<InputAction>
            {
                changeTabAction,
                nextItemAction,
                previousItemAction,
                closeGarageOrStoreAction
            }
        };

        return new InputMap
        {
            InputGroups = new List<InputGroup>
            {
                gameInputGroup,
                menuInputGroup
            },
            MouseSettings = new MouseSettings
            {
                AllowMouseSensitivityAdjustment = false,
                InvertMouseMovement = false
            }
        };

    }
}
#endif
