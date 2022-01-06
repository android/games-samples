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

#include "common.hpp"
#include "input_util.hpp"
#include "joystick-support.hpp"
#include "scene_manager.hpp"
#include "loader_scene.hpp"
#include "native_engine.hpp"

#include "game-activity/GameActivity.h"
#include "paddleboat/paddleboat.h"
#include "swappy/swappyGL.h"
#include "welcome_scene.hpp"

// verbose debug logs on?
#define VERBOSE_LOGGING 1

#if VERBOSE_LOGGING
#define VLOGD ALOGI
#else
#define VLOGD
#endif

// max # of GL errors to print before giving up
#define MAX_GL_ERRORS 200

static NativeEngine *_singleton = NULL;

// workaround for internal bug b/149866792
static NativeEngineSavedState appState = {false};

static void _GameControllerStatusCallback(const int32_t controllerIndex,
                                          const Paddleboat_ControllerStatus status, void *) {
    if (_singleton != NULL) {
        // Always make the most recently connected controller the active one
        if (status == PADDLEBOAT_CONTROLLER_JUST_CONNECTED) {
            _singleton->SetActiveGameControllerIndex(controllerIndex);
        } else if (status == PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED) {
            // We only care if the controller that disconnected was the one
            // we are currently using
            if (controllerIndex == _singleton->GetActiveGameControllerIndex()) {
                // Default to no fallback controller, loop and look for another connected
                // one
                int32_t newControllerIndex = -1;

                for (int32_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
                    if (i != controllerIndex &&
                        Paddleboat_getControllerStatus(i) == PADDLEBOAT_CONTROLLER_ACTIVE) {
                        newControllerIndex = i;
                        break;
                    }
                }
                _singleton->SetActiveGameControllerIndex(newControllerIndex);
            }
        }
    }
}

NativeEngine::NativeEngine(struct android_app *app) {
    ALOGI("NativeEngine: initializing.");
    mApp = app;
    mHasFocus = mIsVisible = mHasWindow = false;
    mHasGLObjects = false;
    mEglDisplay = EGL_NO_DISPLAY;
    mEglSurface = EGL_NO_SURFACE;
    mEglContext = EGL_NO_CONTEXT;
    mEglConfig = 0;
    mSurfWidth = mSurfHeight = 0;
    mGameControllerIndex = -1;
    mApiVersion = 0;
    mActiveAxisIds = 0;
    mJniEnv = NULL;
    mAppJniEnv = NULL;
    memset(&mState, 0, sizeof(mState));
    mIsFirstFrame = true;

    mGameAssetManager = new GameAssetManager(app->activity->assetManager,
                                             app->activity->vm,
                                             app->activity->javaGameActivity);
    mTextureManager = NULL;

    if (app->savedState != NULL) {
        // we are starting with previously saved state -- restore it
        mState = *(struct NativeEngineSavedState *) app->savedState;
    }

    // only one instance of NativeEngine may exist!
    MY_ASSERT(_singleton == NULL);
    _singleton = this;

    Paddleboat_init(GetJniEnv(), app->activity->javaGameActivity);
    Paddleboat_setControllerStatusCallback(_GameControllerStatusCallback, NULL);

    ALOGI("Calling SwappyGL_init");
    SwappyGL_init(GetJniEnv(), mApp->activity->javaGameActivity);
    SwappyGL_setSwapIntervalNS(SWAPPY_SWAP_60FPS);

    mTuningManager = new TuningManager(GetJniEnv(), app->activity->javaGameActivity, app->config);

    WelcomeScene::InitAboutText(GetJniEnv(), app->activity->javaGameActivity);

    // This is needed to allow controller events through to us.
    // By default, only touch-screen events are passed through, to match the
    // behaviour of NativeActivity.
    android_app_set_motion_event_filter(app, nullptr);

    // Flags to control how the IME behaves.
    constexpr int InputType_dot_TYPE_CLASS_TEXT = 1;
    constexpr int IME_ACTION_NONE = 1;
    constexpr int IME_FLAG_NO_FULLSCREEN = 33554432;

    GameActivity_setImeEditorInfo(app->activity, InputType_dot_TYPE_CLASS_TEXT,
                                  IME_ACTION_NONE, IME_FLAG_NO_FULLSCREEN);
}

NativeEngine *NativeEngine::GetInstance() {
    MY_ASSERT(_singleton != NULL);
    return _singleton;
}

