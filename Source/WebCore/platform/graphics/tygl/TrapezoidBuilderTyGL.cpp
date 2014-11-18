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
#include "TrapezoidBuilderTyGL.h"

#include "FloatPoint.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "PathTyGL.h"
#include "TrapezoidListTyGL.h"
#include "WindRule.h"
#include "wtf/MathExtras.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
namespace TyGL {

TrapezoidBuilder::Line* TrapezoidBuilder::s_sentinelLine = 0;

TrapezoidBuilder::TrapezoidBuilder(const PathData* path, const TyGL::ClipRect& clipRect, const TyGL::AffineTransform& transform, WindRule fillRule)
    : m_arena(Arena::create())
    , m_path(path)
    , m_fillRule(fillRule)
    , m_transform(transform)
    , m_clipTop(clipRect.bottomInteger() * kAntiAliasing)
    , m_clipBottom(clipRect.topInteger() * kAntiAliasing)
    , m_clipLeft(clipRect.leftInteger() * kAntiAliasing)
    , m_clipRight(clipRect.rightInteger() * kAntiAliasing)
    , m_trapezoidCount(0)
{
    if (!s_sentinelLine) {
        s_sentinelLine = reinterpret_cast<Line*>(fastMalloc(sizeof(Line)));
        s_sentinelLine->u1.left = s_sentinelLine;
        s_sentinelLine->u2.right = s_sentinelLine;
        s_sentinelLine->u3.parent = 0;
        s_sentinelLine->flagsAndDirection = 0;
    }
    m_clipEverything = (m_clipTop >= m_clipBottom) || (m_clipLeft >= m_clipRight);
    m_rootLine = s_sentinelLine;
}

inline int32_t TrapezoidBuilder::computeX(float y, float x0, float y0, float slope)
{
    return roundf(((y - y0) * slope) + x0);
}

inline int32_t TrapezoidBuilder::computeY(int32_t x, float x0, float y0, float slope)
{
    return roundf(((x - x0) / slope) + y0);
}

inline int32_t TrapezoidBuilder::clampToRange(int32_t value, int32_t min, int32_t max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

void TrapezoidBuilder::rotateLeft(Line* line)
{
    Line* right = line->u2.right;

    line->u2.right = right->u1.left;
    if (right->u1.left != s_sentinelLine)
        right->u1.left->u3.parent = line;

    if (right != s_sentinelLine)
        right->u3.parent = line->u3.parent;

    if (line->u3.parent) {
        if (line == line->u3.parent->u1.left)
            line->u3.parent->u1.left = right;
        else
            line->u3.parent->u2.right = right;
    } else {
        m_rootLine = right;
    }

    right->u1.left = line;
    if (line != s_sentinelLine)
        line->u3.parent = right;
}

void TrapezoidBuilder::rotateRight(Line* line)
{
    Line* left = line->u1.left;

    line->u1.left = left->u2.right;
    if (left->u2.right != s_sentinelLine)
        left->u2.right->u3.parent = line;

    if (left != s_sentinelLine)
        left->u3.parent = line->u3.parent;

    if (line->u3.parent) {
        if (line == line->u3.parent->u2.right)
            line->u3.parent->u2.right = left;
        else
            line->u3.parent->u1.left = left;
    } else {
        m_rootLine = left;
    }

    left->u2.right = line;
    if (line != s_sentinelLine)
        line->u3.parent = left;
}

void TrapezoidBuilder::insertLine(FloatPoint start, FloatPoint end)
{
    int direction = Line::directionDown;

    if (m_clipEverything)
        return;

    if (start.y() > end.y()) {
        FloatPoint temp(start);
        start = end;
        end = temp;
        direction = Line::directionUp;
    }

    float x1 = start.x();
    float y1 = start.y();
    float x2 = end.x();
    float y2 = end.y();

    int topY = roundf(y1 * kAntiAliasing);
    int bottomY = roundf(y2 * kAntiAliasing);

    // Discard horizontal lines.
    if (topY == bottomY)
        return;

    // Discard empty lines.
    if (bottomY <= m_clipTop || topY >= m_clipBottom)
        return;

    float slope = ((x2 - x1) / (y2 - y1));
    int topX, bottomX;

    x1 *= kAntiAliasing;
    y1 *= kAntiAliasing;

    if (topY < m_clipTop) {
        topY = m_clipTop;
        topX = computeX(topY, x1, y1, slope);
    } else
        topX = roundf(x1);

    if (bottomY > m_clipBottom) {
        bottomY = m_clipBottom;
        bottomX = computeX(bottomY, x1, y1, slope);
    } else
        bottomX = roundf(x2 * kAntiAliasing);

    if (topX >= m_clipRight && bottomX >= m_clipRight)
        return;

    if (topX > m_clipRight) {
        topY = computeY(m_clipRight, x1, y1, slope);
        if (topY >= bottomY)
            return;

        topX = m_clipRight;
        if (topY < m_clipTop) {
            topY = m_clipTop;
            topX = computeX(topY, x1, y1, slope);
            if (topX > m_clipRight)
                topX = m_clipRight;
        }
    } else if (bottomX > m_clipRight) {
        bottomY = computeY(m_clipRight, x1, y1, slope);
        if (topY >= bottomY)
            return;

        bottomX = m_clipRight;
        if (bottomY >= m_clipBottom) {
            bottomY = m_clipBottom;
            bottomX = computeX(bottomY, x1, y1, slope);
            if (bottomX > m_clipRight)
                bottomX = m_clipRight;
        }
    }

    if (topX < m_clipLeft) {
        if (bottomX <= m_clipLeft) {
            x1 = m_clipLeft;
            topX = m_clipLeft;
            bottomX = m_clipLeft;
            slope = 0;
        } else {
            int newTopY = computeY(m_clipLeft, x1, y1, slope);

            // Corner case: the x coordinate at bottomY is less than m_clipLeft,
            // but round(x) is equal to m_clipLeft.
            if (newTopY > bottomY)
                newTopY = bottomY;

            float insertLineY1;
            float insertLineY2;
            if (direction > 0) {
                insertLineY1 = topY;
                insertLineY2 = newTopY;
            } else {
                insertLineY1 = newTopY;
                insertLineY2 = topY;
            }
            insertLine(FloatPoint(m_clipLeft / kAntiAliasing, insertLineY1 / kAntiAliasing),
                FloatPoint(m_clipLeft / kAntiAliasing, insertLineY2 / kAntiAliasing));

            topY = newTopY;
            ASSERT(bottomY >= topY);
            if (bottomY <= topY)
                return;

            topX = m_clipLeft;
            if (topY < m_clipTop) {
                topY = m_clipTop;
                topX = computeX(topY, x1, y1, slope);
                if (topX < m_clipLeft)
                    topX = m_clipLeft;
            }
        }
    } else if (bottomX < m_clipLeft) {
        int newBottomY = computeY(m_clipLeft, x1, y1, slope);

        // Corner case: the x coordinate at topY is less than m_clipLeft,
        // but round(x) is equal to m_clipLeft.
        if (newBottomY < topY)
            newBottomY = topY;

        float insertLineY1;
        float insertLineY2;
        if (direction > 0) {
            insertLineY1 = newBottomY;
            insertLineY2 = bottomY;
        } else {
            insertLineY1 = bottomY;
            insertLineY2 = newBottomY;
        }
        insertLine(FloatPoint(m_clipLeft / kAntiAliasing, insertLineY1 / kAntiAliasing),
            FloatPoint(m_clipLeft / kAntiAliasing, insertLineY2 / kAntiAliasing));

        bottomY = newBottomY;
        ASSERT(bottomY >= topY);
        if (bottomY <= topY)
            return;

        bottomX = m_clipLeft;
        if (bottomY >= m_clipBottom) {
            bottomY = m_clipBottom;
            bottomX = computeX(bottomY, x1, y1, slope);
            if (bottomX > m_clipLeft)
                bottomX = m_clipLeft;
        }
    }

    // Insert this line.
    Line* parent = 0;
    Line* current = m_rootLine;
    bool left = true;

    while (current != s_sentinelLine) {
        parent = current;

        if (topY == current->u4.topY) {
            if (topX == current->topX) {
                if (bottomY == current->bottomY && bottomX == current->bottomX) {
                    current->flagsAndDirection += direction;
                    return;
                }
                left = slope < current->slope;
            } else
                left = topX < current->topX;
        } else
            left = topY < current->u4.topY;

        current = left ? current->u1.left : current->u2.right;
    }

    Line* line = reinterpret_cast<Line*>(m_arena->alloc(sizeof(Line)));
    line->u1.left = s_sentinelLine;
    line->u2.right = s_sentinelLine;
    line->u3.parent = parent;
    line->u4.topY = topY;
    line->topX = topX;
    line->bottomY = bottomY;
    line->bottomX = bottomX;
    line->originalTopY = y1;
    line->originalTopX = x1;
    line->slope = slope;
    line->flagsAndDirection = direction | Line::kRed;

    if (!parent) {
        m_rootLine = line;
        line->setBlack();
        return;
    }

    if (left)
        parent->u1.left = line;
    else
        parent->u2.right = line;

    while (line != m_rootLine && line->u3.parent->isRed()) {
        if (line->u3.parent == line->u3.parent->u3.parent->u1.left) {
            Line *right = line->u3.parent->u3.parent->u2.right;
            if (right->isRed()) {
                line->u3.parent->setBlack();
                right->setBlack();
                line = line->u3.parent->u3.parent;
                line->setRed();
            } else {
                if (line == line->u3.parent->u2.right) {
                    line = line->u3.parent;
                    rotateLeft(line);
                }
                line->u3.parent->setBlack();
                line->u3.parent->u3.parent->setRed();
                rotateRight(line->u3.parent->u3.parent);
            }
        } else {
            Line *left = line->u3.parent->u3.parent->u1.left;
            if (left->isRed()) {
                line->u3.parent->setBlack();
                left->setBlack();
                line = line->u3.parent->u3.parent;
                line->setRed();
            } else {
                if (line == line->u3.parent->u1.left) {
                    line = line->u3.parent;
                    rotateRight(line);
                }
                line->u3.parent->setBlack();
                line->u3.parent->u3.parent->setRed();
                rotateLeft(line->u3.parent->u3.parent);
            }
        }
    }
    m_rootLine->setBlack();
}

void TrapezoidBuilder::insertQuadCurve(const FloatPoint& start, const FloatPoint& control, const FloatPoint& end)
{
    static const float twoThird = static_cast<float>(2) / 3;
    insertCubeCurve(start, FloatPoint(twoThird * (control.x() - start.x()) + start.x(), twoThird * (control.y() - start.y()) + start.y()), FloatPoint(twoThird * (control.x() - end.x()) + end.x(), twoThird * (control.y() - end.y()) + end.y()), end);
}

static inline float abs(const float &t)
{
    return (t >= 0) ? t : -t;
}

void TrapezoidBuilder::insertCubeCurve(const FloatPoint& start, const FloatPoint& control1, const FloatPoint& control2, const FloatPoint& end)
{
    // De Casteljau algorithm.
    const int max = 16 * 3 + 1;
    FloatPoint buffer[max];
    FloatPoint* points = buffer;

    points[0] = start;
    points[1] = control1;
    points[2] = control2;
    points[3] = end;

    do {
        if (CurveToElement::curveIsLineSegment(points)) {
            insertLine(points[0], points[3]);
            points -= 3;
            continue;
        }

        CurveToElement::splitCurve(points);
        points += 3;

        if (points >= buffer + max - 4) {
            // This recursive code path is rarely executed.
            insertCubeCurve(points[0], points[1], points[2], points[3]);
            points -= 3;
        }
    } while (points >= buffer);
}

float TrapezoidBuilder::boundingCircleRadius(const ArcToElement& arcToElement)
{
    TyGL::AffineTransform transform(arcToElement.finalTransform());
    return (transform.apply(FloatPoint(1, 1)) - transform.apply(FloatPoint(0, 0))).diagonalLengthSquared();
}

int TrapezoidBuilder::calculateSegments(float angle, float radius)
{
    float epsilon = kTolerance / radius;
    float angleSegment, error;

    int i = 1;
    do {
        angleSegment = piFloat / i++;
        error = 2.0 / 27.0 * pow(sin(angleSegment / 4.0), 6) / pow(cos(angleSegment / 4.0), 2);
    } while (error > epsilon);

    return ceil(abs(angle) / angleSegment);
}

void TrapezoidBuilder::arcToCurve(FloatPoint* result, float startAngle, float endAngle)
{
    float sinStartAngle = sin(startAngle);
    float cosStartAngle = cos(startAngle);
    float sinEndAngle = sin(endAngle);
    float cosEndAngle = cos(endAngle);

    float height = 4.0 / 3.0 * tan((endAngle - startAngle) / 4.0);

    result[0].set(cosStartAngle - height * sinStartAngle, sinStartAngle + height * cosStartAngle);
    result[1].set(cosEndAngle + height * sinEndAngle, sinEndAngle - height * cosEndAngle);
    result[2].set(cosEndAngle, sinEndAngle);
}

void TrapezoidBuilder::insertArc(const FloatPoint& lastEndPoint, const ArcToElement& arcToElement)
{
    float startAngle = arcToElement.startAngle();
    float endAngle = arcToElement.endAngle();
    bool anticlockwise = arcToElement.anticlockwise();

    TyGL::AffineTransform transform(m_transform);
    transform *= arcToElement.finalTransform();

    FloatPoint startPoint = transform.apply(FloatPoint(cos(startAngle), sin(startAngle)));
    insertLine(lastEndPoint, startPoint);

    ASSERT(startAngle != endAngle);

    float deltaAngle = anticlockwise ? startAngle - endAngle : endAngle - startAngle;

    int segments = calculateSegments(deltaAngle, boundingCircleRadius(arcToElement));
    float step = deltaAngle / segments;

    if (anticlockwise)
        step = -step;

    FloatPoint bezierPoints[3];
    for (int i = 0; i < segments; i++, startAngle += step) {
        arcToCurve(bezierPoints, startAngle,
            i == segments - 1 ? endAngle : startAngle + step);
        bezierPoints[0] = transform.apply(bezierPoints[0]);
        bezierPoints[1] = transform.apply(bezierPoints[1]);
        if (i == segments - 1)
            bezierPoints[2] = m_transform.apply(arcToElement.end());
        else
            bezierPoints[2] = transform.apply(bezierPoints[2]);
        insertCubeCurve(startPoint, bezierPoints[0], bezierPoints[1], bezierPoints[2]);
        startPoint = bezierPoints[2];
    }
}

template<bool nonZeroRule>
inline TrapezoidBuilder::Line* TrapezoidBuilder::computeLineList(Line* current)
{
    // The resulting line set is a lexical order of {topY, topX, slope} members.
    while (current->u1.left != s_sentinelLine)
        current = current->u1.left;

    Line* firstLine = current;
    // Setting previous to the current removes an unnecessary check.
    Line* previous = current;

    do {
        // The left pointer can be changed since the tree is not used anymore.
        // This is even true for the first node.
        if ((nonZeroRule && current->direction())
            || (!nonZeroRule && (current->isOddDirection()))) {
            previous->u1.next = current;
            previous = current;
        }

        if (current->u2.right != s_sentinelLine) {
            current = current->u2.right;
            while (current->u1.left != s_sentinelLine)
                current = current->u1.left;
            continue;
        }

        Line* parent = current->u3.parent;
        // Same is true for the right pointer.
        current->u2.cachedIntersectionLine = 0;

        while (parent && parent->u2.right == current) {
           current = parent;
           parent->u2.cachedIntersectionLine = 0;
           parent = current->u3.parent;
        }

        current = parent;
    } while (current);

    previous->u1.next = 0;

    // Checking the first line is performed at the end to
    // reduce the number of checks inside the loop.
    if ((nonZeroRule && !firstLine->direction())
        || (!nonZeroRule && !firstLine->isOddDirection()))
        firstLine = firstLine->u1.next;

    return firstLine;
}

inline int32_t TrapezoidBuilder::computeNextTopX(Line* line, int bottomY)
{
    ASSERT(bottomY <= line->bottomY);
    if (bottomY >= line->bottomY)
        return line->bottomX;
    return computeX(bottomY, line->originalTopX, line->originalTopY, line->slope);
}

inline TrapezoidBuilder::Trapezoid* TrapezoidBuilder::createTrapezoids(Line* current, Trapezoid*& lastTrapezoid,
    Trapezoid* nextMergeCandidate, Trapezoid*& lastInpreciseTrapezoid, int topY, int bottomY)
{
    ASSERT(!nextMergeCandidate || nextMergeCandidate->bottomY() == topY);

    if (!current)
        return 0;

    int32_t minTopX = computeNextTopX(current, bottomY);
    current->u4.nextTopX = minTopX;

    Trapezoid* scanlineStart = 0;
    int32_t topLeftX = 0;
    int32_t bottomLeftX = 0;
    float leftSlope = 0;
    bool inpreciseLeft = false;
    bool hasLeftSide = false;
    int fillLevel = 0;
    bool resetInprecise = true;
    bool inprecise;

    do {
        ASSERT(!current || bottomY <= current->bottomY);

        int32_t topX;
        int32_t bottomX;
        float slope;
        if (resetInprecise)
            inprecise = false;
        resetInprecise = true;

        if (current) {
            topX = current->topX;
            bottomX = current->u4.nextTopX;
            slope = current->slope;

            if (bottomX < minTopX) {
                bottomX = minTopX;
                inprecise = true;
            } else
                minTopX = bottomX;

            if (m_fillRule == RULE_NONZERO)
                fillLevel += current->direction();
            else
                fillLevel ^= 0x1;

            current = current->u1.next;
            if (current) {
                int32_t nextTopX = computeNextTopX(current, bottomY);
                current->u4.nextTopX = nextTopX;
                if (topX == current->topX && bottomX >= nextTopX) {
                    resetInprecise = false;
                    continue;
                }
            }
        } else {
            ASSERT(hasLeftSide);
            topX = m_clipRight;
            bottomX = m_clipRight;
            slope = 0;
            fillLevel = 0;
        }

        if (!hasLeftSide) {
            // The fillLevel check allows us to skip empty trapezoids
            // (where the two sides are in the same position).
            if (fillLevel) {
                topLeftX = topX;
                bottomLeftX = bottomX;
                leftSlope = slope;
                inpreciseLeft = inprecise;
                hasLeftSide = true;
            }
        } else if (!fillLevel) {
            hasLeftSide = false;
            if (topLeftX == topX && bottomLeftX == bottomX)
                continue;

            if (inpreciseLeft || inprecise) {
                Trapezoid* trapezoid = Trapezoid::createAndAppend(m_arena.get(), topY, topLeftX, topX,
                    bottomY, bottomLeftX, bottomX, leftSlope, slope, lastInpreciseTrapezoid);
                ++m_trapezoidCount;
                lastInpreciseTrapezoid = trapezoid;
                continue;
            }

            bool mergeAllowed = false;
            while (nextMergeCandidate) {
                if (nextMergeCandidate->bottomY() != topY) {
                    // No more candidates.
                    nextMergeCandidate = 0;
                    break;
                }
                int32_t leftX = nextMergeCandidate->bottomLeftX();
                if (leftX >= topLeftX && leftX != nextMergeCandidate->bottomRightX()) {
                    if (leftX == topLeftX)
                        mergeAllowed = nextMergeCandidate->compareForMerge(topX, leftSlope, slope);
                    break;
                }
                nextMergeCandidate = nextMergeCandidate->next();
            }

            Trapezoid* trapezoid;
            // Combine trapezoids whenever it is possible.
            if (mergeAllowed) {
                trapezoid = nextMergeCandidate;
                nextMergeCandidate = nextMergeCandidate->next();
                trapezoid->updateBottomAndReappend(bottomY, bottomLeftX, bottomX, lastTrapezoid);
            } else {
                trapezoid = Trapezoid::createAndAppend(m_arena.get(), topY, topLeftX, topX,
                    bottomY, bottomLeftX, bottomX, leftSlope, slope, lastTrapezoid);
                ++m_trapezoidCount;
            }

            lastTrapezoid = trapezoid;
            if (!scanlineStart)
                scanlineStart = trapezoid;
        }
    } while (current || fillLevel);

    ASSERT(!hasLeftSide);
    return scanlineStart;
}

inline void TrapezoidBuilder::checkIntersection(Line* leftLine, Line* rightLine, int bottomY, int& nextBottomY)
{
    ASSERT(leftLine->topX < rightLine->topX);

    if (leftLine->slope <= rightLine->slope)
        return;

    int intersectionY;
    if (leftLine->u2.cachedIntersectionLine == rightLine) {
        // Cache hit ratio is usually >= 90%.
        intersectionY = leftLine->u3.cachedIntersectionY;
    } else {
        float y, x1, x2;

        if (leftLine->originalTopY == rightLine->originalTopY) {
            y = leftLine->originalTopY;
            x1 = leftLine->originalTopX;
            x2 = rightLine->originalTopX;
        } else if (leftLine->originalTopY < rightLine->originalTopY) {
            y = rightLine->originalTopY;
            x1 = leftLine->slope * (y - leftLine->originalTopY) + leftLine->originalTopX;
            x2 = rightLine->originalTopX;
        } else {
            y = leftLine->originalTopY;
            x1 = leftLine->originalTopX;
            x2 = rightLine->slope * (y - rightLine->originalTopY) + rightLine->originalTopX;
        }

        intersectionY = roundf(y + ((x2 - x1) / (leftLine->slope - rightLine->slope)));
        int32_t leftX = computeX(intersectionY, leftLine->originalTopX, leftLine->originalTopY, leftLine->slope);
        int32_t rightX = computeX(intersectionY, rightLine->originalTopX, rightLine->originalTopY, rightLine->slope);

        if (leftX < rightX && intersectionY >= bottomY)
            ++intersectionY;

        leftLine->u2.cachedIntersectionLine = rightLine;
        leftLine->u3.cachedIntersectionY = intersectionY;
    }

    if (intersectionY < nextBottomY && intersectionY > bottomY)
        nextBottomY = intersectionY;
}

inline TrapezoidBuilder::Line* TrapezoidBuilder::updateActiveLineSet(Line* currentActiveLine,
    Line*& nextInactiveLine, int topY, int bottomY, int& nextBottomY)
{
    Line* lineForInsert;
    Line* firstLine = 0;
    Line* lastLine = 0;
    Line* lastLineWithSmallerTopX = 0;
    bool hasInactiveLine = (nextInactiveLine && nextInactiveLine->u4.topY <= bottomY);

    while (true) {
        ASSERT(!nextInactiveLine || nextInactiveLine->u4.topY >= bottomY);

        if (currentActiveLine && (!hasInactiveLine || nextInactiveLine->topX > currentActiveLine->u4.nextTopX)) {
            ASSERT(!currentActiveLine->u1.next || ((currentActiveLine->topX < currentActiveLine->u1.next->topX
                || (currentActiveLine->topX == currentActiveLine->u1.next->topX
                && currentActiveLine->slope <= currentActiveLine->u1.next->slope))));

            lineForInsert = currentActiveLine;
            currentActiveLine = currentActiveLine->u1.next;
            if (lineForInsert->bottomY <= bottomY) {
                ASSERT(lineForInsert->bottomY == bottomY);
                // Drop this line.
                continue;
            }
            lineForInsert->topX = lineForInsert->u4.nextTopX;
        } else if (hasInactiveLine) {
            lineForInsert = nextInactiveLine;
            nextInactiveLine = nextInactiveLine->u1.next;
            hasInactiveLine = (nextInactiveLine && nextInactiveLine->u4.topY <= bottomY);
        } else {
            // No more lines are part of this scanline.
            break;
        }

        lineForInsert->u1.next = 0;
        if (lineForInsert->bottomY < nextBottomY)
            nextBottomY = lineForInsert->bottomY;

        if (UNLIKELY(!firstLine)) {
            firstLine = lineForInsert;
            lastLine = lineForInsert;
            continue;
        }

        if (lastLine->topX < lineForInsert->topX) {
            // The chance of entering here is usually > 95%.
            lastLineWithSmallerTopX = lastLine;
            lastLine->u1.next = lineForInsert;
            checkIntersection(lastLine, lineForInsert, bottomY, nextBottomY);
            lastLine = lineForInsert;
            continue;
        }

        if (lastLine->topX == lineForInsert->topX) {
            // The chance of entering here is usually > 90%.
            // Sort by slope. Note: lines starting from the
            // same topX cannot intersect with each other.

            if (lastLine->slope <= lineForInsert->slope) {
                // The chance of entering here is usually > 55%.
                lastLine->u1.next = lineForInsert;
                lastLine = lineForInsert;
                continue;
            }

            Line* previousLine;
            if (!lastLineWithSmallerTopX) {
                if (lineForInsert->slope <= firstLine->slope) {
                    lineForInsert->u1.next = firstLine;
                    firstLine = lineForInsert;
                    continue;
                }
                previousLine = firstLine;
            } else {
                ASSERT(lastLineWithSmallerTopX->u1.next);
                if (lineForInsert->slope <= lastLineWithSmallerTopX->u1.next->slope) {
                    lineForInsert->u1.next = lastLineWithSmallerTopX->u1.next;
                    lastLineWithSmallerTopX->u1.next = lineForInsert;
                    checkIntersection(lastLineWithSmallerTopX, lineForInsert, bottomY, nextBottomY);
                    continue;
                }
                previousLine = lastLineWithSmallerTopX->u1.next;
            }

            // The only case where we perform a search loop. Usually new lines are
            // inserted in order, intersecting lines are in reversed order, so
            // the chance of entering here is usually very low (< 2%).
            while (previousLine->u1.next && previousLine->u1.next->slope < lineForInsert->slope)
                previousLine = previousLine->u1.next;

            ASSERT(previousLine != lastLine);
            lineForInsert->u1.next = previousLine->u1.next;
            previousLine->u1.next = lineForInsert;
            continue;
        }

        if (lastLineWithSmallerTopX && lastLineWithSmallerTopX->lessTopPosition(lineForInsert)) {
            // The chance of entering here is usually > 95%.
            // We are lucky, the line is after lastLineWithSmallerTopX.
            lineForInsert->u1.next = lastLineWithSmallerTopX->u1.next;
            lastLineWithSmallerTopX->u1.next = lineForInsert;
            lastLineWithSmallerTopX = lineForInsert;
            continue;
        }

        if (lineForInsert->lessTopPosition(firstLine)) {
            // The line is before the first line.
            lineForInsert->u1.next = firstLine;
            firstLine = lineForInsert;
            if (!lastLineWithSmallerTopX)
                lastLineWithSmallerTopX = lineForInsert;
            continue;
        }

        ASSERT(lastLineWithSmallerTopX);

        Line* previousLine = firstLine;
        ASSERT(previousLine->u1.next);
        while (previousLine->u1.next->lessTopPosition(lineForInsert)) {
            previousLine = previousLine->u1.next;
            ASSERT(previousLine && previousLine->u1.next && previousLine != lastLineWithSmallerTopX);
        }
        lineForInsert->u1.next = previousLine->u1.next;
        previousLine->u1.next = lineForInsert;
    }

    if (nextInactiveLine && nextInactiveLine->u4.topY < nextBottomY)
        nextBottomY = nextInactiveLine->u4.topY;
    return firstLine;
}

PassOwnPtr<TrapezoidList> TrapezoidBuilder::trapezoidList()
{
    // Convert path to scanstripes.
    PathElement* element = m_path->firstElement();
    PathElement* shapeFirstElement = 0;
    FloatPoint start;

    QuadCurveToElement* quadToElement;
    CurveToElement* curveToElement;
    ArcToElement* arcToElement;

    while (element) {
        FloatPoint end = m_transform.apply(element->end());

        switch (element->type()) {
        case PathElement::MoveTo:
            if (shapeFirstElement)
                insertLine(start, m_transform.apply(shapeFirstElement->end()));

            shapeFirstElement = element;
            break;
        case PathElement::CloseSubpath:
            shapeFirstElement = element;
            // Fall through.
        case PathElement::LineTo:
            insertLine(start, end);
            break;
        case PathElement::QuadCurveTo:
            quadToElement = reinterpret_cast<QuadCurveToElement*>(element);
            insertQuadCurve(start, m_transform.apply(quadToElement->control()), end);
            break;
        case PathElement::CurveTo:
            curveToElement = reinterpret_cast<CurveToElement*>(element);
            insertCubeCurve(start, m_transform.apply(curveToElement->control1()), m_transform.apply(curveToElement->control2()), end);
            break;
        case PathElement::ArcTo:
            arcToElement = reinterpret_cast<ArcToElement*>(element);
            insertArc(start, *arcToElement);
            break;
        }

        start = end;
        element = element->next();
    }

    if (shapeFirstElement)
        insertLine(start, m_transform.apply(shapeFirstElement->end()));

    Line* nextInactiveLine = nullptr;

    if (m_rootLine != s_sentinelLine)
        nextInactiveLine = (m_fillRule == RULE_NONZERO) ? computeLineList<true>(m_rootLine) : computeLineList<false>(m_rootLine);

    if (!nextInactiveLine) {
        OwnPtr<TrapezoidList> trapezoidList = TrapezoidList::create(0);
        trapezoidList->setBoundingBox(m_clipLeft, m_clipTop, 0, 0);
        return trapezoidList.release();
    }

    Trapezoid* firstTrapezoid = Trapezoid::createSentinel(m_arena.get());
    Trapezoid* lastTrapezoid = firstTrapezoid;
    Trapezoid* firstInpreciseTrapezoid = Trapezoid::createSentinel(m_arena.get());
    Trapezoid* lastInpreciseTrapezoid = firstInpreciseTrapezoid;
    Trapezoid* previousScanlineStart = 0;
    Line* activeLines = 0;
    int topY = nextInactiveLine->u4.topY;
    int bottomY = topY;
    int nextBottomY;

    while (nextInactiveLine || activeLines) {
        previousScanlineStart = createTrapezoids(activeLines, lastTrapezoid,
            previousScanlineStart, lastInpreciseTrapezoid, topY, bottomY);

        nextBottomY = m_clipBottom;
        activeLines = updateActiveLineSet(activeLines,
            nextInactiveLine, topY, bottomY, nextBottomY);

        topY = bottomY;
        ASSERT(nextBottomY > bottomY || nextBottomY == m_clipBottom);
        bottomY = nextBottomY;
    }

    // This is correct even if there are no inprecise trapezoids.
    lastTrapezoid->setNext(firstInpreciseTrapezoid->next());

    int minX = m_clipRight;
    int minY = m_clipBottom;
    int maxX = m_clipLeft;
    int maxY = m_clipTop;

    OwnPtr<TrapezoidList> trapezoidList = TrapezoidList::create(m_trapezoidCount);

    Trapezoid* currentTrapezoid = firstTrapezoid->next();
    if (!currentTrapezoid) {
        ASSERT(!m_trapezoidCount);
        trapezoidList->setBoundingBox(m_clipLeft, m_clipTop, 0, 0);
        return trapezoidList.release();
    }

    if (m_trapezoidCount == 1 && currentTrapezoid->topLeftX() == currentTrapezoid->bottomLeftX()
        && currentTrapezoid->topRightX() == currentTrapezoid->bottomRightX())
        trapezoidList->setShapeType(TrapezoidList::ShapeTypeRect);

    float antiAliasingAsFloat = kAntiAliasing;
    do {
        if (minY > currentTrapezoid->topY())
            minY = currentTrapezoid->topY();
        if (maxY < currentTrapezoid->bottomY())
            maxY = currentTrapezoid->bottomY();
        if (minX > currentTrapezoid->topLeftX())
            minX = currentTrapezoid->topLeftX();
        if (minX > currentTrapezoid->bottomLeftX())
            minX = currentTrapezoid->bottomLeftX();
        if (maxX < currentTrapezoid->topRightX())
            maxX = currentTrapezoid->topRightX();
        if (maxX < currentTrapezoid->bottomRightX())
            maxX = currentTrapezoid->bottomRightX();

        TrapezoidList::Trapezoid newTrapezoid;
        newTrapezoid.bottomY = currentTrapezoid->topY() / antiAliasingAsFloat;
        newTrapezoid.bottomLeftX = currentTrapezoid->topLeftX() / antiAliasingAsFloat;
        newTrapezoid.bottomRightX = currentTrapezoid->topRightX() / antiAliasingAsFloat;
        newTrapezoid.topY = currentTrapezoid->bottomY() / antiAliasingAsFloat;
        newTrapezoid.topLeftX = currentTrapezoid->bottomLeftX() / antiAliasingAsFloat;
        newTrapezoid.topRightX = currentTrapezoid->bottomRightX() / antiAliasingAsFloat;

        trapezoidList->insertTrapezoid(newTrapezoid);
        currentTrapezoid = currentTrapezoid->next();
    } while (currentTrapezoid);

    minX /= kAntiAliasing;
    minY /= kAntiAliasing;
    maxX = (maxX + (kAntiAliasing - 1)) / kAntiAliasing;
    maxY = (maxY + (kAntiAliasing - 1)) / kAntiAliasing;
    trapezoidList->setBoundingBox(minX, minY, maxX - minX, maxY - minY);

    return trapezoidList.release();
}

} // namespace TyGL
} // namespace WebCore
