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

#ifndef agdktunnel_loader_scene_hpp
#define agdktunnel_loader_scene_hpp

#include "engine.hpp"
#include "ui_scene.hpp"

#define DATA_LOAD_DELTA 0.5f

/* Loader Scene, displays load progress at startup */
class LoaderScene : public UiScene {
 private:
  class TextureLoader;

 protected:
    // text to be shown
    const char *mLoadingText;

    // Widget for the loading text
    UiWidget *mLoadingWidget;

    // ID for the loading text
    int mTextBoxId;

    uint64_t mStartTime;

    std::unique_ptr<TextureLoader> mTextureLoader;

    virtual void OnCreateWidgets() override;

    virtual void RenderBackground() override;

    DataLoaderStateMachine *mDataStateMachine;

public:

    LoaderScene();

    ~LoaderScene();

    virtual void DoFrame() override;

    LoaderScene *SetText(const char *text) {
        mLoadingText = text;
        return this;
    }

};

#endif
