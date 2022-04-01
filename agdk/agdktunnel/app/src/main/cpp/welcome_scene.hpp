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

#ifndef agdktunnel_welcome_scene_hpp
#define agdktunnel_welcome_scene_hpp

#include <string>

#include "engine.hpp"
#include "our_shader.hpp"
#include "tex_quad.hpp"
#include "ui_scene.hpp"
#include "util.hpp"

struct OwnedGameTextInputState {
    OwnedGameTextInputState(const std::string &initial_string);

    OwnedGameTextInputState &operator=(const GameTextInputState &rhs);

    GameTextInputState inner;
    std::string owned_string;
};

/* The "welcome scene" (main menu) */
class WelcomeScene : public UiScene {
protected:
    // IDs for our buttons:
    int mPlayButtonId;
    int mStoryButtonId;
    int mAboutButtonId;
    UiWidget *mNameEdit;
    int mTestButtonId;
    int mQuitButtonId;
    int mMemoryButtonId;

    OwnedGameTextInputState mTextInputState;

    virtual void RenderBackground() override;

    virtual void OnButtonClicked(int id) override;

    void UpdateWidgetStates();

    virtual void OnTextInput() override;

public:
    WelcomeScene();

    ~WelcomeScene();

    virtual void OnCreateWidgets() override;

    virtual void OnStartGraphics() override;

    virtual void OnKillGraphics() override;

    virtual void DoFrame() override;

    // Static info, including app and sdk versions.
    static void InitAboutText(JNIEnv* env, jobject context);

 private:
    // Complete about text, including insets.
    std::string AboutMessage();

};

#endif
