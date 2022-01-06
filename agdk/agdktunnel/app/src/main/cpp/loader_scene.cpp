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
#include "game_asset_manager.hpp"
#include "game_asset_manifest.hpp"
#include "loader_scene.hpp"
#include "tuning_manager.hpp"
#include "welcome_scene.hpp"
#include "strings.inl"

#define TEXT_COLOR 0.0f, 1.0f, 0.0f
#define TEXT_POS center, 0.60f
#define TEXT_FONT_SCALE 1.0f

#define LOADING_TIMEOUT 5000

#define MAX_ASSET_TEXTURES 16

class LoaderScene::TextureLoader {
 private:
    int _totalLoadCount = 0;
    int _currentLoadIndex = 0;
    int _remainingLoadCount = 0;

    struct LoadedTextureData {
        LoadedTextureData() : textureSize(0), textureData(NULL), textureName(NULL) {}

        size_t textureSize;
        void *textureData;
        const char *textureName;
    };

    LoadedTextureData _loadedTextures[MAX_ASSET_TEXTURES];

 public:
    ~TextureLoader() {
        // Wait for any textures to finish loading so we don't accidentally call
        // callbacks on a deleted loader.
        constexpr int MAX_WAITS = 20;
        int waits = MAX_WAITS;
        while(_remainingLoadCount != 0 && waits > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            waits--;
        }
        if (waits == 0) {
            ALOGE("Timed-out waiting for textures to load");
            exit(1);
        }
    }

    int TotalNumberToLoad() const { return _totalLoadCount; }
    int NumberCompetedLoading() const { return _totalLoadCount - _remainingLoadCount; }
    int NumberRemainingToLoad() const { return _remainingLoadCount; }

    void LoadingCallback(const LoadingCompleteMessage *message) {
        if (message->loadSuccessful) {
            if (_currentLoadIndex < MAX_ASSET_TEXTURES) {
                _loadedTextures[_currentLoadIndex].textureSize = message->bytesRead;
                _loadedTextures[_currentLoadIndex].textureData = message->loadBuffer;
                _loadedTextures[_currentLoadIndex].textureName = message->assetName;
                ++_currentLoadIndex;
            }
            ALOGI("Finished async load %s", message->assetName);
        } else {
            ALOGE("Async load failed for %s", message->assetName);
        }
        --_remainingLoadCount;
    }

    static void LoadingCallbackProxy(const LoadingCompleteMessage *message) {
        TextureLoader* loader = (TextureLoader*)message->userData;
        loader->LoadingCallback(message);
    }

    void LoadTexturesFromAssetPack(const char *assetPackName) {
        GameAssetManager *gameAssetManager = NativeEngine::GetInstance()->GetGameAssetManager();
        int assetPackFileCount = 0;
        const char **assetPackFiles = gameAssetManager->GetGameAssetPackFileList(assetPackName,
                &assetPackFileCount);
        if (assetPackFiles != NULL) {
            for (int i = 0; i < assetPackFileCount; ++i) {
                uint64_t fileSize = gameAssetManager->GetGameAssetSize(assetPackFiles[i]);
                if (fileSize > 0) {
                    uint8_t *fileBuffer = static_cast<uint8_t *>(malloc(fileSize));
                    if (gameAssetManager->LoadGameAssetAsync(assetPackFiles[i], fileSize,
                                                             fileBuffer, LoadingCallbackProxy,
                                                             this)) {
                        ++_totalLoadCount;
                        ++_remainingLoadCount;
                        ALOGI("Started async load %s", assetPackFiles[i]);
                    }
                }
            }
        }
    }

    void CreateTextures() {
        TextureManager *textureManager = NativeEngine::GetInstance()->GetTextureManager();
        for (int i = 0; i < _currentLoadIndex; ++i) {
            textureManager->CreateTexture(_loadedTextures[i].textureName,
                _loadedTextures[i].textureSize,
                static_cast<const uint8_t *>(_loadedTextures[i].textureData));
        }
    }
}; // class LoaderScene::TextureLoader

LoaderScene::LoaderScene() : mTextureLoader(new LoaderScene::TextureLoader()) {
    mLoadingText = NULL;
    mLoadingWidget = NULL;
    mTextBoxId = -1;
    mStartTime = 0;
}

LoaderScene::~LoaderScene() {
}

void LoaderScene::DoFrame() {
    if (mTextureLoader->NumberRemainingToLoad() == 0) {
        mTextureLoader->CreateTextures();

        // Inform performance tuner we are done loading
        TuningManager *tuningManager = NativeEngine::GetInstance()->GetTuningManager();
        tuningManager->FinishLoading();

        timespec currentTimeSpec;
        clock_gettime(CLOCK_MONOTONIC, &currentTimeSpec);
        uint64_t currentTime = currentTimeSpec.tv_sec * 1000 + (currentTimeSpec.tv_nsec / 1000000);
        uint64_t deltaTime = currentTime - mStartTime;
        float loadTime = deltaTime;
        loadTime /= 1000.0f;
        ALOGI("Load complete in %.1f seconds", loadTime);
        SceneManager *mgr = SceneManager::GetInstance();
        mgr->RequestNewScene(new WelcomeScene());
    } else {
        float totalLoad = mTextureLoader->TotalNumberToLoad();
        float completedLoad = mTextureLoader->NumberCompetedLoading();
        int loadingPercentage = static_cast<int>((completedLoad / totalLoad) * 100.0f);
        char progressString[64];
        sprintf(progressString, "%s... %d%%", S_LOADING, loadingPercentage);
        mLoadingWidget->SetText(progressString);
    }

    UiScene::DoFrame();
}

void LoaderScene::OnCreateWidgets() {
    float maxX = SceneManager::GetInstance()->GetScreenAspect();
    float center = 0.5f * maxX;

    mLoadingWidget = NewWidget()->SetText(S_LOADING)->SetTextColor(TEXT_COLOR)
            ->SetCenter(TEXT_POS);
    mTextBoxId = mLoadingWidget->GetId();

    timespec currentTimeSpec;
    clock_gettime(CLOCK_MONOTONIC, &currentTimeSpec);
    mStartTime = currentTimeSpec.tv_sec * 1000 + (currentTimeSpec.tv_nsec / 1000000);
    mTextureLoader->LoadTexturesFromAssetPack(GameAssetManifest::MAIN_ASSETPACK_NAME);
    GameAssetManager *gameAssetManager = NativeEngine::GetInstance()->GetGameAssetManager();
    if (gameAssetManager->GetGameAssetPackStatus(GameAssetManifest::EXPANSION_ASSETPACK_NAME) ==
        GameAssetManager::GAMEASSET_READY) {
        mTextureLoader->LoadTexturesFromAssetPack(GameAssetManifest::EXPANSION_ASSETPACK_NAME);
    }
}

void LoaderScene::RenderBackground() {
    RenderBackgroundAnimation(mShapeRenderer);
}
