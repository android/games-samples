/*
 * Copyright 2013 The Android Open Source Project
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

//--------------------------------------------------------------------------------
// Box Renderer.h
// The class renders cubes.
//--------------------------------------------------------------------------------
#ifndef _BOXRENDERER_H
#define _BOXRENDERER_H

//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/sensor.h>
#include <jni.h>
#else

#include <GL/glew.h>
#include <errno.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include <vector>

#endif

#include "ndk_helper/NDKHelper.h"

// Decls of shader parameters.
struct BOX_VERTEX {
  float pos[3];
  float normal[3];
};

enum SHADER_ATTRIBUTES {
  ATTRIB_VERTEX,
  ATTRIB_NORMAL,
  ATTRIB_UV,
};

struct SHADER_PARAMS {
  GLuint program_;
  GLuint light0_;
  GLuint material_diffuse_;
  GLuint material_ambient_;
  GLuint material_specular_;

  GLuint matrix_projection_;
  GLuint matrix_view_;
};

// Box renerer implementation.
class BoxRenderer {
 public:
  // Ctor.
  BoxRenderer();

  // Dtor.
  virtual ~BoxRenderer();

  // Initialize shaders and buffers used to render the cube.
  void Init();

  // Update camera etc for each frame.
  void Update(float dTime);

  // Bind camera interface to the rendrer.
  bool Bind(ndk_helper::TapCamera *camera);

  // Unload shaders and buffers.
  void Unload();

  // Rendering API to render multiple cubes.
  void BeginMultipleRender();
  void RenderMultiple(const float *const matrix, float width, float height,
                      float depth, const float *const color);
  void EndMultipleRender();

 private:
  // Method to initialize the viewport.
  void UpdateViewport();
  // Helper to load shader.
  bool LoadShaders(SHADER_PARAMS *params, const char *strVsh,
                   const char *strFsh);
  int32_t num_indices_;
  int32_t num_vertices_;
  GLuint ibo_;
  GLuint vbo_;

  SHADER_PARAMS shader_param_;

  ndk_helper::Mat4 mat_projection_;
  ndk_helper::Mat4 mat_view_;
  ndk_helper::Mat4 mat_model_;

  ndk_helper::TapCamera *camera_;
};

#endif  //_BOXRENDERER_H