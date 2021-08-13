/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// GameApplication.cpp : This file contains the 'main' function.
// Program execution begins and ends there.
//

#include "pch.h"


// Old (blue)
//float TEAPOT_COLOR[] = { 100 / 255.0f, 149 / 255.0f, 237 / 255.0f };
// New (green)
float TEAPOT_COLOR[] = {61 / 255.0f, 220 / 255.0f, 132 / 255.0f};

#ifdef __ANDROID__
#include <jni.h>
#include <errno.h>

#include <android/sensor.h>
#include <android/log.h>
#include "android_native_app_glue.h"
#include <android/native_window_jni.h>

#include "TeapotRenderer.h"
#include "NDKHelper.h"

//-------------------------------------------------------------------------
// Preprocessor
//-------------------------------------------------------------------------
#define HELPER_CLASS_NAME \
  "com/example/gameapplication/NDKHelper"  // Class name of helper function
//-------------------------------------------------------------------------
// Shared state for our app.
//-------------------------------------------------------------------------
struct android_app;

class Engine {
    TeapotRenderer renderer_;

    ndk_helper::GLContext* gl_context_;

    bool initialized_resources_;
    bool has_focus_;

    ndk_helper::DoubletapDetector doubletap_detector_;
    ndk_helper::PinchDetector pinch_detector_;
    ndk_helper::DragDetector drag_detector_;
    ndk_helper::PerfMonitor monitor_;

    ndk_helper::TapCamera tap_camera_;

    android_app* app_;

    ASensorManager* sensor_manager_;
    const ASensor* accelerometer_sensor_;
    ASensorEventQueue* sensor_event_queue_;

    void UpdateFPS(float fFPS);
    void ShowUI();
    void TransformPosition(ndk_helper::Vec2& vec);

public:
    static void HandleCmd(struct android_app* app, int32_t cmd);
    static int32_t HandleInput(android_app* app, AInputEvent* event);

    Engine();
    ~Engine();
    void SetState(android_app* app);
    int InitDisplay(android_app* app);
    void LoadResources();
    void UnloadResources();
    void DrawFrame();
    void TermDisplay();
    void TrimMemory();
    bool IsReady();

    void UpdatePosition(AInputEvent* event, int32_t iIndex, float& fX, float& fY);

    void InitSensors();
    void ProcessSensors(int32_t id);
    void SuspendSensors();
    void ResumeSensors();
};

//-------------------------------------------------------------------------
// Ctor
//-------------------------------------------------------------------------
Engine::Engine()
    : initialized_resources_(false),
    has_focus_(false),
    app_(NULL),
    sensor_manager_(NULL),
    accelerometer_sensor_(NULL),
    sensor_event_queue_(NULL) {
    gl_context_ = ndk_helper::GLContext::GetInstance();
    //this->
}

//-------------------------------------------------------------------------
// Dtor
//-------------------------------------------------------------------------
Engine::~Engine() {}

/**
 * Load resources
 */
void Engine::LoadResources() {
    renderer_.Init();
    renderer_.Bind(&tap_camera_);
}

/**
 * Unload resources
 */
void Engine::UnloadResources() { renderer_.Unload(); }

/**
 * Initialize an EGL context for the current display.
 */
int Engine::InitDisplay(android_app* app) {
    if (!initialized_resources_) {
        gl_context_->Init(app_->window);
        LoadResources();
        initialized_resources_ = true;
    }
    else if (app->window != gl_context_->GetANativeWindow()) {
        // Re-initialize ANativeWindow.
        // On some devices, ANativeWindow is re-created when the app is resumed
        assert(gl_context_->GetANativeWindow());
        UnloadResources();
        gl_context_->Invalidate();
        app_ = app;
        gl_context_->Init(app->window);
        LoadResources();
        initialized_resources_ = true;
    }
    else {
        // initialize OpenGL ES and EGL
        if (EGL_SUCCESS == gl_context_->Resume(app_->window)) {
            UnloadResources();
            LoadResources();
        }
        else {
            assert(0);
        }
    }

    ShowUI();

    // Initialize GL state.
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Note that screen size might have been changed
    glViewport(0, 0, gl_context_->GetScreenWidth(),
        gl_context_->GetScreenHeight());
    renderer_.UpdateViewport();

    tap_camera_.SetFlip(1.f, -1.f, -1.f);
    tap_camera_.SetPinchTransformFactor(2.f, 2.f, 8.f);

    return 0;
}