NativeEngine::~NativeEngine() {
    VLOGD("NativeEngine: destructor running");
    delete mTuningManager;
    Paddleboat_setControllerStatusCallback(NULL, NULL);
    Paddleboat_destroy(GetJniEnv());
    SwappyGL_destroy();
    if (mTextureManager != NULL) {
        delete mTextureManager;
    }
    KillContext();
    delete mGameAssetManager;
    if (mJniEnv) {
        ALOGI("Detaching current thread from JNI.");
        mApp->activity->vm->DetachCurrentThread();
        ALOGI("Current thread detached from JNI.");
        mJniEnv = NULL;
    }
    _singleton = NULL;
}

static void _handle_cmd_proxy(struct android_app *app, int32_t cmd) {
    NativeEngine *engine = (NativeEngine *) app->userData;
    engine->HandleCommand(cmd);
}

//static int _handle_input_proxy(struct android_app* app, AInputEvent* event) {
//    NativeEngine *engine = (NativeEngine*) app->userData;
//    return engine->HandleInput(event) ? 1 : 0;
//}

bool NativeEngine::IsAnimating() {
    return mHasFocus && mIsVisible && mHasWindow;
}

static bool _cooked_event_callback(struct CookedEvent *event) {
    SceneManager *mgr = SceneManager::GetInstance();
    PointerCoords coords;
    memset(&coords, 0, sizeof(coords));
    coords.x = event->motionX;
    coords.y = event->motionY;
    coords.minX = event->motionMinX;
    coords.maxX = event->motionMaxX;
    coords.minY = event->motionMinY;
    coords.maxY = event->motionMaxY;
    coords.isScreen = event->motionIsOnScreen;

    switch (event->type) {
        case COOKED_EVENT_TYPE_JOY:
            mgr->UpdateJoy(event->joyX, event->joyY);
            return true;
        case COOKED_EVENT_TYPE_POINTER_DOWN:
            mgr->OnPointerDown(event->motionPointerId, &coords);
            return true;
        case COOKED_EVENT_TYPE_POINTER_UP:
            mgr->OnPointerUp(event->motionPointerId, &coords);
            return true;
        case COOKED_EVENT_TYPE_POINTER_MOVE:
            mgr->OnPointerMove(event->motionPointerId, &coords);
            return true;
        case COOKED_EVENT_TYPE_KEY_DOWN:
            mgr->OnKeyDown(event->keyCode);
            return true;
        case COOKED_EVENT_TYPE_KEY_UP:
            mgr->OnKeyUp(event->keyCode);
            return true;
        case COOKED_EVENT_TYPE_BACK:
            return mgr->OnBackKeyPressed();
        case COOKED_EVENT_TYPE_TEXT_INPUT:
            mgr->OnTextInput();
            return true;
        default:
            return false;
    }
}

void NativeEngine::GameLoop() {
    mApp->userData = this;
    mApp->onAppCmd = _handle_cmd_proxy;
    //mApp->onInputEvent = _handle_input_proxy;

    while (1) {
        int events;
        struct android_poll_source *source;

        // If not animating, block until we get an event; if animating, don't block.
        while ((ALooper_pollAll(IsAnimating() ? 0 : -1, NULL, &events, (void **) &source)) >= 0) {

            // process event
            if (source != NULL) {
                source->process(mApp, source);
            }

            // are we exiting?
            if (mApp->destroyRequested) {
                return;
            }
        }

        mGameAssetManager->UpdateGameAssetManager();
        Paddleboat_update(GetJniEnv());
        HandleGameActivityInput();

        if (mApp->textInputState) {
            struct CookedEvent ev;
            ev.type = COOKED_EVENT_TYPE_TEXT_INPUT;
            ev.textInputState = true;
            _cooked_event_callback(&ev);
            mApp->textInputState = 0;
        }

        if (IsAnimating()) {
            DoFrame();
        }
    }

}

JNIEnv *NativeEngine::GetJniEnv() {
    if (!mJniEnv) {
        ALOGI("Attaching current thread to JNI.");
        if (0 != mApp->activity->vm->AttachCurrentThread(&mJniEnv, NULL)) {
            ALOGE("*** FATAL ERROR: Failed to attach thread to JNI.");
            ABORT_GAME;
        }
        MY_ASSERT(mJniEnv != NULL);
        ALOGI("Attached current thread to JNI, %p", mJniEnv);
    }

    return mJniEnv;
}

