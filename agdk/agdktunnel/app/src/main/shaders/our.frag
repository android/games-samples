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

layout (location = 0) in vec4 v_Color;
layout (location = 1) in vec4 v_Pos;
layout (location = 2) in vec4 v_PointLightPos;
layout (location = 3) in vec2 v_TexCoord;
layout (location = 4) in float v_FogFactor;

layout (binding = 1) uniform sampler2D u_Sampler;

layout(push_constant, std430) uniform PushConstants {
  layout(offset = 80) vec4 u_PointLightColor;
  layout(offset = 96) vec4 u_Tint;
} u_PushConstants;

layout (location = 0) out vec4 o_FragColor;

float ATT_FACT_2 = 0.005;
float ATT_FACT_1 = 0.00;
float SRGB_INVERSE_GAMMA_APPROX = 2.2;

void main()
{
  float d = distance(v_PointLightPos, v_Pos);
  float att = 1.0/(ATT_FACT_1 * d + ATT_FACT_2 * d * d);
  vec4 frag_color = mix(v_Color * u_PushConstants.u_Tint * texture(u_Sampler, v_TexCoord) +
                     u_PushConstants.u_PointLightColor * att, vec4(0), v_FogFactor);

  // The original GL sample was linear color space, but Vulkan is using a
  // sRGB framebuffer, do an approximation conversion
  vec3 rgb = pow(frag_color.rgb, vec3(SRGB_INVERSE_GAMMA_APPROX));
  o_FragColor = vec4(rgb.r, rgb.g, rgb.b, frag_color.a);
}
