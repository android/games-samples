/*
 * Copyright 2021 The Android Open Source Project
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

package com.google.sample.agdktunnel;

import static com.google.android.libraries.play.games.inputmapping.datamodel.InputEnums.REMAP_OPTION_ENABLED;
import static com.google.android.libraries.play.games.inputmapping.datamodel.InputEnums.REMAP_OPTION_DISABLED;

import android.view.KeyEvent;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputGroup;
import com.google.android.libraries.play.games.inputmapping.InputMappingProvider;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputAction;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputControls;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputContext;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputMap;
import com.google.android.libraries.play.games.inputmapping.datamodel.MouseSettings;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputIdentifier;

import java.util.Arrays;
import java.util.Collections;

public class InputSDKProvider implements InputMappingProvider {
    public static final String INPUTMAP_VERSION = "1.0.0";

    public enum InputActionsIds {
        NAVIGATE_UP,
        NAVIGATE_LEFT,
        NAVIGATE_DOWN,
        NAVIGATE_RIGHT,
        ENTER_MENU,
        EXIT_MENU,
        MOVE_UP,
        MOVE_LEFT,
        MOVE_DOWN,
        MOVE_RIGHT,
        PAUSE_GAME,
        MOUSE_MOVEMENT,
        ON_PAUSE_SELECT_OPTION,
        ON_PAUSE_NAVIGATE_UP,
        ON_PAUSE_NAVIGATE_DOWN,
        EXIT_PAUSE,
    }

    public enum InputGroupsIds {
        MENU_DIRECTIONAL_NAVIGATION,
        MENU_ACTION_KEYS,
        BASIC_MOVEMENT,
        MOUSE_MOVEMENT,
        PAUSE_MENU,
    }

    public enum InputContextIds {
        INPUT_CONTEXT_UI_SCENE(1),
        INPUT_CONTEXT_PLAY_SCENE(2),
        INPUT_CONTEXT_PAUSE_MENU(3);

        InputContextIds(int value) { this.value = value; }

        private final int value;
        public int value() { return value; }
    }

    public enum InputMapIds {
        GAME_INPUT_MAP,
    }

    private static final InputAction sNavigateMenuUpInputAction = InputAction.create(
            "Navigate up",
            InputActionsIds.NAVIGATE_UP.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_W),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sNavigateMenuLeftInputAction = InputAction.create(
            "Navigate left",
            InputActionsIds.NAVIGATE_LEFT.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_A),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sNavigateMenuDownInputAction = InputAction.create(
            "Navigate down",
            InputActionsIds.NAVIGATE_DOWN.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_S),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sNavigateMenuRightInputAction = InputAction.create(
            "Navigate right",
            InputActionsIds.NAVIGATE_RIGHT.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_D),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputGroup sBasicMenuNavigationInputGroup = InputGroup.create(
            "Menu navigation keys",
            Arrays.asList(
                    sNavigateMenuUpInputAction,
                    sNavigateMenuLeftInputAction,
                    sNavigateMenuDownInputAction,
                    sNavigateMenuRightInputAction),
            InputGroupsIds.MENU_DIRECTIONAL_NAVIGATION.ordinal(),
            REMAP_OPTION_ENABLED);

    private static final InputAction sEnterMenuInputAction = InputAction.create(
            "Enter menu",
            InputActionsIds.ENTER_MENU.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_ENTER),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sExitMenuInputAction = InputAction.create(
            "Exit menu",
            InputActionsIds.EXIT_MENU.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_ESCAPE),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputGroup sMenuActionKeysInputGroup = InputGroup.create(
            "Menu keys",
            Arrays.asList(
                    sEnterMenuInputAction,
                    sExitMenuInputAction),
            InputGroupsIds.MENU_ACTION_KEYS.ordinal(),
            REMAP_OPTION_ENABLED);

    private static final InputAction sMoveUpInputAction = InputAction.create(
            "Move Up",
            InputActionsIds.MOVE_UP.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_W),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sMoveLeftInputAction = InputAction.create(
            "Move Left",
            InputActionsIds.MOVE_LEFT.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_A),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sMoveDownInputAction = InputAction.create(
            "Move Down",
            InputActionsIds.MOVE_DOWN.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_S),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sMoveRightInputAction = InputAction.create(
            "Move Right",
            InputActionsIds.MOVE_RIGHT.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_D),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sPauseGameInputAction = InputAction.create(
            "Pause game",
            InputActionsIds.PAUSE_GAME.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_ESCAPE),
                    Collections.emptyList()),
            REMAP_OPTION_DISABLED);

    private static final InputGroup sMovementInputGroup = InputGroup.create(
            "Basic movement",
            Arrays.asList(
                    sMoveUpInputAction,
                    sMoveLeftInputAction,
                    sMoveDownInputAction,
                    sMoveRightInputAction,
                    sPauseGameInputAction
            ),
            InputGroupsIds.BASIC_MOVEMENT.ordinal(),
            REMAP_OPTION_ENABLED
    );

    private static final InputAction sMouseInputAction = InputAction.create(
            "Move",
            InputActionsIds.MOUSE_MOVEMENT.ordinal(),
            InputControls.create(
                    Collections.emptyList(),
                    Collections.singletonList(InputControls.MOUSE_LEFT_CLICK)
            ),
            REMAP_OPTION_DISABLED);

    private static final InputGroup sMouseMovementInputGroup = InputGroup.create(
            "Mouse movement",
            Collections.singletonList(sMouseInputAction),
            InputGroupsIds.MOUSE_MOVEMENT.ordinal(),
            REMAP_OPTION_DISABLED
    );

    private static final InputAction sOnPauseNavigateUpInputAction = InputAction.create(
            "Up",
            InputActionsIds.ON_PAUSE_NAVIGATE_UP.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_W),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sOnPauseNavigateDownInputAction = InputAction.create(
            "Down",
            InputActionsIds.ON_PAUSE_NAVIGATE_DOWN.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_S),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sOnPauseSelectOptionInputAction = InputAction.create(
            "Select option",
            InputActionsIds.ON_PAUSE_SELECT_OPTION.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_ENTER),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputAction sOnPauseResumeGameInputAction = InputAction.create(
            "Resume game",
            InputActionsIds.EXIT_PAUSE.ordinal(),
            InputControls.create(
                    Collections.singletonList(KeyEvent.KEYCODE_ESCAPE),
                    Collections.emptyList()),
            REMAP_OPTION_ENABLED);

    private static final InputGroup sPauseMenuInputGroup = InputGroup.create(
            "Pause menu",
            Arrays.asList(
                    sOnPauseNavigateUpInputAction,
                    sOnPauseNavigateDownInputAction,
                    sOnPauseSelectOptionInputAction,
                    sOnPauseResumeGameInputAction),
            InputGroupsIds.PAUSE_MENU.ordinal(),
            REMAP_OPTION_DISABLED
    );

    static final InputContext sPlaySceneInputContext = InputContext.create(
            "In game controls",
            InputContextIds.INPUT_CONTEXT_PLAY_SCENE.ordinal(),
            Arrays.asList(sMovementInputGroup, sMouseMovementInputGroup)
    );

    static final InputContext sUiSceneInputContext = InputContext.create(
            "Main menu",
            InputContextIds.INPUT_CONTEXT_UI_SCENE.ordinal(),
            Arrays.asList(sBasicMenuNavigationInputGroup, sMenuActionKeysInputGroup)
    );

    static final InputContext sPauseMenuInputContext = InputContext.create(
            "Pause menu",
            InputContextIds.INPUT_CONTEXT_PAUSE_MENU.ordinal(),
            Collections.singletonList(sPauseMenuInputGroup)
    );

    static final InputMap sGameInputMap = InputMap.create(
            Arrays.asList(
                    sBasicMenuNavigationInputGroup,
                    sMenuActionKeysInputGroup,
                    sMovementInputGroup,
                    sMouseMovementInputGroup,
                    sPauseMenuInputGroup),
            MouseSettings.create(true, false),
            InputIdentifier.create(
                    INPUTMAP_VERSION,
                    InputMapIds.GAME_INPUT_MAP.ordinal()),
            REMAP_OPTION_ENABLED,
            Arrays.asList(
                    InputControls.create(
                            Collections.singletonList(KeyEvent.KEYCODE_SPACE),
                            Collections.emptyList()
                    )
            )
    );

    @Override
    public InputMap onProvideInputMap() {
        return sGameInputMap;
    }
}