JNIEnv *NativeEngine::GetAppJniEnv() {
    if (!mAppJniEnv) {
        ALOGI("Attaching current thread to JNI.");
        if (0 != mApp->activity->vm->AttachCurrentThread(&mAppJniEnv, NULL)) {
            ALOGE("*** FATAL ERROR: Failed to attach thread to JNI.");
            ABORT_GAME;
        }
        MY_ASSERT(mAppJniEnv != NULL);
        ALOGI("Attached current thread to JNI, %p", mAppJniEnv);
    }

    return mAppJniEnv;
}

static char sInsetsTypeName[][32] = {
    "CAPTION_BAR",
    "DISPLAY_CUTOUT",
    "IME",
    "MANDATORY_SYSTEM_GESTURES",
    "NAVIGATION_BARS",
    "STATUS_BARS",
    "SYSTEM_BARS",
    "SYSTEM_GESTURES",
    "TAPABLE_ELEMENT",
    "WATERFALL",
};

void NativeEngine::HandleCommand(int32_t cmd) {
    SceneManager *mgr = SceneManager::GetInstance();

    VLOGD("NativeEngine: handling command %d.", cmd);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.
            VLOGD("NativeEngine: APP_CMD_SAVE_STATE");
            mState.mHasFocus = mHasFocus;
            mApp->savedState = malloc(sizeof(mState));
            *((NativeEngineSavedState *) mApp->savedState) = mState;
            mApp->savedStateSize = sizeof(mState);
            break;
        case APP_CMD_INIT_WINDOW:
            // We have a window!
            VLOGD("NativeEngine: APP_CMD_INIT_WINDOW");
            if (mApp->window != NULL) {
                mHasWindow = true;
                SwappyGL_setWindow(mApp->window);
                if (mApp->savedStateSize == sizeof(mState) && mApp->savedState != nullptr) {
                    mState = *((NativeEngineSavedState *) mApp->savedState);
                    mHasFocus = mState.mHasFocus;
                } else {
                    // Workaround APP_CMD_GAINED_FOCUS issue where the focus state is not
                    // passed down from GameActivity when restarting Activity
                    mHasFocus = appState.mHasFocus;
                }
            }
            VLOGD("HandleCommand(%d): hasWindow = %d, hasFocus = %d", cmd,
                  mHasWindow ? 1 : 0, mHasFocus ? 1 : 0);
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is going away -- kill the surface
            VLOGD("NativeEngine: APP_CMD_TERM_WINDOW");
            KillSurface();
            mHasWindow = false;
            break;
        case APP_CMD_GAINED_FOCUS:
            VLOGD("NativeEngine: APP_CMD_GAINED_FOCUS");
            mHasFocus = true;
            mState.mHasFocus = appState.mHasFocus = mHasFocus;
            break;
        case APP_CMD_LOST_FOCUS:
            VLOGD("NativeEngine: APP_CMD_LOST_FOCUS");
            mHasFocus = false;
            mState.mHasFocus = appState.mHasFocus = mHasFocus;
            break;
        case APP_CMD_PAUSE:
            VLOGD("NativeEngine: APP_CMD_PAUSE");
            mGameAssetManager->OnPause();
            mgr->OnPause();
            break;
        case APP_CMD_RESUME:
            VLOGD("NativeEngine: APP_CMD_RESUME");
            mGameAssetManager->OnResume();
            mgr->OnResume();
            break;
        case APP_CMD_STOP:
            VLOGD("NativeEngine: APP_CMD_STOP");
            mIsVisible = false;
            Paddleboat_onStop(GetJniEnv());
            break;
        case APP_CMD_START:
            VLOGD("NativeEngine: APP_CMD_START");
            Paddleboat_onStart(GetJniEnv());
            mIsVisible = true;
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
            VLOGD("NativeEngine: %s", cmd == APP_CMD_WINDOW_RESIZED ?
                                      "APP_CMD_WINDOW_RESIZED" : "APP_CMD_CONFIG_CHANGED");
            // Window was resized or some other configuration changed.
            // Note: we don't handle this event because we check the surface dimensions
            // every frame, so that's how we know it was resized. If you are NOT doing that,
            // then you need to handle this event!
            break;
        case APP_CMD_LOW_MEMORY:
            VLOGD("NativeEngine: APP_CMD_LOW_MEMORY");
            // system told us we have low memory. So if we are not visible, let's
            // cooperate by deallocating all of our graphic resources.
            if (!mHasWindow) {
                VLOGD("NativeEngine: trimming memory footprint (deleting GL objects).");
                KillGLObjects();
            }
            break;
        case APP_CMD_CONTENT_RECT_CHANGED:
            VLOGD("NativeEngine: APP_CMD_CONTENT_RECT_CHANGED");
            break;
        case APP_CMD_WINDOW_REDRAW_NEEDED:
            VLOGD("NativeEngine: APP_CMD_WINDOW_REDRAW_NEEDED");
            break;
        case APP_CMD_WINDOW_INSETS_CHANGED:
            VLOGD("NativeEngine: APP_CMD_WINDOW_INSETS_CHANGED");
            ARect insets;
            // Log all the insets types
            for (int type = 0; type < GAMECOMMON_INSETS_TYPE_COUNT; ++type) {
                GameActivity_getWindowInsets(mApp->activity, (GameCommonInsetsType)type, &insets);
                VLOGD("%s insets: left=%d right=%d top=%d bottom=%d",
                      sInsetsTypeName[type], insets.left, insets.right, insets.top, insets.bottom);
            }
            break;
        default:
            VLOGD("NativeEngine: (unknown command).");
            break;
    }

    VLOGD("NativeEngine: STATUS: F%d, V%d, W%d, EGL: D %p, S %p, CTX %p, CFG %p",
          mHasFocus, mIsVisible, mHasWindow, mEglDisplay, mEglSurface, mEglContext,
          mEglConfig);
}

