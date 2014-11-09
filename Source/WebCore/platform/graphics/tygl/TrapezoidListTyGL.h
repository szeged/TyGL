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

#ifndef TrapezoidListTyGL_h
#define TrapezoidListTyGL_h

#include "ArenaTyGL.h"
#include "IntRect.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {
namespace TyGL {

// This variable is used in ShaderPathTyGL and TrapezoidBuilderTyGL.
#define ANTIALIAS_LEVEL 16

class TrapezoidList {
public:
    enum ShapeType {
        ShapeTypeAny,
        ShapeTypeRect
    };

    static PassOwnPtr<TrapezoidList> create(size_t trapezoidCount)
    {
        return adoptPtr(new TrapezoidList(trapezoidCount));
    }

    struct Trapezoid {
        float bottomY;
        float bottomLeftX;
        float bottomRightX;
        float topY;
        float topLeftX;
        float topRightX;
    };

    size_t trapezoidCount() { return m_trapezoids.size(); }
    Trapezoid* trapezoids() { return m_trapezoids.data(); }
    ShapeType shapeType() { return m_shapeType; }
    const IntRect& boundingBox() const { return m_boundingBox; }

    void insertTrapezoid(Trapezoid&);
    void setShapeType(ShapeType value) { m_shapeType = value; }
    void setBoundingBox(int x, int y, int width, int height) {
        m_boundingBox.setX(x);
        m_boundingBox.setY(y);
        m_boundingBox.setWidth(width);
        m_boundingBox.setHeight(height);
    }

private:
    TrapezoidList(size_t trapezoidCount)
        : m_shapeType(ShapeTypeAny)
    {
        m_trapezoids.reserveCapacity(trapezoidCount);
    }

    ShapeType m_shapeType;
    IntRect m_boundingBox;
    Vector<Trapezoid> m_trapezoids;
};

} // namespace TyGL
} // namespace WebCore

#endif // TrapezoidListTyGL_h
