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

#ifndef PathTyGL_h
#define PathTyGL_h

#include "AffineTransform.h"
#include "ArenaTyGL.h"
#include "ClipRectTyGL.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "TransformTyGL.h"
#include "TrapezoidListTyGL.h"
#include "WindRule.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
namespace TyGL {

class PathElement {
public:
    enum Type {
        MoveTo,
        LineTo,
        QuadCurveTo,
        CurveTo,
        ArcTo,
        CloseSubpath,
    };

    PathElement(Type type, FloatPoint end)
        : m_type(type)
        , m_end(end)
        , m_next(0)
    {
    }

    Type type() const { return m_type; }
    bool isMoveTo() const { return m_type == MoveTo; }
    const FloatPoint& end() const { return m_end; }
    PathElement* next() const { return m_next; }

    void setNext(PathElement* next) { m_next = next; }
    void setEnd(FloatPoint end) { m_end = end; }

private:

    Type m_type;
    FloatPoint m_end;
    PathElement* m_next;
};

class MoveToElement : public PathElement {
public:
    MoveToElement(FloatPoint end)
        : PathElement(MoveTo, end)
    {
    }
};

class LineToElement : public PathElement {
public:
    LineToElement(FloatPoint end)
        : PathElement(LineTo, end)
    {
    }
};

class QuadCurveToElement : public PathElement {
public:
    QuadCurveToElement(FloatPoint control, FloatPoint end)
        : PathElement(QuadCurveTo, end)
        , m_control(control)
    {
    }

    const FloatPoint& control() const { return m_control; }
    void setControl(FloatPoint control) { m_control = control; }

    static FloatPoint bezierCurve(float, const FloatPoint&, const FloatPoint&, const FloatPoint&);
    static FloatRect bezierBoundingBox(const FloatPoint&, const FloatPoint&, const FloatPoint&);

private:
    FloatPoint m_control;
};

class CurveToElement : public PathElement {
public:
    CurveToElement(FloatPoint control1, FloatPoint control2, FloatPoint end)
        : PathElement(CurveTo, end)
        , m_control1(control1)
        , m_control2(control2)
    {
    }

    const FloatPoint& control1() const { return m_control1; }
    void setControl1(FloatPoint control1) { m_control1 = control1; }
    const FloatPoint& control2() const { return m_control2; }
    void setControl2(FloatPoint control2) { m_control2 = control2; }

    static void splitCurve(FloatPoint*);
    static bool curveIsLineSegment(FloatPoint* p);
    static FloatPoint bezierCurve(float, const FloatPoint&, const FloatPoint&, const FloatPoint&, const FloatPoint&);
    static FloatRect bezierBoundingBox(const FloatPoint&, const FloatPoint&, const FloatPoint&, const FloatPoint&);

private:
    FloatPoint m_control1;
    FloatPoint m_control2;
};

class ArcToElement : public PathElement {
public:
    ArcToElement(FloatPoint center, FloatSize radius, float startAngle, float endAngle, bool anticlockwise)
        : PathElement(ArcTo, FloatPoint(center.x() + radius.width() * cos(endAngle), center.y() + radius.height() * sin(endAngle)))
        , m_center(center)
        , m_radius(radius)
        , m_startAngle(startAngle)
        , m_endAngle(endAngle)
        , m_anticlockwise(anticlockwise)
    {
        ASSERT(radius.width() >= 0 && radius.height() >= 0);

        TyGL::AffineTransform::Transform transform = { 1, 0, 0, 1, 0, 0 };
        m_transform = transform;
    }

    const FloatPoint center() const { return m_center; }
    const FloatSize radius() const { return m_radius; }
    const float startAngle() const { return m_startAngle; }
    const float endAngle() const { return m_endAngle; }
    const bool anticlockwise() const { return m_anticlockwise; }
    const TyGL::AffineTransform& transform() const { return m_transform; }
    void multiply(const TyGL::AffineTransform& transform) { m_transform *= transform; }

