/*
 * Copyright (c) 2012 Google Inc. All rights reserved.
 * Copyright (c) 2012 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HarfBuzzFace.h"

#include "FontPlatformData.h"
#include "GlyphBuffer.h"
#include "HarfBuzzShaper.h"
#include "Font.h"
#include "TextEncoding.h"
#include "ScaledFontTyGL.h"
typedef WebCore::ScaledFont scaled_font;
#include <freetype/freetype.h>
#include <freetype/tttables.h>
#include <hb.h>
#include <wtf/text/CString.h>

namespace WebCore {

struct HarfBuzzFontData {
    HarfBuzzFontData(WTF::HashMap<uint32_t, uint16_t>* glyphCacheForFaceCacheEntry, scaled_font* scaledFont)
        : m_glyphCacheForFaceCacheEntry(glyphCacheForFaceCacheEntry)
        , m_scaledFont(scaledFont)
    { }
    WTF::HashMap<uint32_t, uint16_t>* m_glyphCacheForFaceCacheEntry;
    scaled_font* m_scaledFont;
};

static hb_position_t floatToHarfBuzzPosition(float value)
{
    return static_cast<hb_position_t>(value * (1 << 16));
}

static hb_position_t doubleToHarfBuzzPosition(double value)
{
    return static_cast<hb_position_t>(value * (1 << 16));
}

static float fromFTPixelPosition(FT_Pos pixelPos)
{
    // In FreeType, scaled pixel positions are all expressed in the 26.6 fixed
    // float format made of a 26-bit integer mantissa, and a 6-bit fractional part.
    return pixelPos / 64.0;
}

static void freeTypeGetGlyphWidthAndExtents(scaled_font* scaledFont, hb_codepoint_t codepoint, hb_position_t* advance, hb_glyph_extents_t* extents)
{
    FT_Face face = scaledFont->fontFace();
    FT_Load_Glyph(face, codepoint, FT_LOAD_DEFAULT);

    bool hasVerticalGlyphs = FT_HAS_VERTICAL(face);

    if (advance)
        *advance = doubleToHarfBuzzPosition(fromFTPixelPosition(hasVerticalGlyphs ? -face->glyph->metrics.vertAdvance : face->glyph->metrics.horiAdvance));
    if (extents) {
        FT_Glyph_Metrics metrics = face->glyph->metrics;
        extents->x_bearing = doubleToHarfBuzzPosition(fromFTPixelPosition(metrics.horiBearingX));
        extents->y_bearing = doubleToHarfBuzzPosition(fromFTPixelPosition(hasVerticalGlyphs ? -metrics.horiBearingY : metrics.horiBearingY));
        extents->width = doubleToHarfBuzzPosition(fromFTPixelPosition(hasVerticalGlyphs ? -metrics.height : metrics.width ));
        extents->height = doubleToHarfBuzzPosition(fromFTPixelPosition(hasVerticalGlyphs ? -metrics.width : metrics.height));
    }
}

static hb_bool_t harfBuzzGetGlyph(hb_font_t*, void* fontData, hb_codepoint_t unicode, hb_codepoint_t, hb_codepoint_t* glyph, void*)
{
    HarfBuzzFontData* hbFontData = reinterpret_cast<HarfBuzzFontData*>(fontData);
    scaled_font* scaledFont = hbFontData->m_scaledFont;
    ASSERT(scaledFont);

    WTF::HashMap<uint32_t, uint16_t>::AddResult result = hbFontData->m_glyphCacheForFaceCacheEntry->add(unicode, 0);
    if (result.isNewEntry) {
        FT_UInt index = FT_Get_Char_Index(scaledFont->fontFace(), unicode);
        if (!index)
            return false;
        result.iterator->value = index;
    }
    *glyph = result.iterator->value;
    return !!*glyph;
}

static hb_position_t harfBuzzGetGlyphHorizontalAdvance(hb_font_t*, void* fontData, hb_codepoint_t glyph, void*)
{
    HarfBuzzFontData* hbFontData = reinterpret_cast<HarfBuzzFontData*>(fontData);
    scaled_font* scaledFont = hbFontData->m_scaledFont;
    ASSERT(scaledFont);

    hb_position_t advance = 0;
    freeTypeGetGlyphWidthAndExtents(scaledFont, glyph, &advance, 0);
    return advance;
}

static hb_bool_t harfBuzzGetGlyphHorizontalOrigin(hb_font_t*, void*, hb_codepoint_t, hb_position_t*, hb_position_t*, void*)
{
    // Just return true, following the way that Harfbuzz-FreeType
    // implementation does.
    return true;
}

static hb_bool_t harfBuzzGetGlyphExtents(hb_font_t*, void* fontData, hb_codepoint_t glyph, hb_glyph_extents_t* extents, void*)
{
    HarfBuzzFontData* hbFontData = reinterpret_cast<HarfBuzzFontData*>(fontData);
    scaled_font* scaledFont = hbFontData->m_scaledFont;
    ASSERT(scaledFont);

    freeTypeGetGlyphWidthAndExtents(scaledFont, glyph, 0, extents);
    return true;
}

static hb_font_funcs_t* harfBuzzFreeTypeGetFontFuncs()
{
    static hb_font_funcs_t* harfBuzzFreeTypeFontFuncs = 0;

    // We don't set callback functions which we can't support.
    // Harfbuzz will use the fallback implementation if they aren't set.
    if (!harfBuzzFreeTypeFontFuncs) {
        harfBuzzFreeTypeFontFuncs = hb_font_funcs_create();
        hb_font_funcs_set_glyph_func(harfBuzzFreeTypeFontFuncs, harfBuzzGetGlyph, 0, 0);
        hb_font_funcs_set_glyph_h_advance_func(harfBuzzFreeTypeFontFuncs, harfBuzzGetGlyphHorizontalAdvance, 0, 0);
        hb_font_funcs_set_glyph_h_origin_func(harfBuzzFreeTypeFontFuncs, harfBuzzGetGlyphHorizontalOrigin, 0, 0);
        hb_font_funcs_set_glyph_extents_func(harfBuzzFreeTypeFontFuncs, harfBuzzGetGlyphExtents, 0, 0);
        hb_font_funcs_make_immutable(harfBuzzFreeTypeFontFuncs);
    }
    return harfBuzzFreeTypeFontFuncs;
}

static hb_blob_t* harfBuzzFreeTypeGetTable(hb_face_t*, hb_tag_t tag, void* userData)
{
    scaled_font* scaledFont = reinterpret_cast<scaled_font*>(userData);
    if (!scaledFont)
        return 0;

    FT_Face ftFont = scaledFont->fontFace();
    if (!ftFont)
        return 0;

    FT_ULong tableSize = 0;
    FT_Error error = FT_Load_Sfnt_Table(ftFont, tag, 0, 0, &tableSize);
    if (error)
        return 0;

    FT_Byte* buffer = reinterpret_cast<FT_Byte*>(fastMalloc(tableSize));
    if (!buffer)
        return 0;
    FT_ULong expectedTableSize = tableSize;
    error = FT_Load_Sfnt_Table(ftFont, tag, 0, buffer, &tableSize);
    if (error || tableSize != expectedTableSize) {
        fastFree(buffer);
        return 0;
    }

    return hb_blob_create(reinterpret_cast<const char*>(buffer), tableSize, HB_MEMORY_MODE_WRITABLE, buffer, fastFree);
}

static void destroyHarfBuzzFontData(void* userData)
{
    HarfBuzzFontData* hbFontData = reinterpret_cast<HarfBuzzFontData*>(userData);
    delete hbFontData;
}

hb_face_t* HarfBuzzFace::createFace()
{
    hb_face_t* face = hb_face_create_for_tables(harfBuzzFreeTypeGetTable, m_platformData->scaledFont(), 0);
    ASSERT(face);
    return face;
}

hb_font_t* HarfBuzzFace::createFont()
{
    hb_font_t* font = hb_font_create(m_face);
    HarfBuzzFontData* hbFontData = new HarfBuzzFontData(m_glyphCacheForFaceCacheEntry, m_platformData->scaledFont());
    hb_font_set_funcs(font, harfBuzzFreeTypeGetFontFuncs(), hbFontData, destroyHarfBuzzFontData);
    const float size = m_platformData->size();
    if (floorf(size) == size)
        hb_font_set_ppem(font, size, size);
    int scale = floatToHarfBuzzPosition(size);
    hb_font_set_scale(font, scale, scale);
    hb_font_make_immutable(font);
    return font;
}

GlyphBufferAdvance HarfBuzzShaper::createGlyphBufferAdvance(float width, float height)
{
    return GlyphBufferAdvance(width, height);
}

} // namespace WebCore
