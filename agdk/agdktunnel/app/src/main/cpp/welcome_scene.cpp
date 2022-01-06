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

#include "anim.hpp"
#include "dialog_scene.hpp"
#include "play_scene.hpp"
#include "welcome_scene.hpp"

#include "blurb.inl"
#include "strings.inl"

#include "game-text-input/gametextinput.h"

// from samples/common/include
#include "Versions.h"

#include <string>
#include <android/window.h>

#define TITLE_POS center, 0.15f
#define TITLE_FONT_SCALE 1.0f
#define TITLE_COLOR 0.0f, 1.0f, 0.0f

#define NAME_EDIT_POS center, 0.85f
#define NAME_EDIT_FONT_SCALE .7f
#define NAME_EDIT_COLOR 1.0f, 1.0f, 0.0f
#define NAME_EDIT_SIZE 1.0f, 0.2f

// button defaults:
#define BUTTON_COLOR 0.0f, 1.0f, 0.0f
#define BUTTON_SIZE 0.2f, 0.2f
#define BUTTON_FONT_SCALE 0.5f

// button geometry
#define BUTTON_PLAY_POS center, 0.5f
#define BUTTON_PLAY_SIZE 0.4f, 0.4f
#define BUTTON_PLAY_FONT_SCALE 1.0f

// size of all side buttons (story, about)
#define BUTTON_SIDEBUTTON_WIDTH (center - 0.4f)
#define BUTTON_SIDEBUTTON_HEIGHT 0.2f
#define BUTTON_SIDEBUTTON_SIZE BUTTON_SIDEBUTTON_WIDTH, BUTTON_SIDEBUTTON_HEIGHT

// position of each side button (the buttons on the sides of the PLAY button)
#define BUTTON_STORY_POS 0.1f + 0.5f * BUTTON_SIDEBUTTON_WIDTH, 0.5f
#define BUTTON_ABOUT_POS center + 0.3f + 0.5f * BUTTON_SIDEBUTTON_WIDTH, 0.5f
#define BUTTON_TEST_POS center + 0.3f + 0.5f * BUTTON_SIDEBUTTON_WIDTH, 0.75f
#define BUTTON_QUIT_POS center + 0.3f + 0.5f * BUTTON_SIDEBUTTON_WIDTH, 0.25f
#define INITIAL_NAME "noname"

static std::string sNameEdit(S_NAME_EDIT);

OwnedGameTextInputState::OwnedGameTextInputState(const std::string &initial_string) : owned_string(
        initial_string) {
    inner.text_UTF8 = owned_string.data();
    inner.text_length = owned_string.length();
    inner.selection.start = 0;
    inner.selection.end = owned_string.length();
    inner.composingRegion.start = -1;
    inner.composingRegion.end = -1;
}

OwnedGameTextInputState &OwnedGameTextInputState::operator=(const GameTextInputState &rhs) {
    inner = rhs;
    owned_string = std::string(rhs.text_UTF8, rhs.text_length);
    inner.text_UTF8 = owned_string.data();
    return *this;
}

WelcomeScene::WelcomeScene() : mTextInputState(INITIAL_NAME) {
}

WelcomeScene::~WelcomeScene() {
}

void WelcomeScene::RenderBackground() {
    RenderBackgroundAnimation(mShapeRenderer);
}

static std::string sAboutStartText;

void WelcomeScene::InitAboutText(JNIEnv* env, jobject context) {
    std::stringstream aboutStream;
    aboutStream << BLURB_ABOUT;
    std::string versionName;
    agdk_samples_util::GetAppVersionInfo(env, context, nullptr, &versionName);
    aboutStream << "\nApp Version: " << versionName;
    aboutStream << "\nTarget SDK Version: "
                << android_get_application_target_sdk_version();
    aboutStream << "\nDevice OS Version: " << android_get_device_api_level();
    sAboutStartText = aboutStream.str();
}

