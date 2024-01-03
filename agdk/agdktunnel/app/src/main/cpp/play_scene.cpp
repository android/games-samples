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

#include <cstdio>
#include "anim.hpp"
#include "ascii_to_geom.hpp"
#include "game_consts.hpp"
#include "gfx_manager.hpp"
#include "play_scene.hpp"
#include "texture_manager.hpp"
#include "tunnel_engine.hpp"
#include "util.hpp"
#include "welcome_scene.hpp"
#include "welcome_scene.hpp"

#include "data/ascii_art.inl"
#include "data/cube_geom.inl"
#include "data/strings.inl"
#include "data/tunnel_geom.inl"

using namespace simple_renderer;

#define WALL_TEXTURE_SIZE 64

// colors for menus
static const float MENUITEM_SEL_COLOR[] = {1.0f, 1.0f, 0.0f};
static const float MENUITEM_COLOR[] = {1.0f, 1.0f, 1.0f};

static const float DEFAULT_TINT[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float LIGHT_OFF[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static const float LIGHT_POS[4] = {0.0f, 0.0f, 0.0f, 1.0f};

// obstacle colors
static const float OBS_COLORS[] = {
        0.0f, 0.0f, 0.0f, // style 0 (not used)
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f
};

static const char *TONE_BONUS[] = {
        "d70 f150. f250. f350. f450.",
        "d70 f200. f300. f400. f500.",
        "d70 f250. f350. f450. f550.",
        "d70 f300. f400. f500. f600.",
        "d70 f350. f450. f550. f650.",
        "d70 f400. f500. f600. f700.",
        "d70 f450. f550. f650. f750.",
        "d70 f500. f600. f700. f800.",
        "d70 f550. f650. f750. f850."
};

PlayScene::PlayScene(int savedLevel) : Scene() {
    mSavedLevel = (savedLevel / LEVELS_PER_CHECKPOINT) * LEVELS_PER_CHECKPOINT;
    ALOGI("Normalized check-point: level %d", mSavedLevel);
    mTextRenderer = NULL;
    mShapeRenderer = NULL;
#ifdef TOUCH_INDICATOR_MODE
    mRectRenderer = NULL;
#endif // TOUCH_INDICATOR_MODE
    mShipSteerX = mShipSteerZ = 0.0f;
    mFilteredSteerX = mFilteredSteerZ = 0.0f;
    mMotionKeyBitmask = 0;

    mPlayerPos = glm::vec3(0.0f, 0.0f, 0.0f); // center
    mPlayerDir = glm::vec3(0.0f, 1.0f, 0.0f); // forward
    mDifficulty = 0;

    mCubeGeom = NULL;
    mTunnelGeom = NULL;

    mObstacleCount = 0;
    mFirstObstacle = 0;
    mFirstSection = 0;
    mSteering = STEERING_NONE;
    mPointerId = -1;
    mPointerAnchorX = mPointerAnchorY = 0.0f;

    for (int i = 0; i < MAX_WALL_TEXTURES; ++i) {
        mWallTextures[i] = NULL;
    }

    memset(mMenuItemText, 0, sizeof(mMenuItemText));
    mMenuItemText[MENUITEM_UNPAUSE] = (char *)S_UNPAUSE;
    mMenuItemText[MENUITEM_QUIT] = (char *)S_QUIT;
    mMenuItemText[MENUITEM_START_OVER] = (char *)S_START_OVER;
    mMenuItemText[MENUITEM_RESUME] = (char *)S_RESUME;
    mMenuItemText[MENUITEM_LOADING] = new char[64];
    mMenuItemText[MENUITEM_RESUME_CLOUD] = (char *)S_RESUME_CLOUD;

    memset(mMenuItems, 0, sizeof(mMenuItems));
    mMenuItemCount = 0;

    mMenu = MENU_NONE;
    mMenuSel = 0;

    mSignText = NULL;
    mSignTimeLeft = 0.0f;

    mShowedHowto = false;
    mLifeGeom = NULL;

    mLives = PLAYER_LIVES;

    mRollAngle = 0.0f;

    mPlayerSpeed = 0.0f;
    mBlinkingHeart = false;
    mGameStartTime = Clock();

    mBonusInARow = 0;
    mLastCrashSection = -1;

    mFrameClock.SetMaxDelta(MAX_DELTA_T);
    mLastAmbientBeepEmitted = 0;
    mMenuTouchActive = false;

    mCheckpointSignPending = false;

    mActiveWallTextureCount = 0;

    SetScore(0);

    if (mSavedLevel > 0) {
        // start with the menu that asks whether or not to start from the saved level
        // or start over from scratch
        ShowMenu(MENU_LEVEL);
    }
}

static unsigned char *_gen_wall_texture() {
    static unsigned char pixel_data[WALL_TEXTURE_SIZE * WALL_TEXTURE_SIZE * 3];
    unsigned char *p;
    int x, y;
    for (y = 0, p = pixel_data; y < WALL_TEXTURE_SIZE; y++) {
        for (x = 0; x < WALL_TEXTURE_SIZE; x++, p += 3) {
            p[0] = p[1] = p[2] = 128 + ((x > 2 && y > 2) ? Random(128) : 0);
        }
    }
    return pixel_data;
}

void PlayScene::OnStartGraphics() {
    // build projection matrix
    UpdateProjectionMatrix();

    GfxManager *gfxManager = TunnelEngine::GetInstance()->GetGfxManager();

    // build tunnel geometry
    IndexBuffer::IndexBufferCreationParams tunnelIndexParams = {
        TUNNEL_GEOM_INDICES, sizeof(TUNNEL_GEOM_INDICES)
    };
    VertexBuffer::VertexBufferCreationParams tunnelVertexParams = {
        TUNNEL_GEOM, VertexBuffer::kVertexFormat_P3T2C4,
        sizeof(TUNNEL_GEOM)
    };
    Renderer& renderer = Renderer::GetInstance();

    std::shared_ptr<IndexBuffer> indexBuffer = renderer.CreateIndexBuffer(tunnelIndexParams);
    std::shared_ptr<VertexBuffer> vertexBuffer = renderer.CreateVertexBuffer(tunnelVertexParams);
    mTunnelGeom = new SimpleGeom(indexBuffer, vertexBuffer);

    VertexBuffer::VertexBufferCreationParams cubeVertexParams = {
        CUBE_GEOM, VertexBuffer::kVertexFormat_P3T2C4,
        sizeof(CUBE_GEOM)
    };

    std::shared_ptr<VertexBuffer> cubeVertexBuffer = renderer.CreateVertexBuffer(cubeVertexParams);
    mCubeGeom = new SimpleGeom(nullptr, cubeVertexBuffer);

    // make the wall texture
    TextureManager *textureManager = TunnelEngine::GetInstance()->GetTextureManager();
    char textureName[64];
    for (int wallIndex = 0; wallIndex < MAX_WALL_TEXTURES; ++wallIndex) {
#if defined NO_ASSET_PACKS
        snprintf(textureName, 64, "no_asset_packs_textures/wall%d.ktx", wallIndex + 1);
#else
        snprintf(textureName, 64, "textures/wall%d.tex", wallIndex + 1);
#endif
        std::shared_ptr<Texture> textureReference = textureManager->GetTexture(textureName);
        if (textureReference.get() != nullptr) {
            mWallTextures[wallIndex] = textureReference;
            ++mActiveWallTextureCount;
        }
    }

    // If no loaded wall textures, use a random noise texture
    if (mActiveWallTextureCount == 0) {
        void *tempWallTexture = _gen_wall_texture();
        const uint32_t textureSize = WALL_TEXTURE_SIZE * WALL_TEXTURE_SIZE * 3;
        simple_renderer::Texture::TextureCreationParams textureParams = {
            simple_renderer::Texture::kTextureFormat_RGB_888,
            simple_renderer::Texture::kTextureCompression_None,
            Texture::kMinFilter_Linear, Texture::kMagFilter_Linear,
            Texture::kWrapS_Repeat,Texture::kWrapT_Repeat,
            WALL_TEXTURE_SIZE, WALL_TEXTURE_SIZE, 1,
            &textureSize, tempWallTexture
        };
        mWallTextures[0] = renderer.CreateTexture(textureParams);
        ++mActiveWallTextureCount;
    }

    // reset frame clock so the animation doesn't jump
    mFrameClock.Reset();

    // life icon geometry
    mLifeGeom = AsciiArtToGeom(ART_LIFE, LIFE_ICON_SCALE);

    // create text renderer and shape renderer
    mTextRenderer = new TextRenderer(gfxManager->GetUniformBuffer(
        GfxManager::kGfxType_BasicThickLinesNoDepthTest));
    mShapeRenderer = new ShapeRenderer(gfxManager->GetUniformBuffer(
        GfxManager::kGfxType_BasicTrisNoDepthTest));
#ifdef TOUCH_INDICATOR_MODE
    mRectRenderer = new ShapeRenderer(gfxManager->GetUniformBuffer(
        GfxManager::kGfxType_BasicTrisNoDepthTest));
#endif // TOUCH_INDICATOR_MODE
}

void PlayScene::OnKillGraphics() {
    CleanUp(&mTextRenderer);
    CleanUp(&mShapeRenderer);
#ifdef TOUCH_INDICATOR_MODE
    CleanUp(&mRectRenderer);
#endif // TOUCH_INDICATOR_MODE
    CleanUp(&mTunnelGeom);
    CleanUp(&mCubeGeom);
    Renderer& renderer = Renderer::GetInstance();
    for (int i = 0; i < mActiveWallTextureCount; ++i) {
        mWallTextures[i] = nullptr;
    }
    mActiveWallTextureCount = 0;
    CleanUp(&mLifeGeom);
}

void PlayScene::DoFrame() {
    float deltaT = mFrameClock.ReadDelta();
    float previousY = mPlayerPos.y;

    GfxManager *gfxManager = TunnelEngine::GetInstance()->GetGfxManager();
    gfxManager->SetMainRenderPass();
    gfxManager->SetRenderState(GfxManager::kGfxType_OurTris);

    // rotate the view matrix according to current roll angle
    glm::vec3 upVec = glm::vec3(-sin(mRollAngle), 0, cos(mRollAngle));

    // set up view matrix according to player's ship position and direction
    mViewMat = glm::lookAt(mPlayerPos, mPlayerPos + mPlayerDir, upVec);

    // render tunnel walls
    RenderTunnel(gfxManager);

    // render obstacles
    RenderObstacles(gfxManager);

    if (mMenu) {
        if (mMenu == MENU_LOADING) {
            DataLoaderStateMachine *dataStateMachine =
                TunnelEngine::GetInstance()->GetDataStateMachine();
            if (dataStateMachine->isLoadingDataCompleted()) {
                // resume from saved level
                HandleMenu(MENUITEM_LOADING);
            } else {
                int loadingPercentage = dataStateMachine->getStepsCompleted()
                        * 100 / dataStateMachine->getTotalSteps();
                snprintf(mMenuItemText[MENUITEM_LOADING], 64, "%s... %d%%",
                        S_LOADING, loadingPercentage);
            }
        }

        RenderMenu(gfxManager);
        // nothing more to do
        return;
    }

    // render HUD (lives, score, etc)
    RenderHUD(gfxManager);

    // deduct from the time remaining to remove a sign from the screen
    if (mSignText && mSignExpires) {
        mSignTimeLeft -= deltaT;
        if (mSignTimeLeft < 0.0f) {
            mSignText = NULL;
        }
    }

    // if a "saved checkpoint" sign pending? Can we show it right now?
    if (!mSignText && mCheckpointSignPending) {
        mCheckpointSignPending = false;
        ShowSign(S_CHECKPOINT_SAVED, SIGN_DURATION);
    }

    // did we already show the howto?
    if (!mShowedHowto && mDifficulty == 0) {
        mShowedHowto = true;
        ShowSign(S_HOWTO_WITHOUT_JOY, SIGN_DURATION);
    }

    // deduct from the time remaining on the blinking heart animation
    if (mBlinkingHeart && Clock() > mBlinkingHeartExpire) {
        mBlinkingHeart = false;
    }

    // update speed
    float targetSpeed = PLAYER_SPEED + PLAYER_SPEED_INC_PER_LEVEL * mDifficulty;
    float accel = mPlayerSpeed >= 0.0f ? PLAYER_ACCELERATION_POSITIVE_SPEED :
                  PLAYER_ACCELERATION_NEGATIVE_SPEED;
    if (mLives <= 0) {
        targetSpeed = 0.0f;
    }
    mPlayerSpeed = Approach(mPlayerSpeed, targetSpeed, deltaT * accel);

    // apply movement if key is pressed
    if (mSteering == STEERING_KEY) {
        OnMovementKey();
    }

    // apply noise filter on steering
    mFilteredSteerX = (mFilteredSteerX * (NOISE_FILTER_SAMPLES - 1) + mShipSteerX)
                      / NOISE_FILTER_SAMPLES;
    mFilteredSteerZ = (mFilteredSteerZ * (NOISE_FILTER_SAMPLES - 1) + mShipSteerZ)
                      / NOISE_FILTER_SAMPLES;

    // move player
    if (mLives > 0) {
        float steerX = mFilteredSteerX, steerZ = mFilteredSteerZ;
        if (mSteering == STEERING_TOUCH) {
            // touch steering
            mPlayerPos.x = Approach(mPlayerPos.x, steerX, PLAYER_MAX_LAT_SPEED * deltaT);
            mPlayerPos.z = Approach(mPlayerPos.z, steerZ, PLAYER_MAX_LAT_SPEED * deltaT);
        } else if (mSteering == STEERING_JOY) {
            // joystick steering
            mPlayerPos.x += deltaT * steerX;
            mPlayerPos.z += deltaT * steerZ;
        } else if (mSteering == STEERING_KEY) {
            // keyboard steering
            mPlayerPos.x += deltaT * steerX;
            mPlayerPos.z += deltaT * steerZ;
        }
    }
    mPlayerPos.y += deltaT * mPlayerSpeed;

    // make sure player didn't leave tunnel
    mPlayerPos.x = Clamp(mPlayerPos.x, PLAYER_MIN_X, PLAYER_MAX_X);
    mPlayerPos.z = Clamp(mPlayerPos.z, PLAYER_MIN_Z, PLAYER_MAX_Z);

    // shift sections if needed
    ShiftIfNeeded();

    // generate more obstacles!
    GenObstacles();

#ifndef GHOST_MODE
    // detect collisions
    DetectCollisions(previousY);
#endif // GHOST_MODE

    // update ship's roll speed according to level
    static float roll_speeds[] = ROLL_SPEEDS;
    int count = sizeof(roll_speeds) / sizeof(float);
    float speed = roll_speeds[mDifficulty % count];
    mRollAngle += deltaT * speed;
    while (mRollAngle < 0) {
        mRollAngle += 2 * M_PI;
    }
    while (mRollAngle > 2 * M_PI) {
        mRollAngle -= 2 * M_PI;
    }

    // did the game expire?
    if (mLives <= 0 && Clock() > mGameOverExpire) {
        SceneManager::GetInstance()->RequestNewScene(new WelcomeScene());
    }

    // produce the ambient sound
    int soundPoint = (int) floor(mPlayerPos.y / (TUNNEL_SECTION_LENGTH / 3));
    if (soundPoint % 3 != 0 && soundPoint > mLastAmbientBeepEmitted) {
        mLastAmbientBeepEmitted = soundPoint;
        SfxMan::GetInstance()->PlayTone(soundPoint % 2 ? TONE_AMBIENT_0 : TONE_AMBIENT_1);
    }
}

static float GetSectionCenterY(int i) {
    return (float) i * TUNNEL_SECTION_LENGTH;
}

static float GetSectionEndY(int i) {
    return GetSectionCenterY(i) + 0.5f * TUNNEL_SECTION_LENGTH;
}

static void _get_obs_color(int style, float *r, float *g, float *b) {
    style = Clamp(style, 1, 6);
    *r = OBS_COLORS[style * 3];
    *g = OBS_COLORS[style * 3 + 1];
    *b = OBS_COLORS[style * 3 + 2];
}

void PlayScene::RenderTunnel(GfxManager *gfxManager) {
    glm::mat4 modelMat;
    glm::mat4 mvpMat;
    int i, oi;

    Renderer& renderer = Renderer::GetInstance();
    const glm::mat4 &rotateMat = SceneManager::GetInstance()->GetRotationMatrix();
    std::shared_ptr<UniformBuffer> ourBuffer =
        gfxManager->GetUniformBuffer(GfxManager::kGfxType_OurTris);

    bool useIndexBuffer = (mTunnelGeom->index_buffer_.get() != NULL);
    if (useIndexBuffer) {
        renderer.BindIndexBuffer(mTunnelGeom->index_buffer_);
    }
    renderer.BindTexture(mWallTextures[0]);
    renderer.BindVertexBuffer(mTunnelGeom->vertex_buffer_);

    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_Tint,
                                    DEFAULT_TINT, UniformBuffer::kElementSize_Float4);
    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_PointLightColor,
                                    LIGHT_OFF, UniformBuffer::kElementSize_Float4);
    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_PointLightPos,
                                    LIGHT_POS, UniformBuffer::kElementSize_Float4);

    for (i = mFirstSection, oi = 0; i <= mFirstSection + RENDER_TUNNEL_SECTION_COUNT; ++i, ++oi) {
        float segCenterY = GetSectionCenterY(i);
        modelMat = glm::translate(glm::mat4(1.0), glm::vec3(0.0, segCenterY, 0.0));
        mvpMat = mProjMat * mViewMat * modelMat;
        mvpMat = rotateMat * mvpMat;
        const float* matrixData = glm::value_ptr(mvpMat);

        Obstacle *o = oi >= mObstacleCount ? NULL : GetObstacleAt(oi);

        // the point light is given in model coordinates, which is 0,0,0 is ok (center of
        // tunnel section)
        if (o) {
            float red, green, blue;
            _get_obs_color(o->style, &red, &green, &blue);
            const float lightColor[4] = {red, green, blue, 1.0f};
            ourBuffer->SetBufferElementData(GfxManager::kOurUniform_PointLightColor,
                                            lightColor, UniformBuffer::kElementSize_Float4);

        } else {
            ourBuffer->SetBufferElementData(GfxManager::kOurUniform_PointLightColor,
                                            LIGHT_OFF, UniformBuffer::kElementSize_Float4);

        }

        // render tunnel section
        ourBuffer->SetBufferElementData(GfxManager::kOurUniform_MVP,
                                        matrixData, UniformBuffer::kElementSize_Matrix44);

        if (useIndexBuffer) {
            renderer.DrawIndexed(mTunnelGeom->index_buffer_->GetBufferElementCount(), 0);
        } else {
            renderer.Draw(mTunnelGeom->vertex_buffer_->GetBufferElementCount(), 0);
        }
    }
}

