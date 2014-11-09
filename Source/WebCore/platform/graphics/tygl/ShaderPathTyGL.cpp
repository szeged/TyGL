/*
 * Copyright (C) 2013, 2014 University of Szeged. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformContextTyGL.h"

#include "FloatPoint.h"
#include "PathTyGL.h"
#include "ShaderCommonTyGL.h"
#include "TrapezoidListTyGL.h"
#include "WindRule.h"

namespace WebCore {

#define FILL_PATH_SHADER_ARGUMENTS(argument) \
    argument(a, indexAndChannelAndY, IndexAndChannelAndY, 4) \
    argument(a, bottomAndTopX, BottomAndTopX, 4)

DEFINE_SHADER(
    FillPathShader,
    FILL_PATH_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_indexAndChannelAndY;
        attribute vec4 a_bottomAndTopX;

        // To reduce the rounding issues of float16 variables,
        // the numbers are spearated for integer and fractional parts.
        // On the downside, this doubles the required arguments.
        varying vec4 v_y1y2;
        varying vec4 v_x1x2;
        varying vec3 v_dx1dx2AndChannel;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        float bottomY = a_indexAndChannelAndY[2];
        float topY = a_indexAndChannelAndY[3];
        float bottomLeftX = a_bottomAndTopX[0];
        float bottomRightX = a_bottomAndTopX[1];
        float topLeftX = a_bottomAndTopX[2];
        float topRightX = a_bottomAndTopX[3];

        float height = topY - bottomY;
        float dx1 = (topLeftX - bottomLeftX) / height;
        float dx2 = (topRightX - bottomRightX) / height;

        float distance = fract(bottomY);
        float x1 = bottomLeftX - distance * dx1;
        float x2 = bottomRightX - distance * dx2;

        // Fake attribute.
        vec2 a_position = vec2(0.0, 0.0);
        float floorBottomY = floor(bottomY);
        const float minHeight = 3.0;

        if (a_indexAndChannelAndY[0] <= 2.0) {
            if (a_indexAndChannelAndY[0] == 0.0) {
                if (height <= minHeight || abs(bottomLeftX - topLeftX) >= (abs(dx1) * 2.0))
                    a_position.x = floor(min(bottomLeftX, topLeftX));
                else
                    a_position.x = floor(x1 - abs(dx1));
            } else {
                if (height <= minHeight || abs(bottomRightX - topRightX) >= (abs(dx2) * 2.0))
                    a_position.x = ceil(max(bottomRightX, topRightX));
                else
                    a_position.x = ceil(x2 + abs(dx2));
            }
            a_position.y = floorBottomY;
            v_y1y2[0] = 0.0;
        } else {
            distance = 1.0 - fract(topY);
            if (a_indexAndChannelAndY[0] == 3.0) {
                if (height <= minHeight || abs(bottomLeftX - topLeftX) >= (abs(dx1) * 2.0))
                    a_position.x = floor(min(bottomLeftX, topLeftX));
                else
                    a_position.x = floor((topLeftX - distance * dx1) - abs(dx1));
            } else {
                if (height <= minHeight || abs(bottomRightX - topRightX) >= (abs(dx2) * 2.0))
                    a_position.x = ceil(max(bottomRightX, topRightX));
                else
                    a_position.x = ceil((topRightX - distance * dx2) + abs(dx2));
            }
            a_position.y = ceil(topY);
            v_y1y2[0] = a_position.y - floorBottomY;
        }

        v_y1y2[1] = floor(topY) - floorBottomY;
        v_y1y2[2] = fract(bottomY);
        v_y1y2[3] = fract(topY);

        float floorX1 = floor(x1);
        v_x1x2[0] = a_position.x - floorX1;
        v_x1x2[1] = floor(x2) - floorX1;
        v_x1x2[2] = fract(x1);
        v_x1x2[3] = fract(x2);

        v_dx1dx2AndChannel[0] = dx1 * (1.0 / ANTIALIAS_LEVEL.0);
        v_dx1dx2AndChannel[1] = dx2 * (1.0 / ANTIALIAS_LEVEL.0);
        v_dx1dx2AndChannel[2] = a_indexAndChannelAndY[1];
    ),
    // Fragment variables
    TyGL_PROGRAM(
        varying vec4 v_y1y2;
        varying vec4 v_x1x2;
        varying vec3 v_dx1dx2AndChannel;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        const float step = 1.0 / ANTIALIAS_LEVEL.0;
        const float rounding = 1.0 / 32.0;

        float y = floor(v_y1y2[0]);
        float from = max(-y + v_y1y2[2], 0.0);
        float to = min(v_y1y2[1] - y + v_y1y2[3], 1.0) - from;

        vec2 dx1dx2 = v_dx1dx2AndChannel.xy;
        vec2 x1x2 = (y + from) * (dx1dx2 * ANTIALIAS_LEVEL.0);

        float x = floor(v_x1x2[0]);
        x1x2[0] = (-x) + (x1x2[0] + v_x1x2[2]);
        x1x2[1] = (v_x1x2[1] - x) + (x1x2[1] + v_x1x2[3]);

        // Alpha value to must be > 0.
        float resultAlpha = 1.0;

        float sum = (clamp(x1x2[1], 0.0, 1.0) - clamp(x1x2[0], 0.0, 1.0));
        if (to > 1.0 - rounding) {
            vec2 last = x1x2 + dx1dx2 * (ANTIALIAS_LEVEL.0 - 1.0);
            sum += (clamp(last[1], 0.0, 1.0) - clamp(last[0], 0.0, 1.0));
            to -= step;
        }

        if (sum <= 2.0 - rounding) {
            x1x2 += dx1dx2;

            while (to >= rounding) {
                sum += (clamp(x1x2[1], 0.0, 1.0) - clamp(x1x2[0], 0.0, 1.0));
                x1x2 += dx1dx2;
                to -= step;
            }

            resultAlpha = sum * step;
        }

        vec4 resultColor = vec4(0.0, 0.0, 0.0, 0.0);
        int channel = int(v_dx1dx2AndChannel.z);
        resultColor[channel] = resultAlpha;
    )
)

static GLuint s_fillPathShader[5];

static inline void setupAttributes(TyGL::TrapezoidList::Trapezoid& trapezoid, float channel, GLfloat* attributes, float offsetX, float offsetY)
{
    for (int i = 0; i < 4; i++) {
        // Numbers produced: 0, 3, 2, 5.
        *attributes++ = i + ((i & 0x1) << 1);
        *attributes++ = channel;
        *attributes++ = trapezoid.bottomY - offsetY;
        *attributes++ = trapezoid.topY - offsetY;

        *attributes++ = trapezoid.bottomLeftX - offsetX;
        *attributes++ = trapezoid.bottomRightX - offsetX;
        *attributes++ = trapezoid.topLeftX - offsetX;
        *attributes++ = trapezoid.topRightX - offsetX;
    }
}

void PlatformContextTyGL::fillPathAlphaTexture(TyGL::TrapezoidList* trapezoidList, PassRefPtr<NativeImageTyGL> pathAlphaTexture)
{
    pipelineBuffer()->flushBatchingQueueIfNeeded();

    pathAlphaTexture->bindFbo();

    glClear(GL_COLOR_BUFFER_BIT);

    const int shaderProgramCount = 1;
    const ShaderProgram* shaderProgram[shaderProgramCount] = { &FillPathShader::s_program };
    if (!compileAndUseShader(s_fillPathShader, shaderProgramCount, shaderProgram))
        return;

    glBlendFunc(GL_ONE, GL_ONE);
    GLuint offset = s_fillPathShader[1];

    TyGL::TrapezoidList::Trapezoid* trapezoids = trapezoidList->trapezoids();
    size_t trapezoidCount = trapezoidList->trapezoidCount();

    const int numberOfAttributes = 8;
    const int strideLength = numberOfAttributes * sizeof(GLfloat);
    int trapezoidIndex = 0;

    GLfloat* attributes = pipelineBuffer()->attributes;
    float x = trapezoidList->boundingBox().x();
    float y = trapezoidList->boundingBox().y();
    for (size_t i = 0; i < trapezoidCount; ++i) {
        setupAttributes(trapezoids[i], 3, attributes + trapezoidIndex * numberOfAttributes * 4, x, y);
        ++trapezoidIndex;
        if (trapezoidIndex >= TyGL::min(PipelineBuffer::kMaximumNumberOfUshortQuads, PipelineBuffer::kMaximumNumberOfAttributes / (numberOfAttributes * 4))) {
            glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aIndexAndChannelAndY], 4, GL_FLOAT, GL_FALSE, strideLength, attributes);
            glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aBottomAndTopX], 4, GL_FLOAT, GL_FALSE, strideLength, attributes + 4);
            glDrawElements(GL_TRIANGLES, trapezoidIndex * 6, GL_UNSIGNED_SHORT, nullptr);
            trapezoidIndex = 0;
        }
    }

    if (trapezoidIndex) {
        glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aIndexAndChannelAndY], 4, GL_FLOAT, GL_FALSE, strideLength, attributes);
        glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aBottomAndTopX], 4, GL_FLOAT, GL_FALSE, strideLength, attributes + 4);
        if (trapezoidIndex == 1)
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        else
            glDrawElements(GL_TRIANGLES, trapezoidIndex * 6, GL_UNSIGNED_SHORT, nullptr);
    }
}

#define FILL_COLOR_SHADER_ARGUMENTS(argument) \
    argument(u, texture, Texture, 1) \
    argument(a, position, Position, 2) \
    argument(a, texturePositionAndChannel, TexturePositionAndChannel, 3)

DEFINE_SHADER(
    FillColorShader,
    FILL_COLOR_SHADER_ARGUMENTS,
    PlatformContextTyGL::kAttributeSolidColoring | PlatformContextTyGL::kComplexFill,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec2 a_position;
        attribute vec3 a_texturePositionAndChannel;

        varying vec3 v_texturePositionAndChannel;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_texturePositionAndChannel = a_texturePositionAndChannel;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_texture;
        varying vec3 v_texturePositionAndChannel;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        int channel = int(v_texturePositionAndChannel.z);
        vec4 resultColor = vec4(0.0, 0.0, 0.0, texture2D(u_texture, v_texturePositionAndChannel.xy)[channel]);
    )
)

static inline int max(int a, int b)
{
    return a > b ? a : b;
}

bool PlatformContextTyGL::pathBeforeFlushAction(PlatformContextTyGL* context)
{
    PipelineBuffer* pipelineBuffer = context->pipelineBuffer();
    ASSERT(pipelineBuffer->quadCount >= 1);
    int textureSize = 2048;
    int offsetY[PipelineBuffer::kMaximumNumberOfPathAttributes];

    int currentOffsetY = 0;
    int maxWidth = 0;
    int maxHeight = 0;
    for (int pathIndex = 0; pathIndex < pipelineBuffer->quadCount; ++pathIndex) {
        TyGL::TrapezoidList* path = pipelineBuffer->pathAttributes[pathIndex].path.get();
        maxWidth = max(maxWidth, path->boundingBox().width());
        if (!(pathIndex & 0x3)) {
            currentOffsetY += maxHeight;
            maxHeight = path->boundingBox().height();
        } else
            maxHeight = max(maxHeight, path->boundingBox().height());

        offsetY[pathIndex] = currentOffsetY;
    }

    maxHeight += currentOffsetY;

    static RefPtr<NativeImageTyGL> pathAlphaTexture;
    if (!pathAlphaTexture) {
        pathAlphaTexture = NativeImageTyGL::create(IntSize(textureSize, textureSize));
        if (!pathAlphaTexture)
            return false;
    }

    pipelineBuffer->primaryShaderImage = pathAlphaTexture;
    pathAlphaTexture->bindFbo();

    const int shaderProgramCount = 1;
    const ShaderProgram* shaderProgram[shaderProgramCount] = { &FillPathShader::s_program };
    if (!compileAndUseShader(s_fillPathShader, shaderProgramCount, shaderProgram))
        return false;

    glBlendFunc(GL_ONE, GL_ONE);
    GLuint offset = s_fillPathShader[1];

    const int numberOfAttributes = 8;
    const int strideLength = numberOfAttributes * sizeof(GLfloat);
    int trapezoidIndex = 0;

    int attributeStartOffset = pipelineBuffer->quadCount * pipelineBuffer->numberOfAttributes * 4;
    GLfloat* attributes = pipelineBuffer->attributes + attributeStartOffset;
    int maximumNumberOfTrapezoids = TyGL::min(PipelineBuffer::kMaximumNumberOfUshortQuads, (PipelineBuffer::kMaximumNumberOfAttributes - attributeStartOffset) / (numberOfAttributes * 4));

    const int tileAlignment = 16;
    maxWidth = (maxWidth + tileAlignment - 1) & ~(tileAlignment - 1);
    maxHeight = (maxHeight + tileAlignment - 1) & ~(tileAlignment - 1);

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, maxWidth, maxHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int pathIndex = 0; pathIndex < pipelineBuffer->quadCount; ++pathIndex) {
        TyGL::TrapezoidList* path = pipelineBuffer->pathAttributes[pathIndex].path.get();
        TyGL::TrapezoidList::Trapezoid* trapezoids = path->trapezoids();
        size_t trapezoidCount = path->trapezoidCount();
        float x = path->boundingBox().x();
        float y = path->boundingBox().y() - offsetY[pathIndex];

        for (size_t i = 0; i < trapezoidCount; ++i) {
            setupAttributes(trapezoids[i], pathIndex & 0x3, attributes + trapezoidIndex * numberOfAttributes * 4, x, y);
            ++trapezoidIndex;
            if (trapezoidIndex >= maximumNumberOfTrapezoids) {
                glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aIndexAndChannelAndY], 4, GL_FLOAT, GL_FALSE, strideLength, attributes);
                glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aBottomAndTopX], 4, GL_FLOAT, GL_FALSE, strideLength, attributes + 4);
                glDrawElements(GL_TRIANGLES, trapezoidIndex * 6, GL_UNSIGNED_SHORT, nullptr);
                trapezoidIndex = 0;
            }
        }
    }

    if (trapezoidIndex) {
        glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aIndexAndChannelAndY], 4, GL_FLOAT, GL_FALSE, strideLength, attributes);
        glVertexAttribPointer(s_fillPathShader[offset + FillPathShader::aBottomAndTopX], 4, GL_FLOAT, GL_FALSE, strideLength, attributes + 4);
        if (trapezoidIndex == 1)
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        else
            glDrawElements(GL_TRIANGLES, trapezoidIndex * 6, GL_UNSIGNED_SHORT, nullptr);
    }
    glDisable(GL_SCISSOR_TEST);

    attributes = pipelineBuffer->attributes + 2;
    for (int pathIndex = 0; pathIndex < pipelineBuffer->quadCount; ++pathIndex) {
        TyGL::TrapezoidList* path = pipelineBuffer->pathAttributes[pathIndex].path.get();
        GLfloat width = static_cast<GLfloat>(path->boundingBox().width()) / textureSize;
        GLfloat topY = static_cast<float>(offsetY[pathIndex]) / textureSize;
        GLfloat bottomY = topY + static_cast<GLfloat>(path->boundingBox().height()) / textureSize;
        int channel = pathIndex & 0x3;

        attributes[1] = topY;
        attributes[2] = channel;
        attributes += pipelineBuffer->numberOfAttributes;

        attributes[0] = width;
        attributes[1] = topY;
        attributes[2] = channel;
        attributes += pipelineBuffer->numberOfAttributes;

        attributes[1] = bottomY;
        attributes[2] = channel;
        attributes += pipelineBuffer->numberOfAttributes;

        attributes[0] = width;
        attributes[1] = bottomY;
        attributes[2] = channel;
        attributes += pipelineBuffer->numberOfAttributes;

        pipelineBuffer->pathAttributes[pathIndex].path = nullptr;
    }

    return true;
}

GLfloat* PlatformContextTyGL::pathSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setPrimaryTexture(shaderArguments[FillColorShader::uTexture]);
    glVertexAttribPointer(shaderArguments[FillColorShader::aPosition], 2, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    glVertexAttribPointer(shaderArguments[FillColorShader::aTexturePositionAndChannel], 3, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart + 2);
    return attributeStart + 5;
}

void PlatformContextTyGL::fillPath(const TyGL::PathData* path, Coloring coloring, WindRule fillRule)
{
    m_coloring = &coloring;
    fillPath(path, fillRule);
    resetColoring();
}

void PlatformContextTyGL::fillPath(const TyGL::PathData* path, WindRule fillRule)
{
    OwnPtr<TyGL::TrapezoidList> trapezoidList = path->trapezoidList(m_state.boundingClipRect, transform(), fillRule);
    const IntSize size = trapezoidList->boundingBox().size();

    if (!size.width() || !size.height())
        return;

    if (trapezoidList->shapeType() == TyGL::TrapezoidList::ShapeTypeRect) {
        TyGL::TrapezoidList::Trapezoid* trapezoid = trapezoidList->trapezoids();
        fillAbsoluteRect(trapezoid->bottomLeftX, trapezoid->bottomY, trapezoid->bottomRightX, trapezoid->topY);
        return;
    }

    BeforeFlushAction beforeFlushAction(pathBeforeFlushAction, nullptr);
    beginAttributeAdding(&FillColorShader::s_program, nullptr, pathSetShaderArguments, trapezoidList->boundingBox(), &beforeFlushAction);
    addAttributeAbsolutePosition();
    addAttributeRepeated(0, 0, 0);
    addTrapezoidList(trapezoidList.release());
    endAttributeAdding();

    if (pipelineBuffer()->quadCount >= PipelineBuffer::kMaximumNumberOfPathAttributes)
        pipelineBuffer()->flushBatchingQueue();
}

} // namespace WebCore
