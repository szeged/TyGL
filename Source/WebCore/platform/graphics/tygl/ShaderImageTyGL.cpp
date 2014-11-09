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

#include "ShaderCommonTyGL.h"

namespace WebCore {

using TyGL::min;
using TyGL::max;

#define IMAGE_SHADER_ARGUMENTS(argument) \
    argument(u, texture, Texture, 1) \
    argument(a, position, Position, 4)

DEFINE_SHADER(
    ImageShader,
    IMAGE_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_position;

        varying vec2 v_texturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_texturePosition = a_position.zw;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_texture;
        varying vec2 v_texturePosition;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec4 resultColor = texture2D(u_texture, v_texturePosition);
    )
)

GLfloat* PlatformContextTyGL::imageSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setPrimaryTexture(shaderArguments[ImageShader::uTexture]);
    glVertexAttribPointer(shaderArguments[ImageShader::aPosition], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 4;
}

inline void PlatformContextTyGL::copyImageFastPath(GLfloat left, GLfloat bottom, GLfloat right, GLfloat top, GLfloat v0X, GLfloat v1Y,
    PassRefPtr<NativeImageTyGL> texture, const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode)
{
    if (left >= boundingClipRect().right() || right <= boundingClipRect().left()
            || bottom >= boundingClipRect().top() || top <= boundingClipRect().bottom())
        return;

    RefPtr<NativeImageTyGL> sourceTexture = texture;
    GLfloat width = sourceTexture->width();
    GLfloat height = sourceTexture->height();
    GLfloat textureLeft = srcRect.x() / width;
    GLfloat textureBottom = srcRect.y() / height;
    GLfloat textureRight = (srcRect.x() + srcRect.width()) / width;
    GLfloat textureTop = (srcRect.y() + srcRect.height()) / height;
    width = textureRight - textureLeft;
    height = textureTop - textureBottom;

    // Clipping.
    GLfloat limit = boundingClipRect().left();
    if (left < limit) {
        textureLeft += ((limit - left) / v0X) * width;
        left = limit;
    }
    limit = boundingClipRect().bottom();
    if (bottom < limit) {
        textureBottom += ((limit - bottom) / v1Y) * height;
        bottom = limit;
    }
    limit = boundingClipRect().right();
    if (right > limit) {
        textureRight -= ((right - limit) / v0X) * width;
        right = limit;
    }
    limit = boundingClipRect().top();
    if (top > limit) {
        textureTop -= ((top - limit) / v1Y) * height;
        top = limit;
    }
    if (left >= right || bottom >= top)
        return;

    CompositeOperator previousCompositeOperator = m_state.compositeOperator;
    BlendMode previousBlendMode = m_state.blendMode;
    if (op != previousCompositeOperator || blendMode != previousBlendMode)
        setCompositeOperation(op, blendMode);

    beginAttributeAdding(&ImageShader::s_program, sourceTexture, imageSetShaderArguments, left, bottom, right, top);
    addAttributeAbsolutePosition();
    addAttribute(textureLeft, textureBottom, textureRight, textureBottom, textureLeft, textureTop, textureRight, textureTop);
    endAttributeAdding();

    if (op != previousCompositeOperator || blendMode != previousBlendMode)
        setCompositeOperation(previousCompositeOperator, previousBlendMode);
}

#define TRANSFORMED_IMAGE_SHADER_ARGUMENTS(argument) \
    argument(u, texture, Texture, 1) \
    argument(a, position, Position, 4) \
    argument(a, delta, Delta, 4) \
    argument(a, texturePosition, TexturePosition, 4)

DEFINE_SHADER(
    TransformedImageShader,
    TRANSFORMED_IMAGE_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_position;
        attribute vec4 a_delta;
        attribute vec4 a_texturePosition;

        varying vec2 v_relativePosition;
        varying vec4 v_delta;
        varying vec4 v_texturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_relativePosition = a_position.zw;
        v_delta = a_delta;
        v_texturePosition = a_texturePosition;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_texture;
        varying vec2 v_relativePosition;
        varying vec4 v_delta;
        varying vec4 v_texturePosition;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        float centerS = v_relativePosition[0];
        float centerT = v_relativePosition[1];

        vec4 resultColor = vec4(0.0, 0.0, 0.0, 0.0);
        for (float i = -1.0; i <= 1.0; ++i) {
            for (float j = -1.0; j <= 1.0; ++j) {
                float s = centerS + i * v_delta[0] + j * v_delta[1];
                float t = centerT + i * v_delta[2] + j * v_delta[3];
                if (s == clamp(s, 0.0, 1.0) && t == clamp(t, 0.0, 1.0))
                    resultColor += texture2D(u_texture, vec2(v_texturePosition[0] + s * v_texturePosition[2], v_texturePosition[1] + t * v_texturePosition[3]));
            }
        }

        resultColor *= (1.0 / 9.0);
    )
)

