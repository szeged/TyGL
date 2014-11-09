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
#include "ClipRectTyGL.h"

#include "TyGLDefs.h"

namespace WebCore {
namespace TyGL {

ClipRect::ClipRect()
{
    m_intPoints[0] = 0;
    m_intPoints[1] = 0;
    m_intPoints[2] = 0;
    m_intPoints[3] = 0;

    m_floatPoints[0] = 0;
    m_floatPoints[1] = 0;
    m_floatPoints[2] = 0;
    m_floatPoints[3] = 0;
}

ClipRect::ClipRect(int left, int bottom, int right, int top)
{
    ASSERT(left <= right && bottom <= top);

    m_intPoints[0] = left;
    m_intPoints[1] = bottom;
    m_intPoints[2] = right;
    m_intPoints[3] = top;

    m_floatPoints[0] = left;
    m_floatPoints[1] = bottom;
    m_floatPoints[2] = right;
    m_floatPoints[3] = top;
}

void ClipRect::intersect(int left, int bottom, int right, int top)
{
    ASSERT(left <= right && bottom <= top);

    m_intPoints[0] = clamp(left, m_intPoints[0], m_intPoints[2]);
    m_intPoints[1] = clamp(bottom, m_intPoints[1], m_intPoints[3]);
    m_intPoints[2] = clamp(right, m_intPoints[0], m_intPoints[2]);
    m_intPoints[3] = clamp(top, m_intPoints[1], m_intPoints[3]);

    m_floatPoints[0] = m_intPoints[0];
    m_floatPoints[1] = m_intPoints[1];
    m_floatPoints[2] = m_intPoints[2];
    m_floatPoints[3] = m_intPoints[3];
}

ClipRect& ClipRect::operator=(const IntRect& rect)
{
    m_intPoints[0] = rect.x();
    m_intPoints[1] = rect.y();
    m_intPoints[2] = rect.maxX();
    m_intPoints[3] = rect.maxY();

    m_floatPoints[0] = m_intPoints[0];
    m_floatPoints[1] = m_intPoints[1];
    m_floatPoints[2] = m_intPoints[2];
    m_floatPoints[3] = m_intPoints[3];
}

void ClipRect::setDimension(int width, int height)
{
    ASSERT(width >= 0 && height >= 0);

    m_intPoints[0] = 0;
    m_intPoints[1] = 0;
    m_intPoints[2] = width;
    m_intPoints[3] = height;

    m_floatPoints[0] = 0;
    m_floatPoints[1] = 0;
    m_floatPoints[2] = width;
    m_floatPoints[3] = height;
}

} // namespace TyGL
} // namespace WebCore
