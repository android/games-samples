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
layout (location = 0) out vec4 o_FragColor;

float SRGB_INVERSE_GAMMA_APPROX = 2.2;

void main() {
  // The original GL sample was linear color space, but Vulkan is using a
  // sRGB framebuffer, do an approximation conversion
  vec3 rgb = pow(v_Color.rgb, vec3(SRGB_INVERSE_GAMMA_APPROX));
  o_FragColor = vec4(rgb.r, rgb.g, rgb.b, v_Color.a);
}