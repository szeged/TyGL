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
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "Color.h"
#include "FloatConversion.h"
#include "Font.h"
#include "ImageBuffer.h"
#include "NotImplemented.h"
#include "Path.h"
// #include "PathTyGL.h"
#include "Pattern.h"
#include "PlatformContextTyGL.h"
#include <transforms/AffineTransform.h>
#include "TransformationMatrix.h"
// #include "WindRule.h"

#include <wtf/MathExtras.h>

namespace WebCore {

void GraphicsContext::platformInit(PlatformGraphicsContext* context)
{
    m_data = reinterpret_cast<GraphicsContextPlatformPrivate*>(context);

    setPaintingDisabled(!context);
}

PlatformContextTyGL* GraphicsContext::platformContext() const
{
    return reinterpret_cast<PlatformGraphicsContext*>(m_data);
}

void GraphicsContext::platformDestroy()
{
//    while (!m_data->layers.isEmpty())
//      endTransparencyLayer();

    platformContext()->deref();
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    return AffineTransform(platformContext()->transform().transform().m_matrix[0],
                           platformContext()->transform().transform().m_matrix[1],
                           platformContext()->transform().transform().m_matrix[2],
                           platformContext()->transform().transform().m_matrix[3],
                           platformContext()->transform().transform().m_matrix[4],
                           platformContext()->transform().transform().m_matrix[5]);
}

void GraphicsContext::savePlatformState()
{
    platformContext()->save();
}

void GraphicsContext::restorePlatformState()
{
    platformContext()->restore();
}

void GraphicsContext::drawRect(const FloatRect& rect, float)
{
    ASSERT(strokeStyle() == SolidStroke || strokeStyle() == NoStroke);

    fillRect(rect);
}

static double calculateStrokePatternOffset(int distance, int patternWidth)
{
    // This function copied from Cairo into TyGL.

    // Example: 80 pixels with a width of 30 pixels. Remainder is 20.
    // The maximum pixels of line we could paint will be 50 pixels.
    int remainder = distance % patternWidth;
    int numSegments = (distance - remainder) / patternWidth;

    // Special case 1px dotted borders for speed.
    if (patternWidth == 1)
        return 1;

    bool evenNumberOfSegments = numSegments & 0x1;
    if (remainder)
        evenNumberOfSegments = !evenNumberOfSegments;

    if (evenNumberOfSegments) {
        if (remainder)
            return patternWidth - remainder + remainder / 2;
        return patternWidth / 2;
    }

    // Odd number of segments.
    if (remainder)
        return (patternWidth - remainder) / 2.f;
    return 0;
}

static void drawLineOnTyGLContext(GraphicsContext* graphicsContext, PlatformContextTyGL* context, const FloatPoint& point1, const FloatPoint& point2)
{
    // This function adapted from Cairo into TyGL.

    ASSERT(point1.x() == point2.x() || point1.y() == point2.y());

    StrokeStyle style = graphicsContext->strokeStyle();
    if (style == NoStroke)
        return;

    const Color& strokeColor = graphicsContext->strokeColor();
    int strokeThickness = graphicsContext->strokeThickness();
    if (graphicsContext->strokeThickness() < 1)
        strokeThickness = 1;

    int patternWidth = 0;
    if (style == DottedStroke)
        patternWidth = strokeThickness;
    else if (style == DashedStroke)
        patternWidth = 3 * strokeThickness;

    const bool isVerticalLine = (point1.x() == point2.x());
    FloatPoint sideBegin = point1;
    FloatPoint sideEnd = point2;
    double patternOffset = 0;
    const double patternWidthAsDouble = patternWidth;

    if (patternWidth) {
        // Border neatening - basic approach: Do a rect fill of our endpoints (which would be corners in a border) - this ensures
        // we always have the appearance of being a border. We then draw the actual dotted/dashed line.

        // There are some pixel-perfect issues with gaps between the corner pieces and the main edges;
        // overlapAdjust removes these.
        const int overlapAdjust = 2;
        if (isVerticalLine) {
            sideBegin.setY(sideBegin.y() + patternWidth - overlapAdjust);
            sideEnd.setY(sideEnd.y() - patternWidth + overlapAdjust);
        } else {
            sideBegin.setX(sideBegin.x() + patternWidth - overlapAdjust);
            sideEnd.setX(sideEnd.x() - patternWidth + overlapAdjust);
        }

        const int distance = (isVerticalLine ? (point2.y() - point1.y()) : (point2.x() - point1.x())) - 2 * strokeThickness;
        patternOffset = calculateStrokePatternOffset(distance, patternWidth);
        // More pixel-perfect issues: cornerPieceEndOffset can prevent misalignment of the "right" and "bottom" corner-pieces.
        const int cornerPieceEndOffset = ((strokeThickness % 2) == 1) ? 1 : 0;
        // "Corner" piece rectangles. In TyGL, we actually re-use drawBorderLine instead of fillRect to draw the
        // cornerpiece, endpoint "rects" - this way, we benefit from batching.
        if (isVerticalLine) {
            context->drawBorderLine(point1, point1 + IntPoint(0, patternWidth + 1), strokeColor, strokeThickness, patternWidth, 0);
            context->drawBorderLine(point2 + -IntPoint(0, cornerPieceEndOffset),  point2 + -IntPoint(0, patternWidth + 1), strokeColor, strokeThickness, patternWidth, 0);
        } else {
            context->drawBorderLine(point1, point1 + IntPoint(patternWidth, 0), strokeColor, strokeThickness, patternWidth, 0);
            context->drawBorderLine(point2 + -IntPoint(cornerPieceEndOffset, 0), point2 + -IntPoint(patternWidth, 0), strokeColor, strokeThickness, patternWidth, 0);
        }
    }
    // Draw the actual side.
    context->drawBorderLine(IntPoint(sideBegin.x(), sideBegin.y()), IntPoint(sideEnd.x(), sideEnd.y()), strokeColor, strokeThickness, patternWidthAsDouble, patternOffset);
}

// This is only used to draw borders.
// Must not cast any shadow.
void GraphicsContext::drawLine(const FloatPoint& point1, const FloatPoint& point2)
{
    drawLineOnTyGLContext(this, platformContext(), point1, point2);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    platformContext()->drawBulletPoint(rect, strokeColor());
}

static Path pathFromConvexPolygon(size_t npoints, const FloatPoint* points)
{
    ASSERT(npoints >= 1);

    // We can probably do better than this: the special structure would make it easy to split into
    // quads without going through TrapezoidBuilder, but I think the gains would be marginal.
    Path polygonPath;
    polygonPath.moveTo(points[0]);
    for (int pointIndex = 1; pointIndex < npoints; pointIndex++)
        polygonPath.addLineTo(points[pointIndex]);

    polygonPath.addLineTo(points[0]);
    return polygonPath;
}

void GraphicsContext::drawConvexPolygon(size_t numPoints, const FloatPoint* points, bool /*shouldAntialias*/)
{
    // TODO - need a shouldAntialias flag for filling TyGL paths.
    platformContext()->fillPath(pathFromConvexPolygon(numPoints, points).platformPath(), platformContext()->parseFillColoring(state()));
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool /*antialiased*/)
{
    // TODO - need an antialiased flag for clipping TyGL paths.
    platformContext()->clip(pathFromConvexPolygon(numPoints, points).platformPath(), RULE_NONZERO);
}

void GraphicsContext::fillPath(const Path& path)
{
    platformContext()->fillPath(path.platformPath(), platformContext()->parseFillColoring(state()), fillRule());
}

void GraphicsContext::strokePath(const Path& path)
{
    platformContext()->strokePath(path.platformPath(), platformContext()->parseStrokeColoring(state()));
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    platformContext()->fillRect(rect, platformContext()->parseFillColoring(state()));
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    UNUSED_PARAM(colorSpace);
    platformContext()->fillRect(rect, PlatformContextTyGL::Coloring(color));
}

void GraphicsContext::fillRoundedRect(const FloatRoundedRect& rect, const Color& color, ColorSpace, BlendMode)
{
    // We could probably get a more optimised version by using a special shader to handle the rounded corners, but using a path
    // will do for now.
    if (paintingDisabled())
        return;

    Path path;
    path.addRoundedRect(rect);
    platformContext()->fillPath(path.platformPath(), PlatformContextTyGL::Coloring(color));
}

void GraphicsContext::clip(const FloatRect& rect)
{
    platformContext()->clip(rect);
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    clip(path, clipRule);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blendMode)
{
    platformContext()->setCompositeOperation(op, blendMode);
}

void GraphicsContext::clip(const Path& path, WindRule windRule)
{
    platformContext()->clip(path.platformPath(), windRule);
}

void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    clip(path, windRule);
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int offset, const Color& color)
{
    Path path;
    const unsigned rectCount = rects.size();
    for (unsigned i = 0; i < rectCount; ++i) {
        if (i > 0)
            path.clear();
        path.addRect(rects[i]);
    }
    drawFocusRing(path, width, offset, color);
}

