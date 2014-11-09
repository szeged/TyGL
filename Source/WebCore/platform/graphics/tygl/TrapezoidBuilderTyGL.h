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

#ifndef TrapezoidBuilderTyGL_h
#define TrapezoidBuilderTyGL_h

#include "ArenaTyGL.h"
#include "FloatPoint.h"
#include "IntSize.h"
#include "PathTyGL.h"
#include "TransformTyGL.h"
#include "TrapezoidListTyGL.h"
#include "WindRule.h"
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {
namespace TyGL {

class TrapezoidBuilder {
private:
    static const int kAntiAliasing = ANTIALIAS_LEVEL;

public:
    static constexpr float kTolerance = 1.0 / static_cast<float>(kAntiAliasing);

    TrapezoidBuilder(const PathData* path, const TyGL::ClipRect& clipRect, const TyGL::AffineTransform& transform, WindRule fillRule = RULE_NONZERO);

    PassOwnPtr<TrapezoidList> trapezoidList();
    static float boundingCircleRadius(const ArcToElement&);
    static int calculateSegments(float angle, float radius);
    static void arcToCurve(FloatPoint* result, float startAngle, float endAngle);

private:

    struct Line {
        static const int32_t kRed = 1 << 0;
        static const int32_t kBottomInprecise = 1 << 0;
        static const int32_t kTopInprecise = 1 << 1;
        static const int32_t kInpreciseBits = kBottomInprecise | kTopInprecise;
        static const int32_t directionShift = 2;
        static const int32_t directionDown = 1 << directionShift;
        static const int32_t directionUp = -directionDown;

        inline bool isRed() { return flagsAndDirection & kRed; }
        inline void setRed() { flagsAndDirection |= kRed; }
        inline void setBlack() { flagsAndDirection &= ~kRed; }
        inline bool isInprecise() { return flagsAndDirection & kInpreciseBits; }
        inline void clearInprecise() { flagsAndDirection &= ~kInpreciseBits; }
        inline void setBottomInprecise() { flagsAndDirection |= kBottomInprecise; }

        inline void shiftInpreciseBit()
        {
            // Clear bit 1.
            flagsAndDirection &= ~kTopInprecise;
            // Effect: copy bit 0 to bit 1, and invert bit 0.
            ++flagsAndDirection;
            // Clear bit 0.
            flagsAndDirection &= ~kBottomInprecise;
        }

        inline int32_t direction() { return flagsAndDirection >> directionShift; }
        inline bool isOddDirection() { return flagsAndDirection & (1 << directionShift); }

        // During the insertion phase, the lines are stored
        // in a red-black tree. Later, the active lines are
        // moved to a queue. When a line becomes active, its
        // members are change their role (see the unions).
        union {
            Line* left;
            Line* next;
        } u1;
        union {
            Line* right;
            Line* cachedIntersectionLine;
        } u2;
        union {
            Line* parent;
            int32_t cachedIntersectionY;
        } u3;
        union {
            int32_t topY;
            int32_t nextTopX;
        } u4;
        int32_t topX;
        int32_t bottomY;
        int32_t bottomX;
        float originalTopY;
        float originalTopX;
        float slope;
        int32_t flagsAndDirection;
    };

    class Trapezoid {
    public:
        static inline Trapezoid* createAndAppend(Arena* arena, int32_t topY, int32_t topLeftX, int32_t topRightX,
            int32_t bottomY, int32_t bottomLeftX, int32_t bottomRightX,
            float leftSlope, float rightSlope, Trapezoid* lastTrapezoid)
        {
            ASSERT(topY < bottomY && topLeftX <= topRightX && bottomLeftX <= bottomRightX);
            // Empty trapezoids are not allowed.
            ASSERT(topLeftX < topRightX || bottomLeftX < bottomRightX);

            return new (reinterpret_cast<Trapezoid*>(arena->alloc(sizeof(Trapezoid)))) Trapezoid(
                topY, topLeftX, topRightX, bottomY, bottomLeftX, bottomRightX, leftSlope, rightSlope, lastTrapezoid);
        }

        static inline Trapezoid* createSentinel(Arena* arena)
        {
            return new (reinterpret_cast<Trapezoid*>(arena->alloc(sizeof(Trapezoid)))) Trapezoid();
        }

        int32_t topY() { return m_topY; }
        int32_t topLeftX() { return m_topLeftX; }
        int32_t topRightX() { return m_topRightX; }
        int32_t bottomY() { return m_bottomY; }
        int32_t bottomLeftX() { return m_bottomLeftX; }
        int32_t bottomRightX() { return m_bottomRightX; }
        float leftSlope() { return m_leftSlope; }
        float rightSlope() { return m_rightSlope; }
        Trapezoid* previous() { return m_previous; }
        Trapezoid* next() { return m_next; }
        void setNext(Trapezoid* value) { m_next = value; }

        inline void updateBottomAndReappend(int32_t bottomY, int32_t bottomLeftX, int32_t bottomRightX, Trapezoid* lastTrapezoid)
        {
            ASSERT(m_topY < bottomY && bottomLeftX <= bottomRightX);
            // Note: sometimes we construct empty trapezoids because of rounding issues.
            // Since their number is low, we ignore them at the moment.

            m_bottomY = bottomY;
            m_bottomLeftX = bottomLeftX;
            m_bottomRightX = bottomRightX;

            if (lastTrapezoid == this)
                return;

            m_previous->m_next = m_next;
            m_next->m_previous = m_previous;
            lastTrapezoid->m_next = this;
            m_previous = lastTrapezoid;
            m_next = 0;
        }

        inline bool compareForMerge(int32_t topRightX, float leftSlope, float rightSlope)
        {
            // Hourglass style merge is not allowed.
            ASSERT(m_bottomLeftX != m_bottomRightX);
            return m_bottomRightX == topRightX && m_leftSlope == leftSlope && m_rightSlope == rightSlope;
        }

    private:
        // No need to initialize any member.
        Trapezoid(int32_t topY, int32_t topLeftX, int32_t topRightX,
            int32_t bottomY, int32_t bottomLeftX, int32_t bottomRightX,
            float leftSlope, float rightSlope, Trapezoid* lastTrapezoid)
            : m_topY(topY)
            , m_topLeftX(topLeftX)
            , m_topRightX(topRightX)
            , m_bottomY(bottomY)
            , m_bottomLeftX(bottomLeftX)
            , m_bottomRightX(bottomRightX)
            , m_leftSlope(leftSlope)
            , m_rightSlope(rightSlope)
            , m_previous(lastTrapezoid)
            , m_next(0)
        {
            ASSERT(lastTrapezoid);
            lastTrapezoid->m_next = this;
        }

        Trapezoid()
            : m_previous(0)
            , m_next(0)
        {
        }

        int32_t m_topY;
        int32_t m_topLeftX;
        int32_t m_topRightX;
        int32_t m_bottomY;
        int32_t m_bottomLeftX;
        int32_t m_bottomRightX;
        float m_leftSlope;
        float m_rightSlope;
        Trapezoid* m_previous;
        Trapezoid* m_next;
    };

    // To improve readability, the trapezoid() function is
    // split into several smaller sub-functions.
    template<bool nonZeroRule> static inline Line* computeLineList(Line*);
    static inline int32_t computeNextTopX(Line*, int bottomY);
    inline Trapezoid* createTrapezoids(Line*, Trapezoid*&, Trapezoid*, Trapezoid*&, int topY, int bottomY);
    static inline void checkIntersection(Line*, Line*, int bottomY, int& nextBottomY);
    static inline Line* updateActiveLineSet(Line*, Line*&, int topY, int bottomY, int& nextBottomY);

    // Other utility functions.
    static inline int32_t computeX(float y, float x0, float y0, float slope);
    static inline int32_t computeY(int32_t x, float x0, float y0, float slope);
    static inline int32_t clampToRange(int32_t value, int32_t min, int32_t max);
    void rotateLeft(Line*);
    void rotateRight(Line*);

    // Insert path elements.
    void insertLine(FloatPoint, FloatPoint);
    void insertQuadCurve(const FloatPoint&, const FloatPoint&, const FloatPoint&);
    void insertCubeCurve(const FloatPoint&, const FloatPoint&, const FloatPoint&, const FloatPoint&);
    inline void insertArc(const FloatPoint&, const ArcToElement&);

    OwnPtr<Arena> m_arena;

    const PathData* m_path;
    WindRule m_fillRule;
    const TyGL::AffineTransform& m_transform;

    int m_clipTop;
    int m_clipBottom;
    int m_clipLeft;
    int m_clipRight;
    bool m_clipEverything;

    Line* m_rootLine;
    int m_trapezoidCount;

    static Line* s_sentinelLine;
};

} // namespace TyGL
} // namespace WebCore

#endif  // TrapezoidBuilderTyGL_h
