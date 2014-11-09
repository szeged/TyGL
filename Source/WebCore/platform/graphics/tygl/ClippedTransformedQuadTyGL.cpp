/*
 * Copyright (C) 2014 Samsung Electronics Ltd. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformContextTyGL.h"

namespace WebCore {

PlatformContextTyGL::ClippedTransformedQuad::ClippedTransformedQuad(const FloatPoint preTransformCoords[4], const TyGL::AffineTransform& transform, const TyGL::ClipRect& boundingClipRect)
{
    for (int i = 0; i < 4; i++)
        m_transformedCoords[i] =  transform.apply(preTransformCoords[i]);

    compute(transform, boundingClipRect);
}

PlatformContextTyGL::ClippedTransformedQuad::ClippedTransformedQuad(const FloatRect& preTransformRect, const TyGL::AffineTransform& transform, const TyGL::ClipRect& boundingClipRect)
{
    // 0 = "top-left"; 1 = "top-right"; 2 = "bottom-left"; 3 = "bottom-right".
    m_transformedCoords[0] = transform.apply(preTransformRect.location());
    FloatPoint point(preTransformRect.x() + preTransformRect.width(), preTransformRect.y());
    m_transformedCoords[1] = transform.apply(point);
    point.setY(preTransformRect.y() + preTransformRect.height());
    m_transformedCoords[3] = transform.apply(point);
    point.setX(preTransformRect.x());
    m_transformedCoords[2] = transform.apply(point);

    compute(transform, boundingClipRect);
}

void PlatformContextTyGL::ClippedTransformedQuad::addToPipeline(PipelineBuffer* pipelineBuffer)
{
    const GLfloat adjustedMinX = m_minX - m_bottomLeftTransformed.x();
    const GLfloat adjustedMinY = m_minY - m_bottomLeftTransformed.y();
    const GLfloat adjustedMaxX = m_maxX - m_bottomLeftTransformed.x();
    const GLfloat adjustedMaxY = m_maxY - m_bottomLeftTransformed.y();
    pipelineBuffer->addAttribute(adjustedMinY * m_v1X - adjustedMinX * m_v1Y, adjustedMinY * m_v0X - adjustedMinX * m_v0Y,
        adjustedMinY * m_v1X - adjustedMaxX * m_v1Y, adjustedMinY * m_v0X - adjustedMaxX * m_v0Y,
        adjustedMaxY * m_v1X - adjustedMinX * m_v1Y, adjustedMaxY * m_v0X - adjustedMinX * m_v0Y,
        adjustedMaxY * m_v1X - adjustedMaxX * m_v1Y, adjustedMaxY * m_v0X - adjustedMaxX * m_v0Y);
}

void PlatformContextTyGL::ClippedTransformedQuad::compute(const TyGL::AffineTransform& transform, const TyGL::ClipRect& boundingClipRect)
{
    // Arrange so that the relative positions in the transformed quad are accessible in the shader via a_position.zw,
    // (so (0,0) is the "bottom left"; (1,1) the "top right", etc), and arrange further that these accessed values are
    // outside the range 0..1 if they are outside the boundingClipRect() - this way, we do not need to pass the boundingClipRect()
    // into the shader and have it clipped there. Since we do not use glScissor with the batching queue, this is very useful.

    m_hasVisiblePixels = true;

    m_bottomLeftTransformed = m_transformedCoords[0];

    m_v0X = m_transformedCoords[1].x() - m_transformedCoords[0].x();
    m_v0Y = m_transformedCoords[1].y() - m_transformedCoords[0].y();
    m_v1X = m_transformedCoords[2].x() - m_transformedCoords[0].x();
    m_v1Y = m_transformedCoords[2].y() - m_transformedCoords[0].y();

    const GLfloat discriminant = m_v1X * m_v0Y - m_v1Y * m_v0X;
    // Parallel or zero length vectors.
    if (!discriminant) {
        m_hasVisiblePixels = false;
        return;
    }

    m_minX = std::min(m_transformedCoords[0].x(), m_transformedCoords[1].x());
    m_minX = std::min(m_minX, m_transformedCoords[2].x());
    m_minX = std::floor(std::min(m_minX, m_transformedCoords[3].x()));
    m_maxX = std::max(m_transformedCoords[0].x(), m_transformedCoords[1].x());
    m_maxX = std::max(m_maxX, m_transformedCoords[2].x());
    m_maxX = std::ceil(std::max(m_maxX, m_transformedCoords[3].x()));

    m_minY = std::min(m_transformedCoords[0].y(), m_transformedCoords[1].y());
    m_minY = std::min(m_minY, m_transformedCoords[2].y());
    m_minY = std::floor(std::min(m_minY, m_transformedCoords[3].y()));
    m_maxY = std::max(m_transformedCoords[0].y(), m_transformedCoords[1].y());
    m_maxY = std::max(m_maxY, m_transformedCoords[2].y());
    m_maxY = std::ceil(std::max(m_maxY, m_transformedCoords[3].y()));

    m_minX = std::max(m_minX, boundingClipRect.left());
    m_maxX = std::min(m_maxX, boundingClipRect.right());
    m_minY = std::max(m_minY, boundingClipRect.bottom());
    m_maxY = std::min(m_maxY, boundingClipRect.top());

    if (m_minX >= m_maxX || m_minY >= m_maxY) {
        m_hasVisiblePixels = false;
        return;
    }

    m_v1X /= discriminant;
    m_v1Y /= discriminant;
    m_v0X /= -discriminant;
    m_v0Y /= -discriminant;
    }

} // namespace WebCore
