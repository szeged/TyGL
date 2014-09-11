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
#include "Path.h"

#include "AffineTransform.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "NotImplemented.h"
#include "PathTyGL.h"
#include "TransformTyGL.h"
#include "TrapezoidBuilderTyGL.h"
#include "TrapezoidListTyGL.h"
#include "WindRule.h"
#include "wtf/MathExtras.h"

namespace WebCore {
namespace TyGL {

PathElement* PathData::alloc(int size)
{
    PathElement* next = reinterpret_cast<PathElement*>(m_arena->alloc(size));

    if (!m_firstElement)
        m_firstElement = next;

    if (m_lastElement)
        m_lastElement->setNext(next);
    m_lastElement = next;

    return next;
}

void PathData::addArc(const FloatPoint& center, const FloatSize& radius, float startAngle, float endAngle, bool anticlockwise)
{
    FloatPoint start = FloatPoint(center.x() + radius.width() * cos(startAngle), center.y() + radius.height() * sin(startAngle));
    if(!m_lastElement)
        moveTo(start);

    if (!radius.width() || !radius.height() || startAngle == endAngle) {
        addLineTo(start);
        return;
    }

    if (m_lastElement->end() != start)
        addLineTo(start);

    const float twoPiFloat = 2 * piFloat;
    if (anticlockwise && startAngle - endAngle >= twoPiFloat) {
        startAngle = fmod(startAngle, twoPiFloat);
        endAngle = startAngle - twoPiFloat;
    } else if (!anticlockwise && endAngle - startAngle >= twoPiFloat) {
        startAngle = fmod(startAngle, twoPiFloat);
        endAngle = startAngle + twoPiFloat;
    } else {
        bool equal = startAngle == endAngle;

        startAngle = fmod(startAngle, twoPiFloat);
        if (startAngle < 0)
            startAngle += twoPiFloat;

        endAngle = fmod(endAngle, twoPiFloat);
        if (endAngle < 0)
            endAngle += twoPiFloat;

        if (!anticlockwise) {
            if (startAngle > endAngle || (startAngle == endAngle && !equal))
                endAngle += twoPiFloat;
            ASSERT(0 <= startAngle && startAngle <= twoPiFloat);
            ASSERT(startAngle <= endAngle && endAngle - startAngle <= twoPiFloat);
        } else {
            if (startAngle < endAngle || (startAngle == endAngle && !equal))
                endAngle -= twoPiFloat;
            ASSERT(0 <= startAngle && startAngle <= twoPiFloat);
            ASSERT(endAngle <= startAngle && startAngle - endAngle <= twoPiFloat);
        }
    }

    m_isEmpty = false;
    new (alloc(sizeof(ArcToElement))) ArcToElement(center, radius, startAngle, endAngle, anticlockwise);
}

void PathData::addArcTo(const FloatPoint& control, const FloatPoint& end, float radius)
{
    if (!m_lastElement) {
        moveTo(control);
        return;
    }

    if (m_lastElement->end() == control || control == end || !radius)
        addLineTo(control);

    FloatPoint start(m_lastElement->end());

    FloatPoint delta1(start - control);
    FloatPoint delta2(end - control);
    float delta1Length = sqrtf(delta1.lengthSquared());
    float delta2Length = sqrtf(delta2.lengthSquared());

    // Normalized dot product.
    double cosPhi = delta1.dot(delta2) / (delta1Length * delta2Length);

    // All three points are on the same straight line (HTML5, 4.8.11.1.8).
    if (abs(cosPhi) >= 0.9999) {
        addLineTo(control);
        return;
    }

    float tangent = radius / tan(acos(cosPhi) / 2);
    float delta1Factor = tangent / delta1Length;
    FloatPoint arcStartPoint((control.x() + delta1Factor * delta1.x()), (control.y() + delta1Factor * delta1.y()));

    FloatPoint orthoStartPoint(delta1.y(), -delta1.x());
    float orthoStartPointLength = sqrt(orthoStartPoint.lengthSquared());
    float radiusFactor = radius / orthoStartPointLength;

    double cosAlpha = (orthoStartPoint.x() * delta2.x() + orthoStartPoint.y() * delta2.y()) / (orthoStartPointLength * delta2Length);
    if (cosAlpha < 0.f)
        orthoStartPoint = FloatPoint(-orthoStartPoint.x(), -orthoStartPoint.y());

    FloatPoint center((arcStartPoint.x() + radiusFactor * orthoStartPoint.x()), (arcStartPoint.y() + radiusFactor * orthoStartPoint.y()));

    // Calculate angles for addArc.
    orthoStartPoint = FloatPoint(-orthoStartPoint.x(), -orthoStartPoint.y());
    float startAngle = acos(orthoStartPoint.x() / orthoStartPointLength);
    if (orthoStartPoint.y() < 0.f)
        startAngle = 2 * piFloat - startAngle;

    bool anticlockwise = false;

    float delta2Factor = tangent / delta2Length;
    FloatPoint arcEndPoint((control.x() + delta2Factor * delta2.x()), (control.y() + delta2Factor * delta2.y()));
    FloatPoint orthoEndPoint(arcEndPoint - center);
    float orthoEndPointLength = sqrtf(orthoEndPoint.lengthSquared());
    float endAngle = acos(orthoEndPoint.x() / orthoEndPointLength);
    if (orthoEndPoint.y() < 0)
        endAngle = 2 * piFloat - endAngle;
    if ((startAngle > endAngle) && ((startAngle - endAngle) < piFloat))
        anticlockwise = true;
    if ((startAngle < endAngle) && ((endAngle - startAngle) > piFloat))
        anticlockwise = true;

    addArc(center, FloatSize(radius, radius), startAngle, endAngle, anticlockwise);
}

static inline float abs(const float &t)
{
    return (t >= 0) ? t : -t;
}

static inline float root(float a, float b, float sqrtD)
{
    return (-b + sqrtD) / (2 * a);
}

static inline void calculateRoots(float* t, float a, float b, float c)
{
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        return;

    if (!a) {
        t[0] = -c / b;
        return;
    }

    if (!discriminant) {
        t[0] = root(a, b, 0);
        return;
    }

    t[0] = root(a, b, sqrt(discriminant));
    t[1] = root(a, b, -sqrt(discriminant));
}

bool CurveToElement::curveIsLineSegment(FloatPoint* p)
{
    float tolerance = 1.0;
    float x0 = p[0].x();
    float y0 = p[0].y();
    float x1 = p[1].x();
    float y1 = p[1].y();
    float x2 = p[2].x();
    float y2 = p[2].y();
    float x3 = p[3].x();
    float y3 = p[3].y();

    float dt1 = abs((x3 - x0) * (y0 - y1) - (x0 - x1) * (y3 - y0));
    float dt2 = abs((x3 - x0) * (y0 - y2) - (x0 - x2) * (y3 - y0));

    if (dt1 > tolerance || dt2 > tolerance)
        return false;

    float minX, minY, maxX, maxY;

    if (x0 < x3) {
        minX = x0 - tolerance;
        maxX = x3 + tolerance;
    } else {
        minX = x3 - tolerance;
        maxX = x0 + tolerance;
    }

    if (y0 < y3) {
        minY = y0 - tolerance;
        maxY = y3 + tolerance;
    } else {
        minY = y3 - tolerance;
        maxY = y0 + tolerance;
    }

    return !(x1 < minX || x1 > maxX || y1 < minY || y1 > maxY
        || x2 < minX || x2 > maxX || y2 < minY || y2 > maxY);
}

void CurveToElement::splitCurve(FloatPoint* p)
{
    float a, b, c, d;
    float ab, bc, cd;
    float abbc, bccd;
    float mid;

    a = p[0].x();
    b = p[1].x();
    c = p[2].x();
    d = p[3].x();
    ab = (a + b) / 2.0;
    bc = (b + c) / 2.0;
    cd = (c + d) / 2.0;
    abbc = (ab + bc) / 2.0;
    bccd = (bc + cd) / 2.0;
    mid = (abbc + bccd) / 2.0;

    p[0].setX(a);
    p[1].setX(ab);
    p[2].setX(abbc);
    p[3].setX(mid);
    p[4].setX(bccd);
    p[5].setX(cd);
    p[6].setX(d);

    a = p[0].y();
    b = p[1].y();
    c = p[2].y();
    d = p[3].y();
    ab = (a + b) / 2.0;
    bc = (b + c) / 2.0;
    cd = (c + d) / 2.0;
    abbc = (ab + bc) / 2.0;
    bccd = (bc + cd) / 2.0;
    mid = (abbc + bccd) / 2.0;

    p[0].setY(a);
    p[1].setY(ab);
    p[2].setY(abbc);
    p[3].setY(mid);
    p[4].setY(bccd);
    p[5].setY(cd);
    p[6].setY(d);
}

FloatPoint QuadCurveToElement::bezierCurve(float t, const FloatPoint& startPoint, const FloatPoint& controlPoint, const FloatPoint& endPoint)
{
    if (t > 1.0)
        t = 1.0;

    float x = startPoint.x() * (1.0 - t) * (1.0 - t)
                + 2.0 * controlPoint.x() * (1.0 - t) * t
                + endPoint.x() * t * t;
    float y = startPoint.y() * (1.0 - t) * (1.0 - t)
                + 2.0 * controlPoint.y() * (1.0 - t) * t
                + endPoint.y() * t * t;

    return FloatPoint(x, y);
}


FloatRect QuadCurveToElement::bezierBoundingBox(const FloatPoint& startPoint, const FloatPoint& controlPoint, const FloatPoint& endPoint)
{
    FloatRect boundingBox(startPoint, FloatSize());

    float t = (startPoint.x() - controlPoint.x()) / (startPoint.x() - 2 * controlPoint.x() + endPoint.x());

    if (t >= 0 && t <= 1)
        boundingBox.extend(bezierCurve(t, startPoint, controlPoint, endPoint));

    t = (startPoint.y() - controlPoint.y()) / (startPoint.y() - 2 * controlPoint.y() + endPoint.y());

    if (t >= 0 && t <= 1)
        boundingBox.extend(bezierCurve(t, startPoint, controlPoint, endPoint));

    boundingBox.extend(endPoint);

    return boundingBox;
}

FloatPoint CurveToElement::bezierCurve(float t, const FloatPoint& startPoint, const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& endPoint)
{
    if (t > 1.0)
        t = 1.0;

    float x = startPoint.x() * (1.0 - t) * (1.0 - t)  * (1.0 - t)
                + 3.0 * controlPoint1.x() * (1.0 - t) * (1.0 - t) * t
                + 3.0 * controlPoint2.x() * (1.0 - t) * t * t
                + endPoint.x() * t * t * t;
    float y = startPoint.y() * (1.0 - t) * (1.0 - t)  * (1.0 - t)
                + 3.0 * controlPoint1.y() * (1.0 - t) * (1.0 - t)  * t
                + 3.0 * controlPoint2.y() * (1.0 - t) * t * t
                + endPoint.y() * t * t * t;

    return FloatPoint(x, y);
}

FloatRect CurveToElement::bezierBoundingBox(const FloatPoint& startPoint, const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& endPoint)
{
    FloatRect boundingBox(startPoint, FloatSize());

    float t[4] = {-1, -1, -1, -1};

    // The x roots.
    float a = -startPoint.x() + 3 * controlPoint1.x() - 3 * controlPoint2.x() + endPoint.x();
    float b = 2 * startPoint.x() - 4 * controlPoint1.x() + 2 * controlPoint2.x();
    float c = controlPoint1.x() - startPoint.x();

    calculateRoots(t, a, b, c);

    // The y roots.
    a = -startPoint.y() + 3 * controlPoint1.y() - 3 * controlPoint2.y() + endPoint.y();
    b = 2 * startPoint.y() - 4 * controlPoint1.y() + 2 * controlPoint2.y();
    c = controlPoint1.y() - startPoint.y();

    calculateRoots(t + 2, a, b, c);

    for (int i = 0; i < 4; i++) {
        if (t[i] >= 0 && t[i] <= 1)
            boundingBox.extend(bezierCurve(t[i], startPoint, controlPoint1, controlPoint2, endPoint));
    }

    boundingBox.extend(endPoint);

    return boundingBox;
}

const FloatRect PathData::boundingBox() const
{
    if (!firstElement())
        return FloatRect();

    FloatRect boundingBox(firstElement()->end(), FloatSize());

    PathElement* previousElement = firstElement();
    PathElement* currentElement = previousElement->next();
    while (currentElement) {
        switch (currentElement->type()) {
        case PathElement::MoveTo:
            if (!currentElement->next() || (currentElement->next()->type() == PathElement::CloseSubpath))
                break;
            // Fall through.
        case PathElement::LineTo:
            boundingBox.extend(currentElement->end());
            break;
        case PathElement::QuadCurveTo: {
            QuadCurveToElement* quadToElement = reinterpret_cast<QuadCurveToElement*>(currentElement);
            boundingBox.unite(QuadCurveToElement::bezierBoundingBox(previousElement->end(), quadToElement->control(), quadToElement->end()));
            break;
        }
        case PathElement::CurveTo: {
            CurveToElement* curveToElement = reinterpret_cast<CurveToElement*>(currentElement);
            boundingBox.unite(CurveToElement::bezierBoundingBox(previousElement->end(), curveToElement->control1(), curveToElement->control2(), curveToElement->end()));
            break;
        }
        case PathElement::ArcTo: {
            ArcToElement* arcToElement = reinterpret_cast<ArcToElement*>(currentElement);

            float startAngle = arcToElement->startAngle();
            float endAngle = arcToElement->endAngle();
            bool anticlockwise = arcToElement->anticlockwise();

            TyGL::AffineTransform transform(arcToElement->finalTransform());

            float deltaAngle = anticlockwise ? startAngle - endAngle : endAngle - startAngle;

            int segments = TrapezoidBuilder::calculateSegments(deltaAngle, TrapezoidBuilder::boundingCircleRadius(*arcToElement));
            float step = deltaAngle / segments;

            if (anticlockwise)
                step = -step;

            FloatPoint startPoint = transform.apply(FloatPoint(cos(startAngle), sin(startAngle)));
            FloatPoint bezierPoints[3];
            for (int i = 0; i < segments; i++, startAngle += step) {
                TrapezoidBuilder::arcToCurve(bezierPoints, startAngle,
                    i == segments - 1 ? endAngle : startAngle + step);
                bezierPoints[0] = transform.apply(bezierPoints[0]);
                bezierPoints[1] = transform.apply(bezierPoints[1]);
                bezierPoints[2] = transform.apply(bezierPoints[2]);
                boundingBox.unite(CurveToElement::bezierBoundingBox(startPoint, bezierPoints[0], bezierPoints[1], bezierPoints[2]));
                startPoint = bezierPoints[2];
            }
            boundingBox.extend(startPoint);
            boundingBox.extend(arcToElement->end());
            break;
        }
        case PathElement::CloseSubpath:
            break;
        }

        previousElement = currentElement;
        currentElement = currentElement->next();
    }

    return boundingBox;
}

PassOwnPtr<TrapezoidList> PathData::trapezoidList(const TyGL::ClipRect& clipRect, const TyGL::AffineTransform& transform, WindRule fillRule) const
{
    TrapezoidBuilder builder(this, clipRect, transform, fillRule);
    return builder.trapezoidList();
}

void PathData::addRect(const FloatRect& rect)
{
    moveTo(rect.location());
    FloatPoint point(rect.x() + rect.width(), rect.y());
    addLineTo(point);
    point.setY(rect.y() + rect.height());
    addLineTo(point);
    point.setX(rect.x());
    addLineTo(point);
    closeSubpath();
}

PathData* PathData::clone() const
{
    PathElement* element = firstElement();
    PathData* newPath = new PathData();

    QuadCurveToElement* quadToElement;
    CurveToElement* curveToElement;
    ArcToElement* arcToElement;

    while (element) {
        switch (element->type()) {
        case PathElement::MoveTo:
            newPath->moveTo(element->end());
            break;
        case PathElement::CloseSubpath:
            newPath->closeSubpath();
            break;
        case PathElement::LineTo:
            newPath->addLineTo(element->end());
            break;
        case PathElement::QuadCurveTo:
            quadToElement = reinterpret_cast<QuadCurveToElement*>(element);
            newPath->addQuadCurveTo(quadToElement->control(), quadToElement->end());
            break;
        case PathElement::CurveTo:
            curveToElement = reinterpret_cast<CurveToElement*>(element);
            newPath->addBezierCurveTo(curveToElement->control1(), curveToElement->control2(), curveToElement->end());
            break;
        case PathElement::ArcTo:
            arcToElement = reinterpret_cast<ArcToElement*>(element);
            newPath->addArc(arcToElement->center(), FloatSize(arcToElement->radius().width(), arcToElement->radius().height()),
                arcToElement->startAngle(), arcToElement->endAngle(), arcToElement->anticlockwise());
            break;
        }

        element = element->next();
    }
    return newPath;
}

void PathData::transform(const WebCore::AffineTransform &webCoreTransform)
{
    PathElement* element = firstElement();
    TyGL::AffineTransform::Transform matrix = {
        webCoreTransform.a(), webCoreTransform.b(), webCoreTransform.c(),
        webCoreTransform.d(), webCoreTransform.e(), webCoreTransform.f()
    };
    TyGL::AffineTransform transform(matrix);

    QuadCurveToElement* quadToElement;
    CurveToElement* curveToElement;
    ArcToElement* arcToElement;

    while (element) {
        switch (element->type()) {
        case PathElement::MoveTo:
        case PathElement::CloseSubpath:
        case PathElement::LineTo:
            element->setEnd(transform.apply(element->end()));
            break;
        case PathElement::QuadCurveTo:
            quadToElement = reinterpret_cast<QuadCurveToElement*>(element);
            quadToElement->setEnd(transform.apply(quadToElement->end()));
            quadToElement->setControl(transform.apply(quadToElement->control()));
            break;
        case PathElement::CurveTo:
            curveToElement = reinterpret_cast<CurveToElement*>(element);
            curveToElement->setEnd(transform.apply(curveToElement->end()));
            curveToElement->setControl1(transform.apply(curveToElement->control1()));
            curveToElement->setControl2(transform.apply(curveToElement->control2()));
            break;
        case PathElement::ArcTo:
            arcToElement = reinterpret_cast<ArcToElement*>(element);
            arcToElement->setEnd(transform.apply(arcToElement->end()));
            arcToElement->multiply(transform);
            break;
        }

        element = element->next();
    }
}

} // namespace TyGL

Path::Path()
    : m_path(new TyGL::PathData())
{
}

Path::Path(const Path& other)
{
    m_path = other.m_path->clone();
}

Path::~Path()
{
    delete m_path;
}

Path& Path::operator=(const Path& other)
{
    delete m_path;
    m_path = other.m_path->clone();

    return *this;
}

bool Path::isEmpty() const
{
    return m_path->isEmpty();
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    notImplemented();
    return false;
}

bool Path::hasCurrentPoint() const
{
    return m_path->lastElement();
}

FloatPoint Path::currentPoint() const
{
    return m_path->currentPoint();
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    notImplemented();
    return false;
}

FloatRect Path::boundingRect() const
{
    return m_path->boundingBox();
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    notImplemented();
    return FloatRect();
}

void Path::moveTo(const FloatPoint& point)
{
    m_path->moveTo(point);
}

void Path::addRect(const FloatRect& rect)
{
    m_path->addRect(rect);
}

void Path::addLineTo(const FloatPoint& point)
{
    m_path->addLineTo(point);
}

void Path::addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& point)
{
    m_path->addQuadCurveTo(controlPoint, point);
}