bool NativeEngine::HandleInput(AInputEvent *event) {
    return false;
}

int32_t NativeEngine::GetActiveGameControllerIndex() {
    return mGameControllerIndex;
}

void NativeEngine::SetActiveGameControllerIndex(const int32_t controllerIndex) {
    mGameControllerIndex = controllerIndex;
}

void NativeEngine::HandleGameActivityInput() {
    // If we get any key or motion events that were handled by a game controller,
    // read controller data and cook it into an event
    bool cookGameControllerEvent = false;

    // Swap input buffers so we don't miss any events while processing inputBuffer.
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(mApp);
    // Early exit if no events.
    if (inputBuffer == nullptr) return;

    if (inputBuffer->keyEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->keyEventsCount; ++i) {
            GameActivityKeyEvent* keyEvent = &inputBuffer->keyEvents[i];
            if (Paddleboat_processGameActivityKeyInputEvent(keyEvent,
                                                            sizeof(GameActivityKeyEvent))) {
                cookGameControllerEvent = true;
            } else {
                CookGameActivityKeyEvent(keyEvent, _cooked_event_callback);
            }
        }
        android_app_clear_key_events(inputBuffer);
    }
    if (inputBuffer->motionEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
            GameActivityMotionEvent* motionEvent = &inputBuffer->motionEvents[i];

            if (Paddleboat_processGameActivityMotionInputEvent(motionEvent,
                                                               sizeof(GameActivityMotionEvent))) {
                cookGameControllerEvent = true;
            } else {
                // Didn't belong to a game controller, process it ourselves if it is a touch event
                CookGameActivityMotionEvent(motionEvent,
                                            _cooked_event_callback);
            }
        }
        android_app_clear_motion_events(inputBuffer);
    }

    if (cookGameControllerEvent) {
        CookGameControllerEvent(mGameControllerIndex, _cooked_event_callback);
    }
}

void NativeEngine::CheckForNewAxis() {
    // Tell GameActivity about any new axis ids so it reports
    // their events
    const uint64_t activeAxisIds = Paddleboat_getActiveAxisMask();
    uint64_t newAxisIds = activeAxisIds ^mActiveAxisIds;
    if (newAxisIds != 0) {
        mActiveAxisIds = activeAxisIds;
        int32_t currentAxisId = 0;
        while (newAxisIds != 0) {
            if ((newAxisIds & 1) != 0) {
                ALOGI("Enable Axis: %d", currentAxisId);
                GameActivityPointerAxes_enableAxis(currentAxisId);
            }
            ++currentAxisId;
            newAxisIds >>= 1;
        }
    }
}