/**
 * Just the current frame in the display.
 */
void Engine::DrawFrame() {
    float fps;
    if (monitor_.Update(fps)) {
        UpdateFPS(fps);
    }
     renderer_.Update(monitor_.GetCurrentTime());

    // Just fill the screen with a color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer_.Render(TEAPOT_COLOR);

    // Swap
    if (EGL_SUCCESS != gl_context_->Swap()) {
        UnloadResources();
        LoadResources();
    }
}

/**
 * Tear down the EGL context currently associated with the display.
 */
void Engine::TermDisplay() { gl_context_->Suspend(); }

void Engine::TrimMemory() {
    LOGI("Trimming memory");
    gl_context_->Invalidate();
}
/**
 * Process the next input event.
 */
int32_t Engine::HandleInput(android_app* app, AInputEvent* event) {
    Engine* eng = (Engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        ndk_helper::GESTURE_STATE doubleTapState =
            eng->doubletap_detector_.Detect(event);
        ndk_helper::GESTURE_STATE dragState = eng->drag_detector_.Detect(event);
        ndk_helper::GESTURE_STATE pinchState = eng->pinch_detector_.Detect(event);

        // Double tap detector has a priority over other detectors
        if (doubleTapState == ndk_helper::GESTURE_STATE_ACTION) {
            // Detect double tap
            eng->tap_camera_.Reset(true);
        }
        else {
            // Handle drag state
            if (dragState & ndk_helper::GESTURE_STATE_START) {
                // Otherwise, start dragging
                ndk_helper::Vec2 v;
                eng->drag_detector_.GetPointer(v);
                eng->TransformPosition(v);
                eng->tap_camera_.BeginDrag(v);
            }
            else if (dragState & ndk_helper::GESTURE_STATE_MOVE) {
                ndk_helper::Vec2 v;
                eng->drag_detector_.GetPointer(v);
                eng->TransformPosition(v);
                eng->tap_camera_.Drag(v);
            }
            else if (dragState & ndk_helper::GESTURE_STATE_END) {
                eng->tap_camera_.EndDrag();
            }

            // Handle pinch state
            if (pinchState & ndk_helper::GESTURE_STATE_START) {
                // Start new pinch
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                eng->pinch_detector_.GetPointers(v1, v2);
                eng->TransformPosition(v1);
                eng->TransformPosition(v2);
                eng->tap_camera_.BeginPinch(v1, v2);
            }
            else if (pinchState & ndk_helper::GESTURE_STATE_MOVE) {
                // Multi touch
                // Start new pinch
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                eng->pinch_detector_.GetPointers(v1, v2);
                eng->TransformPosition(v1);
                eng->TransformPosition(v2);
                eng->tap_camera_.Pinch(v1, v2);
            }
        }
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
void Engine::HandleCmd(struct android_app* app, int32_t cmd) {
    Engine* eng = (Engine*)app->userData;
    switch (cmd) {
    case APP_CMD_SAVE_STATE:
        break;
    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        if (app->window != NULL) {
            eng->InitDisplay(app);
            eng->has_focus_ = true;
            eng->DrawFrame();
        }
        break;
    case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        eng->TermDisplay();
        eng->has_focus_ = false;
        break;
    case APP_CMD_STOP:
        break;
    case APP_CMD_GAINED_FOCUS:
        eng->ResumeSensors();
        // Start animation
        eng->has_focus_ = true;
        break;
    case APP_CMD_LOST_FOCUS:
        eng->SuspendSensors();
        // Also stop animating.
        eng->has_focus_ = false;
        eng->DrawFrame();
        break;
    case APP_CMD_LOW_MEMORY:
        // Free up GL resources
        eng->TrimMemory();
        break;
    }
}

//-------------------------------------------------------------------------
// Sensor handlers
//-------------------------------------------------------------------------
void Engine::InitSensors() {
    sensor_manager_ = ndk_helper::AcquireASensorManagerInstance(app_);
    accelerometer_sensor_ = ASensorManager_getDefaultSensor(
        sensor_manager_, ASENSOR_TYPE_ACCELEROMETER);
    sensor_event_queue_ = ASensorManager_createEventQueue(
        sensor_manager_, app_->looper, LOOPER_ID_USER, NULL, NULL);
}

void Engine::ProcessSensors(int32_t id) {
    // If a sensor has data, process it now.
    if (id == LOOPER_ID_USER) {
        if (accelerometer_sensor_ != NULL) {
            ASensorEvent event;
            while (ASensorEventQueue_getEvents(sensor_event_queue_, &event, 1) > 0) {
            }
        }
    }
}

void Engine::ResumeSensors() {
    // When our app gains focus, we start monitoring the accelerometer.
    if (accelerometer_sensor_ != NULL) {
        ASensorEventQueue_enableSensor(sensor_event_queue_, accelerometer_sensor_);
        // We'd like to get 60 events per second (in us).
        ASensorEventQueue_setEventRate(sensor_event_queue_, accelerometer_sensor_,
            (1000L / 60) * 1000);
    }
}

void Engine::SuspendSensors() {
    // When our app loses focus, we stop monitoring the accelerometer.
    // This is to avoid consuming battery while not being used.
    if (accelerometer_sensor_ != NULL) {
        ASensorEventQueue_disableSensor(sensor_event_queue_, accelerometer_sensor_);
    }
}

//-------------------------------------------------------------------------
// Misc
//-------------------------------------------------------------------------
void Engine::SetState(android_app* state) {
    app_ = state;
    doubletap_detector_.SetConfiguration();
    drag_detector_.SetConfiguration();
    pinch_detector_.SetConfiguration();
}

bool Engine::IsReady() {
    if (has_focus_) return true;

    return false;
}

void Engine::TransformPosition(ndk_helper::Vec2& vec) {
    vec = ndk_helper::Vec2(2.0f, 2.0f) * vec /
        ndk_helper::Vec2(gl_context_->GetScreenWidth(),
            gl_context_->GetScreenHeight()) -
        ndk_helper::Vec2(1.f, 1.f);
}

void Engine::ShowUI() {
    JNIEnv* jni;
    app_->activity->vm->AttachCurrentThread(&jni, NULL);

    // Default class retrieval
    jclass clazz = jni->GetObjectClass(app_->activity->clazz);
    jmethodID methodID = jni->GetMethodID(clazz, "showUI", "()V");
    jni->CallVoidMethod(app_->activity->clazz, methodID);

    app_->activity->vm->DetachCurrentThread();
    return;
}

void Engine::UpdateFPS(float fFPS) {
    JNIEnv* jni;
    app_->activity->vm->AttachCurrentThread(&jni, NULL);

    // Default class retrieval
    jclass clazz = jni->GetObjectClass(app_->activity->clazz);
    jmethodID methodID = jni->GetMethodID(clazz, "updateFPS", "(F)V");
    jni->CallVoidMethod(app_->activity->clazz, methodID, fFPS);

    app_->activity->vm->DetachCurrentThread();
    return;
}

Engine g_engine;

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app* state) {

    g_engine. SetState(state);

    // Init helper functions
    ndk_helper::JNIHelper::Init(state->activity, HELPER_CLASS_NAME);

    state->userData = &g_engine;
    state->onAppCmd = Engine::HandleCmd;
    state->onInputEvent = Engine::HandleInput;

#ifdef USE_NDK_PROFILER
    monstartup("libTeapotNativeActivity.so");
#endif

    // Prepare to monitor accelerometer
    g_engine.InitSensors();

    // loop waiting for stuff to do.
    while (1) {
        // Read all pending events.
        int id;
        int events;
        android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, NULL, &events,
            (void**)&source)) >= 0) {
            // Process this event.
            if (source != NULL) source->process(state, source);

            g_engine.ProcessSensors(id);

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                g_engine.TermDisplay();
                return;
            }
        }

        if (g_engine.IsReady()) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            g_engine.DrawFrame();
        }
    }
}