GLfloat* PlatformContextTyGL::transformedImageSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    GLsizei strideLength = context->pipelineBuffer()->strideLength;

    context->setPrimaryTexture(shaderArguments[TransformedImageShader::uTexture]);
    glVertexAttribPointer(shaderArguments[TransformedImageShader::aPosition], 4, GL_FLOAT, GL_FALSE, strideLength, attributeStart);
    glVertexAttribPointer(shaderArguments[TransformedImageShader::aDelta], 4, GL_FLOAT, GL_FALSE, strideLength, attributeStart + 4);
    glVertexAttribPointer(shaderArguments[TransformedImageShader::aTexturePosition], 4, GL_FLOAT, GL_FALSE, strideLength, attributeStart + 8);
    return attributeStart + 12;
}

void PlatformContextTyGL::copyImage(const FloatRect& destRect, PassRefPtr<NativeImageTyGL> texture, const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode)
{
    ClippedTransformedQuad clippedTransformedQuad(destRect, transform(), boundingClipRect());
    if (!clippedTransformedQuad.hasVisiblePixels())
        return;

    GLfloat v0X = clippedTransformedQuad.transformedCoords(1).x() - clippedTransformedQuad.transformedCoords(0).x();
    GLfloat v0Y = clippedTransformedQuad.transformedCoords(1).y() - clippedTransformedQuad.transformedCoords(0).y();
    GLfloat v1X = clippedTransformedQuad.transformedCoords(2).x() - clippedTransformedQuad.transformedCoords(0).x();
    GLfloat v1Y = clippedTransformedQuad.transformedCoords(2).y() - clippedTransformedQuad.transformedCoords(0).y();

    const GLfloat discriminant = v1X * v0Y - v1Y * v0X;

    RefPtr<NativeImageTyGL> sourceTexture = texture;
    if (v0X > 0 && !v0Y && !v1X && v1Y > 0
            && isInteger(clippedTransformedQuad.transformedCoords(0).x()) && isInteger(clippedTransformedQuad.transformedCoords(0).y()) && isInteger(v0X) && isInteger(v1Y)
            && v0X * 1.25 >= srcRect.width() && v0X * 0.75 <= srcRect.width()
            && v1Y * 1.25 >= srcRect.height() && v1Y * 0.75 <= srcRect.height()) {
        copyImageFastPath(clippedTransformedQuad.transformedCoords(0).x(), clippedTransformedQuad.transformedCoords(0).y(),
                          clippedTransformedQuad.transformedCoords(1).x(), clippedTransformedQuad.transformedCoords(2).y(), v0X, v1Y, sourceTexture, srcRect, op, blendMode);
        return;
    }

    CompositeOperator previousCompositeOperator = m_state.compositeOperator;
    BlendMode previousBlendMode = m_state.blendMode;
    if (op != previousCompositeOperator || blendMode != previousBlendMode)
        setCompositeOperation(op, blendMode);

    beginAttributeAdding(&TransformedImageShader::s_program, sourceTexture, transformedImageSetShaderArguments, clippedTransformedQuad.minX(), clippedTransformedQuad.minY(), clippedTransformedQuad.maxX(), clippedTransformedQuad.maxY());

    addAttributeAbsolutePosition();

    clippedTransformedQuad.addToPipeline(pipelineBuffer());

    v1X /= discriminant;
    v1Y /= discriminant;
    v0X /= -discriminant;
    v0Y /= -discriminant;
    addAttributeRepeated(v1X / 3, v1Y / 3, v0X / 3, v0Y / 3);

    GLfloat width = sourceTexture->width();
    GLfloat height = sourceTexture->height();
    addAttributeRepeated(srcRect.x() / width, srcRect.y() / height, srcRect.width() / width, srcRect.height() / height);
    endAttributeAdding();

    if (op != previousCompositeOperator || blendMode != previousBlendMode)
        setCompositeOperation(previousCompositeOperator, previousBlendMode);
}

void PlatformContextTyGL::paintTexturedQuads(int numberOfQuads, PassRefPtr<NativeImageTyGL> sourceTexture, float* coordinates, float* textureCoordinates, const IntRect& boundingBox)
{
    beginAttributeAdding(&ImageShader::s_program, sourceTexture, imageSetShaderArguments, boundingBox);

    for (int i = 0; i < numberOfQuads * 4; i += 4) {
        if (!hasFreeSpace())
            flushBatchingQueueIfNeeded(false);

        addAttributePosition(coordinates[i], coordinates[i + 1], coordinates[i + 2], coordinates[i + 3]);
        addAttributePosition(textureCoordinates[i], textureCoordinates[i + 1], textureCoordinates[i + 2], textureCoordinates[i + 3]);
        endAttributeAdding();
    }
}

} // namespace WebCore
