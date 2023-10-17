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

#include "ascii_to_geom.hpp"
#include "gfx_manager.hpp"
#include "scene_manager.hpp"
#include "text_renderer.hpp"
#include "util.hpp"

#include "alphabet.inl"

#define ALPHABET_SCALE 0.01f
#define CHAR_SPACING_F 0.1f // as a fraction of char width
#define LINE_SPACING_F 0.1f // as a fraction of char height
#define TEXT_LINE_WIDTH 4.0f

#define CORRECTION_Y -0.02f

TextRenderer::TextRenderer(std::shared_ptr<simple_renderer::UniformBuffer> uniformBuffer) {
  mUniformBuffer = uniformBuffer;
  memset(mCharGeom, 0, sizeof(mCharGeom));
  mFontScale = 1.0f;
  mMatrix = glm::mat4(1.0f);
  mColor[0] = mColor[1] = mColor[2] = mColor[3] = 1.0f;

  ALOGI("Loading alphabet glyphs.");
  int i;
  for (i = 0; i < CHAR_CODES; ++i) {
    if (ALPHABET_ART[i]) {
      ALOGI("Creating glyph for chr %d.", i);
      mCharGeom[i] = AsciiArtToGeom(ALPHABET_ART[i], ALPHABET_SCALE);
    }
  }
}

TextRenderer::~TextRenderer() {
  int i;
  for (i = 0; i < CHAR_CODES; i++) {
    CleanUp(&mCharGeom[i]);
  }
}

void TextRenderer::SetFontScale(float scale) {
  mFontScale = scale;
}

static void _count_rows_cols(const char *p, int *outCols, int *outRows) {
  int textCols = 0, textRows = 1;
  int curCols = 0;
  for (; *p; ++p) {
    if (*p == '\n') {
      ++textRows;
      curCols = 0;
    } else {
      ++curCols;
      if (textCols < curCols) {
        textCols = curCols;
      }
    }
  }
  *outCols = textCols;
  *outRows = textRows;
}

void TextRenderer::SetMatrix(glm::mat4 m) {
  mMatrix = m;
}

void TextRenderer::MeasureText(const char *str, float fontScale, float *outWidth,
                               float *outHeight) { // static!
  int rows, cols;
  _count_rows_cols(str, &cols, &rows);
  if (outWidth) {
    *outWidth = cols * ALPHABET_GLYPH_COLS * ALPHABET_SCALE * fontScale;
  }
  if (outHeight) {
    *outHeight = rows * ALPHABET_GLYPH_ROWS * ALPHABET_SCALE * fontScale;
  }
}

void TextRenderer::RenderText(const char *str, float centerX, float centerY) {
  SceneManager *sceneManager = SceneManager::GetInstance();
  float aspect = sceneManager->GetScreenAspect();
  const glm::mat4 &rotateMat = sceneManager->GetRotationMatrix();

  glm::mat4 orthoMat = glm::ortho(0.0f, aspect, 0.0f, 1.0f);
  glm::mat4 modelMat, mat, scaleMat;
  int cols, rows;

  simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();

  centerY += CORRECTION_Y * mFontScale;

  mUniformBuffer->SetBufferElementData(GfxManager::kBasicUniform_Tint, mColor,
                                       simple_renderer::UniformBuffer::kElementSize_Float4);

  _count_rows_cols(str, &cols, &rows);
  scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(mFontScale, mFontScale, 1.0f));
  float charWidth = ALPHABET_GLYPH_COLS * ALPHABET_SCALE * mFontScale;
  float charHeight = ALPHABET_GLYPH_ROWS * ALPHABET_SCALE * mFontScale;
  float charSpacing = CHAR_SPACING_F * charWidth;
  float lineSpacing = LINE_SPACING_F * charHeight;
  float width = cols * charWidth + (cols - 1) * charSpacing;
  float height = rows * charHeight + (rows - 1) * lineSpacing;
  float startX = centerX - width * 0.5f + 0.5f * charWidth;
  float startY = centerY + height * 0.5f - 0.5f * charHeight;
  float y = startY;

  modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(startX, startY, 0.0f));
  for (; *str; ++str) {
    if (*str == '\n') {
      y -= charHeight + lineSpacing;
      modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(startX, y, 0.0f));
    } else {
      int code = (int) *str;
      if (code >= 0 && code < CHAR_CODES && mCharGeom[code]) {
        mat = orthoMat * modelMat * scaleMat * mMatrix;
        mat = rotateMat * mat;
        const float* matrixData = glm::value_ptr(mat);
        mUniformBuffer->SetBufferElementData(GfxManager::kBasicUniform_MVP,
                                             matrixData,
                                             simple_renderer::UniformBuffer::kElementSize_Matrix44);
        renderer.BindIndexBuffer(mCharGeom[code]->index_buffer_);
        renderer.BindVertexBuffer(mCharGeom[code]->vertex_buffer_);
        renderer.DrawIndexed(mCharGeom[code]->index_buffer_->GetBufferElementCount(), 0);
      }
      modelMat = glm::translate(modelMat, glm::vec3(charWidth + charSpacing, 0.0f, 0.0f));
    }
  }
}
