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

namespace WebCore {

using TyGL::min;
using TyGL::max;

// Basic algorithm: we might have to draw many, many small points, *or* a smaller number
// of larger rectangles with anti-aliasing, or anything in between. Rather than
// make each dash a dot, line, or quad primitive, I thought it might be more efficient
// to instead have just have one quad (a "thicker" version of the line to draw) and utilise the
// shader to do the splitting into dashes and anti-aliasing.
// In order to work out where we are "along" and "across" the line, we (ab?)use the fact that
// OpenGL ES interpolates along a (imaginary, here) texture that covers the quad - except if
// the line is horizontal or vertical, in which case we only get an acceptable degree of accuracy
// if we special-case it.
// The weakness is that there is a lot of special-casing and magic to try and get horizontal/ vertical
// lines or lines with short strokeThickness or dash length looking good, and it doesn't always work.
#define BORDER_LINE_SHADER_ARGUMENTS(argument) \
    argument(a, position, Position, 4) \
    argument(a, thicknessAndDashData, ThicknessAndDashData, 4) \
    argument(a, packedData, PackedData, 4)

DEFINE_SHADER(
    BorderLineShader,
    BORDER_LINE_SHADER_ARGUMENTS,
    PlatformContextTyGL::kAttributeSolidColoring,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_position;
        attribute vec4 a_thicknessAndDashData;
        attribute vec4 a_packedData;

        varying vec2 v_relativePosition;
        varying vec4 v_thicknessAndDashData;
        varying vec4 v_packedData;

    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_relativePosition = a_position.zw;
        v_thicknessAndDashData = a_thicknessAndDashData;
        v_packedData = a_packedData;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        varying vec2 v_relativePosition;
        varying vec4 v_thicknessAndDashData;
        varying vec4 v_packedData;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        float s = v_relativePosition[0];
        float t = v_relativePosition[1];
        if (s != clamp(s, 0.0, 1.0))
            discard;
        if (t != clamp(t, 0.0, 1.0))
            discard;

        float alpha = 1.0;
        float dashLength = v_thicknessAndDashData[2];
        // 0: some arbitrary rotation.
        // 1: Horizontal.
        // 2: Vertical.
        int orientationType = int(v_packedData[0]);
        vec2 bottomLeft = vec2(v_packedData[2], v_packedData[3]);

        if (dashLength == 1.0 && orientationType != 0) {
            // Simple "checkerboard" pattern of dots - this really needs to be special-cased, as we
            // don't get enough accuracy otherwise.
            if (mod(floor(gl_FragCoord.x), 2.0) == mod(floor(gl_FragCoord.y), 2.0))
                discard;
        } else {
            float lineLength = v_packedData[1];
            float dashOffset = v_thicknessAndDashData[3];
            float lengthAlongLine;
            // v_relativePosition isn't quite accurate enough if we have a horizontal or vertical line, so
            // we special-case the computation of lengthAlongLine.
            if (orientationType == 0) {
                // Will be between 0 and twice the dash length. If it lies within
                // the beginning half (0 - dashLength), then draw the dash!
                lengthAlongLine = lineLength * t;
            } else if (orientationType == 1) {
                // Horizontal.
                lengthAlongLine = abs(floor(gl_FragCoord.x + 0.5) - bottomLeft[0]);
            } else if (orientationType == 2) {
                // Vertical.
                lengthAlongLine = abs(floor(gl_FragCoord.y + 0.5) - bottomLeft[1]);
            }
            lengthAlongLine += dashOffset;
            float lengthAlongPeriodicDash = mod(lengthAlongLine, dashLength * 2.0);
            if (dashLength == 1.0) {
                // Attempt to bias so that we don't end up with so much whitespace from coincidentally sampling at an
                // unfortunately large percentage of pixels that aren't on a dash.
                if (lengthAlongPeriodicDash >= 1.0)
                    alpha = (2.0 - lengthAlongPeriodicDash) / 4.0;
            } else if (lengthAlongPeriodicDash < dashLength) {
                // Don't anti-alias dashes that are too short - it looks bad.
                if (orientationType == 0 && dashLength >= 3.0) {
                    if (lengthAlongPeriodicDash < 1.0) {
                        // Anti-alias towards the "rear" of the dash.
                        alpha = lengthAlongPeriodicDash;
                    } else if (dashLength - lengthAlongPeriodicDash < 1.0) {
                        // Anti-alias towards the "front" of the dash.
                        alpha = dashLength - lengthAlongPeriodicDash;
                    }
                }
            } else
                discard;
        }

        // Crop to strokeThickness thickness, as the quad we're shading is thicker than that. For lines with stroke thickness == 1.0,
        // we need to add a bit on to prevent a "patchy" look due to, with bad pairs of line angle, sampling points that "miss" more dots than
        // we would like.
        float strokeThickness = v_thicknessAndDashData[1];
        float quadThickness = v_thicknessAndDashData[0];
        float adjustedStrokeThickess = (strokeThickness == 1.0 && orientationType == 0) ? 1.5 : strokeThickness;
        float distanceFromOutsideEdge;
        if (orientationType == 0) {
            distanceFromOutsideEdge = abs(s - 0.5) * quadThickness - adjustedStrokeThickess / 2.0;
        } else if (orientationType == 1) {
            // Horizontal.
            distanceFromOutsideEdge = abs(abs(floor(gl_FragCoord.y + 0.5) - bottomLeft[1]) - quadThickness / 2.0) - adjustedStrokeThickess / 2.0;
        } else if (orientationType == 2) {
            // Vertical.
            distanceFromOutsideEdge = abs(abs(floor(gl_FragCoord.x + 0.5) - bottomLeft[0]) - quadThickness / 2.0) - adjustedStrokeThickess / 2.0;
        }

        if (distanceFromOutsideEdge > 0.0)
            discard;
        else if (distanceFromOutsideEdge >= -1.0 && orientationType == 0) {
            // Anti-alias along the horizontal edges.
            alpha *= -distanceFromOutsideEdge;
        }
        vec4 resultColor = vec4(0.0, 0.0, 0.0, alpha);
    )
)