    TyGL::AffineTransform finalTransform() const
    {
        TyGL::AffineTransform arcToTransform(m_transform);
        TyGL::AffineTransform::Transform arcAxes =
            { { m_radius.width(), 0, 0, m_radius.height(), m_center.x(), m_center.y() } };
        arcToTransform *= arcAxes;
        return arcToTransform;
    }

private:
    FloatPoint m_center;
    FloatSize m_radius;
    float m_startAngle;
    float m_endAngle;
    bool m_anticlockwise;
    WebCore::TyGL::AffineTransform m_transform;
};

class CloseSubpathElement : public PathElement {
public:
    CloseSubpathElement(FloatPoint end)
        : PathElement(CloseSubpath, end)
    {
    }
};

class PathData {
public:

    PathData()
        : m_arena(Arena::create())
        , m_firstElement(0)
        , m_lastElement(0)
        , m_shapeFirstElement(0)
        , m_isEmpty(true)
    {
    }

    inline void moveTo(const FloatPoint&);
    inline void addLineTo(const FloatPoint&);
    inline void addQuadCurveTo(const FloatPoint&, const FloatPoint&);
    inline void addBezierCurveTo(const FloatPoint&, const FloatPoint&, const FloatPoint&);
    void addArcTo(const FloatPoint&, const FloatPoint&, float);
    inline void closeSubpath();

    void addArc(const FloatPoint&, const FloatSize&, float, float, bool);

    bool isEmpty() const { return m_isEmpty; }

    const FloatRect boundingBox() const;

    PathElement* firstElement() const { return m_firstElement; }
    PathElement* lastElement() const { return m_lastElement; }
    FloatPoint currentPoint() const { return FloatPoint(m_lastElement->end()); }

    PassOwnPtr<TrapezoidList> trapezoidList(const TyGL::ClipRect&, const TyGL::AffineTransform&, WindRule = RULE_NONZERO) const;

    void addRect(const FloatRect&);
    PathData* clone() const;

    void transform(const WebCore::AffineTransform&);

private:
    PathElement* alloc(int);

    void appendBoundingBox(const FloatPoint&);
    void appendBoundingBox(const FloatPoint&, const FloatPoint&);
    void appendBoundingBox(const FloatPoint&, const FloatPoint&, const FloatPoint&);

    OwnPtr<Arena> m_arena;
    PathElement* m_firstElement;
    PathElement* m_lastElement;
    PathElement* m_shapeFirstElement;

    bool m_isEmpty;
};

inline void PathData::moveTo(const FloatPoint& end)
{
    if (m_lastElement && m_lastElement->isMoveTo()) {
        m_lastElement->setEnd(end);
        return;
    }

    m_shapeFirstElement = new (alloc(sizeof(MoveToElement))) MoveToElement(end);
}

inline void PathData::addLineTo(const FloatPoint& end)
{
    if (!m_lastElement) {
        moveTo(end);
        return;
    }

    if (!m_lastElement->isMoveTo() && m_lastElement->end() == end)
        return;

    m_isEmpty = false;
    new (alloc(sizeof(LineToElement))) LineToElement(end);
}

inline void PathData::addQuadCurveTo(const FloatPoint& control, const FloatPoint& end)
{
    if (!m_lastElement)
        moveTo(control);

    m_isEmpty = false;
    new (alloc(sizeof(QuadCurveToElement))) QuadCurveToElement(control, end);
}

inline void PathData::addBezierCurveTo(const FloatPoint& control1, const FloatPoint& control2, const FloatPoint& end)
{
    if (!m_lastElement)
        moveTo(control1);

    m_isEmpty = false;
    new (alloc(sizeof(CurveToElement))) CurveToElement(control1, control2, end);
}

inline void PathData::closeSubpath()
{
    if (!m_shapeFirstElement)
        return;

    ASSERT(!m_isEmpty);
    m_shapeFirstElement = new (alloc(sizeof(CloseSubpathElement))) CloseSubpathElement(m_shapeFirstElement->end());
}

} // namespace TyGL
} // namespace WebCore

#endif // PathTyGL_h
