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

#ifndef agdktunnel_anim_hpp
#define agdktunnel_anim_hpp

#include "shape_renderer.hpp"

/* Renders the background animation seen on the main screen and menus (the parallax
 * rectangles scrolling by). */
void RenderBackgroundAnimation(ShapeRenderer *r);

#endif
