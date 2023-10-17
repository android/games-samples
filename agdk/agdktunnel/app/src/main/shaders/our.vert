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
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec4 a_Color;

layout (location = 0) out vec4 v_Color;
layout (location = 1) out vec4 v_Pos;
layout (location = 2) out vec4 v_PointLightPos;
layout (location = 3) out vec2 v_TexCoord;
layout (location = 4) out float v_FogFactor;

layout(push_constant, std430) uniform PushConstants {
  layout(offset = 0) mat4 u_MVP;
  layout(offset = 64) vec4 u_PointLightPos;
} u_PushConstants;

float FOG_START = 100.0;
float FOG_END = 200.0;

void main()
{
  v_Color = a_Color;
  vec4 position = u_PushConstants.u_MVP * vec4(a_Position.x, a_Position.y, a_Position.z, 1.0);
  gl_Position = position;
  v_Pos = position;
  v_PointLightPos = u_PushConstants.u_MVP * u_PushConstants.u_PointLightPos;
  v_TexCoord = a_TexCoord;
  v_FogFactor = clamp((v_Pos.z - FOG_START) / (FOG_END - FOG_START), 0.0, 1.0);
}