#else

#include<Windows.h>
// first include Windows.h header file which is required
#include<stdio.h>
#include<gl/glew.h>
#include<gl/GL.h>   // GL.h header file
#include<gl/GLU.h> // GLU.h header file
#include<gl/glut.h>  // glut.h header file from freeglut\include\GL folder
#include<conio.h>
#include<stdio.h>
#include<math.h>
#include<string.h>
#include "TeapotRenderer.h"
#include "TapCamera.h"

TeapotRenderer renderer_;
ndk_helper::TapCamera camera_;

// Init_OpenGL() function
void Init_OpenGL() {
    GLint GlewInitResult = glewInit();
    if (GLEW_OK != GlewInitResult) {
        printf("ERROR: %s", glewGetErrorString(GlewInitResult));
        exit(EXIT_FAILURE);
    }
    // set background color to Black
    glClearColor(0.0, 0.0, 0.0, 0.0);
    renderer_.Init();
    renderer_.Bind(&camera_);
    renderer_.UpdateViewport();
    // set shade model to Flat
    glShadeModel(GL_FLAT);
}

// Display_Objects() function
void Display_Objects(void) {
    camera_.Update();
    renderer_.Update(0.13f);
    // clearing the window or remove all drawn objects
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer_.Render(TEAPOT_COLOR);
    glutSwapBuffers();
    glutPostRedisplay();
}

