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

using Java.Lang;
using Java.Util;
using Google.Android.Libraries.Play.Games.Inputmapping;
using Google.Android.Libraries.Play.Games.Inputmapping.Datamodel;

public class InputSDKMappingProvider : InputMappingProviderCallbackHelper
{
    public static readonly string INPUT_MAP_VERSION = "1.0.0";

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

    public enum InputGroupsIds
    {
        ROAD_CONTROLS,
        MENU_CONTROLS,
    }

    public enum InputContextsIds
    {
        ROAD_CONTEXT,
        MENU_CONTEXT,
    }

    public enum InputMapIds
    {
        INPUT_MAP,
    }

    private static readonly InputAction driveInputAction = InputAction.Create(
        "Drive",
        (long)InputEventIds.DRIVE,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_SPACE) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputAction turboInputAction = InputAction.Create(
        "Turbo",
        (long)InputEventIds.TURBO,
        InputControls.Create(
            new[] {
                new Integer(AndroidKeyCode.KEYCODE_SHIFT_LEFT),
                new Integer(AndroidKeyCode.KEYCODE_SPACE)
            }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_DISABLED
    );

    private static readonly InputAction openGarageInputAction = InputAction.Create(
        "Open garage",
        (long)InputEventIds.OPEN_GARAGE,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_G) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputAction openPgsInputAction = InputAction.Create(
        "Open PGS",
        (long)InputEventIds.OPEN_PGS,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_P) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputAction openStoreInputAction = InputAction.Create(
        "Open store",
        (long)InputEventIds.OPEN_STORE,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_S) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputGroup roadInputGroup = InputGroup.Create(
        "Road controls",
        new[]
        {
            driveInputAction,
            turboInputAction,
            openGarageInputAction,
            openPgsInputAction,
            openStoreInputAction,
        }.ToJavaList(),
        (long)InputGroupsIds.ROAD_CONTROLS,
        // All input actions of this group will be remappable unless specified
        // the contrary by the individual input actions.
        InputEnums.REMAP_OPTION_ENABLED
    );

    public static readonly InputContext roadControlsContext = InputContext.Create(
        "Road context",
        InputIdentifier.Create(
            INPUT_MAP_VERSION, (long)InputContextsIds.ROAD_CONTEXT),
        new[] { roadInputGroup }.ToJavaList()
    );

    private static readonly InputAction returnRoadInputAction = InputAction.Create(
        "Return to the road",
        (long)InputEventIds.RETURN_TO_ROAD,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_ESCAPE) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_DISABLED
    );

    private static readonly InputAction changeTabInputAction = InputAction.Create(
        "Change tab",
        (long)InputEventIds.CHANGE_TAB,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_TAB) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputAction nextItemInputAction = InputAction.Create(
        "Select next car/background",
        (long)InputEventIds.SELECT_NEXT_CAR_OR_BACKGROUND,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_DPAD_RIGHT) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputAction previousItemInputAction = InputAction.Create(
        "Select previous car/background",
        (long)InputEventIds.SELECT_PREVIOUS_CAR_OR_BACKGROUND,
        InputControls.Create(
            new[] { new Integer(AndroidKeyCode.KEYCODE_DPAD_LEFT) }.ToJavaList(),
            new ArrayList<Integer>()),
        InputEnums.REMAP_OPTION_ENABLED
    );

    private static readonly InputGroup menuInputGroup = InputGroup.Create(
        "Menu controls",
        new[]
        {
            returnRoadInputAction,
            changeTabInputAction,
            nextItemInputAction,
            previousItemInputAction,
        }.ToJavaList(),
        (long)InputGroupsIds.MENU_CONTROLS,
        // All input actions of this group will be remappable unless specified
        // the contrary by the individual input actions.
        InputEnums.REMAP_OPTION_ENABLED
    );

    public static readonly InputContext menuControlsContext = InputContext.Create(
        "Menu context",
        InputIdentifier.Create(
            INPUT_MAP_VERSION, (long)InputContextsIds.MENU_CONTEXT),
        new[] { menuInputGroup }.ToJavaList()
    );

    public static readonly InputMap inputMap = InputMap.Create(
        new[] { roadInputGroup, menuInputGroup }.ToJavaList(),
        MouseSettings.Create(false, false),
        InputIdentifier.Create(
            INPUT_MAP_VERSION, (long)InputMapIds.INPUT_MAP),
        // Use ESC as reserved key
        InputEnums.REMAP_OPTION_ENABLED,
        new[]
        {
            InputControls.Create(
                new[] { new Integer(AndroidKeyCode.KEYCODE_ESCAPE) }.ToJavaList(),
                new ArrayList<Integer>()
            ),
        }.ToJavaList()
    );

    public override InputMap OnProvideInputMap()
    {
        return inputMap;
    }
}
#endif