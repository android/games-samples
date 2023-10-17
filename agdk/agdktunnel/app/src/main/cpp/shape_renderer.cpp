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

#include "shape_renderer.hpp"
#include "gfx_manager.hpp"
#include "util.hpp"

using namespace simple_renderer;

// geometry
static GLfloat RECT_VERTICES[] = {
        //  x      y     z      r     g     b     a
        -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // vertex 0
        0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // vertex 1
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // vertex 2
        -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f   // vertex 3
};

// indices
static GLushort RECT_INDICES[] = {0, 1, 2, 0, 2, 3};

ShapeRenderer::ShapeRenderer(std::shared_ptr<simple_renderer::UniformBuffer> uniformBuffer) {
    mUniformBuffer = uniformBuffer;
    mColor[0] = mColor[1] = mColor[2] = mColor[3] = 1.0f;
    mGeom = NULL;

    // create geometry
//    VertexBuf *vbuf = new VertexBuf(RECT_VERTICES, sizeof(RECT_VERTICES), 7 * sizeof(GLfloat));
//    vbuf->SetColorsOffset(3 * sizeof(GLfloat));
//    IndexBuf *ibuf = new IndexBuf(RECT_INDICES, sizeof(RECT_INDICES));
//    mGeom = new SimpleGeom(vbuf, ibuf);

    IndexBuffer::IndexBufferCreationParams index_params = {
        RECT_INDICES, sizeof(RECT_INDICES)
    };
    VertexBuffer::VertexBufferCreationParams vertex_params = {
        RECT_VERTICES, VertexBuffer::kVertexFormat_P3C4,
        sizeof(RECT_VERTICES)
    };
    Renderer& renderer = Renderer::GetInstance();

    std::shared_ptr<IndexBuffer> index_buffer = renderer.CreateIndexBuffer(index_params);
    std::shared_ptr<VertexBuffer> vertex_buffer = renderer.CreateVertexBuffer(vertex_params);
    mGeom = new SimpleGeom(index_buffer, vertex_buffer);
}

ShapeRenderer::~ShapeRenderer() {
    // destroy geometry
    CleanUp(&mGeom);
}

void ShapeRenderer::RenderRect(float centerX, float centerY, float width, float height) {
    SceneManager *sceneManager = SceneManager::GetInstance();
    float aspect = sceneManager->GetScreenAspect();
    const glm::mat4 &rotateMat = sceneManager->GetRotationMatrix();

    glm::mat4 orthoMat = glm::ortho(0.0f, aspect, 0.0f, 1.0f);
    glm::mat4 modelMat, mat;
    modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(centerX, centerY, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(width, height, 1.0f));
    mat = orthoMat * modelMat;
    mat = rotateMat * mat;

    const float* matrixData = glm::value_ptr(mat);
    simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();

    mUniformBuffer->SetBufferElementData(GfxManager::kBasicUniform_MVP,
                                         matrixData, UniformBuffer::kElementSize_Matrix44);
    mUniformBuffer->SetBufferElementData(GfxManager::kBasicUniform_Tint,
                                         mColor, UniformBuffer::kElementSize_Float4);
    renderer.BindVertexBuffer(mGeom->vertex_buffer_);
    if (mGeom->index_buffer_.get() != nullptr) {
        renderer.BindIndexBuffer(mGeom->index_buffer_);
        renderer.DrawIndexed(mGeom->index_buffer_->GetBufferElementCount(), 0);
    } else {
        renderer.Draw(mGeom->vertex_buffer_->GetBufferElementCount(), 0);
    }
}
