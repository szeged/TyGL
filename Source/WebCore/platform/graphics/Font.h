/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2008, 2010, 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef Font_h
#define Font_h

#include "FloatRect.h"
#include "FontBaseline.h"
#include "FontMetrics.h"
#include "FontPlatformData.h"
#include "GlyphBuffer.h"
#include "GlyphMetricsMap.h"
#include "GlyphPage.h"
#include "OpenTypeMathData.h"
#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif
#include "TypesettingFeatures.h"
#include <wtf/TypeCasts.h>
#include <wtf/text/StringHash.h>

#if PLATFORM(COCOA)
#include "WebCoreSystemInterface.h"
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(WIN)
#include <usp10.h>
#endif

#if USE(CAIRO)
#include <cairo.h>
#endif

#if USE(CG)
#include <WebCore/CoreGraphicsSPI.h>
#endif

namespace WebCore {

class GlyphPage;
class FontDescription;
class SharedBuffer;
struct GlyphData;
struct WidthIterator;

enum FontVariant { AutoVariant, NormalVariant, SmallCapsVariant, EmphasisMarkVariant, BrokenIdeographVariant };
enum Pitch { UnknownPitch, FixedPitch, VariablePitch };

class Font : public RefCounted<Font> {
public:
    class SVGData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual ~SVGData() { }

        virtual void initializeFont(Font*, float fontSize) = 0;
        virtual float widthForSVGGlyph(Glyph, float fontSize) const = 0;
        virtual bool fillSVGGlyphPage(GlyphPage*, unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const Font*) const = 0;
    };

    // Used to create platform fonts.
    static Ref<Font> create(const FontPlatformData& platformData, bool isCustomFont = false, bool isLoading = false, bool isTextOrientationFallback = false)
    {
        return adoptRef(*new Font(platformData, isCustomFont, isLoading, isTextOrientationFallback));
    }

    // Used to create SVG Fonts.
    static Ref<Font> create(std::unique_ptr<SVGData> svgData, float fontSize, bool syntheticBold, bool syntheticItalic)
    {
        return adoptRef(*new Font(WTF::move(svgData), fontSize, syntheticBold, syntheticItalic));
    }

    ~Font();

    static const Font* systemFallback() { return reinterpret_cast<const Font*>(-1); }

    const FontPlatformData& platformData() const { return m_platformData; }
    const OpenTypeMathData* mathData() const;
#if ENABLE(OPENTYPE_VERTICAL)
    const OpenTypeVerticalData* verticalData() const { return m_verticalData.get(); }
#endif

    PassRefPtr<Font> smallCapsFont(const FontDescription&) const;
    PassRefPtr<Font> emphasisMarkFont(const FontDescription&) const;
    PassRefPtr<Font> brokenIdeographFont() const;
    PassRefPtr<Font> nonSyntheticItalicFont() const;

    PassRefPtr<Font> variantFont(const FontDescription& description, FontVariant variant) const
    {
        switch (variant) {
        case SmallCapsVariant:
            return smallCapsFont(description);
        case EmphasisMarkVariant:
            return emphasisMarkFont(description);
        case BrokenIdeographVariant:
            return brokenIdeographFont();
        case AutoVariant:
        case NormalVariant:
            break;
        }
        ASSERT_NOT_REACHED();
        return const_cast<Font*>(this);
    }

    PassRefPtr<Font> verticalRightOrientationFont() const;
    PassRefPtr<Font> uprightOrientationFont() const;

    bool hasVerticalGlyphs() const { return m_hasVerticalGlyphs; }
    bool isTextOrientationFallback() const { return m_isTextOrientationFallback; }

    FontMetrics& fontMetrics() { return m_fontMetrics; }
    const FontMetrics& fontMetrics() const { return m_fontMetrics; }
    float sizePerUnit() const { return platformData().size() / (fontMetrics().unitsPerEm() ? fontMetrics().unitsPerEm() : 1); }

    float maxCharWidth() const { return m_maxCharWidth; }
    void setMaxCharWidth(float maxCharWidth) { m_maxCharWidth = maxCharWidth; }

    float avgCharWidth() const { return m_avgCharWidth; }
    void setAvgCharWidth(float avgCharWidth) { m_avgCharWidth = avgCharWidth; }

