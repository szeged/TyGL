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
#include "Gradient.h"
#include "GraphicsContext.h"
#include "PlatformContextTyGL.h"

namespace WebCore {

void Gradient::platformDestroy()
{
}

static inline bool compareStops(const Gradient::ColorStop& a, const Gradient::ColorStop& b)
{
    return a.stop < b.stop;
}

const Vector<Gradient::ColorStop, 2>& Gradient::getStops() const
{
    if (m_stopsSorted)
        return m_stops;

    if (m_stops.size())
        std::stable_sort(m_stops.begin(), m_stops.end(), compareStops);
    m_stopsSorted = true;
    return m_stops;
}

void Gradient::fill(GraphicsContext* context, const FloatRect& rect)
{
    context->platformContext()->fillRect(rect, PlatformContextTyGL::Coloring(RefPtr<Gradient>(this)));
}

}//namespace WebCore
