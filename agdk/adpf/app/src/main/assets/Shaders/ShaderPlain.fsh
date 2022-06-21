#version 100
#extension GL_OES_standard_derivatives : enable
precision highp float;
precision highp int;

//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//  ShaderPlain.fsh
//
uniform lowp vec3       vMaterialAmbient;
uniform lowp vec4       vMaterialSpecular;
uniform lowp vec4       vMaterialDiffuse;

uniform highp vec3  vLight0;
varying mediump vec3 position;
varying mediump vec3 normal;

void main()
{
    vec3 N = normalize(normal);
    vec3 L = normalize(vLight0 - position);

    float lambertian = max(dot(N, L), 0.0);
    float specular = 0.0;
    if (lambertian > 0.0) {
        vec3 R = reflect(-L, N);
        vec3 V = normalize(-position);
        float NdotH = max(dot(R, V), 0.0);
        float fPower = vMaterialSpecular.w;
        specular = pow(NdotH, fPower);
    }

    lowp vec4 colorSpecular = vec4( vMaterialSpecular.xyz * specular + vMaterialAmbient, 1 );
    gl_FragColor = lambertian * vMaterialDiffuse + colorSpecular;
}