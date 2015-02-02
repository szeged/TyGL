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
#include "NotImplemented.h"
#include "UTF16UChar32Iterator.h"
#include <fontconfig/fcfreetype.h>
#include <unicode/normlzr.h>

namespace WebCore{

static inline float fromFTPixelPosition(FT_Pos pixelPos)
{
    // In FreeType, scaled pixel positions are all expressed in the 26.6 fixed
    // float format made of a 26-bit integer mantissa, and a 6-bit fractional part.
    return pixelPos / 64.0;
}

void Font::platformInit()
{
    m_platformData.textureFont()->setFont(this);

    if (!m_platformData.m_size)
        return;

    ASSERT(m_platformData.scaledFont());
    FT_Face face = m_platformData.scaledFont()->fontFace();

    FT_Set_Pixel_Sizes(face, m_platformData.m_size, m_platformData.m_size);
    m_platformData.scaledFont()->setSize(m_platformData.m_size);

    float ascent = fromFTPixelPosition(face->size->metrics.ascender);
    float descent = -fromFTPixelPosition(face->size->metrics.descender);
    float lineGap = fromFTPixelPosition(face->size->metrics.height) - ascent - descent;

    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));
    m_fontMetrics.setLineGap(lineGap);

    bool hasHorizontalOrientation = platformData().orientation() == Horizontal;
    FT_Load_Glyph(face, FT_Get_Char_Index(face, 'x'), FT_LOAD_DEFAULT);
    m_fontMetrics.setXHeight(fromFTPixelPosition(hasHorizontalOrientation ? face->glyph->metrics.height : face->glyph->metrics.width));

    FT_Load_Glyph(face, FT_Get_Char_Index(face, ' '), FT_LOAD_DEFAULT);
    m_spaceWidth = fromFTPixelPosition(hasHorizontalOrientation ? face->glyph->metrics.horiAdvance : face->glyph->metrics.vertAdvance);

    if (!hasHorizontalOrientation && !isTextOrientationFallback())
        m_fontMetrics.setUnitsPerEm(face->units_per_EM);

    // FIXME: Ideally ALL font has to be scaled because there is never one to one relationship between request fonts size and received
    // from font config. But quality of scaling now is so poor so it is disabled at the moment.
    // To fix it properly we need to start using SDF approach which will require rewriting font shader.
    // m_platformData.horizontalOrientationTransform()->scale(m_platformData.size() / m_fontMetrics.height(), m_platformData.size() / m_fontMetrics.height());
    // m_platformData.textureFont()->setTransform(*m_platformData.horizontalOrientationTransform());
}

void Font::platformDestroy()
{
}

void Font::determinePitch()
{
    m_treatAsFixedPitch = m_platformData.isFixedPitch();
}

PassRefPtr<Font> Font::platformCreateScaledFont(const FontDescription& fontDescription, float scaleFactor) const
{
    ASSERT(m_platformData.scaledFont());

    return Font::create(FontPlatformData(m_platformData.m_scaledFont->fontFace(),
                                                   scaleFactor * fontDescription.computedSize(),
                                                   m_platformData.syntheticBold(),
                                                   m_platformData.syntheticOblique(),
                                                   fontDescription.orientation()), isCustomFont(), false);
}

FloatRect Font::platformBoundsForGlyph(unsigned short glyph) const
{
    if (!m_platformData.size())
        return FloatRect();

    FT_Face face = m_platformData.scaledFont()->fontFace();
    FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);

    // FIXME: We are also ignoring possible error status, which cairo checks for
    FT_Glyph_Metrics metrics = face->glyph->metrics;
    return FloatRect(fromFTPixelPosition(metrics.horiBearingX), fromFTPixelPosition(metrics.horiBearingY), fromFTPixelPosition(metrics.width), fromFTPixelPosition(metrics.height));
}

float Font::platformWidthForGlyph(Glyph glyph) const
{
    if (!m_platformData.size())
        return 0;

    FT_Face face = m_platformData.scaledFont()->fontFace();
    bool hasHorizontalOrientation = platformData().orientation() == Horizontal;

    FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);

    float width = fromFTPixelPosition(hasHorizontalOrientation ? face->glyph->metrics.horiAdvance : -face->glyph->metrics.vertAdvance);
    // FIXME: See same concerns mentioned in platformBoundsForGlyph
    return width ? width : m_spaceWidth;
}

void Font::platformCharWidthInit()
{
    m_avgCharWidth = 0;
    m_maxCharWidth = 0;
    initCharWidths();
    // Now we know maximum char size and we can estimate the texture atlas size.
    m_platformData.textureFont()->createAtlas();
}

#if USE(HARFBUZZ)
bool Font::canRenderCombiningCharacterSequence(const UChar* characters, size_t length) const
{
    if (!m_combiningCharacterSequenceSupport)
        m_combiningCharacterSequenceSupport = std::make_unique<HashMap<String, bool>>();

    WTF::HashMap<String, bool>::AddResult addResult = m_combiningCharacterSequenceSupport->add(String(characters, length), false);
    if (!addResult.isNewEntry)
        return addResult.iterator->value;

    UErrorCode error = U_ZERO_ERROR;
    Vector<UChar, 4> normalizedCharacters(length);
    int32_t normalizedLength = unorm_normalize(characters, length, UNORM_NFC, UNORM_UNICODE_3_2, &normalizedCharacters[0], length, &error);
    // Can't render if we have an error or no composition occurred.
    if (U_FAILURE(error) || (static_cast<size_t>(normalizedLength) == length))
        return false;

    FT_Face face = m_platformData.scaledFont()->fontFace();
    if (!face)
        return false;

    if (FcFreeTypeCharIndex(face, normalizedCharacters[0]))
        addResult.iterator->value = true;

    return addResult.iterator->value;
}
#endif

} // namespace WebCore
