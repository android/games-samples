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

#ifndef agdktunnel_obstacle_geom_hpp
#define agdktunnel_obstacle_geom_hpp

#include "engine.hpp"

/*
    H+-------+G
    /.      /|
 D / .    C/ |
  +-------+..+F
  | /E    | /
  |/      |/
 A+-------+B

*/
#define V_A  -0.5f, -0.5f,  0.5f
#define V_B   0.5f, -0.5f,  0.5f
#define V_C   0.5f,  0.5f,  0.5f
#define V_D  -0.5f,  0.5f,  0.5f
#define V_E  -0.5f, -0.5f, -0.5f
#define V_F   0.5f, -0.5f, -0.5f
#define V_G   0.5f,  0.5f, -0.5f
#define V_H  -0.5f,  0.5f, -0.5f
#define COLOR_1  1.0f, 1.0f, 1.0f, 1.0f
#define COLOR_2  0.8f, 0.8f, 0.8f, 1.0f
#define COLOR_3  0.6f, 0.6f, 0.6f, 1.0f

// max tex coordinate (i.e. texture repeats across cube)
#define TC_R 3.0f

static GLfloat CUBE_GEOM[] = {
        // front face of cube:
        V_A, 0.0f, 0.0f, COLOR_2,
        V_B, TC_R, 0.0f, COLOR_2,
        V_D, 0.0f, TC_R, COLOR_2,
        V_D, 0.0f, TC_R, COLOR_2,
        V_B, TC_R, 0.0f, COLOR_2,
        V_C, TC_R, TC_R, COLOR_2,
        // right face of cube:
        V_B, 0.0f, 0.0f, COLOR_3,
        V_F, TC_R, 0.0f, COLOR_3,
        V_C, 0.0f, TC_R, COLOR_3,
        V_C, 0.0f, TC_R, COLOR_3,
        V_F, TC_R, 0.0f, COLOR_3,
        V_G, TC_R, TC_R, COLOR_3,
        // left face of cube
        V_A, 0.0f, TC_R, COLOR_3,
        V_D, TC_R, TC_R, COLOR_3,
        V_E, 0.0f, 0.0f, COLOR_3,
        V_E, 0.0f, 0.0f, COLOR_3,
        V_D, TC_R, TC_R, COLOR_3,
        V_H, 0.0f, TC_R, COLOR_3,
        // back face of cube
        V_E, TC_R, 0.0f, COLOR_2,
        V_H, TC_R, TC_R, COLOR_2,
        V_F, 0.0f, 0.0f, COLOR_2,
        V_F, 0.0f, 0.0f, COLOR_2,
        V_H, TC_R, TC_R, COLOR_2,
        V_G, 0.0f, TC_R, COLOR_2,
        // bottom of cube
        V_A, 0.0f, TC_R, COLOR_1,
        V_E, 0.0f, 0.0f, COLOR_1,
        V_B, TC_R, TC_R, COLOR_1,
        V_B, TC_R, TC_R, COLOR_1,
        V_E, 0.0f, 0.0f, COLOR_1,
        V_F, TC_R, 0.0f, COLOR_1,
        // top of cube
        V_D, 0.0f, 0.0f, COLOR_1,
        V_C, TC_R, 0.0f, COLOR_1,
        V_H, 0.0f, TC_R, COLOR_1,
        V_H, 0.0f, TC_R, COLOR_1,
        V_C, TC_R, 0.0f, COLOR_1,
        V_G, TC_R, TC_R, COLOR_1
};
static const int CUBE_GEOM_COLOR_OFFSET = 3 * sizeof(GLfloat);
static const int CUBE_GEOM_TEXCOORD_OFFSET = 7 * sizeof(GLfloat);
static const int CUBE_GEOM_STRIDE = 9 * sizeof(GLfloat);

/* this is something of a trivial index buffer (and could just as well be
   replaced by an array), but we have it here for demonstration purposes:
static unsigned short CUBE_GEOM_INDICES[] = {
    0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35
};
*/
#endif
