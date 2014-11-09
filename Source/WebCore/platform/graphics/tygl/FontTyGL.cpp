/*
 * Copyright (C) 2013 University of Szeged
 * Copyright (C) 2014 Samsung Electronics Ltd.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Font.h"

#include "FloatRect.h"
#include "GraphicsContext.h"
#include "HarfBuzzShaper.h"
#include "LayoutRect.h"
#include "NotImplemented.h"
#include "PlatformContextTyGL.h"
#include "TextureFontTyGL.h"

namespace WebCore {

// FIXME: Instead of copying all FontCairoHarfbuzzNG methods here, we could
// remove its -unused- Cairo dependencies and move it to HarfBuzz directory.

void Font::adjustSelectionRectForComplexText(const TextRun& run, LayoutRect& selectionRect, int from, int to) const
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape()) {
        // FIXME: This should mimic Mac port.
        FloatRect rect = shaper.selectionRect(FloatPoint(selectionRect.location()), selectionRect.height().toInt(), from, to);
        selectionRect = LayoutRect(rect);
        return;
    }
    LOG_ERROR("Shaper couldn't shape text run.");
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>*, GlyphOverflow*) const
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
        return shaper.totalWidth();
    LOG_ERROR("Shaper couldn't shape text run.");
    return 0;
}

float Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int/* from*/, int/* to*/) const
{
    GlyphBuffer glyphBuffer;
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape(&glyphBuffer)) {
        FloatPoint startPoint = point;
        float startX = startPoint.x();
        drawGlyphBuffer(context, run, glyphBuffer, startPoint);
        return startPoint.x() - startX;
    }
    LOG_ERROR("Shaper couldn't shape glyphBuffer.");
    return 0;
}

int Font::offsetForPositionForComplexText(const TextRun& run, float x, bool) const
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
        return shaper.offsetForPosition(x);
    LOG_ERROR("Shaper couldn't shape text run.");
    return 0;
}

void Font::drawGlyphs(GraphicsContext* gc, const SimpleFontData* font, const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point) const
{
    if (!font->platformData().size())
        return;

    gc->platformContext()->fillText(font->platformData().textureFont(), &glyphBuffer, from, numGlyphs, point, gc->platformContext()->parseFillColoring(gc->state()));
}

DashArray Font::dashesForIntersectionsWithRect(const TextRun& run, const FloatPoint& textOrigin, const FloatRect& lineExtents) const
{
    //FIXME: Implement as soon as possible, see same function in FontCairo.cpp.
    UNUSED_PARAM(run);
    UNUSED_PARAM(textOrigin);
    UNUSED_PARAM(lineExtents);
    DashArray array;
    return array;
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    notImplemented();
}

} //namespace WebCore