void Path::addBezierCurveTo(const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& ep)
{
    m_path->addBezierCurveTo(p1, p2, ep);
}

void Path::addEllipse(const FloatRect& rect)
{
    FloatSize radius(0.5 * rect.width(), 0.5 * rect.height());
    FloatPoint center(rect.x() + radius.width(), rect.y() + radius.height());

    m_path->addArc(center, radius, 0, 2 * piFloat, false);
}

void Path::addArc(const FloatPoint& p, float r, float sa, float ea, bool anticlockwise)
{
    m_path->addArc(p, FloatSize(r, r), sa, ea, anticlockwise);
}

void Path::addArcTo(const FloatPoint& control, const FloatPoint& end, float radius)
{
    m_path->addArcTo(control, end, radius);
}

void Path::apply(void* info, PathApplierFunction function) const
{
    notImplemented();
}

void Path::closeSubpath()
{
    m_path->closeSubpath();
}

void Path::clear()
{
    delete m_path;
    m_path = new TyGL::PathData();
}

void Path::transform(const AffineTransform& xform)
{
    m_path->transform(xform);
}

void Path::translate(const FloatSize& p)
{
    m_path->transform(AffineTransform::translation(p.width(), p.height()));
}

void Path::addPath(const Path&, const AffineTransform&)
{
    notImplemented();
}

} // namespace WebCore