    FloatRect boundsForGlyph(Glyph) const;
    float widthForGlyph(Glyph) const;
    FloatRect platformBoundsForGlyph(Glyph) const;
    float platformWidthForGlyph(Glyph) const;

    float spaceWidth() const { return m_spaceWidth; }
    float adjustedSpaceWidth() const { return m_adjustedSpaceWidth; }
    void setSpaceWidths(float spaceWidth)
    {
        m_spaceWidth = spaceWidth;
        m_adjustedSpaceWidth = spaceWidth;
    }

#if USE(CG) || USE(CAIRO)
    float syntheticBoldOffset() const { return m_syntheticBoldOffset; }
#endif

    Glyph spaceGlyph() const { return m_spaceGlyph; }
    void setSpaceGlyph(Glyph spaceGlyph) { m_spaceGlyph = spaceGlyph; }
    Glyph zeroWidthSpaceGlyph() const { return m_zeroWidthSpaceGlyph; }
    void setZeroWidthSpaceGlyph(Glyph spaceGlyph) { m_zeroWidthSpaceGlyph = spaceGlyph; }
    bool isZeroWidthSpaceGlyph(Glyph glyph) const { return glyph == m_zeroWidthSpaceGlyph && glyph; }
    Glyph zeroGlyph() const { return m_zeroGlyph; }
    void setZeroGlyph(Glyph zeroGlyph) { m_zeroGlyph = zeroGlyph; }

    GlyphData glyphDataForCharacter(UChar32) const;
    Glyph glyphForCharacter(UChar32) const;

    RefPtr<Font> systemFallbackFontForCharacter(UChar32, const FontDescription&, bool isForPlatformFont) const;

    const GlyphPage* glyphPage(unsigned pageNumber) const;

    void determinePitch();
    Pitch pitch() const { return m_treatAsFixedPitch ? FixedPitch : VariablePitch; }

    const SVGData* svgData() const { return m_svgData.get(); }
    bool isSVGFont() const { return !!m_svgData; }

    bool isCustomFont() const { return m_isCustomFont; }
    bool isLoading() const { return m_isLoading; }

#ifndef NDEBUG
    String description() const;
#endif

#if USE(APPKIT)
    const Font* compositeFontReferenceFont(NSFont *key) const;
    NSFont* getNSFont() const { return m_platformData.nsFont(); }
#endif

#if PLATFORM(IOS)
    CTFontRef getCTFont() const { return m_platformData.font(); }
    bool shouldNotBeUsedForArabic() const { return m_shouldNotBeUsedForArabic; };
#endif
#if PLATFORM(COCOA)
    CFDictionaryRef getCFStringAttributes(TypesettingFeatures, FontOrientation) const;
#endif

#if PLATFORM(COCOA) || USE(HARFBUZZ)
    bool canRenderCombiningCharacterSequence(const UChar*, size_t) const;
#endif

    bool applyTransforms(GlyphBufferGlyph*, GlyphBufferAdvance*, size_t glyphCount, TypesettingFeatures) const;

#if PLATFORM(WIN)
    bool isSystemFont() const { return m_isSystemFont; }
    SCRIPT_FONTPROPERTIES* scriptFontProperties() const;
    SCRIPT_CACHE* scriptCache() const { return &m_scriptCache; }
    static void setShouldApplyMacAscentHack(bool);
    static bool shouldApplyMacAscentHack();
    static float ascentConsideringMacAscentHack(const WCHAR*, float ascent, float descent);
#endif

private:
    Font(const FontPlatformData&, bool isCustomFont = false, bool isLoading = false, bool isTextOrientationFallback = false);

    Font(std::unique_ptr<SVGData>, float fontSize, bool syntheticBold, bool syntheticItalic);

    void platformInit();
    void platformGlyphInit();
    void platformCharWidthInit();
    void platformDestroy();

    void initCharWidths();

    PassRefPtr<Font> createScaledFont(const FontDescription&, float scaleFactor) const;
    PassRefPtr<Font> platformCreateScaledFont(const FontDescription&, float scaleFactor) const;

    void removeFromSystemFallbackCache();

#if PLATFORM(WIN)
    void initGDIFont();
    void platformCommonDestroy();
    FloatRect boundsForGDIGlyph(Glyph) const;
    float widthForGDIGlyph(Glyph) const;
#endif

    FontMetrics m_fontMetrics;
    float m_maxCharWidth;
    float m_avgCharWidth;

