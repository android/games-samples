/*
 * Copyright 2023 The Android Open Source Project
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

#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;

layout (push_constant, std430) uniform PushConstants {
  mat4 u_MVP;
  vec4 u_Tint;
} u_PushConstants;

layout(location = 0) out vec4 v_Color;

void main() {
  v_Color = a_Color * u_PushConstants.u_Tint;
  gl_Position = u_PushConstants.u_MVP * vec4(a_Position.x, a_Position.y, a_Position.z, 1.0);
}