bool NativeEngine::InitDisplay() {
    if (mEglDisplay != EGL_NO_DISPLAY) {
        // nothing to do
        ALOGI("NativeEngine: no need to init display (already had one).");
        return true;
    }

    ALOGI("NativeEngine: initializing display.");
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (EGL_FALSE == eglInitialize(mEglDisplay, 0, 0)) {
        ALOGE("NativeEngine: failed to init display, error %d", eglGetError());
        return false;
    }
    return true;
}

bool NativeEngine::InitSurface() {
    // need a display
    MY_ASSERT(mEglDisplay != EGL_NO_DISPLAY);

    if (mEglSurface != EGL_NO_SURFACE) {
        // nothing to do
        ALOGI("NativeEngine: no need to init surface (already had one).");
        return true;
    }

    ALOGI("NativeEngine: initializing surface.");

    EGLint numConfigs;

    const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, // request OpenGL ES 3.0
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
    };

    // since this is a simple sample, we have a trivial selection process. We pick
    // the first EGLConfig that matches:
    eglChooseConfig(mEglDisplay, attribs, &mEglConfig, 1, &numConfigs);

    // create EGL surface
    mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, mApp->window, NULL);
    if (mEglSurface == EGL_NO_SURFACE) {
        ALOGE("Failed to create EGL surface, EGL error %d", eglGetError());
        return false;
    }

    ALOGI("NativeEngine: successfully initialized surface.");
    return true;
}

bool NativeEngine::InitContext() {
    // need a display
    MY_ASSERT(mEglDisplay != EGL_NO_DISPLAY);

    EGLint attribList[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE}; // OpenGL ES 3.0

    if (mEglContext != EGL_NO_CONTEXT) {
        // nothing to do
        ALOGI("NativeEngine: no need to init context (already had one).");
        return true;
    }

    ALOGI("NativeEngine: initializing context.");

    // create EGL context
    mEglContext = eglCreateContext(mEglDisplay, mEglConfig, NULL, attribList);
    if (mEglContext == EGL_NO_CONTEXT) {
        ALOGE("Failed to create EGL context, EGL error %d", eglGetError());
        return false;
    }

    ALOGI("NativeEngine: successfully initialized context.");

    return true;
}

void NativeEngine::ConfigureOpenGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}


bool NativeEngine::PrepareToRender() {
    if (mEglDisplay == EGL_NO_DISPLAY || mEglSurface == EGL_NO_SURFACE ||
        mEglContext == EGL_NO_CONTEXT) {

        // create display if needed
        if (!InitDisplay()) {
            ALOGE("NativeEngine: failed to create display.");
            return false;
        }

        // create surface if needed
        if (!InitSurface()) {
            ALOGE("NativeEngine: failed to create surface.");
            return false;
        }

        // create context if needed
        if (!InitContext()) {
            ALOGE("NativeEngine: failed to create context.");
            return false;
        }

        ALOGI("NativeEngine: binding surface and context (display %p, surface %p, context %p)",
              mEglDisplay, mEglSurface, mEglContext);

        // bind them
        if (EGL_FALSE == eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
            ALOGE("NativeEngine: eglMakeCurrent failed, EGL error %d", eglGetError());
            HandleEglError(eglGetError());
        }

        // configure our global OpenGL settings
        ConfigureOpenGL();

        if (mTextureManager == NULL) {
            mTextureManager = new TextureManager();
        }
    }
    if (!mHasGLObjects) {
        ALOGI("NativeEngine: creating OpenGL objects.");
        if (!InitGLObjects()) {
            ALOGE("NativeEngine: unable to initialize OpenGL objects.");
            return false;
        }
    }

    // ready to render
    return true;
}

void NativeEngine::KillGLObjects() {
    if (mHasGLObjects) {
        SceneManager *mgr = SceneManager::GetInstance();
        mgr->KillGraphics();
        mHasGLObjects = false;
    }
}

void NativeEngine::KillSurface() {
    ALOGI("NativeEngine: killing surface.");
    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (mEglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mEglDisplay, mEglSurface);
        mEglSurface = EGL_NO_SURFACE;
    }
    ALOGI("NativeEngine: Surface killed successfully.");
}

void NativeEngine::KillContext() {
    ALOGI("NativeEngine: killing context.");

    // since the context is going away, we have to kill the GL objects
    KillGLObjects();

    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (mEglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(mEglDisplay, mEglContext);
        mEglContext = EGL_NO_CONTEXT;
    }
    ALOGI("NativeEngine: Context killed successfully.");
}

