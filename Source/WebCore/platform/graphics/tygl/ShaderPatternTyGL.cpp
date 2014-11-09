/*
 * Copyright (C) 2013, 2014 University of Szeged
 * Copyright (C) 2014 Samsung Electronics Ltd.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AffineTransform.h"
#include "PlatformContextTyGL.h"
#include "PathTyGL.h"
#include "ShaderCommonTyGL.h"
#include "NativeImageTyGL.h"

#include <transforms/AffineTransform.h>
#include <wtf/RefPtr.h>

namespace WebCore {

using TyGL::min;
using TyGL::max;

#define FAST_PATTERN_SHADER_ARGUMENTS(argument) \
    argument(u, patternTexture, PatternTexture, 1) \
    argument(u, patternOffset, PatternOffset, 2) \
    argument(u, textureSize, TextureSize, 2) \
    argument(u, repeatXY, RepeatXY, 2)

DEFINE_SHADER(
    FastPatternShader,
    FAST_PATTERN_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_EMPTY_PROGRAM,
    // Vertex shader
    TyGL_EMPTY_PROGRAM,
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_patternTexture;
        uniform vec2 u_patternOffset;
        uniform vec2 u_textureSize;
        uniform vec2 u_repeatXY;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec2 FastPatternShader_coord = (gl_FragCoord.xy + u_patternOffset) / u_textureSize;

        if (u_repeatXY[0] > 0.5)
            FastPatternShader_coord[0] = fract(FastPatternShader_coord[0]);

        if (u_repeatXY[1] > 0.5)
            FastPatternShader_coord[1] = fract(FastPatternShader_coord[1]);

        resultColor = texture2D(u_patternTexture, FastPatternShader_coord) * resultColor.a;
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_fastPatternShaderProgram = &FastPatternShader::s_program;

GLfloat* PlatformContextTyGL::fastPatternSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    RefPtr<Pattern> pattern = context->pipelineBuffer()->pattern;
    ASSERT(pattern);

    RefPtr<NativeImageTyGL> tiledImage = pattern->tileImage()->nativeImageForCurrentFrame();
    context->setUniformTexture(shaderArguments[FastPatternShader::uPatternTexture], tiledImage);
    glUniform2f(shaderArguments[FastPatternShader::uRepeatXY], pattern->repeatX(),  pattern->repeatY());
    glUniform2f(shaderArguments[FastPatternShader::uTextureSize], tiledImage->size().width(),  tiledImage->size().height());
    glUniform2f(shaderArguments[FastPatternShader::uPatternOffset],
        -(context->pipelineBuffer()->complexFillTransform.xTranslation() + pattern->getPatternSpaceTransform().e()),
        -(context->pipelineBuffer()->complexFillTransform.yTranslation() + pattern->getPatternSpaceTransform().f()));

    return attributeStart;
}

#define TRANSFORMED_PATTERN_SHADER_ARGUMENTS(argument) \
    argument(u, patternTexture, PatternTexture, 1) \
    argument(u, patternOffset, PatternOffset, 2) \
    argument(u, neighbourDelta, NeighbourDelta, 2) \
    argument(u, repeatXY, RepeatXY, 2) \
    argument(u, transform, Transform, 4)

DEFINE_SHADER(
    TransformedPatternShader,
    TRANSFORMED_PATTERN_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_EMPTY_PROGRAM,
    // Vertex shader
    TyGL_EMPTY_PROGRAM,
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_patternTexture;
        uniform vec2 u_patternOffset;
        uniform vec2 u_repeatXY;
        uniform vec2 u_neighbourDelta;
        uniform vec4 u_transform;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec2 TransformedPatternShader_center = mat2(u_transform) * gl_FragCoord.xy + u_patternOffset;
        vec4 TransformedPatternShader_color = vec4(0.0, 0.0, 0.0, 0.0);

        for (float i = -1.0; i <= 1.0; ++i) {
            for (float j = -1.0; j <= 1.0; ++j) {
                vec2 TransformedPatternShader_coord = TransformedPatternShader_center + vec2(i, j) * u_neighbourDelta;

                if (u_repeatXY[0] > 0.5)
                    TransformedPatternShader_coord[0] = fract(TransformedPatternShader_coord[0]);

                if (u_repeatXY[1] > 0.5)
                    TransformedPatternShader_coord[1] = fract(TransformedPatternShader_coord[1]);

                TransformedPatternShader_color += texture2D(u_patternTexture, TransformedPatternShader_coord);
            }
        }

        TransformedPatternShader_color /= 9.0;
        resultColor = TransformedPatternShader_color * resultColor.a;
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_transformedPatternShaderProgram = &TransformedPatternShader::s_program;

GLfloat* PlatformContextTyGL::transformedPatternSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    RefPtr<Pattern> pattern = context->pipelineBuffer()->pattern;
    ASSERT(pattern);

    RefPtr<NativeImageTyGL> tiledImage = pattern->tileImage()->nativeImageForCurrentFrame();
    context->setUniformTexture(shaderArguments[TransformedPatternShader::uPatternTexture], tiledImage);
    glUniform2f(shaderArguments[TransformedPatternShader::uRepeatXY], pattern->repeatX(),  pattern->repeatY());
    // We handle the transform within the shader itself, rather than let OpenGL ES interpolate for us: this is because patterns
    // can have their own transform that differs from the GraphicsContext's transform (via SVG's "patternTransform" attribute), and we
    // can't guarantee that the shape to fill has set the texture coordinates appropriately.
    // Note that the shader maps *rendered* coordinates (i.e. pixels) back to the "pattern" coordinate-space i.e. the inverse of what
    // we usually do: hence the usage of "inverse" below.
    TyGL::AffineTransform globalTransformInverse = context->pipelineBuffer()->complexFillTransform.inverse();
    const WebCore::AffineTransform patternTransformWebCore = pattern->getPatternSpaceTransform().inverse();
    const TyGL::AffineTransform::Transform patternTransform = {patternTransformWebCore.a(), patternTransformWebCore.b(), patternTransformWebCore.c(), patternTransformWebCore.d(), patternTransformWebCore.e(), patternTransformWebCore.f() };
    TyGL::AffineTransform finalTransform = patternTransform;
    finalTransform.multiply(globalTransformInverse.transform());
    // Scale to texture coordinates.
    finalTransform.scale(1.0 / tiledImage->width(), 1.0 / tiledImage-> height());

    glUniform4f(shaderArguments[TransformedPatternShader::uTransform], finalTransform.transform().m_matrix[0], finalTransform.transform().m_matrix[1], finalTransform.transform().m_matrix[2], finalTransform.transform().m_matrix[3]);
    // We do the pattern offset after we have mapped to texture coordinates, so divide by texture size.
    const float translateX = finalTransform.xTranslation() / tiledImage->width();
    const float translateY = finalTransform.yTranslation() / tiledImage->height();
    glUniform2f(shaderArguments[TransformedPatternShader::uPatternOffset], translateX, translateY);
    // We look at the neighbouring pixels after we have mapped into texture coordinates, so divide the one pixel by the texture size.
    glUniform2f(shaderArguments[TransformedPatternShader::uNeighbourDelta], 1.0 / tiledImage->width(), 1.0 / tiledImage->height() );
    return attributeStart;
}

} // namespace WebCore
