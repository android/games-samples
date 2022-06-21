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
// BoxRenderer.cpp
// Render boxes
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include "box_renderer.h"

const float CAM_X = -5.f;
const float CAM_Y = -5.f;
const float CAM_Z = 170.f;
const float CAM_NEAR = 5.f;
const float CAM_FAR = 1000.f;

//--------------------------------------------------------------------------------
// Box model data
//--------------------------------------------------------------------------------
float box_vertices[8 * 3] = {-0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,
                             0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
                             -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
                             0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f};

float box_normals[6 * 3] = {0.f, -1.f, 0.f,  -1.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                            0.f, 0.f,  -1.f, 1.f,  0.f, 0.f, 0.f, 1.f, 0.f};

uint16_t box_indices[6 * 6] = {0,  1,  2,  2,  3,  0,  7,  4,  8,  8,  11, 7,
                               12, 5,  9,  9,  16, 12, 6,  15, 19, 19, 10, 6,
                               13, 14, 18, 18, 17, 13, 23, 22, 21, 21, 20, 23};

//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
BoxRenderer::BoxRenderer() : camera_(nullptr) {}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
BoxRenderer::~BoxRenderer() { Unload(); }

//--------------------------------------------------------------------------------
// Initialize shaders and buffers used to render the cube.
//--------------------------------------------------------------------------------
void BoxRenderer::Init() {
  // Settings
  glEnable(GL_DEPTH_TEST);

  // Load shader
  LoadShaders(&shader_param_, "Shaders/VS_ShaderPlain.vsh",
              "Shaders/ShaderPlain.fsh");

  // Create Index buffer
  num_indices_ = sizeof(box_indices) / sizeof(box_indices[0]);
  glGenBuffers(1, &ibo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(box_indices), box_indices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Create VBO
  const int32_t NUM_FACES = 6;
  num_vertices_ = NUM_FACES * 4;  // 4 vertices * 6 faces.
  int32_t stride = sizeof(BOX_VERTEX);
  BOX_VERTEX *p = new BOX_VERTEX[num_vertices_];

  auto index = 0;
  for (auto face = 0; face < NUM_FACES; ++face) {
    for (auto v = 0; v < 6; ++v) {
      auto i = box_indices[index];
      p[i].pos[0] = box_vertices[(i % 8) * 3 + 0];
      p[i].pos[1] = box_vertices[(i % 8) * 3 + 1];
      p[i].pos[2] = box_vertices[(i % 8) * 3 + 2];

      p[i].normal[0] = box_normals[face * 3 + 0];
      p[i].normal[1] = box_normals[face * 3 + 1];
      p[i].normal[2] = box_normals[face * 3 + 2];

      index++;
    }
  }
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, stride * num_vertices_, p, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  delete[] p;

  UpdateViewport();
  mat_view_ = ndk_helper::Mat4::LookAt(ndk_helper::Vec3(CAM_X, CAM_Y, CAM_Z),
                                       ndk_helper::Vec3(0.f, 0.f, 0.f),
                                       ndk_helper::Vec3(0.f, 1.f, 0.f));
}

//--------------------------------------------------------------------------------
// Method to initialize the viewport.
//--------------------------------------------------------------------------------
void BoxRenderer::UpdateViewport() {
  // Init Projection matrices
  int32_t viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  if (viewport[2] < viewport[3]) {
    float aspect =
        static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
    mat_projection_ =
        ndk_helper::Mat4::Perspective(aspect, 1.0f, CAM_NEAR, CAM_FAR);
  } else {
    float aspect =
        static_cast<float>(viewport[3]) / static_cast<float>(viewport[2]);
    mat_projection_ =
        ndk_helper::Mat4::Perspective(1.0f, aspect, CAM_NEAR, CAM_FAR);
  }
}

//--------------------------------------------------------------------------------
// Unload shaders and buffers.
//--------------------------------------------------------------------------------
void BoxRenderer::Unload() {
  if (vbo_) {
    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
  }

  if (ibo_) {
    glDeleteBuffers(1, &ibo_);
    ibo_ = 0;
  }

  if (shader_param_.program_) {
    glDeleteProgram(shader_param_.program_);
    shader_param_.program_ = 0;
  }
}

//--------------------------------------------------------------------------------
// Update camera etc for each frame.
//--------------------------------------------------------------------------------
void BoxRenderer::Update(float fTime) {
  (void)fTime;  // Unused
  mat_view_ = ndk_helper::Mat4::LookAt(ndk_helper::Vec3(CAM_X, CAM_Y, CAM_Z),
                                       ndk_helper::Vec3(0.f, 0.f, 0.f),
                                       ndk_helper::Vec3(0.f, 1.f, 0.f));

  if (camera_) {
    camera_->Update();
    mat_view_ = camera_->GetTransformMatrix() * mat_view_ *
                camera_->GetRotationMatrix() * mat_model_;
  } else {
    mat_view_ = mat_view_ * mat_model_;
  }
}

//--------------------------------------------------------------------------------
// Set up rendering of cubes.
//--------------------------------------------------------------------------------
void BoxRenderer::BeginMultipleRender() {
  glEnable(GL_DEPTH_TEST);

  // Bind the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  // Pass the vertex data
  int32_t iStride = sizeof(BOX_VERTEX);
  glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, iStride,
                        BUFFER_OFFSET(0));
  glEnableVertexAttribArray(ATTRIB_VERTEX);

  glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, iStride,
                        BUFFER_OFFSET(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(ATTRIB_NORMAL);

  // Bind the IB
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

  glUseProgram(shader_param_.program_);

  // Update uniforms
  glUniform3f(shader_param_.light0_, -5.f, -5.f, -5.f);
}

//--------------------------------------------------------------------------------
// Render an instance of a cube.
//--------------------------------------------------------------------------------
void BoxRenderer::RenderMultiple(const float *const mat, float width,
                                 float height, float depth,
                                 const float *const color) {
  float diffuse_color[3] = {0.5f * color[0], 0.5f * color[1], 0.5f * color[2]};
  float specular_color[4] = {0.3f, 0.3f, 0.3f, 10.f};
  float ambient_color[3] = {0.1f, 0.1f, 0.1f};

  //
  // using glUniform3fv here was troublesome
  //
  glUniform3f(shader_param_.material_ambient_, ambient_color[0],
              ambient_color[1], ambient_color[2]);
  glUniform4f(shader_param_.material_specular_, specular_color[0],
              specular_color[1], specular_color[2], specular_color[3]);
  glUniform4f(shader_param_.material_diffuse_, diffuse_color[0],
              diffuse_color[1], diffuse_color[2], 1.f);

  //
  // Feed Projection and Model View matrices to the shaders
  auto scale = ndk_helper::Mat4::Scale(width, height, depth);
  auto model = ndk_helper::Mat4(mat);
  auto mat_vm = mat_view_ * model * scale;
  auto mat_vp = mat_projection_ * mat_vm;
  glUniformMatrix4fv(shader_param_.matrix_view_, 1, GL_FALSE, mat_vm.Ptr());
  glUniformMatrix4fv(shader_param_.matrix_projection_, 1, GL_FALSE,
                     mat_vp.Ptr());

  glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_SHORT,
                 BUFFER_OFFSET(0));
}