void NativeEngine::KillDisplay() {
    // causes context and surface to go away too, if they are there
    ALOGI("NativeEngine: killing display.");
    KillContext();
    KillSurface();

    if (mEglDisplay != EGL_NO_DISPLAY) {
        ALOGI("NativeEngine: terminating display now.");
        eglTerminate(mEglDisplay);
        mEglDisplay = EGL_NO_DISPLAY;
    }
    ALOGI("NativeEngine: display killed successfully.");
}

bool NativeEngine::HandleEglError(EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            // nothing to do
            return true;
        case EGL_CONTEXT_LOST:
            ALOGW("NativeEngine: egl error: EGL_CONTEXT_LOST. Recreating context.");
            KillContext();
            return true;
        case EGL_BAD_CONTEXT:
            ALOGW("NativeEngine: egl error: EGL_BAD_CONTEXT. Recreating context.");
            KillContext();
            return true;
        case EGL_BAD_DISPLAY:
            ALOGW("NativeEngine: egl error: EGL_BAD_DISPLAY. Recreating display.");
            KillDisplay();
            return true;
        case EGL_BAD_SURFACE:
            ALOGW("NativeEngine: egl error: EGL_BAD_SURFACE. Recreating display.");
            KillSurface();
            return true;
        default:
            ALOGW("NativeEngine: unknown egl error: %d", error);
            return false;
    }
}

static void _log_opengl_error(GLenum err) {
    switch (err) {
        case GL_NO_ERROR:
            ALOGE("*** OpenGL error: GL_NO_ERROR");
            break;
        case GL_INVALID_ENUM:
            ALOGE("*** OpenGL error: GL_INVALID_ENUM");
            break;
        case GL_INVALID_VALUE:
            ALOGE("*** OpenGL error: GL_INVALID_VALUE");
            break;
        case GL_INVALID_OPERATION:
            ALOGE("*** OpenGL error: GL_INVALID_OPERATION");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            ALOGE("*** OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
        case GL_OUT_OF_MEMORY:
            ALOGE("*** OpenGL error: GL_OUT_OF_MEMORY");
            break;
        default:
            ALOGE("*** OpenGL error: error %d", err);
            break;
    }
}


void NativeEngine::DoFrame() {
    // prepare to render (create context, surfaces, etc, if needed)
    if (!PrepareToRender()) {
        // not ready
        VLOGD("NativeEngine: preparation to render failed.");
        return;
    }

    SceneManager *mgr = SceneManager::GetInstance();

    // how big is the surface? We query every frame because it's cheap, and some
    // strange devices out there change the surface size without calling any callbacks...
    int width, height;
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &width);
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &height);

    if (width != mSurfWidth || height != mSurfHeight) {
        // notify scene manager that the surface has changed size
        ALOGI("NativeEngine: surface changed size %dx%d --> %dx%d", mSurfWidth, mSurfHeight,
              width, height);
        mSurfWidth = width;
        mSurfHeight = height;
        mgr->SetScreenSize(mSurfWidth, mSurfHeight);
        glViewport(0, 0, mSurfWidth, mSurfHeight);
    }

    // if this is the first frame, install the welcome scene
    if (mIsFirstFrame) {
        mIsFirstFrame = false;
        mgr->RequestNewScene(new LoaderScene());
    }

    // render!
    mgr->DoFrame();

    // swap buffers
    if (!SwappyGL_swap(mEglDisplay, mEglSurface)) {        // failed to swap buffers...
        ALOGW("NativeEngine: SwappyGL_swap failed, EGL error %d", eglGetError());
        HandleEglError(eglGetError());
    }

    // print out GL errors, if any
    GLenum e;
    static int errorsPrinted = 0;
    while ((e = glGetError()) != GL_NO_ERROR) {
        if (errorsPrinted < MAX_GL_ERRORS) {
            _log_opengl_error(e);
            ++errorsPrinted;
            if (errorsPrinted >= MAX_GL_ERRORS) {
                ALOGE("*** NativeEngine: TOO MANY OPENGL ERRORS. NO LONGER PRINTING.");
            }
        }
    }
}

android_app *NativeEngine::GetAndroidApp() {
    return mApp;
}

bool NativeEngine::InitGLObjects() {
    if (!mHasGLObjects) {
        SceneManager *mgr = SceneManager::GetInstance();
        mgr->StartGraphics();
        _log_opengl_error(glGetError());
        mHasGLObjects = true;
    }
    return true;
}