// Reshape() function
void Reshape(int w, int h) {
    //adjusts the pixel rectangle for drawing to be the entire new window
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    //matrix specifies the projection transformation
    //glMatrixMode(GL_PROJECTION);
    // load the identity of matrix by clearing it.
    //glLoadIdentity();
    //gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 20.0);
    //matrix specifies the modelview transformation
    //glMatrixMode(GL_MODELVIEW);
    // again  load the identity of matrix
    //glLoadIdentity();
    // gluLookAt() this function is used to specify the eye.
    // it is used to specify the coordinates to view objects from a specific position
    //gluLookAt(-0.3, 0.5, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

#pragma comment(lib, "user32.lib")

void DumpDevice(const DISPLAY_DEVICE &dd, int nSpaceCount) {
    wprintf(TEXT("%*sDevice Name: %s\n"), nSpaceCount, TEXT(""), dd.DeviceName);
    wprintf(TEXT("%*sDevice String: %s\n"), nSpaceCount, TEXT(""), dd.DeviceString);
    wprintf(TEXT("%*sState Flags: %x\n"), nSpaceCount, TEXT(""), dd.StateFlags);
    wprintf(TEXT("%*sDeviceID: %s\n"), nSpaceCount, TEXT(""), dd.DeviceID);
    wprintf(TEXT("%*sDeviceKey: ...%s\n\n"), nSpaceCount, TEXT(""), dd.DeviceKey + 42);
}

bool InitializeWithFirstDevice(char *program) {
    DISPLAY_DEVICE dd;
    dd.cb = sizeof(DISPLAY_DEVICE);

    DWORD deviceNum = 0;
    while (EnumDisplayDevices(NULL, deviceNum, &dd, 0)) {

        DISPLAY_DEVICE newdd = {0};
        newdd.cb = sizeof(DISPLAY_DEVICE);
        DWORD monitorNum = 0;
        while (EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0)) {
            DumpDevice(dd, 0);
            DumpDevice(newdd, 4);
            monitorNum++;
            int argc = 3;
            char *argv[3];
            char hold[250];
            argv[2] = hold;
            size_t size;
            wcstombs_s(&size, hold, dd.DeviceName, 250);
            argv[0] = _strdup(program);
            argv[1] = _strdup("-display");
            glutInit(&argc, argv);
            free(argv[0]);
            free(argv[1]);
            free(argv[2]);
            return true;
        }
        puts("");
        deviceNum++;
    }

    return false;
}

int main(int argc, char **argv) {
    if (argc != 1) {
        // If user passed parameters, then use those.
        glutInit(&argc, argv);
    } else {
        // Otherwise, choose the first device.
        if (!InitializeWithFirstDevice(argv[0])) {
            // Well that didn't work. Try default init afterall.
            glutInit(&argc, argv);
        }
    }

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(700, 500);
    glutInitWindowPosition(250, 50);
    glutCreateWindow("OpenGL Demo");
    Init_OpenGL();
    glutDisplayFunc(Display_Objects);
    glutReshapeFunc(Reshape);
    glutMainLoop();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item
//   to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select
//   the .sln file
#endif