    FontPlatformData m_platformData;
    std::unique_ptr<SVGData> m_svgData;

    mutable RefPtr<GlyphPage> m_glyphPageZero;
    mutable HashMap<unsigned, RefPtr<GlyphPage>> m_glyphPages;
    mutable std::unique_ptr<GlyphMetricsMap<FloatRect>> m_glyphToBoundsMap;
    mutable GlyphMetricsMap<float> m_glyphToWidthMap;

    bool m_treatAsFixedPitch;
    bool m_isCustomFont; // Whether or not we are custom font loaded via @font-face
    bool m_isLoading; // Whether or not this custom font is still in the act of loading.

    bool m_isTextOrientationFallback;
    bool m_isBrokenIdeographFallback;

    bool m_isUsedInSystemFallbackCache { false };

    mutable RefPtr<OpenTypeMathData> m_mathData;
#if ENABLE(OPENTYPE_VERTICAL)
    RefPtr<OpenTypeVerticalData> m_verticalData;
#endif
    bool m_hasVerticalGlyphs;

    Glyph m_spaceGlyph;
    float m_spaceWidth;
    Glyph m_zeroGlyph;
    float m_adjustedSpaceWidth;

    Glyph m_zeroWidthSpaceGlyph;

    struct DerivedFontData {
        explicit DerivedFontData(bool custom)
            : forCustomFont(custom)
        {
        }
        ~DerivedFontData();

        bool forCustomFont;
        RefPtr<Font> smallCaps;
        RefPtr<Font> emphasisMark;
        RefPtr<Font> brokenIdeograph;
        RefPtr<Font> verticalRightOrientation;
        RefPtr<Font> uprightOrientation;
        RefPtr<Font> nonSyntheticItalic;
#if USE(APPKIT)
        HashMap<NSFont*, RefPtr<Font>> compositeFontReferences;
#endif
    };

    mutable std::unique_ptr<DerivedFontData> m_derivedFontData;

#if USE(CG) || USE(CAIRO)
    float m_syntheticBoldOffset;
#endif

#if PLATFORM(COCOA)
    mutable HashMap<unsigned, RetainPtr<CFDictionaryRef>> m_CFStringAttributes;
#endif

#if PLATFORM(COCOA) || USE(HARFBUZZ)
    mutable std::unique_ptr<HashMap<String, bool>> m_combiningCharacterSequenceSupport;
#endif

#if PLATFORM(WIN)
    bool m_isSystemFont;
    mutable SCRIPT_CACHE m_scriptCache;
    mutable SCRIPT_FONTPROPERTIES* m_scriptFontProperties;
#endif
#if PLATFORM(IOS)
    bool m_shouldNotBeUsedForArabic;
#endif
};

ALWAYS_INLINE FloatRect Font::boundsForGlyph(Glyph glyph) const
{
    if (isZeroWidthSpaceGlyph(glyph))
        return FloatRect();

    FloatRect bounds;
    if (m_glyphToBoundsMap) {
        bounds = m_glyphToBoundsMap->metricsForGlyph(glyph);
        if (bounds.width() != cGlyphSizeUnknown)
            return bounds;
    }

    bounds = platformBoundsForGlyph(glyph);
    if (!m_glyphToBoundsMap)
        m_glyphToBoundsMap = std::make_unique<GlyphMetricsMap<FloatRect>>();
    m_glyphToBoundsMap->setMetricsForGlyph(glyph, bounds);
    return bounds;
}

ALWAYS_INLINE float Font::widthForGlyph(Glyph glyph) const
{
    if (isZeroWidthSpaceGlyph(glyph))
        return 0;

    float width = m_glyphToWidthMap.metricsForGlyph(glyph);
    if (width != cGlyphSizeUnknown)
        return width;

    if (isSVGFont())
        width = m_svgData->widthForSVGGlyph(glyph, m_platformData.size());
#if ENABLE(OPENTYPE_VERTICAL)
    else if (m_verticalData)
#if USE(CG) || USE(CAIRO)
        width = m_verticalData->advanceHeight(this, glyph) + m_syntheticBoldOffset;
#else
        width = m_verticalData->advanceHeight(this, glyph);
#endif
#endif
    else
        width = platformWidthForGlyph(glyph);

    m_glyphToWidthMap.setMetricsForGlyph(glyph, width);
    return width;
}

} // namespace WebCore

#endif // Font_h