//--------------------------------------------------------------------------------
// Finish multiple rendering of the cubes.
//--------------------------------------------------------------------------------
void BoxRenderer::EndMultipleRender() {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//--------------------------------------------------------------------------------
// Helper to load a shader.
//--------------------------------------------------------------------------------
bool BoxRenderer::LoadShaders(SHADER_PARAMS *params, const char *strVsh,
                              const char *strFsh) {
  GLuint program;
  GLuint vert_shader, frag_shader;

  // Create shader program
  program = glCreateProgram();
  LOGI("Created Shader %d", program);

  // Create and compile vertex shader
  if (!ndk_helper::shader::CompileShader(&vert_shader, GL_VERTEX_SHADER,
                                         strVsh)) {
    LOGI("Failed to compile vertex shader");
    glDeleteProgram(program);
    return false;
  } else {
    LOGI("SUCCESS: %s\n", strVsh);
  }

  // Create and compile fragment shader
  if (!ndk_helper::shader::CompileShader(&frag_shader, GL_FRAGMENT_SHADER,
                                         strFsh)) {
    LOGI("Failed to compile fragment shader");
    glDeleteProgram(program);
    return false;
  } else {
    LOGI("SUCCESS: %s\n", strFsh);
  }

  // Attach vertex shader to program
  glAttachShader(program, vert_shader);

  // Attach fragment shader to program
  glAttachShader(program, frag_shader);

  // Bind attribute locations
  // this needs to be done prior to linking
  glBindAttribLocation(program, ATTRIB_VERTEX, "myVertex");
  glBindAttribLocation(program, ATTRIB_NORMAL, "myNormal");
  glBindAttribLocation(program, ATTRIB_UV, "myUV");

  // Link program
  if (!ndk_helper::shader::LinkProgram(program)) {
    LOGI("Failed to link program: %d", program);

    if (vert_shader) {
      glDeleteShader(vert_shader);
      vert_shader = 0;
    }
    if (frag_shader) {
      glDeleteShader(frag_shader);
      frag_shader = 0;
    }
    if (program) {
      glDeleteProgram(program);
    }

    return false;
  }

  // Get uniform locations
  params->matrix_projection_ = glGetUniformLocation(program, "uPMatrix");
  params->matrix_view_ = glGetUniformLocation(program, "uMVMatrix");

  params->light0_ = glGetUniformLocation(program, "vLight0");
  params->material_diffuse_ = glGetUniformLocation(program, "vMaterialDiffuse");
  params->material_ambient_ = glGetUniformLocation(program, "vMaterialAmbient");
  params->material_specular_ =
      glGetUniformLocation(program, "vMaterialSpecular");

  // Release vertex and fragment shaders
  if (vert_shader) glDeleteShader(vert_shader);
  if (frag_shader) glDeleteShader(frag_shader);

  params->program_ = program;
  return true;
}

//--------------------------------------------------------------------------------
// Bind camera interface.
//--------------------------------------------------------------------------------
bool BoxRenderer::Bind(ndk_helper::TapCamera *camera) {
  camera_ = camera;
  return true;
}