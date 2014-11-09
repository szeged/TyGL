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

#ifndef TransformTyGL_h
#define TransformTyGL_h

#include "FloatPoint.h"
#include "FloatRect.h"
#include "TyGLDefs.h"

namespace WebCore {
namespace TyGL {

class AffineTransform {
public:
    struct Transform {
        // Bottom row of the 3x3 matrix is always [ 0 0 1 ].
        float m_matrix[6];
    };

    enum Type {
        Move,
        Affine,
    };

    AffineTransform()
    {
        m_type = Move;
        m_transform = kIdentity;
    }

    AffineTransform(Transform transform)
    {
        m_transform = transform;
        autoDetectType();
    }

    Type type() const { return m_type; }
    const Transform& transform() const { return m_transform; }

    bool isIdentity() const
    {
        return m_type == Move && m_transform.m_matrix[4] == 0 && m_transform.m_matrix[5] == 0;
    }

    static FloatRect outboundRect(const FloatPoint* coords)
    {
        float left = min(coords[0].x(), coords[1].x());
        left = min(left, coords[2].x());
        left = floor(min(left, coords[3].x()));
        float right = max(coords[0].x(), coords[1].x());
        right = max(right, coords[2].x());
        right = ceil(max(right, coords[3].x()));
        float bottom = min(coords[0].y(), coords[1].y());
        bottom = min(bottom, coords[2].y());
        bottom = floor(min(bottom, coords[3].y()));
        float top = max(coords[0].y(), coords[1].y());
        top = max(top, coords[2].y());
        top = ceil(max(top, coords[3].y()));
        return FloatRect(left, bottom, right - left, top - bottom);
    }

    float xTranslation() const { return m_transform.m_matrix[4]; };
    float yTranslation() const { return m_transform.m_matrix[5]; };

    FloatPoint apply(const FloatPoint& location) const
    {
        if (m_type == Move)
            return FloatPoint(location.x() + m_transform.m_matrix[4], location.y() + m_transform.m_matrix[5]);

        float x = location.x() * m_transform.m_matrix[0] + location.y() * m_transform.m_matrix[2] + m_transform.m_matrix[4];
        float y = location.x() * m_transform.m_matrix[1] + location.y() * m_transform.m_matrix[3] + m_transform.m_matrix[5];
        return FloatPoint(x, y);
    }

    FloatRect apply(const FloatRect& rectangle, FloatPoint* coords, bool returnOutboundRect = true) const
    {
        float left;
        float bottom;
        float right;
        float top;

        if (m_type == Move) {
            left = rectangle.x() + m_transform.m_matrix[4];
            bottom = rectangle.y() + m_transform.m_matrix[5];
            right = rectangle.maxX() + m_transform.m_matrix[4];
            top = rectangle.maxY() + m_transform.m_matrix[5];

            coords[0].set(left, bottom);
            coords[1].set(right, bottom);
            coords[2].set(left, top);
            coords[3].set(right, top);

            if (!returnOutboundRect)
                return FloatRect();

            left = floor(left);
            bottom = floor(bottom);
            right = ceil(right);
            top = ceil(top);
            return FloatRect(left, bottom, right - left, top - bottom);
        }

        coords[0] = apply(rectangle.location());
        FloatPoint point(rectangle.maxX(), rectangle.y());
        coords[1] = apply(point);
        point.setY(rectangle.maxY());
        coords[3] = apply(point);
        point.setX(rectangle.x());
        coords[2] = apply(point);

        if (!returnOutboundRect)
            return FloatRect();

        return outboundRect(coords);
    }

    void operator= (const AffineTransform& transform)
    {
        m_type = transform.m_type;
        m_transform = transform.m_transform;
    }

    void operator= (Transform transform)
    {
        m_transform = transform;
        autoDetectType();
    }

    void operator*= (const AffineTransform& transform) { multiply(transform.m_transform); }
    void operator*= (const Transform& transform) { multiply(transform); }
    void multiply(const Transform& transform);

    void translate(float, float);
    void scale(float, float);
    void rotate(float);
    const Transform& transform() { return m_transform; }

    AffineTransform inverse() const;

private:
    static const Transform kIdentity;

    inline void inlineAutoDetectType();
    void autoDetectType();

    // Helps optimizing certain operations..
    Type m_type;
    // Optimized for SIMD instruction sets.
    Transform m_transform;
};

} // namespace TyGL
} // namespace WebCore

#endif // TransformTyGL_h