void GraphicsContext::drawFocusRing(const Path& path, int width, int offset, const Color& color)
{
    // A Solid focus ring seems to be perfectly fine according to Cairo, but a Dotted one
    // might be better in the future.
    // Cairo ignores the "offset" parameter, so we'll do the same.
    // Chrome and Cairo (GTK) both assume width 1: let's just do the same.
    const float originalStrokeThickness = strokeThickness();
    platformContext()->setStrokeThickness(1);
    platformContext()->strokePath(path.platformPath(), PlatformContextTyGL::Coloring(color));
    platformContext()->setStrokeThickness(originalStrokeThickness);
}

/**
 * Focus ring handling for form controls is not handled here. Qt style in
 * RenderTheme handles drawing focus on widgets which need it.
 * It is still handled here for links.
 */

FloatRect GraphicsContext::computeLineBoundsForText(const FloatPoint& origin, float width, bool printing)
{
    bool dummyBool;
    Color dummyColor;
    return computeLineBoundsAndAntialiasingModeForText(origin, width, printing, dummyBool, dummyColor);
}

//void GraphicsContext::drawLineForText(const FloatRect& bounds, bool printing)
void GraphicsContext::drawLineForText(const FloatPoint& bounds, float width, bool printing, bool doubleLines)
{
    // Adapted from the Cairo implementation for TyGL.
    if (paintingDisabled())
        return;

    // IntPoint startPoint(bounds.x(), bounds.y());
    // IntPoint endPoint(startPoint + IntSize(bounds.width(), 0));

    FloatPoint startPoint(bounds.x(), bounds.y());
    FloatPoint endPoint(startPoint + FloatSize(width, 0));


    drawLineOnTyGLContext(this, platformContext(), startPoint, endPoint);
}