void PlayScene::RenderObstacles(GfxManager *gfxManager) {
    int i;
    int r, c;
    float red, green, blue;
    glm::mat4 modelMat;
    glm::mat4 mvpMat;

    Renderer& renderer = Renderer::GetInstance();
    const glm::mat4 &rotateMat = SceneManager::GetInstance()->GetRotationMatrix();

    std::shared_ptr<UniformBuffer> ourBuffer =
        gfxManager->GetUniformBuffer(GfxManager::kGfxType_OurTris);
    renderer.BindTexture(mWallTextures[0]);
    renderer.BindVertexBuffer(mCubeGeom->vertex_buffer_);

    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_Tint,
                                    DEFAULT_TINT, UniformBuffer::kElementSize_Float4);
    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_PointLightColor,
                                    LIGHT_OFF, UniformBuffer::kElementSize_Float4);

    for (i = 0; i < mObstacleCount; i++) {
        Obstacle *o = GetObstacleAt(i);
        float posY = GetSectionCenterY(mFirstSection + i);

        if (o->style == Obstacle::STYLE_NULL) {
            // don't render null obstacles
            continue;
        }

        for (r = 0; r < OBS_GRID_SIZE; r++) {
            for (c = 0; c < OBS_GRID_SIZE; c++) {
                bool isBonus = r == o->bonusRow && c == o->bonusCol;
                if (o->grid[c][r]) {
                    // set up matrices
                    modelMat = glm::translate(glm::mat4(1.0f), o->GetBoxCenter(c, r, posY));
                    modelMat = glm::scale(modelMat, o->GetBoxSize(c, r));
                    mvpMat = mProjMat * mViewMat * modelMat;
                    mvpMat = rotateMat * mvpMat;

                    // set up color
                    _get_obs_color(o->style, &red, &green, &blue);

                    // render box
                    const float* matrixData = glm::value_ptr(mvpMat);
                    const float tintColor[4] = {red, green, blue, 1.0f};
                    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_MVP,
                                                    matrixData,
                                                    UniformBuffer::kElementSize_Matrix44);
                    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_Tint,
                                                    tintColor, UniformBuffer::kElementSize_Float4);
                    renderer.Draw(mCubeGeom->vertex_buffer_->GetBufferElementCount(), 0);
                } else if (isBonus) {
                    modelMat = glm::translate(glm::mat4(1.0f), o->GetBoxCenter(c, r, posY));
                    modelMat = glm::scale(modelMat, glm::vec3(OBS_BONUS_SIZE, OBS_BONUS_SIZE,
                                                              OBS_BONUS_SIZE));
                    modelMat = glm::rotate(modelMat, Clock() * 90.0f, glm::vec3(0.0f, 0.0f, 1.0f));
                    mvpMat = mProjMat * mViewMat * modelMat;
                    mvpMat = rotateMat * mvpMat;
                    const float* matrixData = glm::value_ptr(mvpMat);
                    const float tintColor[4] = {SineWave(0.8f, 1.0f, 0.5f, 0.0f),
                                                SineWave(0.8f, 1.0f, 0.5f, 0.0f),
                                                SineWave(0.8f, 1.0f, 0.5f, 0.0f), 1.0f};
                    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_MVP,
                                                    matrixData,
                                                    UniformBuffer::kElementSize_Matrix44);
                    ourBuffer->SetBufferElementData(GfxManager::kOurUniform_Tint,
                                                    tintColor, UniformBuffer::kElementSize_Float4);
                    renderer.Draw(mCubeGeom->vertex_buffer_->GetBufferElementCount(), 0);
                }
            }
        }
    }
}

