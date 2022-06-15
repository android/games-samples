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

import android.view.KeyEvent;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputGroup;
import com.google.android.libraries.play.games.inputmapping.InputMappingProvider;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputAction;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputControls;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputMap;
import com.google.android.libraries.play.games.inputmapping.datamodel.MouseSettings;

import java.util.Arrays;
import java.util.Collections;

public class InputSDKProvider implements InputMappingProvider {
    public enum InputEventIds {
        MOVE_UP,
        MOVE_LEFT,
        MOVE_DOWN,
        MOVE_RIGHT,
        MOUSE_MOVEMENT,
    }

    @Override
    public InputMap onProvideInputMap() {
        InputAction moveUpInputAction = InputAction.create(
            "Move Up",
            InputEventIds.MOVE_UP.ordinal(),
            InputControls.create(
                Collections.singletonList(KeyEvent.KEYCODE_W),
                Collections.emptyList()
            )
        );

        InputAction moveLeftInputAction = InputAction.create(
            "Move Left",
            InputEventIds.MOVE_LEFT.ordinal(),
            InputControls.create(
                Collections.singletonList(KeyEvent.KEYCODE_A),
                Collections.emptyList()
            )
        );

        InputAction moveDownInputAction = InputAction.create(
            "Move Down",
            InputEventIds.MOVE_DOWN.ordinal(),
            InputControls.create(
                Collections.singletonList(KeyEvent.KEYCODE_S),
                Collections.emptyList()
            )
        );

        InputAction moveRightInputAction = InputAction.create(
            "Move Right",
            InputEventIds.MOVE_RIGHT.ordinal(),
            InputControls.create(
                Collections.singletonList(KeyEvent.KEYCODE_D),
                Collections.emptyList()
            )
        );

        InputGroup movementInputGroup = InputGroup.create(
            "Basic movement",
            Arrays.asList(
                moveUpInputAction,
                moveLeftInputAction,
                moveDownInputAction,
                moveRightInputAction
            )
        );

        InputAction mouseInputAction = InputAction.create(
            "Move",
            InputEventIds.MOUSE_MOVEMENT.ordinal(),
            InputControls.create(
                Collections.emptyList(),
                Collections.singletonList(InputControls.MOUSE_LEFT_CLICK)
            )
        );

        InputGroup mouseMovementInputGroup = InputGroup.create(
            "Mouse movement",
            Collections.singletonList(mouseInputAction)
        );

        return InputMap.create(
            Arrays.asList(movementInputGroup, mouseMovementInputGroup),
            MouseSettings.create(true, false)
        );
    }
}