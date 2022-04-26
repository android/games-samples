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

public class InputSDKMappingProvider : PlayInputMappingProvider
{
    public enum InputEventIds
    {
        DRIVE,
        TURBO,
        OPEN_GARAGE,
        OPEN_PGS,
        OPEN_STORE,
        RETURN_TO_ROAD,
        CHANGE_TAB,
        SELECT_NEXT_CAR_OR_BACKGROUND,
        SELECT_PREVIOUS_CAR_OR_BACKGROUND,
    }

    public PlayInputMap OnProvideInputMap()
    {
        var driveAction = PlayInputAction.Create(
            "Drive",
            (int)InputEventIds.DRIVE,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_SPACE },
                new List<PlayMouseAction>()
            )
        );

        var turboAction = PlayInputAction.Create(
            "Turbo",
            (int)InputEventIds.TURBO,
            PlayInputControls.Create(
                new[]
                {
                    AndroidKeyCode.KEYCODE_SHIFT_LEFT,
                    AndroidKeyCode.KEYCODE_SPACE
                },
                new List<PlayMouseAction>()
            )
        );

        var openGarageAction = PlayInputAction.Create(
            "Open Garage",
            (int)InputEventIds.OPEN_GARAGE,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_G },
                new List<PlayMouseAction>()
            )
        );

        var openPgsAction = PlayInputAction.Create(
            "Open PGS",
            (int)InputEventIds.OPEN_PGS,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_P },
                null
            )
        );

        var openStoreAction = PlayInputAction.Create(
            "Open Store",
            (int)InputEventIds.OPEN_STORE,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_S },
                new List<PlayMouseAction>()
            )
        );

        var closeGarageOrStoreAction = PlayInputAction.Create(
            "Return to the road",
            (int)InputEventIds.RETURN_TO_ROAD,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_ESCAPE },
                new List<PlayMouseAction>()
            )
        );

        var changeTabAction = PlayInputAction.Create(
            "Change tab",
            (int)InputEventIds.CHANGE_TAB,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_TAB },
                new List<PlayMouseAction>()
            )
        );

        var nextItemAction = PlayInputAction.Create(
            "Select next car/background",
            (int)InputEventIds.SELECT_NEXT_CAR_OR_BACKGROUND,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_DPAD_RIGHT },
                new List<PlayMouseAction>()
            )
        );

        var previousItemAction = PlayInputAction.Create(
            "Select previous car/background",
            (int)InputEventIds.SELECT_PREVIOUS_CAR_OR_BACKGROUND,
            PlayInputControls.Create(
                new[] { AndroidKeyCode.KEYCODE_DPAD_LEFT },
                new List<PlayMouseAction>()
            )
        );

        var gameInputGroup = PlayInputGroup.Create(
            "Game controls",
            new List<PlayInputAction>
            {
                driveAction,
                turboAction,
                openGarageAction,
                openPgsAction,
                openStoreAction
            }
        );

        var menuInputGroup = PlayInputGroup.Create(
            "Garage and store controls",
            new List<PlayInputAction>
            {
                changeTabAction,
                nextItemAction,
                previousItemAction,
                closeGarageOrStoreAction
            }
        );

        return PlayInputMap.Create(
            new List<PlayInputGroup>
            {
                gameInputGroup,
                menuInputGroup
            },
            PlayMouseSettings.Create(false, false)
        );
    }
}
#endif