void PlayScene::GenObstacles() {
    while (mObstacleCount < MAX_OBS) {
        // generate a new obstacle
        int index = (mFirstObstacle + mObstacleCount) % MAX_OBS;

        int section = mFirstSection + mObstacleCount;
        if (section < OBS_START_SECTION) {
            // generate an empty obstacle
            mObstacleCircBuf[index].Reset();
            mObstacleCircBuf[index].style = Obstacle::STYLE_NULL;
        } else {
            // generate a normal obstacle
            mObstacleGen.Generate(&mObstacleCircBuf[index]);
        }
        mObstacleCount++;
    }
}

void PlayScene::ShiftIfNeeded() {
    // is it time to discard a section and shift forward?
    while (mPlayerPos.y > GetSectionEndY(mFirstSection) + SHIFT_THRESH) {
        // shift to the next turnnel section
        mFirstSection++;

        // discard obstacle corresponding to the deleted section
        if (mObstacleCount > 0) {
            // discarding first object (shifting) is easy because it's a circular buffer!
            mFirstObstacle = (mFirstObstacle + 1) % MAX_OBS;
            --mObstacleCount;
        }
    }
}

void PlayScene::UpdateMenuSelFromTouch(float /*x*/, float y) {
    float sh = SceneManager::GetInstance()->GetScreenHeight();
    int item = (int) floor((y / sh) * (mMenuItemCount));
    mMenuSel = Clamp(item, 0, mMenuItemCount - 1);
}

