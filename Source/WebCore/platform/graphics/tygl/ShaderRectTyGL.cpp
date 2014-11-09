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

#include "PathTyGL.h"
#include "ShaderCommonTyGL.h"

namespace WebCore {

using TyGL::min;
using TyGL::max;

#define RECT_SHADER_ARGUMENTS(argument) \
    argument(a, rectPosition, RectPosition, 4)

DEFINE_SHADER(
    RectShader,
    RECT_SHADER_ARGUMENTS,
    PlatformContextTyGL::kAttributeSolidColoring | PlatformContextTyGL::kComplexFill,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_rectPosition;

        varying vec4 v_rectFractions;
        varying vec4 v_offsetAndSize;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        vec4 rectPosition = a_rectPosition;
        rectPosition[2] = rectPosition[0] + abs(rectPosition[2]);
        rectPosition[3] = rectPosition[1] + abs(rectPosition[3]);
        vec4 boundingBox = vec4(floor(rectPosition.xy), ceil(rectPosition.zw));
        vec2 a_position = vec2(0.0, 0.0);
        vec2 offset = vec2(0.0, 0.0);

        if (a_rectPosition[2] < 0.0) {
            a_position.x = boundingBox[2];
            offset.x = boundingBox[2] - boundingBox[0];
        } else
            a_position.x = boundingBox[0];

        if (a_rectPosition[3] < 0.0) {
            a_position.y = boundingBox[3];
            offset.y = boundingBox[3] - boundingBox[1];
        } else
            a_position.y = boundingBox[1];

        rectPosition = fract(rectPosition);
        v_rectFractions = rectPosition;
        v_offsetAndSize = vec4(offset, boundingBox.zw - boundingBox.xy - ceil(rectPosition.zw));
    ),
    // Fragment variables
    TyGL_PROGRAM(
        varying vec4 v_rectFractions;
        varying vec4 v_offsetAndSize;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        float x1 = v_offsetAndSize[0] < 1.0 ? v_rectFractions[0] : 0.0;
        float y1 = v_offsetAndSize[1] < 1.0 ? v_rectFractions[1] : 0.0;
        float x2 = v_offsetAndSize[0] > v_offsetAndSize[2] ? v_rectFractions[2] : 1.0;
        float y2 = v_offsetAndSize[1] > v_offsetAndSize[3] ? v_rectFractions[3] : 1.0;

        vec4 resultColor = vec4(0.0, 0.0, 0.0, (x2 - x1) * (y2 - y1));
    )
)

GLfloat* PlatformContextTyGL::rectSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    glVertexAttribPointer(shaderArguments[RectShader::aRectPosition], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 4;
}

void PlatformContextTyGL::fillAbsoluteRect(GLfloat left, GLfloat bottom, GLfloat right, GLfloat top)
{
    beginAttributeAdding(&RectShader::s_program, 0, rectSetShaderArguments, floor(left), floor(bottom), ceil(right), ceil(top));
    GLfloat width = right - left;
    GLfloat height = top - bottom;
    addAttribute(left, bottom, width, height,
        left, bottom, -width, height,
        left, bottom, width, -height,
        left, bottom, -width, -height);
    endAttributeAdding();
}

inline bool PlatformContextTyGL::fillRectFastPath(const FloatRect& rect)
{
    if (transform().type() != TyGL::AffineTransform::Move)
        return false;

    GLfloat left = rect.x() + transform().xTranslation();
    GLfloat bottom = rect.y() + transform().yTranslation();
    GLfloat right = left + rect.width();
    GLfloat top = bottom + rect.height();

    left = max(left, boundingClipRect().left());
    bottom = max(bottom, boundingClipRect().bottom());
    right = min(right, boundingClipRect().right());
    top = min(top, boundingClipRect().top());
    if (left >= right || bottom >= top)
        return true;

    fillAbsoluteRect(left, bottom, right, top);
    return true;
}

void PlatformContextTyGL::fillRect(const FloatRect& rect, Coloring coloring)
{
    m_coloring = &coloring;

    if (!fillRectFastPath(rect)) {
        TyGL::PathData path;
        path.addRect(rect);
        fillPath(&path);
    }

    resetColoring();
}

void PlatformContextTyGL::premultiplyUnmultipliedImageData(NativeImageTyGL* image)
{
    image->bindFbo();

    GLfloat position[16];
    SET_POSITION16(position, 0, 0, image->width(), image->height(),
        image->width(), 0, 0, image->height(),
        0, image->height(), image->width(), 0,
        image->width(), image->height(), 0, 0);

    static GLuint shaderArguments[10];

    const ShaderProgram* shaderSource[1] = { &RectShader::s_program };

    if (!compileAndUseShader(shaderArguments, 1, shaderSource))
        return;

    int offset = shaderArguments[1];

    glVertexAttribPointer(shaderArguments[RectShader::aRectPosition + offset], 4, GL_FLOAT, GL_FALSE, 0, position);
    glBlendFuncSeparate(GL_ZERO, GL_DST_ALPHA, GL_ZERO, GL_ONE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace WebCore
