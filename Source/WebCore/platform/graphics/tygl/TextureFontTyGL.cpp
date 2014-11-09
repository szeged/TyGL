/*
 * Copyright (C) 2014 Samsung Electronics Ltd. All Rights Reserved.
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
#include "TextureFontTyGL.h"

#include "Color.h"
#include "Font.h"
#include "GLDefs.h"
#include "GLPlatformContext.h"
#include "IntRect.h"
#include "PlatformContextTyGL.h"
#include "ShaderCommonTyGL.h"
#include "SimpleFontData.h"

namespace WebCore {
namespace TyGL {

const TextureFont::GlyphMetrics& TextureFont::glyph(int glyphCode, int& errorCode)
{
    errorCode = 0;
    GlyphMetricsCache::iterator iterator = m_glyphs.find(glyphCode);
    if (iterator != m_glyphs.end())
        return iterator->value;

    const float atlasWidth = m_textureAtlas->width();
    const float atlasHeight = m_textureAtlas->height();
    FT_Face fontFace = m_scaledFont->fontFace();

    FT_Error error = FT_Load_Glyph(fontFace, glyphCode, FT_LOAD_RENDER);
    if (error) {
        LOG_ERROR("Failed to load a glyph with the code %d.", glyphCode);
        errorCode = error;
        return GlyphMetrics();
    }

    FT_GlyphSlot slot = fontFace->glyph;
    const int glyphBitmapWidth = slot->bitmap.width;
    const int glyphBitmapHeight = slot->bitmap.rows;

    IntRect region = m_textureAtlas->region(glyphBitmapWidth + 1, glyphBitmapHeight + 1);
    if (region.x() < 0)  {
        errorCode = AtlasFullError;
        LOG_ERROR("Texture atlas is full.");
        return GlyphMetrics();
    }

    region.setWidth(glyphBitmapWidth);
    region.setHeight(glyphBitmapHeight);
    m_textureAtlas->copyGlyphBitmapToRegion(region, slot->bitmap.buffer, slot->bitmap.pitch, 1);

    GlyphMetrics glyph;
    glyph.size = IntSize(glyphBitmapWidth, glyphBitmapHeight);
    glyph.offset = IntPoint(slot->bitmap_left, slot->bitmap_top);
    glyph.location = IntPoint(region.x(), region.y());
    glyph.atlasRegion = AtlasRegion(region.x() / atlasWidth, region.y() / atlasHeight,
        (region.x() + glyphBitmapWidth) / atlasWidth, (region.y() + glyphBitmapHeight) / atlasHeight);

    m_glyphs.add(glyphCode, glyph);
    return m_glyphs.find(glyphCode)->value;
}

bool TextureFont::createAtlas()
{
    if (!m_textureAtlas)
        m_textureAtlas = FontTextureAtlasTyGL::create(calculateAtlasSize());

    if (!m_textureAtlas) {
        LOG_ERROR("Failed to allocate texture atlas.");
        return false;
    }
    return true;
}

bool TextureFont::expandAtlas()
{
    IntSize oldAtlasSize = m_textureAtlas->size();
    IntSize newAtlasSize;

    // Try to expand the atlas vertically first.
    if (oldAtlasSize.height() < FontTextureAtlasTyGL::MaximumAtlasSide) {
        int newAtlasHeight = oldAtlasSize.height() << 1;
        if (newAtlasHeight > FontTextureAtlasTyGL::MaximumAtlasSide)
            newAtlasHeight = FontTextureAtlasTyGL::MaximumAtlasSide;
        newAtlasSize.setWidth(oldAtlasSize.width());
        newAtlasSize.setHeight(newAtlasHeight);
    }

    // Then horizontally if the atlas height reached the limit.
    if (newAtlasSize == IntSize() && oldAtlasSize.width() < FontTextureAtlasTyGL::MaximumAtlasSide) {
        int newAtlasWidth = oldAtlasSize.width() << 1;
        if (newAtlasWidth > FontTextureAtlasTyGL::MaximumAtlasSide)
            newAtlasWidth = FontTextureAtlasTyGL::MaximumAtlasSide;
        newAtlasSize.setWidth(newAtlasWidth);
        newAtlasSize.setHeight(oldAtlasSize.height());
    }

    // Size of the texture atlas exceeded the limit.
    if (newAtlasSize == IntSize())
        return false;

    m_textureAtlas.clear();
    m_textureAtlas = FontTextureAtlasTyGL::create(newAtlasSize);
    if (!m_textureAtlas) {
        LOG_ERROR("Failed to allocate texture atlas.");
        return false;
    }

    // FIXME: for horizontal increment we can recalculate only heights of the glyphs and upload the whole texture.
    GlyphMetricsCache oldGlyphs;
    oldGlyphs.swap(m_glyphs);

    FT_Error errorCode = 0;
    GlyphMetricsCache::iterator iterator = oldGlyphs.begin();
    GlyphMetricsCache::const_iterator end = oldGlyphs.end();
    while (iterator != end) {
        glyph(iterator->key, errorCode);
        ++iterator;
    }

    return true;
}

IntSize TextureFont::calculateAtlasSize()
{
    int charWidth = m_simpleFontData->maxCharWidth() + 1;
    int charHeight = m_simpleFontData->fontMetrics().height() + 1;

    // After checking popular web sites 32 seems to be close to an average number of characters used per font.
    int count = 32;
    int atlasSide = FontTextureAtlasTyGL::DefaultAtlasSide;

    do {
        int atlasReservedSpace = charWidth * charHeight * count;

        while (atlasReservedSpace > atlasSide * atlasSide)
            atlasSide <<= 1;

        // If atlasSide is more than 1024 then return if the space is reserved for the minimum amount of the characters.
        if (atlasSide <= 1024 || count == 2)
            break;

        // Try to get sensible size for the atlas by reducing number of characters for the purpose of the atlas space calculation.
        atlasSide = 1024;
        count >>= 1;
    } while (count);

    return IntSize(atlasSide, atlasSide);
}

FloatRect GlyphBufferFontData::calculateBoundingBox() const
{
    int fromIndex = m_fromIndex;
    int numGlyphs = m_numGlyphs;

    const GlyphBuffer* glyphBuffer = m_glyphBuffer;
    const FontMetrics& fontMetrics = m_textureFont->simpleFontData()->fontMetrics();

    FloatRect boundingBox;
    boundingBox.setLocation(FloatPoint(m_point.x(), m_point.y() - fontMetrics.ascent()));

    float offset = 0;
    for (int i = 0; i < numGlyphs; ++i)
        offset += glyphBuffer->advanceAt(fromIndex + i).width();

    boundingBox.setSize(FloatSize(offset, fontMetrics.lineSpacing()));
    return boundingBox;
}

} // namespace TyGL
} // namespace WebCore