void PlayScene::OnPointerDown(int pointerId, const struct PointerCoords *coords) {
    float x = coords->x, y = coords->y;
    if (mMenu) {
        if (coords->isScreen) {
            UpdateMenuSelFromTouch(x, y);
            mMenuTouchActive = true;
        }
    } else if (mSteering != STEERING_TOUCH) {
        mPointerId = pointerId;
        mPointerAnchorX = x;
        mPointerAnchorY = y;
        mShipAnchorX = mPlayerPos.x;
        mShipAnchorZ = mPlayerPos.z;
        mSteering = STEERING_TOUCH;
    }
}

void PlayScene::OnPointerUp(int pointerId, const struct PointerCoords *coords) {
    if (mMenu && mMenuTouchActive) {
        if (coords->isScreen) {
            mMenuTouchActive = false;
            HandleMenu(mMenuItems[mMenuSel]);
        }
    } else if (mSteering == STEERING_TOUCH && pointerId == mPointerId) {
        mSteering = STEERING_NONE;
    }
}

void PlayScene::OnPointerMove(int pointerId, const struct PointerCoords *coords) {
    float rangeY = coords->isScreen ? SceneManager::GetInstance()->GetScreenHeight() :
                   (coords->maxY - coords->minY);
    float x = coords->x, y = coords->y;

    if (mMenu && mMenuTouchActive) {
        UpdateMenuSelFromTouch(x, y);
    } else if (mSteering == STEERING_TOUCH && pointerId == mPointerId) {
        float deltaX = (x - mPointerAnchorX) * TOUCH_CONTROL_SENSIVITY / rangeY;
        float deltaY = -(y - mPointerAnchorY) * TOUCH_CONTROL_SENSIVITY / rangeY;
        float rotatedDx = cos(mRollAngle) * deltaX - sin(mRollAngle) * deltaY;
        float rotatedDy = sin(mRollAngle) * deltaX + cos(mRollAngle) * deltaY;

        mShipSteerX = mShipAnchorX + rotatedDx;
        mShipSteerZ = mShipAnchorZ + rotatedDy;
    }
}