std::string WelcomeScene::AboutMessage() {
    std::stringstream aboutStream;
    aboutStream << sAboutStartText;
    // Add window insets to help debugging
    aboutStream << "\n[Debug Insets (left, right, top, bottom)]\n";
    auto activity = NativeEngine::GetInstance()->GetAndroidApp()->activity;
    ARect insets;
    GameActivity_getWindowInsets(activity, GAMECOMMON_INSETS_TYPE_SYSTEM_BARS, &insets);
    aboutStream << "System bars: (" << insets.left << ", " << insets.right << ", " << insets.top
                << ", " << insets.bottom << ")\n";
    GameActivity_getWindowInsets(activity, GAMECOMMON_INSETS_TYPE_DISPLAY_CUTOUT, &insets);
    aboutStream << "Cutout: (" << insets.left << ", " << insets.right << ", " << insets.top
                << ", " << insets.bottom << ")\n";
    GameActivity_getWindowInsets(activity, GAMECOMMON_INSETS_TYPE_WATERFALL, &insets);
    aboutStream << "Waterfall: (" << insets.left << ", " << insets.right << ", "
                << insets.top << ", " << insets.bottom << ")";
    return aboutStream.str();
}

void WelcomeScene::OnButtonClicked(int id) {
    SceneManager *mgr = SceneManager::GetInstance();

    if (id == mPlayButtonId) {
        mgr->RequestNewScene(new PlayScene());
    } else if (id == mStoryButtonId) {
        mgr->RequestNewScene((new DialogScene())->SetText(BLURB_STORY)->SetSingleButton(S_OK,
                DialogScene::ACTION_RETURN));
    } else if (id == mAboutButtonId) {
        std::string aboutText = AboutMessage();
        mgr->RequestNewScene((new DialogScene())->SetText(aboutText.c_str())->SetSingleButton(S_OK,
                DialogScene::ACTION_RETURN));
    } else if (id == mNameEdit->GetId()) {
        auto activity = NativeEngine::GetInstance()->GetAndroidApp()->activity;
        // NB: the UI is resized when the IME is shown and OnCreateWidgets is called again.
        sNameEdit = mTextInputState.inner.text_UTF8;
        mNameEdit->SetText(sNameEdit.c_str());
        GameActivity_setTextInputState(activity, &mTextInputState.inner);
        GameActivity_showSoftInput(NativeEngine::GetInstance()->GetAndroidApp()->activity, 0);
    } else if (id == mTestButtonId) {
        auto activity = NativeEngine::GetInstance()->GetAndroidApp()->activity;
        GameActivity_setWindowFlags(activity,
                                    AWINDOW_FLAG_KEEP_SCREEN_ON | AWINDOW_FLAG_TURN_SCREEN_ON |
                                    AWINDOW_FLAG_FULLSCREEN |
                                    AWINDOW_FLAG_SHOW_WHEN_LOCKED,
                                    0);
    } else if (id == mQuitButtonId) {
        auto activity = NativeEngine::GetInstance()->GetAndroidApp()->activity;
        GameActivity_finish(activity);
    }
}

void WelcomeScene::OnTextInput() {
    auto activity = NativeEngine::GetInstance()->GetAndroidApp()->activity;
    GameActivity_getTextInputState(activity, [](void *context, const GameTextInputState *state) {
        auto thiz = static_cast<WelcomeScene *>(context);
        thiz->mTextInputState = *state;
        sNameEdit = thiz->mTextInputState.owned_string;
    }, this);
    __android_log_print(ANDROID_LOG_DEBUG, "WelcomeScene", "Got game text %s", sNameEdit.c_str());
    mNameEdit->SetText(sNameEdit.c_str());
    ARect insets;
    GameTextInput_getImeInsets(GameActivity_getTextInput(activity), &insets);
    __android_log_print(ANDROID_LOG_DEBUG,
                        "WelcomeScene", "IME insets: left=%d right=%d top=%d bottom=%d",
                        insets.left, insets.right, insets.top, insets.bottom);
}

void WelcomeScene::DoFrame() {
    // update widget states based on signed-in status
    UpdateWidgetStates();

    // if the sign in or cloud save process is in progress, show a wait screen. Otherwise, not:
    SetWaitScreen(false);

    // draw the UI
    UiScene::DoFrame();
}