void GraphicsContext::drawLinesForText(const FloatPoint& point, const DashArray& widths, bool printing, bool doubleLines)
{
    //FIXME: Implement as soon as possible, see same function in GraphicsContextCairo.cpp.
    UNUSED_PARAM(point);
    UNUSED_PARAM(widths);
    UNUSED_PARAM(printing);
    UNUSED_PARAM(doubleLines);
    return;
}

void GraphicsContext::updateDocumentMarkerResources()
{
    // Unnecessary, since our document markers don't use resources.
}

/*
 *   NOTE: This code is completely based upon the one from
 *   Source/WebCore/platform/graphics/cairo/DrawErrorUnderline.{h|cpp}
 *
 *   Draws an error underline that looks like one of:
 *
 *               H       E                H
 *      /\      /\      /\        /\      /\               -
 *    A/  \    /  \    /  \     A/  \    /  \              |
 *     \   \  /    \  /   /D     \   \  /    \             |
 *      \   \/  C   \/   /        \   \/   C  \            | height = heightSquares * square
 *       \      /\  F   /          \  F   /\   \           |
 *        \    /  \    /            \    /  \   \G         |
 *         \  /    \  /              \  /    \  /          |
 *          \/      \/                \/      \/           -
 *          B                         B
 *          |---|
 *        unitWidth = (heightSquares - 1) * square
 *
 *  The x, y, width, height passed in give the desired bounding box;
 *  x/width are adjusted to make the underline a integer number of units wide.
 */
static void drawErrorUnderline(GraphicsContext* graphicsContext, double x, double y, double width, double height, const Color& color)
{
    // This method adapted for TyGL from Cairo.
    static const double heightSquares = 2.5;

    double square = height / heightSquares;
    double halfSquare = 0.5 * square;

    double unitWidth = (heightSquares - 1.0) * square;
    int widthUnits = static_cast<int>((width + 0.5 * unitWidth) / unitWidth);

    x += 0.5 * (width - widthUnits * unitWidth);
    width = widthUnits * unitWidth;

    double bottom = y + height;
    double top = y;

    Path squigglePath;

    // Bottom of squiggle
    squigglePath.moveTo(FloatPoint(x - halfSquare, top + halfSquare)); // A

    int i = 0;
    for (i = 0; i < widthUnits; i += 2) {
        double middle = x + (i + 1) * unitWidth;
        double right = x + (i + 2) * unitWidth;

        squigglePath.addLineTo(FloatPoint(middle, bottom)); // B

        if (i + 2 == widthUnits)
            squigglePath.addLineTo(FloatPoint(right + halfSquare, top + halfSquare)); // D
        else if (i + 1 != widthUnits)
            squigglePath.addLineTo(FloatPoint(right, top + square)); // C
    }

    // Top of squiggle
    for (i -= 2; i >= 0; i -= 2) {
        double left = x + i * unitWidth;
        double middle = x + (i + 1) * unitWidth;
        double right = x + (i + 2) * unitWidth;

        if (i + 1 == widthUnits)
            squigglePath.addLineTo(FloatPoint(middle + halfSquare, bottom - halfSquare)); // G
        else {
            if (i + 2 == widthUnits)
                squigglePath.addLineTo(FloatPoint(right, top)); // E

            squigglePath.addLineTo(FloatPoint(middle, bottom - halfSquare)); // F
        }

        squigglePath.addLineTo(FloatPoint(left, top)); // H
    }

    graphicsContext->platformContext()->fillPath(squigglePath.platformPath(), PlatformContextTyGL::Coloring(color));
}


