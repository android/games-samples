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

#include "tex_quad.hpp"
#include "gfx_manager.hpp"

using namespace simple_renderer;

static void _put_vertex(float *v, float x, float y, float tex_u, float tex_v) {
    // position
    v[0] = x;
    v[1] = y;
    v[2] = 0.0f;

    // color
    v[3] = 1.0f;
    v[4] = 1.0f;
    v[5] = 1.0f;
    v[6] = 1.0f;

    // texture coords
    v[7] = tex_u;
    v[8] = tex_v;
}

void TexQuad::CreateGeom(float umin, float vmin, float umax, float vmax) {
    const int stride_floats = 9; // 3 for coords, 4 for color, 2 for tex coordinates
    const int stride_bytes = stride_floats * sizeof(GLfloat);
    int vertices = stride_floats * 4; // 4 vertices
    GLfloat *geom = new GLfloat[vertices];
    int geom_size = sizeof(GLfloat) * vertices;
    GLushort *indices = new GLushort[6]; // 6 indices
    const size_t indices_size = sizeof(GLushort) * 6;
    float left = -mAspect * 0.5f;
    float right = mAspect * 0.5f;
    float bottom = -0.5f;
    float top = 0.5f;

    /*
      D+----------+C
       |          |
       |          |
      A+----------+B
    */

    _put_vertex(geom, left, bottom, umin, vmin); // point A
    _put_vertex(geom + stride_floats, right, bottom, umax, vmin); // point B
    _put_vertex(geom + 2 * stride_floats, right, top, umax, vmax); // point C
    _put_vertex(geom + 3 * stride_floats, left, top, umin, vmax); // point D

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 0;
    indices[4] = 2;
    indices[5] = 3;

    // prepare geometry

    IndexBuffer::IndexBufferCreationParams index_params = {
        indices, indices_size
    };
    VertexBuffer::VertexBufferCreationParams vertex_params = {
        geom, VertexBuffer::kVertexFormat_P3T2C4,
        static_cast<size_t>(4 * stride_bytes)
    };
    Renderer& renderer = Renderer::GetInstance();

    std::shared_ptr<IndexBuffer> index_buffer = renderer.CreateIndexBuffer(index_params);
    std::shared_ptr<VertexBuffer> vertex_buffer = renderer.CreateVertexBuffer(vertex_params);
    mGeom = new SimpleGeom(index_buffer, vertex_buffer);
    // clean up our temporary buffers
    delete[] geom;
    geom = NULL;
    delete[] indices;
    indices = NULL;
}

void TexQuad::Render(glm::mat4 *transform) {
    SceneManager *sceneManager = SceneManager::GetInstance();
    float aspect = sceneManager->GetScreenAspect();
    const glm::mat4 &rotateMat = sceneManager->GetRotationMatrix();
    glm::mat4 orthoMat = glm::ortho(0.0f, aspect, 0.0f, 1.0f);
    glm::mat4 modelMat, mat;

    modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(mCenterX, mCenterY, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(mScale * mHeight, mScale * mHeight, 0.0f));
    if (transform) {
        mat = orthoMat * (*transform) * modelMat;
    } else {
        mat = orthoMat * modelMat;
    }
    mat = rotateMat * mat;

    const float* matrixData = glm::value_ptr(mat);
    simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();

    mUniformBuffer->SetBufferElementData(GfxManager::kOurUniform_MVP,
                                         matrixData, UniformBuffer::kElementSize_Matrix44);
    const float tintData[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    mUniformBuffer->SetBufferElementData(GfxManager::kOurUniform_Tint,
                                         tintData, UniformBuffer::kElementSize_Float4);
    renderer.BindIndexBuffer(mGeom->index_buffer_);
    renderer.BindVertexBuffer(mGeom->vertex_buffer_);
    renderer.BindTexture(mTexture);
    renderer.DrawIndexed(mGeom->index_buffer_->GetBufferElementCount(), 0);

}