void PlayScene::RenderHUD(GfxManager *gfxManager) {
    gfxManager->SetRenderState(GfxManager::kGfxType_BasicThickLinesNoDepthTest);

    SceneManager *sceneManager = SceneManager::GetInstance();
    float aspect = sceneManager->GetScreenAspect();
    const glm::mat4 &rotateMat = sceneManager->GetRotationMatrix();

    glm::mat4 orthoMat = glm::ortho(0.0f, aspect, 0.0f, 1.0f);
    glm::mat4 modelMat;
    glm::mat4 mat;

    // render score digits
    int i, unit;
    static char score_str[6];
    int score = GetScore();
    for (i = 0, unit = 10000; i < 5; i++, unit /= 10) {
        score_str[i] = '0' + (score / unit) % 10;
    }
    score_str[i] = '\0';

    mTextRenderer->SetFontScale(SCORE_FONT_SCALE);
    mTextRenderer->RenderText(score_str, SCORE_POS_X, SCORE_POS_Y);

    // render current sign
    if (mSignText) {
        modelMat = glm::mat4(1.0f);
        float t = Clock() - mSignStartTime;
        if (t < SIGN_ANIM_DUR) {
            float scale = t / SIGN_ANIM_DUR;
            modelMat = glm::scale(modelMat, glm::vec3(1.0f, scale, 1.0f));
        } else if (mSignTimeLeft < SIGN_ANIM_DUR) {
            float scale = mSignTimeLeft / SIGN_ANIM_DUR;
            modelMat = glm::scale(modelMat, glm::vec3(1.0f, scale, 1.0f));
        }

        mTextRenderer->SetMatrix(modelMat);
        mTextRenderer->SetFontScale(SIGN_FONT_SCALE);
        mTextRenderer->RenderText(mSignText, aspect * 0.5f, 0.5f);
        mTextRenderer->ResetMatrix();
    }

    float lifeX = LIFE_POS_X < 0.0f ? aspect + LIFE_POS_X : LIFE_POS_X;
    modelMat = glm::translate(glm::mat4(1.0), glm::vec3(lifeX, LIFE_POS_Y, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(1.0f, LIFE_SCALE_Y, 1.0f));
    int ubound = (mBlinkingHeart && BlinkFunc(0.2f)) ? mLives + 1 : mLives;
    for (int idx = 0; idx < ubound; idx++) {
        mat = orthoMat * modelMat;
        mat = rotateMat * mat;

        gfxManager->RenderSimpleGeom(GfxManager::kGfxType_BasicThickLinesNoDepthTest,
                                     glm::value_ptr(mat), mLifeGeom);
        modelMat = glm::translate(modelMat, glm::vec3(LIFE_SPACING_X, 0.0f, 0.0f));
    }

#ifdef TOUCH_INDICATOR_MODE
    if (mSteering == STEERING_TOUCH) {
        mRectRenderer->SetColor(1.0f, 1.0f, 0.0f);
    } else {
        mRectRenderer->SetColor(0.0f, 0.0f, 1.0f);
    }
    mRectRenderer->RenderRect(0.18f, 0.18f, 0.3f, 0.3f);
#endif // TOUCH_INDICATOR_MODE
}

void PlayScene::RenderMenu(GfxManager *gfxManager) {
    float aspect = SceneManager::GetInstance()->GetScreenAspect();
    gfxManager->SetRenderState(GfxManager::kGfxType_BasicTrisNoDepthTest);
    RenderBackgroundAnimation(mShapeRenderer);
    gfxManager->SetRenderState(GfxManager::kGfxType_BasicThickLinesNoDepthTest);

    float scaleFactor = SineWave(1.0f, MENUITEM_PULSE_AMOUNT, MENUITEM_PULSE_PERIOD, 0.0f);

    int i;
    for (i = 0; i < mMenuItemCount; i++) {
        float thisFactor = (mMenuSel == i) ? scaleFactor : 1.0f;
        float y = 1.0f - (i + 1) / ((float) mMenuItemCount + 1);
        float x = aspect * 0.5f;
        mTextRenderer->SetFontScale(thisFactor * MENUITEM_FONT_SCALE);
        mTextRenderer->SetColor(mMenuSel == i ? MENUITEM_SEL_COLOR : MENUITEM_COLOR);
        mTextRenderer->RenderText(mMenuItemText[mMenuItems[i]], x, y);
    }
    mTextRenderer->ResetColor();
}

void PlayScene::DetectCollisions(float previousY) {
    Obstacle *o = GetObstacleAt(0);
    float obsCenter = GetSectionCenterY(mFirstSection);
    float obsMin = obsCenter - OBS_BOX_SIZE;
    float curY = mPlayerPos.y;

    if (!o || !(previousY < obsMin && curY >= obsMin)) {
        // no collision
        return;
    }

    // what row/column is the player on?
    int col = o->GetColAt(mPlayerPos.x);
    int row = o->GetRowAt(mPlayerPos.z);

    if (o->grid[col][row]) {
        TunnelEngine::GetInstance()->GetVibrationHelper()->DoVibrateEffect();
#ifndef GOD_MODE
        // crashed against obstacle
        mLives--;
        if (mLives > 0) {
            ShowSign(S_OUCH, SIGN_DURATION);
            SfxMan::GetInstance()->PlayTone(TONE_CRASHED);
        } else {
            // say "Game Over"
            ShowSign(S_GAME_OVER, SIGN_DURATION_GAME_OVER);
            SfxMan::GetInstance()->PlayTone(TONE_GAME_OVER);
            mGameOverExpire = Clock() + GAME_OVER_EXPIRE;
        }
        mPlayerPos.y = obsMin - PLAYER_RECEDE_AFTER_COLLISION;
        mPlayerSpeed = PLAYER_SPEED_AFTER_COLLISION;
        mBlinkingHeart = true;
        mBlinkingHeartExpire = Clock() + BLINKING_HEART_DURATION;

        mLastCrashSection = mFirstSection;
#endif // GOD_MODE
    } else if (row == o->bonusRow && col == o->bonusCol) {
        ShowSign(S_GOT_BONUS, SIGN_DURATION_BONUS);
        o->DeleteBonus();
        AddScore(BONUS_POINTS);
        mBonusInARow++;

        if (mBonusInARow >= 10) {
            mBonusInARow = 0;
        }

        // update difficulty level, if applicable
        int score = GetScore();
        if (mDifficulty < score / SCORE_PER_LEVEL) {
            mDifficulty = score / SCORE_PER_LEVEL;
            ShowLevelSign();
            mObstacleGen.SetDifficulty(mDifficulty);
            SfxMan::GetInstance()->PlayTone(TONE_LEVEL_UP);

            // save progress, if needed
            if (TunnelEngine::GetInstance()->SaveProgress(mDifficulty)) {
                // Show a "checkpoint saved" sign when possible. We don't show it right away
                // because will already be showing the "Level N" sign, so we just set this flag
                // to remind us to show it right after.
                mCheckpointSignPending = true;
            }
        } else {
            int tone = (score % SCORE_PER_LEVEL) / BONUS_POINTS - 1;
            tone = tone < 0 ? 0 :
                   tone >= static_cast<int>(sizeof(TONE_BONUS) / sizeof(char *)) ?
                   static_cast<int>(sizeof(TONE_BONUS) / sizeof(char *) - 1) : tone;
            SfxMan::GetInstance()->PlayTone(TONE_BONUS[tone]);
        }

    } else if (o->HasBonus()) {
        // player missed bonus!
        mBonusInARow = 0;
    }

    // was it a close call?
    if (!o->grid[col][row]) {
        bool isCloseCall = false;
        for (int i = -1; i <= 1 && !isCloseCall; i++) {
            for (int j = -1; j <= 1; j++) {
                int other_row = o->GetColAt(mPlayerPos.x + i * CLOSE_CALL_CALC_DELTA);
                int other_col = o->GetRowAt(mPlayerPos.z + j * CLOSE_CALL_CALC_DELTA);
                if (o->grid[other_col][other_row]) {
                    isCloseCall = true;
                    break;
                }
            }
        }

    }
}

bool PlayScene::OnBackKeyPressed() {
    if (mMenu) {
        // reset frame clock so that the animation doesn't jump:
        mFrameClock.Reset();

        // leave menu
        ShowMenu(MENU_NONE);
    } else {
        // enter pause menu
        ShowMenu(MENU_PAUSE);
    }
    return true;
}


void PlayScene::OnJoy(float joyX, float joyY) {
    if (!mSteering || mSteering == STEERING_JOY) {
        float deltaX = joyX * JOYSTICK_CONTROL_SENSIVITY;
        float deltaY = joyY * JOYSTICK_CONTROL_SENSIVITY;
        float rotatedDx = cos(-mRollAngle) * deltaX - sin(-mRollAngle) * deltaY;
        float rotatedDy = sin(-mRollAngle) * deltaX + cos(-mRollAngle) * deltaY;
        mShipSteerX = rotatedDx;
        mShipSteerZ = -rotatedDy;
        mSteering = STEERING_JOY;

        // If player is going faster than the reference speed, PLAYER_SPEED, adjust it.
        // This makes the steering react faster as the ship accelerates in more difficult
        // levels.
        if (mPlayerSpeed > PLAYER_SPEED) {
            mShipSteerX *= mPlayerSpeed / PLAYER_SPEED;
            mShipSteerZ *= mPlayerSpeed / PLAYER_SPEED;
        }
    }
}

void PlayScene::OnMovementKey() {
    float deltaX = 0, deltaY = 0;
    if (mMotionKeyBitmask & UP_MOVEMENT_BIT) {
        deltaY -= KEY_CONTROL_VERTICAL_SENSIVITY;
    }
    if (mMotionKeyBitmask & LEFT_MOVEMENT_BIT) {
        deltaX -= KEY_CONTROL_HORIZONTAL_SENSIVITY;
    }
    if (mMotionKeyBitmask & DOWN_MOVEMENT_BIT) {
        deltaY += KEY_CONTROL_VERTICAL_SENSIVITY;
    }
    if (mMotionKeyBitmask & RIGHT_MOVEMENT_BIT) {
        deltaX += KEY_CONTROL_HORIZONTAL_SENSIVITY;
    }

    float rotatedDx = cos(-mRollAngle) * deltaX - sin(-mRollAngle) * deltaY;
    float rotatedDy = sin(-mRollAngle) * deltaX + cos(-mRollAngle) * deltaY;
    mShipSteerX = rotatedDx;
    mShipSteerZ = -rotatedDy;

    // If player is going faster than the reference speed, PLAYER_SPEED, adjust it.
    // This makes the steering react faster as the ship accelerates in more difficult
    // levels.
    if (mPlayerSpeed > PLAYER_SPEED) {
        mShipSteerX *= mPlayerSpeed / PLAYER_SPEED;
        mShipSteerZ *= mPlayerSpeed / PLAYER_SPEED;
    }
}

void PlayScene::OnKeyUp(int ourKeyCode) {
    if (isDirectionalKey(ourKeyCode) && mSteering == STEERING_KEY) {
        if (ourKeyCode == OURKEY_UP) {
            mMotionKeyBitmask &= ~UP_MOVEMENT_BIT;
        } else if (ourKeyCode == OURKEY_LEFT) {
            mMotionKeyBitmask &= ~LEFT_MOVEMENT_BIT;
        } else if (ourKeyCode == OURKEY_DOWN) {
            mMotionKeyBitmask &= ~DOWN_MOVEMENT_BIT;
        } else if (ourKeyCode == OURKEY_RIGHT) {
            mMotionKeyBitmask &= ~RIGHT_MOVEMENT_BIT;
        }

        if (!mMotionKeyBitmask) {
            mSteering = STEERING_NONE;
        }
    }
}

void PlayScene::OnKeyDown(int ourKeyCode) {
    if (isDirectionalKey(ourKeyCode)) {
        mSteering = STEERING_KEY;

        if (ourKeyCode == OURKEY_UP) {
            mMotionKeyBitmask |= UP_MOVEMENT_BIT;
        } else if (ourKeyCode == OURKEY_LEFT) {
            mMotionKeyBitmask |= LEFT_MOVEMENT_BIT;
        } else if (ourKeyCode == OURKEY_DOWN) {
            mMotionKeyBitmask |= DOWN_MOVEMENT_BIT;
        } else if (ourKeyCode == OURKEY_RIGHT) {
            mMotionKeyBitmask |= RIGHT_MOVEMENT_BIT;
        }
    }

    if (mMenu) {
        if (ourKeyCode == OURKEY_UP) {
            mMenuSel = mMenuSel > 0 ? mMenuSel - 1 : mMenuSel;
        } else if (ourKeyCode == OURKEY_DOWN) {
            mMenuSel = mMenuSel + 1 < mMenuItemCount ? mMenuSel + 1 : mMenuSel;
        } else if (ourKeyCode == OURKEY_ENTER) {
            HandleMenu(mMenuItems[mMenuSel]);
        }
    }
}

void PlayScene::ShowMenu(int menu) {
    mMenu = menu;
    mMenuSel = 0;
    TunnelEngine *instance = TunnelEngine::GetInstance();
    DataLoaderStateMachine *dataStateMachine = instance->GetDataStateMachine();
    switch (menu) {
        case MENU_PAUSE:
            mMenuItems[0] = MENUITEM_UNPAUSE;
            mMenuItems[1] = MENUITEM_QUIT;

            if (instance->IsCloudSaveEnabled() &&
                    dataStateMachine->getLevelLoaded() > mSavedLevel) {
                mMenuItems[2] = MENUITEM_RESUME_CLOUD;
                mMenuItemCount = 3;
            } else {
                mMenuItemCount = 2;
            }
            break;
        case MENU_LEVEL:
            mMenuItems[0] = MENUITEM_RESUME;
            mMenuItems[1] = MENUITEM_START_OVER;
            mMenuItemCount = 2;
            break;
        case MENU_LOADING:
            mMenuItems[0] = MENUITEM_LOADING;
            mMenuItemCount = 1;
            break;
        default:
            // since we're leaving the menu, reset the frame clock to avoid a skip
            // in the animation
            mFrameClock.Reset();
    }
    if (mMenu) {
        instance->SetInputSdkContext(INPUT_CONTEXT_PAUSE_MENU);
    } else {
        instance->SetInputSdkContext(INPUT_CONTEXT_PLAY_SCENE);
    }
}

void PlayScene::HandleMenu(int menuItem) {
    switch (menuItem) {
        case MENUITEM_QUIT:
            SceneManager::GetInstance()->RequestNewScene(new LoaderScene());
            break;
        case MENUITEM_UNPAUSE:
            ShowMenu(MENU_NONE);
            break;
        case MENUITEM_RESUME:
            // resume from saved level
            mDifficulty = (mSavedLevel / LEVELS_PER_CHECKPOINT) * LEVELS_PER_CHECKPOINT;
            SetScore(SCORE_PER_LEVEL * mDifficulty);
            mObstacleGen.SetDifficulty(mDifficulty);
            ShowLevelSign();
            ShowMenu(MENU_NONE);
            break;
        case MENUITEM_START_OVER:
            // start over from scratch
            TunnelEngine::GetInstance()->SaveProgress(/* level = */ 0, /* forceSave = */ true);
            ShowMenu(MENU_NONE);
            break;
        case MENUITEM_LOADING:
            ShowMenu(MENU_PAUSE);
            break;
        case MENUITEM_RESUME_CLOUD:
            DataLoaderStateMachine *dataStateMachine =
                TunnelEngine::GetInstance()->GetDataStateMachine();
            mSavedLevel = (dataStateMachine->getLevelLoaded() / LEVELS_PER_CHECKPOINT)
                          * LEVELS_PER_CHECKPOINT;

            if (mSavedLevel > mDifficulty) {
                mDifficulty = mSavedLevel;
                SetScore(SCORE_PER_LEVEL * mDifficulty);
                mObstacleGen.SetDifficulty(mDifficulty);
            }
            ShowLevelSign();
            ShowMenu(MENU_NONE);
            break;
    }
}

void PlayScene::ShowLevelSign() {
    static char level_str[] = "LEVEL XX";
    int level = mDifficulty + 1;
    level_str[6] = '0' + ((level > 9) ? (level / 10) % 10 : level % 10);
    level_str[7] = (level > 9) ? ('0' + level % 10) : '\0';
    level_str[8] = '\0';
    ShowSign(level_str, SIGN_DURATION);
}

void PlayScene::OnPause() {
    if (mMenu == MENU_NONE) {
        ShowMenu(MENU_PAUSE);
    }
}

void PlayScene::OnResume() {
    if (TunnelEngine::GetInstance()->IsCloudSaveEnabled() &&
            (mMenu == MENU_NONE || mMenu == MENU_PAUSE)) {
        ShowMenu(MENU_LOADING);
    }
}

void PlayScene::SetInputSdkContext() {
    TunnelEngine::GetInstance()->SetInputSdkContext(INPUT_CONTEXT_PLAY_SCENE);
}

void PlayScene::OnScreenResized(int /*width*/, int /*height*/) {
    UpdateProjectionMatrix();
}

void PlayScene::UpdateProjectionMatrix() {
    SceneManager *mgr = SceneManager::GetInstance();
    mProjMat = glm::perspective(RENDER_FOV, mgr->GetScreenAspect(), RENDER_NEAR_CLIP,
                                RENDER_FAR_CLIP);
}
