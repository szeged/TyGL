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

#include "FloatPoint.h"
#include "FloatSize.h"
#include "GraphicsTypes.h"
#include "PathTyGL.h"
#include "TrapezoidBuilderTyGL.h"
#include <math.h>
#include <memory>

namespace WebCore {
namespace TyGL {

class StrokePathBuilder {
public:
    StrokePathBuilder(float width, float miterLimit, LineJoin joinMode, LineCap lineCap)
        : m_hasShapeFirstLine(false)
        , m_halfWidth(width / 2.0)
        , m_miterLimitSquared(miterLimit * miterLimit)
        , m_joinMode(joinMode)
        , m_lineCap(lineCap)
        , m_path(std::make_unique<PathData>())
    {
        m_shapeFirstLine = &m_lines[0];
        m_lastLine = &m_lines[1];
        m_currentLine = &m_lines[2];
        m_lastLine->next = m_currentLine;
        m_currentLine->next = m_lastLine;
    }

    inline void addMoveToShape();
    inline void addCloseSubpathShape(const FloatPoint&, const TyGL::CloseSubpathElement*);
    inline void addLineShape(const FloatPoint&, const TyGL::LineToElement*);
    inline void addBezierCurveShape(const FloatPoint&, const TyGL::QuadCurveToElement*);
    inline void addBezierCurveShape(const FloatPoint&, const TyGL::CurveToElement*);
    inline void addArcShape(const FloatPoint&, const TyGL::ArcToElement*);

    PathData* path() { return m_path.get(); }

    void addCapShapeIfNeeded()
    {
        if (m_hasShapeFirstLine)
            addCapShape(m_lineCap);
    }

private:
    struct LineAttributes {
        enum Direction {
            Same,
            Reverse,
            Positive,
            Negative,
        };

        LineAttributes() : next(0) { }

        void set(const FloatPoint&, const FloatPoint&, float);
        Direction vectorCompare(const LineAttributes*) const;

        LineAttributes* next;

        FloatPoint location;
        FloatSize vector;
        FloatSize unit;
        float length;

        FloatSize thicknessOffsets;
        FloatPoint startTop, startBottom;
        FloatPoint endTop, endBottom;
    };

    inline FloatSize miterLength(const FloatSize&, const FloatSize&);

    inline void addTriangle(const FloatPoint&, const FloatPoint&, const FloatPoint&);
    inline void addTriangle(const FloatPoint&, const FloatPoint&, const FloatPoint&, float);
    inline void addQuadShape(const FloatPoint&, const FloatPoint&, const FloatPoint&, const FloatPoint&);
    inline void addRoundShape(const FloatPoint&, const FloatPoint&, const FloatPoint&, const FloatPoint&);
    void addJoinShape(const LineAttributes*, const LineAttributes*);
    void addCapShape(LineCap, bool = false);
    void bezierCurveShape(const FloatPoint&, const FloatPoint&, const FloatPoint&, const FloatPoint&);

    void setCurrentLineAttribute()
    {
        m_lastLine = m_lastLine->next;
        m_currentLine = m_currentLine->next;
    }

    LineAttributes m_lines[3];
    LineAttributes* m_shapeFirstLine;
    LineAttributes* m_lastLine;
    LineAttributes* m_currentLine;
    bool m_hasShapeFirstLine;

    float m_halfWidth;
    float m_miterLimitSquared;
    LineJoin m_joinMode;
    LineCap m_lineCap;

