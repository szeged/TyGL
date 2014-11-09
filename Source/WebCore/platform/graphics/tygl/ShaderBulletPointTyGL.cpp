/*
 * Copyright (C) 2013 University of Szeged
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
#include "PlatformContextTyGL.h"

#include "ShaderCommonTyGL.h"

// Since we are dealing with (usually small) ellipses that must be anti-aliased,
// carving the ellipse out of a rectangle via shaders seems to be a good approach.

namespace WebCore {

using TyGL::min;
using TyGL::max;

#define BULLET_POINT_SHADER_ARGUMENTS(argument) \
    argument(a, position, Position, 4) \
    argument(a, antiAliasThreshold, AntiAliasThreshold, 2)

DEFINE_SHADER(
    BulletPointShader,
    BULLET_POINT_SHADER_ARGUMENTS,
    PlatformContextTyGL::kAttributeSolidColoring,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_position;
        attribute vec2 a_antiAliasThreshold;

        varying vec2 v_relativePosition;
        varying vec2 v_antiAliasThreshold;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_relativePosition = a_position.zw;
        v_antiAliasThreshold = a_antiAliasThreshold;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        varying vec2 v_relativePosition;
        varying vec2 v_antiAliasThreshold;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        float s = v_relativePosition[0];
        float t = v_relativePosition[1];
        if (s != clamp(s, 0.0, 1.0))
            discard;
        if (t != clamp(t, 0.0, 1.0))
            discard;

        // As an optimisation, we've arranged things so that we don't need to divide by 2.0 everywhere, so this looks a bit cryptic.
        // Progress along the rectangle bounding the ellipse, from 0.0 to 2.0.
        float xProgressDoubled = s * 2.0;
        float yProgressDoubled = t * 2.0;

        // Simplified version of general formula for points within an ellipse, ((x - centreX) / w)^2 + ((y - centreY) / h)^2 <= 1
        // where centreX = width / 2 and centreY = height / 2. Adapted for use with a texture, where width and height are both 1.0,
        // and also optimised so we don't have to divide by 2.0.
        float ellipseEquation = ((xProgressDoubled - 1.0) * (xProgressDoubled - 1.0) + (yProgressDoubled - 1.0) * (yProgressDoubled - 1.0));
        if (ellipseEquation > 1.0)
            discard;

        vec4 resultColor = vec4(0.0, 0.0, 0.0, 1.0);
        float antiAliasThreshold = v_antiAliasThreshold[0];
        if (ellipseEquation >= 1.0 - antiAliasThreshold) {
            // Anti-alias, where points whose ellipseEquation result equals (1.0 - antiAliasThreshold) are solid, through
            // to points whose ellipseEquation result is 1.0 which are transparent.
            resultColor[3] = 1.0 / antiAliasThreshold * (1.0 - ellipseEquation);
        }
    )
)

GLfloat* PlatformContextTyGL::bulletPointSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    glVertexAttribPointer(shaderArguments[BulletPointShader::aPosition], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    glVertexAttribPointer(shaderArguments[BulletPointShader::aAntiAliasThreshold], 2, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart + 4);
    return attributeStart + 6;
}

void PlatformContextTyGL::drawBulletPoint(const FloatRect& rect, const Color& color)
{
    ClippedTransformedQuad clippedTransformedQuad(rect, transform(), boundingClipRect());

    if (!clippedTransformedQuad.hasVisiblePixels())
        return;

    Coloring coloring(premultipliedARGBFromColor(color.rgb()));
    m_coloring = &coloring;

    beginAttributeAdding(&BulletPointShader::s_program, nullptr, bulletPointSetShaderArguments, clippedTransformedQuad.minX(), clippedTransformedQuad.minY(), clippedTransformedQuad.maxX(), clippedTransformedQuad.maxY());
    addAttributeAbsolutePosition();
    clippedTransformedQuad.addToPipeline(pipelineBuffer());
    // Anti-aliasing an ellipse correctly is surprisingly hard, but this gives a relatively good approximation
    // as long as the difference between width and height is not too extreme.
    // Mathematically, "1.0" would make the most sense, but from trial-and-error, 1.3 looks better.
    const float bulletPointWidth = (clippedTransformedQuad.transformedCoords(1) - clippedTransformedQuad.transformedCoords(0)).diagonalLength();
    const float bulletPointHeight = (clippedTransformedQuad.transformedCoords(2) - clippedTransformedQuad.transformedCoords(0)).diagonalLength();
    const GLfloat antiAliasThreshold = 1.3 / min(bulletPointWidth, bulletPointHeight);
    addAttributeRepeated(antiAliasThreshold, 0.0);
    endAttributeAdding();

    resetColoring();
}

} // namespace WebCore
