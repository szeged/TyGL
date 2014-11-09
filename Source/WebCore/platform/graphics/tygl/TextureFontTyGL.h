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

#ifndef TextureFontTyGL_h
#define TextureFontTyGL_h

#include "FloatPoint.h"
#include "FloatRect.h"
#include "FontTextureAtlasTyGL.h"
#include "ScaledFontTyGL.h"
#include "TransformTyGL.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class GlyphBuffer;
class PlatformContextTyGL;
class SimpleFontData;

namespace TyGL {

class TextureFont: public RefCounted<TextureFont> {
public:
    // From fterrdef.h 0xBA is last code. So we define atlas full error on top of existing errors range.
    static const int AtlasFullError = 0x100;
    // From fterrdef.h 0x40 is out of memory error code.
    static const int OutOfMemoryError = 0x40;

    struct AtlasRegion {
        // Top left texture coordinate.
        GLfloat s0;
        GLfloat t0;

        // Bottom right texture coordinate.
        GLfloat s1;
        GLfloat t1;

        AtlasRegion()
            : s0(0)
            , t0(0)
            , s1(0)
            , t1(0)
        {
        }

        AtlasRegion(const float s0, const float t0, const float s1, const float t1)
            : s0(s0)
            , t0(t0)
            , s1(s1)
            , t1(t1)
        {
        }
    };

    struct GlyphMetrics {
        // Glyph's size in pixels.
        IntSize size;

        // Glyph's left/top offset in pixels.
        IntPoint offset;

        // Glyph's location in atlas.
        IntPoint location;

        // Cached glyph's atlas texture region relative coordinates.
        AtlasRegion atlasRegion;

        GlyphMetrics()
            : size()
            , offset()
            , location()
            , atlasRegion()
        {
        }
    };

    static PassRefPtr<TextureFont> create(PassRefPtr<WebCore::ScaledFont> scaledFont) { return adoptRef(new TextureFont(scaledFont)); }
    const GlyphMetrics& glyph(const int glyphCode, int& errorCode);
    WebCore::SimpleFontData* simpleFontData() const { return m_simpleFontData; }
    void setSimpleFontData(WebCore::SimpleFontData* simpleFontData) { m_simpleFontData = simpleFontData; }
    WebCore::ScaledFont* scaledFont() const { return m_scaledFont.get(); }
    FontTextureAtlasTyGL* fontTextureAtlas() { return m_textureAtlas.get(); }
    bool createAtlas();
    bool expandAtlas();
    const TyGL::AffineTransform& transform() const { return m_transform; }
    void setTransform(const TyGL::AffineTransform& transform) { m_transform = transform; }
    const FloatPoint& normalisedTranslateVector() const { return m_normalisedTranslateVector; }
    void setNormalisedTranslateVector(const FloatPoint& offset) { m_normalisedTranslateVector = offset; }

private:
    TextureFont(PassRefPtr<WebCore::ScaledFont> scaledFont)
        : m_simpleFontData(0)
        , m_scaledFont(scaledFont)
    {
    }
    IntSize calculateAtlasSize();

    RefPtr<FontTextureAtlasTyGL> m_textureAtlas;
    typedef HashMap<uint64_t, GlyphMetrics, WTF::IntHash<uint64_t>, WTF::UnsignedWithZeroKeyHashTraits<uint64_t> > GlyphMetricsCache;
    GlyphMetricsCache m_glyphs;
    WebCore::SimpleFontData* m_simpleFontData;
    RefPtr<WebCore::ScaledFont> m_scaledFont;
    TyGL::AffineTransform m_transform;
    FloatPoint m_normalisedTranslateVector;
};

class GlyphBufferFontData {
public:
    GlyphBufferFontData(TextureFont* textureFont, const GlyphBuffer* glyphBuffer, int fromIndex, int numGlyphs, const FloatPoint& point)
        : m_textureFont(textureFont)
        , m_glyphBuffer(glyphBuffer)
        , m_fromIndex(fromIndex)
        , m_numGlyphs(numGlyphs)
        , m_point(point)
    {
    }

    PassRefPtr<NativeImageTyGL> atlasTexture() const { return m_textureFont->fontTextureAtlas(); }
    FloatRect calculateBoundingBox() const;
    TextureFont* textureFont() { return m_textureFont; }
    const GlyphBuffer* glyphBuffer() { return m_glyphBuffer; }
    int fromIndex() { return m_fromIndex; }
    int numberOfGlyphs() { return m_numGlyphs; }
    FloatPoint location() { return m_point; }

private:
    mutable TextureFont* m_textureFont;
    const GlyphBuffer* m_glyphBuffer;

    int m_fromIndex;
    int m_numGlyphs;
    FloatPoint m_point;
};

} // namespace TyGL
} // namespace WebCore

#endif // TextureFontTyGL_h