    std::unique_ptr<PathData> m_path;
};

void StrokePathBuilder::LineAttributes::set(const FloatPoint& from, const FloatPoint& to, float halfWidth)
{
    location = from;
    vector = to - from;

    if (vector.isZero())
        return;

    length = vector.diagonalLength();
    unit = vector * (1.0 / length);

    thicknessOffsets.setWidth(halfWidth * unit.width());
    thicknessOffsets.setHeight(halfWidth * unit.height());

    startTop.set(from.x() + thicknessOffsets.height(), from.y() - thicknessOffsets.width());
    startBottom.set(from.x() - thicknessOffsets.height(), from.y() + thicknessOffsets.width());
    endBottom.set(to.x() - thicknessOffsets.height(), to.y() + thicknessOffsets.width());
    endTop.set(to.x() + thicknessOffsets.height(), to.y() - thicknessOffsets.width());
}

static inline float dotProduct(const FloatSize& u1, const FloatSize& u2)
{
    return u1.width() * u2.width() + u1.height() * u2.height();
}

static inline float crossProduct(const FloatSize& v1, const FloatSize v2)
{
    return v1.width() * v2.height() - v1.height() * v2.width();
}

static inline FloatSize normal(const FloatSize& v)
{
    return FloatSize(v.height(), -v.width());
}

static inline FloatSize normalSize(const FloatSize& size, float halfWidth)
{
    FloatSize normalVector = normal(size);
    float lenght = normalVector.diagonalLength();

    if (!lenght)
        return FloatSize(0, 0);

    return normalVector * (halfWidth / lenght);
}

StrokePathBuilder::LineAttributes::Direction StrokePathBuilder::LineAttributes::vectorCompare(const LineAttributes* line) const
{
    float direction = crossProduct(unit, line->unit);

    if (direction > 0)
        return Direction::Positive;
    if (direction < 0)
        return Direction::Negative;

    if (!(unit.height() + line->unit.height()))
        return Direction::Reverse;

    return Direction::Same;
}

inline void StrokePathBuilder::addTriangle(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2, float crossProduct)
{
    if (!crossProduct)
        return;

    m_path->moveTo(p0);
    if (crossProduct > 0) {
        m_path->addLineTo(p1);
        m_path->addLineTo(p2);
    } else {
        m_path->addLineTo(p2);
        m_path->addLineTo(p1);
    }
    m_path->closeSubpath();
}

inline void StrokePathBuilder::addTriangle(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2)
{
    float crossProduct = (p1.x() - p0.x()) * (p2.y() - p0.y()) - (p1.y() - p0.y()) * (p2.x() - p0.x());

    addTriangle(p0, p1, p2, -crossProduct);
}

inline void StrokePathBuilder::addQuadShape(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3)
{
    FloatSize p1p0 = p1 - p0;
    FloatSize p2p0 = p2 - p0;
    FloatSize p3p0 = p3 - p0;

    // Handle the most frequent case first: no intersection.
    float p2p0Xp1p0 = crossProduct(p2p0, p1p0);
    float p2p0Xp3p0 = crossProduct(p2p0, p3p0);

    if (p2p0Xp1p0 * p2p0Xp3p0 < 0) {
        addTriangle(p0, p1, p2, p2p0Xp1p0);
        addTriangle(p0, p3, p2, p2p0Xp3p0);
        return;
    }

    FloatSize p3p1 = p3 - p1;
    FloatSize p2p1 = p2 - p1;
    float p3p1Xp0p1 = crossProduct(p3p1, -p1p0);
    float p3p1Xp2p1 = crossProduct(p3p1, p2p1);

    if (p3p1Xp0p1 * p3p1Xp2p1 < 0) {
        addTriangle(p1, p0, p3, p3p1Xp0p1);
        addTriangle(p1, p2, p3, p3p1Xp2p1);
        return;
    }

    // Intersection computation:
    //  intersectionPoint = p0 + k1 * p1p0
    //  intersectionPoint = p2 + k2 * p3p2
    //    where k1, k2 are constant values
    //
    // k1 and k2 (one of them is enough) can be computed from:
    //  p0.x + k1 * p1p0.x = p2.x + k2 * p3p2.x
    //  p0.y + k1 * p1p0.y = p2.y + k2 * p3p2.y

    float p1p0Xp2p0 = -p2p0Xp1p0;
    float p1p0Xp3p0 = crossProduct(p1p0, p3p0);

    if (p1p0Xp2p0 * p1p0Xp3p0 < 0) {
        FloatPoint intersection;
        if (p3.y() - p2.y()) {
            float m = (p3.x() - p2.x()) / (p3.y() - p2.y());
            float k1 = (m * (p2.y() - p0.y()) - p2.x() + p0.x()) / (m * (p1.y() - p0.y()) - p1.x() + p0.x());
            intersection.set(p0.x() + k1 * p1p0.width(), p0.y() + k1 * p1p0.height());
        } else if (p0.y() - p1.y()) {
            float m = (p0.x() - p1.x()) / (p0.y() - p1.y());
            float k2 = (m * (p1.y() - p3.y()) - p1.x() + p3.x()) / (- p2.x() + p3.x());
            intersection.set(p3.x() + k2 * (p2.x() - p3.x()), p3.y() + k2 * (p2.y() - p3.y()));
        }
        addTriangle(intersection, p2, p1, p1p0Xp2p0);
        addTriangle(p0, p3, intersection, p1p0Xp3p0);
        return;
    }

    FloatPoint intersection;
    if (p2.y() - p1.y()) {
        float m = (p2.x() - p1.x()) / (p2.y() - p1.y());
        float k1 = (m * (p1.y() - p0.y()) - p1.x() + p0.x()) / (m * (p3.y() - p0.y()) - p3.x() + p0.x());
        intersection.set(p0.x() + k1 * p3p0.width(), p0.y() + k1 * p3p0.height());
    } else if (p3.y() - p0.y()) {
        float m = (p2.x() - p1.x()) / (p3.y() - p0.y());
        float k2 = (m * (p0.y() - p1.y()) - p0.x() + p1.x()) / (- p2.x() + p1.x());
        intersection.set(p1.x() + k2 * (p2p1.width()), p1.y() + k2 * (p2p1.height()));
    }
    addTriangle(p0, p1, intersection, crossProduct(p3p0, p1p0));
    addTriangle(intersection, p2, p3, -p2p0Xp3p0);
}

inline void StrokePathBuilder::addRoundShape(const FloatPoint& location, const FloatPoint& from, const FloatPoint& miter, const FloatPoint& to)
{
    if (from == to || location == from || location == to)
        return;

    float crossProduct = (from.x() - location.x()) * (to.y() - location.y()) - (from.y() - location.y()) * (to.x() - location.x());

    m_path->moveTo(location);
    if (crossProduct < 0) {
        m_path->addLineTo(from);
        m_path->addArcTo(miter, to, m_halfWidth);
    } else {
        m_path->addLineTo(to);
        m_path->addArcTo(miter, from, m_halfWidth);
    }
    m_path->closeSubpath();
}

inline FloatSize StrokePathBuilder::miterLength(const FloatSize& u1, const FloatSize& u2)
{
    FloatSize miterDirection(u1 + u2);
    float directionLength = miterDirection.diagonalLength();

    ASSERT(directionLength > 0 && directionLength < 2);

    // Calculating the miterLength:
    // miterLength = halfWidth / sin(phi / 2),
    // 2 * (sin(phi / 2) ^ 2) = 1 - cos(phi),
    // dotProduct = cos(phi) = u1 * u2. Then u1 and u2 are unit vectors.
    float length = m_halfWidth / sqrtf((1 - dotProduct(u1, u2)) * 0.5);

    return miterDirection * (length / directionLength);
}

void StrokePathBuilder::addJoinShape(const LineAttributes* fromLine, const LineAttributes* toLine)
{
    if (!fromLine->length || !toLine->length)
        return;

    LineAttributes::Direction direction = fromLine->vectorCompare(toLine);
    if (direction == LineAttributes::Direction::Same)
        return;

    if (m_joinMode == RoundJoin) {
        FloatSize unitFrom(fromLine->unit);
        FloatSize unitTo(-toLine->unit);

        if (direction == LineAttributes::Direction::Reverse) {
            addCapShape(LineCap::RoundCap, true);
            return;
        }

        FloatPoint miterPoint(toLine->location + miterLength(unitFrom, unitTo));
        if (direction == LineAttributes::Direction::Negative) {
            addRoundShape(toLine->location, fromLine->endBottom, miterPoint, toLine->startBottom);
            return;
        }

        addRoundShape(toLine->location, toLine->startTop, miterPoint, fromLine->endTop);
        return;
    }

    if (direction == LineAttributes::Direction::Reverse)
        return;

    if (m_joinMode == MiterJoin) {
        FloatSize unitFrom(fromLine->unit);
        FloatSize unitTo(-toLine->unit);

        // Check the miter-limit: miterLimit * miterLimit * (1 - cos(Phi)) >= 2.
        // Where dotProduct = cos(Phi) = unitFrom * unitTo.
        if (m_miterLimitSquared * (1 - dotProduct(unitFrom, unitTo)) >= 2) {
            FloatPoint miterPoint =  toLine->location + miterLength(unitFrom, unitTo);

            if (direction == LineAttributes::Direction::Negative) {
                addQuadShape(toLine->location, fromLine->endBottom, miterPoint, toLine->startBottom);
                return;
            }

            addQuadShape(toLine->location, toLine->startTop, miterPoint, fromLine->endTop);
            return;
        }
    }

    // The BevelJoin mode.
    if (direction == LineAttributes::Direction::Negative) {
        addTriangle(toLine->location, fromLine->endBottom, toLine->startBottom);
        return;
    }

    addTriangle(toLine->location, fromLine->endTop, toLine->startTop);
}

void StrokePathBuilder::addCapShape(LineCap lineCap, bool intermediateCap)
{
    if (lineCap == LineCap::ButtCap)
        return;

    FloatPoint startTopMargin(m_shapeFirstLine->startTop.x() - m_shapeFirstLine->thicknessOffsets.width(), m_shapeFirstLine->startTop.y() - m_shapeFirstLine->thicknessOffsets.height());
    FloatPoint startBottomMargin(m_shapeFirstLine->startBottom.x() - m_shapeFirstLine->thicknessOffsets.width(), m_shapeFirstLine->startBottom.y() - m_shapeFirstLine->thicknessOffsets.height());
    FloatPoint endTopMargin(m_lastLine->endTop.x() + m_lastLine->thicknessOffsets.width(), m_lastLine->endTop.y() + m_lastLine->thicknessOffsets.height());
    FloatPoint endBottomMargin(m_lastLine->endBottom.x() + m_lastLine->thicknessOffsets.width(), m_lastLine->endBottom.y() + m_lastLine->thicknessOffsets.height());

    if (lineCap == LineCap::SquareCap) {
        if (m_shapeFirstLine->length)
            addQuadShape(m_shapeFirstLine->startBottom, startBottomMargin, startTopMargin, m_shapeFirstLine->startTop);
        if (m_lastLine->length)
            addQuadShape(m_lastLine->endTop, endTopMargin, endBottomMargin, m_lastLine->endBottom);
        return;
    }

    ASSERT(lineCap == LineCap::RoundCap);

    FloatPoint miter;

    if (!intermediateCap || m_shapeFirstLine->length) {
        miter = startTopMargin + startBottomMargin;
        miter.scale(0.5, 0.5);
        addRoundShape(m_shapeFirstLine->location, m_shapeFirstLine->startBottom, startBottomMargin, miter);
        addRoundShape(m_shapeFirstLine->location, miter, startTopMargin, m_shapeFirstLine->startTop);
    }

    if (m_lastLine->length) {
        miter = endTopMargin + endBottomMargin;
        miter.scale(0.5, 0.5);
        addRoundShape(m_lastLine->location + m_lastLine->vector, m_lastLine->endTop, endTopMargin, miter);
        addRoundShape(m_lastLine->location + m_lastLine->vector, miter, endBottomMargin, m_lastLine->endBottom);
    }
}

inline void StrokePathBuilder::addMoveToShape()
{
    addCapShapeIfNeeded();
    m_shapeFirstLine->set(FloatPoint(0, 0), FloatPoint(-1, 0), m_halfWidth);
    m_currentLine->set(FloatPoint(0, 0), FloatPoint(1, 0), m_halfWidth);
    m_lastLine->set(FloatPoint(0, 0), FloatPoint(1, 0), m_halfWidth);
    m_hasShapeFirstLine = false;
}

inline void StrokePathBuilder::addLineShape(const FloatPoint& start, const TyGL::LineToElement* element)
{
    if (start == element->end())
        return;

    m_currentLine->set(start, element->end(), m_halfWidth);
    if (!m_hasShapeFirstLine) {
        m_shapeFirstLine->set(start, element->end(), m_halfWidth);
        m_hasShapeFirstLine = true;
    } else
        addJoinShape(m_lastLine, m_currentLine);

    addQuadShape(m_currentLine->startTop, m_currentLine->startBottom, m_currentLine->endBottom, m_currentLine->endTop);

    setCurrentLineAttribute();
}

inline void StrokePathBuilder::addCloseSubpathShape(const FloatPoint& start, const TyGL::CloseSubpathElement* element)
{
    if (!m_hasShapeFirstLine || start == element->end())
        return;

    m_currentLine->set(start, element->end(), m_halfWidth);
    addJoinShape(m_lastLine, m_currentLine);
    addJoinShape(m_currentLine, m_shapeFirstLine);

    addQuadShape(m_currentLine->startTop, m_currentLine->startBottom, m_currentLine->endBottom, m_currentLine->endTop);

    m_hasShapeFirstLine = false;
}

static inline float abs(const float &t)
{
    return (t >= 0) ? t : -t;
}

static inline bool collinear(FloatPoint& p0, FloatPoint& p1, FloatPoint& p2)
{
    return abs((p2.x() - p0.x()) * (p0.y() - p1.y()) - (p0.x() - p1.x()) * (p2.y() - p0.y())) <= TrapezoidBuilder::kTolerance;
}

static inline bool curveIsLineSegment(FloatPoint& p0, FloatPoint& p1, FloatPoint& p2)
{
    if (!collinear(p0, p1, p2))
        return false;

    float minX, minY, maxX, maxY;

    if (p0.x() < p2.x()) {
        minX = p0.x() - TrapezoidBuilder::kTolerance;
        maxX = p2.x() + TrapezoidBuilder::kTolerance;
    } else {
        minX = p2.x() - TrapezoidBuilder::kTolerance;
        maxX = p0.x() + TrapezoidBuilder::kTolerance;
    }

    if (p0.y() < p2.y()) {
        minY = p0.y() - TrapezoidBuilder::kTolerance;
        maxY = p2.y() + TrapezoidBuilder::kTolerance;
    } else {
        minY = p2.y() - TrapezoidBuilder::kTolerance;
        maxY = p0.y() + TrapezoidBuilder::kTolerance;
    }

    return !(p1.x() < minX || p1.x() > maxX || p1.y() < minY || p1.y() > maxY);
}

void StrokePathBuilder::bezierCurveShape(const FloatPoint& start, const FloatPoint& control1, const FloatPoint& control2, const FloatPoint& end)
{
    // De Casteljau algorithm.
    const int max = 16 * 3 + 1;
    FloatPoint buffer[max];
    FloatPoint* points = buffer;

    FloatSize startNormal, halfNormal, endNormal;
    FloatPoint involuteStart, involuteHalf, involuteEnd;
    FloatPoint evoluteStart, evoluteHalf, evoluteEnd;

    points[0] = start;
    points[1] = control1;
    points[2] = control2;
    points[3] = end;

    do {
        CurveToElement::splitCurve(points);

        if (points[0] != points[1])
            startNormal = normalSize(points[1] - points[0], m_halfWidth);
        else if (points[0] != points[2])
            startNormal = normalSize(points[2] - points[0], m_halfWidth);
        else
            startNormal = normalSize(points[3] - points[0], m_halfWidth);

        if (points[3] != points[4])
            halfNormal = normalSize(points[4] - points[3], m_halfWidth);
        else if (points[3] != points[5])
            halfNormal = normalSize(points[5] - points[3], m_halfWidth);
        else
            halfNormal = normalSize(points[6] - points[3], m_halfWidth);

        if (points[6] != points[5])
            endNormal = normalSize(points[6] - points[5], m_halfWidth);
        else if (points[6] != points[4])
            endNormal = normalSize(points[6] - points[4], m_halfWidth);
        else
            endNormal = normalSize(points[6] - points[3], m_halfWidth);

        involuteStart = points[0] - startNormal;
        involuteHalf = points[3] - halfNormal;
        involuteEnd = points[6] - endNormal;

        if ((points[2] == points[3]) && (points[3] == points[4])) {
            FloatSize halfWidth = normalSize(points[3] - points[1], m_halfWidth);
            FloatPoint miter = points[3] + normal(-halfWidth);
            addRoundShape(points[3], points[3] + halfWidth, miter + halfWidth, miter);
            addRoundShape(points[3], miter, miter - halfWidth, points[3] - halfWidth);
        } else if (((points[6] - points[0]).diagonalLengthSquared() < TrapezoidBuilder::kTolerance)
            && ((points[5] - points[0]).diagonalLengthSquared() < TrapezoidBuilder::kTolerance)
            && ((points[1] - points[0]).diagonalLengthSquared() < TrapezoidBuilder::kTolerance)) {
            FloatPoint miter = points[0] + normal(-startNormal);
            addRoundShape(points[0], points[0] + startNormal, miter + startNormal, miter);
            addRoundShape(points[0], miter, miter - startNormal, points[0] - startNormal);
            points -= 3;
            continue;
        }

        if (curveIsLineSegment(involuteStart, involuteHalf, involuteEnd)) {
            evoluteStart = points[0] + startNormal;
            evoluteHalf = points[3] + halfNormal;
            evoluteEnd = points[6] + endNormal;

            if(curveIsLineSegment(evoluteStart, evoluteHalf, evoluteEnd)) {
                addQuadShape(evoluteStart, involuteStart, involuteHalf, evoluteHalf);
                addQuadShape(evoluteHalf, involuteHalf, involuteEnd, evoluteEnd);
                points -= 3;
                continue;
            }
        }

        points += 3;

        if (points >= buffer + max - 7) {
            // This recursive code path is rarely executed.
            bezierCurveShape(points[0], points[1], points[2], points[3]);
            points -= 3;
        }
    } while (points >= buffer);
}

inline void StrokePathBuilder::addBezierCurveShape(const FloatPoint& start, const TyGL::QuadCurveToElement* element)
{
    static const float twoThird = static_cast<float>(2) / 3;
    FloatPoint control1(twoThird * (element->control().x() - start.x()) + start.x(), twoThird * (element->control().y() - start.y()) + start.y());
    FloatPoint control2(twoThird * (element->control().x() - element->end().x()) + element->end().x(), twoThird * (element->control().y() - element->end().y()) + element->end().y());
    TyGL::CurveToElement cubicCurve(control1, control2, element->end());
    addBezierCurveShape(start, &cubicCurve);
}

inline void StrokePathBuilder::addBezierCurveShape(const FloatPoint& start, const TyGL::CurveToElement* element)
{
    FloatPoint control1 = element->control1();
    FloatPoint control2 = element->control2();
    FloatPoint end = element->end();

    FloatPoint to = control1;
    FloatPoint from = control2;
    if (control1 == start) {
        if (control1 == control2) {
            if (control1 == end)
                return;
            to = end;
        } else
            to = control2;
    }
    if (control2 == end)
        if (control2 == control1)
            from = start;
        else
            from = control1;
    // Set the continuous vector to join or cap shape method.
    m_currentLine->set(start, to, m_halfWidth);

    if (!m_hasShapeFirstLine) {
        m_shapeFirstLine->set(start, to, m_halfWidth);
        m_hasShapeFirstLine = true;
    } else
        addJoinShape(m_lastLine, m_currentLine);

    // Set the continuous vector to join or cap shape method.
    m_currentLine->set(from, end, m_halfWidth);
    setCurrentLineAttribute();

    // TODO Empty curve: ASSERT()
    bezierCurveShape(start, control1, control2, end);
}

inline void StrokePathBuilder::addArcShape(const FloatPoint& start, const TyGL::ArcToElement* element)
{
    float direction = element->anticlockwise() ? 1.0 : -1.0;
    FloatPoint end(element->end());

    // Set the continuous vector to join or cap shape method.
    m_currentLine->set(start, start + direction * FloatSize(sin(element->startAngle()), -cos(element->startAngle())), m_halfWidth);

    if (!m_hasShapeFirstLine) {
        m_shapeFirstLine->set(start, start + direction * FloatSize(sin(element->startAngle()), -cos(element->startAngle())), m_halfWidth);
        m_hasShapeFirstLine = true;
    } else
        addJoinShape(m_lastLine, m_currentLine);

    // Set the continuous vector to join or cap shape method.
    m_currentLine->set(end - direction * FloatSize(sin(element->endAngle()), -cos(element->endAngle())), end, m_halfWidth);

    FloatSize firstRadius = element->radius() + direction *  FloatSize(m_halfWidth, m_halfWidth);
    FloatSize secondRadius = element->radius() - direction * FloatSize(m_halfWidth, m_halfWidth);

    FloatPoint startPoint(element->center().x() + firstRadius.width() * cos(element->startAngle()), element->center().y() + firstRadius.height() * sin(element->startAngle()));
    FloatPoint endPoint(element->center().x() + secondRadius.width() * cos(element->endAngle()), element->center().y() + secondRadius.height() * sin(element->endAngle()));
    m_path->moveTo(startPoint);
    m_path->addArc(element->center(), firstRadius, element->startAngle(), element->endAngle(), element->anticlockwise());
    m_path->addLineTo(endPoint);
    m_path->addArc(element->center(), secondRadius, element->endAngle(), element->startAngle(), !element->anticlockwise());
    m_path->closeSubpath();

    setCurrentLineAttribute();
}

} // namesapce TyGL

void PlatformContextTyGL::strokePath(const TyGL::PathData* path)
{
    if (!path || path->isEmpty())
        return;

    float miterLimit = m_state.miterLimit ? m_state.miterLimit : 10;

    TyGL::StrokePathBuilder strokePath(m_state.strokeWidth, miterLimit, m_state.lineJoinMode, m_state.lineCapMode);
    TyGL::PathElement* element = path->firstElement();

    FloatPoint start;
    while (element) {
        FloatPoint end = element->end();

        switch (element->type()) {
        case TyGL::PathElement::MoveTo:
            strokePath.addMoveToShape();
            break;
        case TyGL::PathElement::CloseSubpath:
            strokePath.addCloseSubpathShape(start, reinterpret_cast<TyGL::CloseSubpathElement*>(element));
            break;
        case TyGL::PathElement::LineTo:
            strokePath.addLineShape(start, reinterpret_cast<TyGL::LineToElement*>(element));
            break;
        case TyGL::PathElement::QuadCurveTo:
            strokePath.addBezierCurveShape(start, reinterpret_cast<TyGL::QuadCurveToElement*>(element));
            break;
        case TyGL::PathElement::CurveTo:
            strokePath.addBezierCurveShape(start, reinterpret_cast<TyGL::CurveToElement*>(element));
            break;
        case TyGL::PathElement::ArcTo:
            strokePath.addArcShape(start, reinterpret_cast<TyGL::ArcToElement*>(element));
            break;
        }

        start = end;
        element = element->next();
    }

    strokePath.addCapShapeIfNeeded();

    fillPath(strokePath.path());
}

void PlatformContextTyGL::strokePath(const TyGL::PathData* path, Coloring coloring)
{
    m_coloring = &coloring;
    strokePath(path);
    resetColoring();
}

} // namespace WebCore
