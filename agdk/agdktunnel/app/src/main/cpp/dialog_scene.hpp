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

#ifndef agdktunnel_dialog_scene_hpp
#define agdktunnel_dialog_scene_hpp

#include "engine.hpp"
#include "ui_scene.hpp"
#include "loader_scene.hpp"

/* Dialog Scene. Shows a message and buttons. When a button is clicked, performs
 * a given action. */
class DialogScene : public UiScene {
protected:
    // text to be shown
    std::string mText GUARDED_BY(mTextMutex);
    const char *mLeftButtonText;
    const char *mRightButtonText;

    std::mutex mTextMutex;

    // IDs for buttons
    int mLeftButtonId;
    int mRightButtonId;

    // ID for the text box
    int mTextBoxId;

    // action for left button and right button
    int mLeftButtonAction, mRightButtonAction;

    // y position of buttons
    float mButtonY;

    virtual void OnCreateWidgets();

    virtual void RenderBackground();

    virtual void OnButtonClicked(int id);

    virtual bool OnBackKeyPressed();

public:
    // (action) return to main screen
    static const int ACTION_RETURN = 1000;

    // (action) sign in with Google
    static const int ACTION_SIGN_IN = 1001;

    // (action) play without signing in
    static const int ACTION_PLAY_WITHOUT_SIGNIN = 1002;

    // (action) sign out
    static const int ACTION_SIGN_OUT = 1003;

    DialogScene();

    ~DialogScene();

    // We do not take ownership of 'text' here.
    DialogScene *SetText(const char *text) {
        std::lock_guard<std::mutex> lock(mTextMutex);
        mText = text;
        return this;
    }

    // This takes ownership of 'text'.
    DialogScene *SetSingleButton(const char *text, int action) {
        mLeftButtonText = text;
        mLeftButtonAction = action;
        mRightButtonText = NULL;
        return this;
    }

private:
    void CreateWidgetsSetText();

    void CreateWidgetsSingleButton();

    void CreateWidgetsTwoButtons();
};

#endif