void WelcomeScene::UpdateWidgetStates() {
    // Build navigation
    AddNav(mPlayButtonId, UI_DIR_LEFT, mStoryButtonId);
    AddNav(mPlayButtonId, UI_DIR_RIGHT, mAboutButtonId);

    AddNav(mStoryButtonId, UI_DIR_RIGHT, mPlayButtonId);

    AddNav(mAboutButtonId, UI_DIR_LEFT, mPlayButtonId);

}

void WelcomeScene::OnStartGraphics() {
    UiScene::OnStartGraphics();
}

void WelcomeScene::OnCreateWidgets() {

    // create widgets
    float maxX = SceneManager::GetInstance()->GetScreenAspect();
    float center = 0.5f * maxX;

    // create the static title
    NewWidget()->SetText(S_TITLE)->SetCenter(TITLE_POS)->SetTextColor(TITLE_COLOR)
            ->SetFontScale(TITLE_FONT_SCALE)->SetTransition(UiWidget::TRANS_FROM_BOTTOM);

    // create the "play" button
    mPlayButtonId = NewWidget()->SetText(S_PLAY)->SetTextColor(BUTTON_COLOR)
            ->SetCenter(BUTTON_PLAY_POS)->SetSize(BUTTON_PLAY_SIZE)
            ->SetFontScale(BUTTON_PLAY_FONT_SCALE)->SetIsButton(true)
            ->SetTransition(UiWidget::TRANS_SCALE)->GetId();

    // story button
    mStoryButtonId = NewWidget()->SetTextColor(BUTTON_COLOR)->SetText(S_STORY)
            ->SetCenter(BUTTON_STORY_POS)->SetSize(BUTTON_SIDEBUTTON_SIZE)
            ->SetFontScale(BUTTON_FONT_SCALE)->SetIsButton(true)
            ->SetTransition(UiWidget::TRANS_FROM_RIGHT)->GetId();

    // about button
    mAboutButtonId = NewWidget()->SetTextColor(BUTTON_COLOR)->SetText(S_ABOUT)
            ->SetCenter(BUTTON_ABOUT_POS)->SetSize(BUTTON_SIDEBUTTON_SIZE)
            ->SetFontScale(BUTTON_FONT_SCALE)->SetIsButton(true)
            ->SetTransition(UiWidget::TRANS_FROM_RIGHT)->GetId();

    // create editable name field
    mNameEdit = NewWidget()->SetText(sNameEdit.c_str())->SetCenter(NAME_EDIT_POS)->SetTextColor(
                    NAME_EDIT_COLOR)
            ->SetFontScale(NAME_EDIT_FONT_SCALE)->SetTransition(UiWidget::TRANS_FROM_TOP)
            ->SetSize(NAME_EDIT_SIZE)->SetIsButton(true);

    // test button
    mTestButtonId = NewWidget()->SetTextColor(BUTTON_COLOR)->SetText(S_TEST)
            ->SetCenter(BUTTON_TEST_POS)->SetSize(BUTTON_SIDEBUTTON_SIZE)
            ->SetFontScale(BUTTON_FONT_SCALE)->SetIsButton(true)
            ->SetTransition(UiWidget::TRANS_FROM_TOP)->GetId();

    // quit button
    mQuitButtonId = NewWidget()->SetTextColor(BUTTON_COLOR)->SetText(S_QUIT)
            ->SetCenter(BUTTON_QUIT_POS)->SetSize(BUTTON_SIDEBUTTON_SIZE)
            ->SetFontScale(BUTTON_FONT_SCALE)->SetIsButton(true)
            ->SetTransition(UiWidget::TRANS_FROM_BOTTOM)->GetId();

    // "Play" button is the default button
    SetDefaultButton(mPlayButtonId);

    // enable/disable widgets as appropriate to signed in state
    UpdateWidgetStates();
}

void WelcomeScene::OnKillGraphics() {
    UiScene::OnKillGraphics();
}