GLfloat* PlatformContextTyGL::drawBorderLineSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    glVertexAttribPointer(shaderArguments[BorderLineShader::aPosition], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    glVertexAttribPointer(shaderArguments[BorderLineShader::aThicknessAndDashData], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart + 4);
    glVertexAttribPointer(shaderArguments[BorderLineShader::aPackedData], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart + 8);
    return attributeStart + 12;
}

void PlatformContextTyGL::drawBorderLine(const FloatPoint& point1, const FloatPoint& point2, const Color& color, float strokeThickness, float dashLength, float dashOffset)
{
    ASSERT(dashOffset >= 0);

    // Form a quad that is basically the line from point1 to point2 extended outwards by a value slightly larger than the stroke thickness:
    // the shader will ensure that the rendered line is the right thickness.
    // Note that the "direction" of the quad is important (the dashes will be draw *along* the quad, not *across* it!) so we
    // have to split up into several cases dependent on the orientation of the line and the order of point1 and point2.
    const GLfloat quadThickness = strokeThickness + 5.0;
    FloatPoint untransformedQuadCoords[4];
    if (point1.x() == point2.x()) {
        if (point1.y() < point2.y()) {
            untransformedQuadCoords[0] = FloatPoint(point1.x() - quadThickness / 2, point1.y());
            untransformedQuadCoords[1] = FloatPoint(point1.x() + quadThickness / 2, point1.y());
            untransformedQuadCoords[2] = FloatPoint(point1.x() - quadThickness / 2, point2.y() - 1);
            untransformedQuadCoords[3] = FloatPoint(point1.x() + quadThickness / 2, point2.y() - 1);
        } else {
            untransformedQuadCoords[0] = FloatPoint(point2.x() - quadThickness / 2, point2.y() + 1);
            untransformedQuadCoords[1] = FloatPoint(point2.x() + quadThickness / 2, point2.y() + 1);
            untransformedQuadCoords[2] = FloatPoint(point2.x() - quadThickness / 2, point1.y());
            untransformedQuadCoords[3] = FloatPoint(point2.x() + quadThickness / 2, point1.y());
        }
    } else {
        if (point1.x() < point2.x()) {
            untransformedQuadCoords[0] = FloatPoint(point1.x(), point1.y() + quadThickness / 2);
            untransformedQuadCoords[1] = FloatPoint(point1.x(), point1.y() - quadThickness / 2);
            untransformedQuadCoords[2] = FloatPoint(point2.x() - 1, point1.y() + quadThickness / 2);
            untransformedQuadCoords[3] = FloatPoint(point2.x() - 1, point1.y() - quadThickness / 2);
        } else {
            untransformedQuadCoords[0] = FloatPoint(point2.x() + 1, point2.y() + quadThickness / 2);
            untransformedQuadCoords[1] = FloatPoint(point2.x() + 1, point2.y() - quadThickness / 2);
            untransformedQuadCoords[2] = FloatPoint(point1.x(), point2.y() + quadThickness / 2);
            untransformedQuadCoords[3] = FloatPoint(point1.x(), point2.y() - quadThickness / 2);
        }
    }

    ClippedTransformedQuad clippedTransformedQuad(untransformedQuadCoords, transform(), boundingClipRect());

    if (!clippedTransformedQuad.hasVisiblePixels())
        return;

    Coloring coloring(premultipliedARGBFromColor(color.rgb()));
    m_coloring = &coloring;

    beginAttributeAdding(&BorderLineShader::s_program, nullptr, drawBorderLineSetShaderArguments, clippedTransformedQuad.minX(), clippedTransformedQuad.minY(), clippedTransformedQuad.maxX(), clippedTransformedQuad.maxY());
    addAttributeAbsolutePosition();
    clippedTransformedQuad.addToPipeline(pipelineBuffer());

    // Attribute "thicknessAndDashData".
    addAttributeRepeated(quadThickness, strokeThickness, dashLength, dashOffset);

    const FloatPoint point1Transformed = transform().apply(FloatPoint(point1.x(), point1.y()));
    const FloatPoint point2Transformed = transform().apply(FloatPoint(point2.x(), point2.y()));
    int orientationType = 0;
    if (fabs(point1Transformed.y() - point2Transformed.y()) < 0.5)
        orientationType = 1;

    if (fabs(point1Transformed.x() - point2Transformed.x()) < 0.5)
        orientationType = 2;

    // Attribute "packedData".
    const FloatPoint quadBottomLeft(clippedTransformedQuad.transformedCoords(2).x(), clippedTransformedQuad.transformedCoords(2).y());
    const float lineLength = (point1Transformed - point2Transformed).diagonalLength();
    addAttributeRepeated(orientationType + 0.5, lineLength, quadBottomLeft.x(), quadBottomLeft.y());
    endAttributeAdding();

    resetColoring();
}

} // namespace WebCore
