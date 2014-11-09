/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
 * Copyright (C) 2010 Igalia S.L.
 * All rights reserved.
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

#ifndef FontPlatformDataFreeType_h
#define FontPlatformDataFreeType_h

#include "FontCache.h"
#include "FontDescription.h"
#include "FontOrientation.h"
#include "GlyphBuffer.h"
#include "OpenTypeVerticalData.h"

#if USE(HARFBUZZ)
#include "HarfBuzzFace.h"
class HarfBuzzFace;
#endif

#if USE(CAIRO)
#include "RefPtrCairo.h"
typedef cairo_scaled_font_t scaled_font;
#elif USE(TYGL)
#include "ScaledFontTyGL.h"
#include "TextureFontTyGL.h"
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
typedef WebCore::ScaledFont scaled_font;
#endif

#include "SharedBuffer.h"
#include <wtf/Forward.h>
#include <wtf/HashFunctions.h>

typedef struct _FcFontSet FcFontSet;

namespace WebCore {

class FontPlatformData {
public:
    FontPlatformData(WTF::HashTableDeletedValueType hashTableDeletedValue)
        : m_fallbacks(0)
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_scaledFont(hashTableDeletedFontValue())
        , m_orientation(Horizontal)
        { }

    FontPlatformData()
        : m_fallbacks(0)
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_scaledFont(0)
        , m_orientation(Horizontal)
        { }

    FontPlatformData(FcPattern*, const FontDescription&);
#if USE(CAIRO)
    FontPlatformData(cairo_font_face_t*, float size, bool bold, bool italic, FontOrientation);
#elif USE(TYGL)
    FontPlatformData(FT_Face fontFace, float size, bool bold, bool italic, FontOrientation);
#endif
    FontPlatformData(float size, bool bold, bool italic);
    FontPlatformData(const FontPlatformData&);
    FontPlatformData(const FontPlatformData&, float size);

    ~FontPlatformData();

#if USE(HARFBUZZ)
    HarfBuzzFace* harfBuzzFace() const;
#endif

    bool isFixedPitch();
    float size() const { return m_size; }
    void setSize(float size) { m_size = size; }
    bool syntheticBold() const { return m_syntheticBold; }
    bool syntheticOblique() const { return m_syntheticOblique; }
    bool hasCompatibleCharmap();

    FontOrientation orientation() const { return m_orientation; }
    void setOrientation(FontOrientation);
    PassRefPtr<SharedBuffer> openTypeTable(uint32_t table) const;
    PassRefPtr<OpenTypeVerticalData> verticalData() const;

#if USE(CAIRO)
    cairo_scaled_font_t* scaledFont() const { return m_scaledFont; }
#elif USE(TYGL)
    scaled_font* scaledFont() const { return m_scaledFont.get(); }
#endif

    unsigned hash() const
    {
#if USE(TYGL)
        return FcPatternHash(m_pattern.get());
#else
        return PtrHash<scaled_font*>::hash(scaledFont());
#endif
    }

    bool operator==(const FontPlatformData&) const;
    FontPlatformData& operator=(const FontPlatformData&);
    bool isHashTableDeletedValue() const
    {
        return m_scaledFont == hashTableDeletedFontValue();
    }

#ifndef NDEBUG
    String description() const;
#endif

    RefPtr<FcPattern> m_pattern;
    mutable FcFontSet* m_fallbacks; // Initialized lazily.
    float m_size;
    bool m_syntheticBold;
    bool m_syntheticOblique;
    bool m_fixedWidth;
#if USE(CAIRO)
    cairo_scaled_font_t* m_scaledFont;
#elif USE(TYGL)
    RefPtr<scaled_font> m_scaledFont;
#endif

#if USE(HARFBUZZ)
    mutable RefPtr<HarfBuzzFace> m_harfBuzzFace;
#endif
#if USE(TYGL)
    TyGL::TextureFont* textureFont() const { return m_textureFont.get(); }
    TyGL::AffineTransform* horizontalOrientationTransform() { return &m_horizontalOrientationTransform; }
#endif

private:
#if USE(CAIRO)
    void initializeWithFontFace(cairo_font_face_t*, const FontDescription& = FontDescription());
#elif USE(TYGL)
    void initializeWithFontFace(FT_Face, const FontDescription& = FontDescription());
#endif
    static scaled_font* hashTableDeletedFontValue() { return reinterpret_cast<scaled_font*>(-1); }

    FontOrientation m_orientation;
#if USE(CAIRO)
    cairo_matrix_t m_horizontalOrientationMatrix;
#endif

#if USE(TYGL)
    mutable RefPtr<TyGL::TextureFont> m_textureFont;
    TyGL::AffineTransform m_horizontalOrientationTransform;
    FloatPoint m_horizontalNormalisedTranslationVector;
#endif
};

}

#endif // FontPlatformDataFreeType_h
