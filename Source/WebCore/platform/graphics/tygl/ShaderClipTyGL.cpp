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
#include "TrapezoidListTyGL.h"

namespace WebCore {

#define CLIP_SHADER_ARGUMENTS(argument) \
    argument(u, clipMaskImage, ClipMaskImage, 1) \
    argument(a, clipPosition, ClipPosition, 2)

DEFINE_SHADER(
    ClipShader,
    CLIP_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec2 a_clipPosition;

        varying vec2 v_clipPosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_clipPosition = a_clipPosition;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_clipMaskImage;
        varying vec2 v_clipPosition;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        resultColor *= texture2D(u_clipMaskImage, v_clipPosition).a;
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_clipShaderProgram = &ClipShader::s_program;

void PlatformContextTyGL::clipSetShaderAttributes()
{
    GLfloat clipWidth = m_state.boundingClipRect.right() - m_state.boundingClipRect.left();
    GLfloat clipHeight = m_state.boundingClipRect.top() - m_state.boundingClipRect.bottom();
    IntRect& paintRect = pipelineBuffer()->paintRect;
    GLfloat left = (paintRect.x() - m_state.boundingClipRect.leftInteger()) / clipWidth;
    GLfloat bottom = (paintRect.y() - m_state.boundingClipRect.bottomInteger()) / clipHeight;
    GLfloat right = (paintRect.maxX() - m_state.boundingClipRect.leftInteger()) / clipWidth;
    GLfloat top = (paintRect.maxY() - m_state.boundingClipRect.bottomInteger()) / clipHeight;
    addAttributePosition(left, bottom, right, top);
}

void PlatformContextTyGL::clip(const FloatRect& rect)
{
    if (transform().type() == TyGL::AffineTransform::Move) {
        FloatRect transformedRect(rect);
        transformedRect.setLocation(m_state.transform.apply(rect.location()));
        m_state.boundingClipRect.intersect(enclosingIntRect(transformedRect));
        if(isInteger(transformedRect.x()) && isInteger(transformedRect.y())
            && isInteger(transformedRect.width()) && isInteger(transformedRect.height()))
            return;
    }

    TyGL::PathData path;
    path.addRect(rect);
    clip(&path, WindRule::RULE_NONZERO);
}

void PlatformContextTyGL::clip(const TyGL::PathData* path, WindRule windRule)
{
    OwnPtr<TyGL::TrapezoidList> trapezoidList = path->trapezoidList(m_state.boundingClipRect, transform(), windRule);
    if (!trapezoidList->boundingBox().width() || !trapezoidList->boundingBox().height())
        return;

    if (!m_state.clipMaskImage) {
        m_state.boundingClipRect = trapezoidList->boundingBox();

        // FIXME This should be optimized when the clipping mask could be replaced with the boundingClipRect.
        m_state.clipMaskImage = NativeImageTyGL::create(trapezoidList->boundingBox().size());
        fillPathAlphaTexture(trapezoidList.get(), m_state.clipMaskImage);
        return;
    }

    RefPtr<NativeImageTyGL> clipMaskImage = NativeImageTyGL::create(trapezoidList->boundingBox().size());

    fillPathAlphaTexture(trapezoidList.get(), clipMaskImage);

    GLfloat x1, x2, y1, y2;
    GLfloat width = m_state.boundingClipRect.right() - m_state.boundingClipRect.left();
    GLfloat height = m_state.boundingClipRect.top() - m_state.boundingClipRect.bottom();
    GLfloat offsetX = trapezoidList->boundingBox().x() - m_state.boundingClipRect.left();
    GLfloat offsetY = trapezoidList->boundingBox().y() - m_state.boundingClipRect.bottom();
    x1 = offsetX / width;
    x2 = (offsetX + trapezoidList->boundingBox().width()) / width;
    y1 = offsetY / height;
    y2 = (offsetY + trapezoidList->boundingBox().height()) / height;
    GLfloat position[16];

    width = clipMaskImage->width();
    height = clipMaskImage->height();
    SET_POSITION16(position, 0, 0, x1, y1, width, 0, x2, y1, 0, height, x1, y2, width, height, x2, y2);

    clipMaskImage->bindFbo();
    glBlendFunc(GL_ZERO, GL_SRC_ALPHA);

    paintTexture(position, m_state.clipMaskImage);

    m_state.boundingClipRect = trapezoidList->boundingBox();
    m_state.clipMaskImage = clipMaskImage;
}

void PlatformContextTyGL::clipOut(const FloatRect& rect)
{
    TyGL::PathData path;
    path.addRect(rect);
    clipOut(&path);
}

void PlatformContextTyGL::clipOut(const TyGL::PathData* path)
{
    OwnPtr<TyGL::TrapezoidList> trapezoidList = path->trapezoidList(m_state.boundingClipRect, transform());
    if (!trapezoidList->boundingBox().width() || !trapezoidList->boundingBox().height())
        return;

    if (!m_state.clipMaskImage) {
        m_state.clipMaskImage = NativeImageTyGL::create(m_state.boundingClipRect.size());
        m_state.clipMaskImage->bindFbo();
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 0.0);
    }

    RefPtr<NativeImageTyGL> pathImage = NativeImageTyGL::create(trapezoidList->boundingBox().size());
    fillPathAlphaTexture(trapezoidList.get(), pathImage);

    GLfloat position[16];

    SET_POSITION16(position,
            trapezoidList->boundingBox().x(), trapezoidList->boundingBox().y(), 0, 0,
            trapezoidList->boundingBox().maxX(), trapezoidList->boundingBox().y(), 1, 0,
            trapezoidList->boundingBox().x(), trapezoidList->boundingBox().maxY(), 0, 1,
            trapezoidList->boundingBox().maxX(), trapezoidList->boundingBox().maxY(), 1, 1);

    m_state.clipMaskImage->bindFbo();

    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    paintTexture(position, pathImage);
}

GLfloat* PlatformContextTyGL::clipSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setUniformTexture(shaderArguments[ClipShader::uClipMaskImage], context->pipelineBuffer()->clipMaskImage);
    glVertexAttribPointer(shaderArguments[ClipShader::aClipPosition], 2, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 2;
}

void PlatformContextTyGL::clearRect(const FloatRect& rect)
{
    CompositeOperator op = m_state.compositeOperator;
    BlendMode blendMode = m_state.blendMode;

    setCompositeOperation(CompositeDestinationOut, BlendModeNormal);
    fillRect(rect, Color(255, 255, 255, 255));
    setCompositeOperation(op, blendMode);
}

} // namespace WebCore
