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

using System.Text;
using Java.Lang;
using Java.Util;
using Google.Android.Libraries.Play.Games.Inputmapping;
using Google.Android.Libraries.Play.Games.Inputmapping.Datamodel;
using UnityEngine;
using UnityEngine.UI;

public class InputSDKRemappingListener : InputRemappingListenerCallbackHelper
{
    public override void OnInputMapChanged(InputMap inputMap)
    {
        Debug.Log("Received update on remapped controls.");
        if (inputMap.InputRemappingOption() == InputEnums.REMAP_OPTION_DISABLED)
        {
            return;
        }
        List<InputGroup> inputGroups = inputMap.InputGroups();
        for (int i = 0; i < inputGroups.Size(); i ++)
        {
            InputGroup inputGroup = inputGroups.Get(i);
            if (inputGroup.InputRemappingOption()
                    == InputEnums.REMAP_OPTION_DISABLED)
            {
                continue;
            }
            List<InputAction> inputActions = inputGroup.InputActions();
            for (int j = 0; j < inputActions.Size(); j ++)
            {
                InputAction inputAction = inputActions.Get(j);
                if (inputAction.InputRemappingOption()
                        != InputEnums.REMAP_OPTION_DISABLED)
                {
                    // Found action remapped by user
                    ProcessRemappedAction(inputAction);
                }
            }
        }
    }

    private void ProcessRemappedAction(InputAction remappedInputAction)
    {
        InputControls remappedInputControls =
                remappedInputAction.RemappedInputControls();
        List<Integer> remappedKeycodes = remappedInputControls.Keycodes();
        List<Integer> mouseActions = remappedInputControls.MouseActions();
        string version = remappedInputAction.InputActionId().VersionString();
        long remappedActionId = remappedInputAction.InputActionId().UniqueId();
        InputAction currentInputAction;
        if (string.IsNullOrEmpty(version)
                || string.Equals(
                version, InputSDKMappingProvider.INPUT_MAP_VERSION))
        {
            currentInputAction = GetCurrentVersionInputAction(remappedActionId);
        }
        else
        {
            Debug.Log("Detected version of used-saved input action " +
                "is different from current version");
            currentInputAction =
                GetCurrentVersionInputActionFromPreviousVersion(
                    remappedActionId, version);
        }
        if (currentInputAction == null)
        {
            Debug.LogError(string.Format(
                "Input Action with id {0} and version {1} not found",
                remappedActionId,
                string.IsNullOrEmpty(version) ? "UNKNOWN" : version));
            return;
        }
        InputControls originalControls = currentInputAction.InputControls();
        List<Integer> originalKeycodes = originalControls.Keycodes();

        Debug.Log(string.Format(
            "Found Input Action with id {0} remapped from key {1} to key {2}",
            remappedActionId,
            KeyCodesToString(originalKeycodes),
            KeyCodesToString(remappedKeycodes)));

        // Update HUD according to the controls of the user
        switch(remappedActionId)
        {
            case (int)InputSDKMappingProvider.InputEventIds.OPEN_GARAGE:
                GameObject.Find("GarageButton").GetComponentInChildren<Text>()
                    .text = string.Format(
                        "Garage({0})", GetLabelFromKeycode(remappedKeycodes));
                break;
            case (int)InputSDKMappingProvider.InputEventIds.OPEN_PGS:
                GameObject.Find("PGSButton").GetComponentInChildren<Text>()
                    .text = string.Format(
                        "PGS({0})", GetLabelFromKeycode(remappedKeycodes));
                break;
            case (int)InputSDKMappingProvider.InputEventIds.OPEN_STORE:
                GameObject.Find("StoreButton").GetComponentInChildren<Text>()
                    .text = string.Format(
                        "Store({0})", GetLabelFromKeycode(remappedKeycodes));
                break;
            default:
                Debug.Log("Skipping UI update because a non-UI control was remapped");
                break;
        }
    }

    private InputAction
            GetCurrentVersionInputAction(long inputActionId)
    {
        List<InputGroup> inputGroups =
            InputSDKMappingProvider.inputMap.InputGroups();
        for (int i = 0; i < inputGroups.Size(); i ++)
        {
            InputGroup inputGroup = inputGroups.Get(i);
            List<InputAction> inputActions = inputGroup.InputActions();
            for (int j = 0; j < inputActions.Size(); j ++)
            {
                InputAction inputAction = inputActions.Get(j);
                if (inputAction.InputActionId().UniqueId() == inputActionId)
                {
                    return inputAction;
                }
            }
        }
        return null;
    }

    private InputAction
            GetCurrentVersionInputActionFromPreviousVersion(
            long inputActionId, string version)
    {
        // TODO: add logic to this method considering the diff between your
        // current and previous InputMap.
        return null;
    }

    private string KeyCodesToString(List<Integer> keycodes)
    {
        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < keycodes.Size(); i ++)
        {
            Integer keycode = keycodes.Get(i);
            if (builder.Length > 0)
            {
                builder.Append(" + ");
            }
            builder.Append(keycode.IntValue());
        }
        return string.Format("({0})", builder.ToString());
    }

    private string GetLabelFromKeycode(List<Integer> keycodes)
    {
        // Access the keycode names through reflection
        var keycodeLabels = typeof(AndroidKeyCode).GetMembers();
        // Skip the first 4 reflected members: Equals, GetHashCode, GetType and
        // ToString
        int reservedMembers = 4;
        if (keycodes.Size() < 0 || keycodes.Size() > 1)
        {
            // Empty keycodes or multi-key keycodes are invalid remapped keys
            return "?";
        }
        int index = keycodes.Get(0).IntValue() + reservedMembers;
        if (index < reservedMembers || index >= keycodeLabels.Length)
        {
            // Unknown remapped key
            return "?";
        }
        string fullLabel = keycodeLabels[index].Name;
        if (!fullLabel.StartsWith("KEYCODE_"))
        {
            // Invalid operation
            return "?";
        }
        // Retrieve the label after the KEYCODE_ prefix of length 8
        string label = fullLabel.Substring(8);
        if (string.IsNullOrEmpty(label))
        {
            // Error during trimming operation
            return "?";
        }
        // Trim label to 1 char to prevent overlapping UI (example: SPACE -> S)
        return label.Substring(0, 1);
    }
}
#endif