void GraphicsContext::drawLineForDocumentMarker(const FloatPoint& origin, float width, DocumentMarkerLineStyle style)
{
    // Adapted from Cairo into TyGL.  Cairo only tackles the two styles listed in the switch statement below; we shall
    // do likewise, for now.
    // It might be slightly more efficient to write a custom, "wavy" shader for this, but it's such an uncommon case that it's
    // probably not worth doing.
    if (paintingDisabled())
        return;

    Color color;

    switch (style) {
    case DocumentMarkerSpellingLineStyle:
        color = TyGL::red;
        break;
    case DocumentMarkerGrammarLineStyle:
        color = TyGL::green;
        break;
    default:
        return;
    }

    drawErrorUnderline(this, origin.x(), origin.y(), width, cMisspellingLineThickness, color);
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& frect, RoundingMode)
{
    // It is not enough just to round to pixels in device space. The rotation part of the
    // affine transform matrix to device space can mess with this conversion if we have a
    // rotating image like the hands of the world clock widget. We just need the scale, so
    // we get the affine transform matrix and extract the scale.

    return FloatRect(frect);
}

void GraphicsContext::setPlatformShadow(const FloatSize& size, float blur, const Color& color, ColorSpace colorSpace)
{
}

void GraphicsContext::clearPlatformShadow()
{
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
}

void GraphicsContext::endPlatformTransparencyLayer()
{
}

bool GraphicsContext::supportsTransparencyLayers()
{
// it was true!
    return false;
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    platformContext()->clearRect(rect);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    Path path;
    path.addRect(rect);

    float previousStrokeThickness = strokeThickness();
    if (previousStrokeThickness != lineWidth)
        platformContext()->setStrokeThickness(lineWidth);
    strokePath(path);
    if (previousStrokeThickness != lineWidth)
        platformContext()->setStrokeThickness(previousStrokeThickness);
}

void GraphicsContext::setLineCap(LineCap lc)
{
    platformContext()->setLineCap(lc);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
}

void GraphicsContext::setLineJoin(LineJoin lj)
{
    platformContext()->setLineJoin(lj);
}

void GraphicsContext::setMiterLimit(float limit)
{
    platformContext()->setMiterLimit(limit);
}

void GraphicsContext::setAlpha(float opacity)
{
}

void GraphicsContext::clipOut(const Path& path)
{
    platformContext()->clipOut(path.platformPath());
}

void GraphicsContext::translate(float x, float y)
{
    platformContext()->translate(x, y);
}

void GraphicsContext::rotate(float radians)
{
    platformContext()->rotate(radians);
}

void GraphicsContext::scale(const FloatSize& s)
{
    platformContext()->scale(s.width(), s.height());
}

void GraphicsContext::clipOut(const FloatRect& rect)
{
    platformContext()->clipOut(rect);
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    TyGL::AffineTransform::Transform tyglTransform =
        { { transform.a(), transform.b(), transform.c(), transform.d(), transform.e(), transform.f() } };
    platformContext()->multiplyTransform(tyglTransform);
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    TyGL::AffineTransform::Transform tyglTransform =
        { { transform.a(), transform.b(), transform.c(), transform.d(), transform.e(), transform.f() } };
    platformContext()->setTransform(tyglTransform);
}

#if ENABLE(3D_RENDERING)
TransformationMatrix GraphicsContext::get3DTransform() const
{
    return getCTM().toTransformationMatrix();
}

void GraphicsContext::concat3DTransform(const TransformationMatrix& transform)
{
    concatCTM(transform.toAffineTransform());
}

void GraphicsContext::set3DTransform(const TransformationMatrix& transform)
{
    setCTM(transform.toAffineTransform());
}
#endif

void GraphicsContext::setURLForRect(const URL&, const IntRect&)
{
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    platformContext()->setStrokeColor(color);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    platformContext()->setStrokeThickness(thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    platformContext()->setFillColor(color);
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality quality)
{
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return InterpolationDefault;
}

void GraphicsContext::platformFillRoundedRect(const FloatRoundedRect& rect, const Color& color, ColorSpace)
{
}

} //namespace Webcore
