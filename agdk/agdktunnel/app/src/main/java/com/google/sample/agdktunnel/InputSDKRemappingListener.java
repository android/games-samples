package com.google.sample.agdktunnel;

import android.util.Log;

import com.google.android.libraries.play.games.inputmapping.datamodel.InputGroup;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputAction;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputControls;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputMap;
import com.google.android.libraries.play.games.inputmapping.datamodel.InputEnums;
import com.google.android.libraries.play.games.inputmapping.InputRemappingListener;
import java.util.List;
import java.util.Optional;

public class InputSDKRemappingListener implements InputRemappingListener {

    private static final String TAG = "InputSDKRemappingListener";

    @Override
    public void onInputMapChanged(InputMap inputMap) {
        Log.i(TAG, "Received update on input map changed.");
        if (inputMap.inputRemappingOption() == InputEnums.REMAP_OPTION_DISABLED) {
            return;
        }
        for (InputGroup inputGroup : inputMap.inputGroups()) {
            if (inputGroup.inputRemappingOption() == InputEnums.REMAP_OPTION_DISABLED) {
                continue;
            }
            for (InputAction inputAction : inputGroup.inputActions()) {
                if (inputAction.inputRemappingOption() != InputEnums.REMAP_OPTION_DISABLED &&
                        !inputAction.inputControls().equals(inputAction.remappedInputControls())) {
                    // Found InputAction remapped by user
                    processRemappedAction(inputAction);
                }
            }
        }
    }

    private void processRemappedAction(InputAction remappedInputAction) {
        // Get remapped action info
        InputControls remappedControls = remappedInputAction.remappedInputControls();
        List<Integer> remappedKeyCodes = remappedControls.keycodes();
        List<Integer> mouseActions = remappedControls.mouseActions();
        String version = remappedInputAction.inputActionId().versionString();
        long remappedActionId = remappedInputAction.inputActionId().uniqueId();
        Optional<InputAction> currentInputAction;
        if (version == null || version.isEmpty()
                || version.equals(InputSDKProvider.INPUTMAP_VERSION)) {
            currentInputAction = getCurrentVersionInputAction(remappedActionId);
        } else {
            Log.i(TAG,
                    "Detected version of user-saved input action defers from current version");
            currentInputAction = getCurrentVersionInputActionFromPreviousVersion(
                    remappedActionId, version);
        }
        if (!currentInputAction.isPresent()) {
            Log.e(TAG, String.format(
                    "can't find remapped input action with id %d and version %s",
                    remappedActionId, version == null || version.isEmpty() ? "UNKNOWN" : version));
            return;
        }
        InputControls originalControls = currentInputAction.get().inputControls();
        List<Integer> originalKeyCodes = originalControls.keycodes();

        Log.i(TAG, String.format(
                "Found input action with id %d remapped from key %s to key %s",
                remappedActionId,
                keyCodesToString(originalKeyCodes),
                keyCodesToString(remappedKeyCodes)));

        // TODO: game developer can make display changes to match controls used by the user.
    }

    private Optional<InputAction> getCurrentVersionInputAction(long inputActionId) {
        for (InputGroup inputGroup : InputSDKProvider.sGameInputMap.inputGroups()) {
            for (InputAction inputAction : inputGroup.inputActions()) {
                if (inputAction.inputActionId().uniqueId() == inputActionId) {
                    return Optional.of(inputAction);
                }
            }
        }
        return Optional.empty();
    }

    private Optional<InputAction> getCurrentVersionInputActionFromPreviousVersion(
            long inputActionId, String previousVersion) {
        // TODO: game developer can add logic to this method to get the input action considering the
        //  diff between the current and previous InputMap (if any).
        return Optional.empty();
    }

    private String keyCodesToString(List<Integer> keyCodes) {
        StringBuilder builder = new StringBuilder();
        for (Integer keyCode : keyCodes) {
            if (!builder.toString().isEmpty()) {
                builder.append(" + ");
            }
            builder.append(keyCode);
        }
        return String.format("(%s)", builder);
    }
}
