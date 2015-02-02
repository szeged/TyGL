/*
 * Copyright (C) 2006 Apple Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2009, 2010 Igalia S.L.
 * Copyright (C) 2014 University of Szeged
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
 */

#include "config.h"
#include "FontPlatformData.h"

#include "FontDescription.h"
#if USE(CAIRO)
#include <cairo-ft.h>
#include <cairo.h>
#elif USE(TYGL)
#include "ScaledFontTyGL.h"
#include "TextureFontTyGL.h"
#endif
#include <fontconfig/fcfreetype.h>
#include <freetype/tttables.h>
#include <ft2build.h>
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

#if !PLATFORM(EFL)
#include <gdk/gdk.h>
#endif

namespace WebCore {

#if USE(CAIRO)
cairo_subpixel_order_t convertFontConfigSubpixelOrder(int fontConfigOrder)
{
    switch (fontConfigOrder) {
    case FC_RGBA_RGB:
        return CAIRO_SUBPIXEL_ORDER_RGB;
    case FC_RGBA_BGR:
        return CAIRO_SUBPIXEL_ORDER_BGR;
    case FC_RGBA_VRGB:
        return CAIRO_SUBPIXEL_ORDER_VRGB;
    case FC_RGBA_VBGR:
        return CAIRO_SUBPIXEL_ORDER_VBGR;
    case FC_RGBA_NONE:
    case FC_RGBA_UNKNOWN:
        return CAIRO_SUBPIXEL_ORDER_DEFAULT;
    }
    return CAIRO_SUBPIXEL_ORDER_DEFAULT;
}

cairo_hint_style_t convertFontConfigHintStyle(int fontConfigStyle)
{
    switch (fontConfigStyle) {
    case FC_HINT_NONE:
        return CAIRO_HINT_STYLE_NONE;
    case FC_HINT_SLIGHT:
        return CAIRO_HINT_STYLE_SLIGHT;
    case FC_HINT_MEDIUM:
        return CAIRO_HINT_STYLE_MEDIUM;
    case FC_HINT_FULL:
        return CAIRO_HINT_STYLE_FULL;
    }
    return CAIRO_HINT_STYLE_NONE;
}

void setCairoFontOptionsFromFontConfigPattern(cairo_font_options_t* options, FcPattern* pattern)
{
    FcBool booleanResult;
    int integerResult;

    if (FcPatternGetInteger(pattern, FC_RGBA, 0, &integerResult) == FcResultMatch) {
        cairo_font_options_set_subpixel_order(options, convertFontConfigSubpixelOrder(integerResult));

        // Based on the logic in cairo-ft-font.c in the cairo source, a font with
        // a subpixel order implies that is uses subpixel antialiasing.
        if (integerResult != FC_RGBA_NONE)
            cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
    }

    if (FcPatternGetBool(pattern, FC_ANTIALIAS, 0, &booleanResult) == FcResultMatch) {
        // Only override the anti-aliasing setting if was previously turned off. Otherwise
        // we'll override the preference which decides between gray anti-aliasing and
        // subpixel anti-aliasing.
        if (!booleanResult)
            cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_NONE);
        else if (cairo_font_options_get_antialias(options) == CAIRO_ANTIALIAS_NONE)
            cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
    }

    if (FcPatternGetInteger(pattern, FC_HINT_STYLE, 0, &integerResult) == FcResultMatch)
        cairo_font_options_set_hint_style(options, convertFontConfigHintStyle(integerResult));
    if (FcPatternGetBool(pattern, FC_HINTING, 0, &booleanResult) == FcResultMatch && !booleanResult)
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
}

static cairo_font_options_t* getDefaultFontOptions()
{
#if PLATFORM(GTK)
    if (GdkScreen* screen = gdk_screen_get_default()) {
        const cairo_font_options_t* screenOptions = gdk_screen_get_font_options(screen);
        if (screenOptions)
            return cairo_font_options_copy(screenOptions);
    }
#endif
    return cairo_font_options_create();
}

static void rotateCairoMatrixForVerticalOrientation(cairo_matrix_t* matrix)
{
    // The resulting transformation matrix for vertical glyphs (V) is a
    // combination of rotation (R) and translation (T) applied on the
    // horizontal matrix (H). V = H . R . T, where R rotates by -90 degrees
    // and T translates by font size towards y axis.
    cairo_matrix_rotate(matrix, -piOverTwoDouble);
    cairo_matrix_translate(matrix, 0.0, 1.0);
}
#endif // USE(CAIRO)

FontPlatformData::FontPlatformData(FcPattern* pattern, const FontDescription& fontDescription)
    : m_pattern(pattern)
    , m_fallbacks(nullptr)
    , m_size(fontDescription.computedPixelSize())
    , m_syntheticBold(false)
    , m_syntheticOblique(false)
    , m_fixedWidth(false)
    , m_scaledFont(nullptr)
    , m_orientation(fontDescription.orientation())
{
#if USE(CAIRO)
    RefPtr<cairo_font_face_t> fontFace = adoptRef(cairo_ft_font_face_create_for_pattern(m_pattern.get()));
    initializeWithFontFace(fontFace.get(), fontDescription);
#elif USE(TYGL)
    m_scaledFont = ScaledFont::create(m_pattern.get());
    m_textureFont = TyGL::TextureFont::create(m_scaledFont);
    initializeWithFontFace(m_scaledFont->fontFace(), fontDescription);
#endif

    int spacing;
    if (FcPatternGetInteger(pattern, FC_SPACING, 0, &spacing) == FcResultMatch && spacing == FC_MONO)
        m_fixedWidth = true;

    if (fontDescription.weight() >= FontWeightBold) {
        // The FC_EMBOLDEN property instructs us to fake the boldness of the font.
        FcBool fontConfigEmbolden = FcFalse;
        if (FcPatternGetBool(pattern, FC_EMBOLDEN, 0, &fontConfigEmbolden) == FcResultMatch)
            m_syntheticBold = fontConfigEmbolden;

        // Fallback fonts may not have FC_EMBOLDEN activated even though it's necessary.
        int weight = 0;
        if (!m_syntheticBold && FcPatternGetInteger(pattern, FC_WEIGHT, 0, &weight) == FcResultMatch)
            m_syntheticBold = m_syntheticBold || weight < FC_WEIGHT_DEMIBOLD;
    }
}

FontPlatformData::FontPlatformData(float size, bool bold, bool italic)
    : m_fallbacks(nullptr)
    , m_size(size)
    , m_syntheticBold(bold)
    , m_syntheticOblique(italic)
    , m_fixedWidth(false)
    , m_scaledFont(nullptr)
    , m_orientation(Horizontal)
{
    // We cannot create a scaled font here.
}

#if USE(CAIRO)
FontPlatformData::FontPlatformData(cairo_font_face_t* fontFace, float size, bool bold, bool italic, FontOrientation orientation)

#elif USE(TYGL)
FontPlatformData::FontPlatformData(FT_Face fontFace, float size, bool bold, bool italic, FontOrientation orientation)
#endif
    : m_fallbacks(nullptr)
    , m_size(size)
    , m_syntheticBold(bold)
    , m_syntheticOblique(italic)
    , m_fixedWidth(false)
    , m_scaledFont(nullptr)
    , m_orientation(orientation)
{
#if USE(TYGL)
    RefPtr<FcPattern> pattern = adoptRef(FcPatternCreate());
    FcPatternAddFTFace(pattern.get(), FC_FT_FACE, fontFace);
    m_scaledFont = ScaledFont::create(pattern.get());
    m_textureFont = TyGL::TextureFont::create(m_scaledFont);
#endif
    initializeWithFontFace(fontFace);

#if USE(CAIRO)
    FT_Face fontConfigFace = cairo_ft_scaled_font_lock_face(m_scaledFont);
    if (fontConfigFace) {
        m_fixedWidth = fontConfigFace->face_flags & FT_FACE_FLAG_FIXED_WIDTH;
        cairo_ft_scaled_font_unlock_face(m_scaledFont);
    }
#endif
}

FontPlatformData& FontPlatformData::operator=(const FontPlatformData& other)
{
    // Check for self-assignment.
    if (this == &other)
        return *this;

    m_size = other.m_size;
    m_syntheticBold = other.m_syntheticBold;
    m_syntheticOblique = other.m_syntheticOblique;
    m_fixedWidth = other.m_fixedWidth;
    m_pattern = other.m_pattern;
    m_orientation = other.m_orientation;
#if USE(HARFBUZZ)
    m_harfBuzzFace = other.m_harfBuzzFace;
#endif

    if (m_fallbacks) {
        FcFontSetDestroy(m_fallbacks);
        // This will be re-created on demand.
        m_fallbacks = nullptr;
    }

#if USE(CAIRO)
    if (m_scaledFont && m_scaledFont != hashTableDeletedFontValue())
        cairo_scaled_font_destroy(m_scaledFont);
    m_scaledFont = cairo_scaled_font_reference(other.m_scaledFont);
    m_horizontalOrientationMatrix = other.m_horizontalOrientationMatrix;
#elif USE(TYGL)
    m_scaledFont = other.m_scaledFont;
    m_textureFont = other.m_textureFont;
#endif

    return *this;
}

FontPlatformData::FontPlatformData(const FontPlatformData& other)
    : m_fallbacks(nullptr)
    , m_scaledFont(nullptr)
    , m_harfBuzzFace(other.m_harfBuzzFace)
{
    *this = other;
}

FontPlatformData::FontPlatformData(const FontPlatformData& other, float size)
    : m_fallbacks(nullptr)
    , m_scaledFont(nullptr)

#if USE(HARFBUZZ)
    , m_harfBuzzFace(other.m_harfBuzzFace)
#endif
{
    *this = other;

    // We need to reinitialize the instance, because the difference in size 
    // necessitates a new scaled font instance.
    m_size = size;

#if USE(CAIRO)
    initializeWithFontFace(cairo_scaled_font_get_font_face(m_scaledFont));
#elif USE(TYGL)
    FT_Face fontFace = m_scaledFont->fontFace();
    RefPtr<FcPattern> pattern = adoptRef(FcPatternCreate());
    FcPatternAddFTFace(pattern.get(), FC_FT_FACE, fontFace);
    m_scaledFont = ScaledFont::create(pattern.get());
    m_textureFont = TyGL::TextureFont::create(m_scaledFont);
    initializeWithFontFace(fontFace);
#endif
}

FontPlatformData::~FontPlatformData()
{
    if (m_fallbacks) {
        FcFontSetDestroy(m_fallbacks);
        m_fallbacks = nullptr;
    }

    if (m_scaledFont && m_scaledFont != hashTableDeletedFontValue())
#if USE(CAIRO)
        cairo_scaled_font_destroy(m_scaledFont);
#elif USE(TYGL)
        m_scaledFont->deref();
#endif
}

#if USE(HARFBUZZ)
HarfBuzzFace* FontPlatformData::harfBuzzFace() const
{
    if (!m_harfBuzzFace)
        m_harfBuzzFace = HarfBuzzFace::create(const_cast<FontPlatformData*>(this), hash());

    return m_harfBuzzFace.get();
}
#endif

bool FontPlatformData::isFixedPitch()
{
    return m_fixedWidth;
}

bool FontPlatformData::operator==(const FontPlatformData& other) const
{
    // FcPatternEqual does not support null pointers as arguments.
    if ((m_pattern && !other.m_pattern)
        || (!m_pattern && other.m_pattern)
        || (m_pattern != other.m_pattern && !FcPatternEqual(m_pattern.get(), other.m_pattern.get())))
        return false;

    bool scaledFontsAreEqual = m_scaledFont == other.m_scaledFont;

#if USE(TYGL)
    scaledFontsAreEqual = (!m_scaledFont && !other.m_scaledFont)
        || ((m_scaledFont && other.m_scaledFont) && (*m_scaledFont == *other.m_scaledFont) && (m_textureFont == other.m_textureFont));
#endif

    return scaledFontsAreEqual
        && m_size == other.m_size
        && m_syntheticOblique == other.m_syntheticOblique
        && m_orientation == other.m_orientation
        && m_syntheticBold == other.m_syntheticBold;
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return String();
}
#endif

#if USE(CAIRO)
void FontPlatformData::initializeWithFontFace(cairo_font_face_t* fontFace, const FontDescription& fontDescription)
{
    cairo_font_options_t* options = getDefaultFontOptions();

    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    // Scaling a font with width zero size leads to a failed cairo_scaled_font_t instantiations.
    // Instead we scale we scale the font to a very tiny size and just abort rendering later on.
    float realSize = m_size ? m_size : 1;

    cairo_matrix_t fontMatrix;
    if (!m_pattern)
        cairo_matrix_init_scale(&fontMatrix, realSize, realSize);
    else {
        setCairoFontOptionsFromFontConfigPattern(options, m_pattern.get());

        // FontConfig may return a list of transformation matrices with the pattern, for instance,
        // for fonts that are oblique. We use that to initialize the cairo font matrix.
        FcMatrix fontConfigMatrix, *tempFontConfigMatrix;
        FcMatrixInit(&fontConfigMatrix);

        // These matrices may be stacked in the pattern, so it's our job to get them all and multiply them.
        for (int i = 0; FcPatternGetMatrix(m_pattern.get(), FC_MATRIX, i, &tempFontConfigMatrix) == FcResultMatch; i++)
            FcMatrixMultiply(&fontConfigMatrix, &fontConfigMatrix, tempFontConfigMatrix);
        cairo_matrix_init(&fontMatrix, fontConfigMatrix.xx, -fontConfigMatrix.yx,
                          -fontConfigMatrix.xy, fontConfigMatrix.yy, 0, 0);

        // We requested an italic font, but Fontconfig gave us one that was neither oblique nor italic.
        int actualFontSlant;
        if (fontDescription.italic() && FcPatternGetInteger(m_pattern.get(), FC_SLANT, 0, &actualFontSlant) == FcResultMatch)
            m_syntheticOblique = actualFontSlant == FC_SLANT_ROMAN;

        // The matrix from FontConfig does not include the scale. 
        cairo_matrix_scale(&fontMatrix, realSize, realSize);
    }

    if (syntheticOblique()) {
        static const float syntheticObliqueSkew = -tanf(14 * acosf(0) / 90);
        cairo_matrix_t skew = {1, 0, syntheticObliqueSkew, 1, 0, 0};
        cairo_matrix_multiply(&fontMatrix, &skew, &fontMatrix);
    }

    m_horizontalOrientationMatrix = fontMatrix;
    if (m_orientation == Vertical)
        rotateCairoMatrixForVerticalOrientation(&fontMatrix);

    m_scaledFont = cairo_scaled_font_create(fontFace, &fontMatrix, &ctm, options);
    cairo_font_options_destroy(options);
}
#elif USE(TYGL)
void FontPlatformData::initializeWithFontFace(FT_Face fontFace, const FontDescription& fontDescription)
{
    TyGL::AffineTransform fontTransform;
    if (m_pattern)
    {
        // FontConfig may return a list of transformation matrices with the pattern, for instance,
        // for fonts that are oblique. We use that to initialize the font matrix.
        FcMatrix fontConfigMatrix, *tempFontConfigMatrix;
        FcMatrixInit(&fontConfigMatrix);

        // These matrices may be stacked in the pattern, so it's our job to get them all and multiply them.
        for (int i = 0; FcPatternGetMatrix(m_pattern.get(), FC_MATRIX, i, &tempFontConfigMatrix) == FcResultMatch; i++)
            FcMatrixMultiply(&fontConfigMatrix, &fontConfigMatrix, tempFontConfigMatrix);

        TyGL::AffineTransform::Transform matrix = { { fontConfigMatrix.xx, -fontConfigMatrix.yx, -fontConfigMatrix.yx, fontConfigMatrix.yy, 0, 0 } };
        fontTransform = matrix;

        // We requested an italic font, but Fontconfig gave us one that was neither oblique nor italic.
        int actualFontSlant;
        if (fontDescription.italic() && FcPatternGetInteger(m_pattern.get(), FC_SLANT, 0, &actualFontSlant) == FcResultMatch)
            m_syntheticOblique = actualFontSlant == FC_SLANT_ROMAN;
    }

    if (syntheticOblique()) {
        static const float syntheticObliqueSkew = -tanf(14 * acosf(0) / 90);
        TyGL::AffineTransform::Transform matrix = {1, 0, syntheticObliqueSkew, 1, 0, 0};
        fontTransform.multiply(matrix);
    }

    m_horizontalOrientationTransform = fontTransform;
    m_textureFont->setTransform(m_horizontalOrientationTransform);
    if (m_orientation == Vertical) {
        TyGL::AffineTransform::Transform matrix = { { cos(-M_PI_2), sin(-M_PI_2), -sin(-M_PI_2), cos(-M_PI_2), -1, 0 } };
        fontTransform.multiply(matrix);
        m_textureFont->setTransform(fontTransform);
    }
}
#endif

bool FontPlatformData::hasCompatibleCharmap()
{
    ASSERT(m_scaledFont);
#if USE(CAIRO)
    FT_Face freeTypeFace = cairo_ft_scaled_font_lock_face(m_scaledFont);
#elif USE(TYGL)
    FT_Face freeTypeFace = m_scaledFont->fontFace();
#endif
    bool hasCompatibleCharmap = !(FT_Select_Charmap(freeTypeFace, ft_encoding_unicode)
                                && FT_Select_Charmap(freeTypeFace, ft_encoding_symbol)
                                && FT_Select_Charmap(freeTypeFace, ft_encoding_apple_roman));
#if USE(CAIRO)
    cairo_ft_scaled_font_unlock_face(m_scaledFont);
#endif
    return hasCompatibleCharmap;
}

PassRefPtr<OpenTypeVerticalData> FontPlatformData::verticalData() const
{
    ASSERT(hash());
    return fontCache().getVerticalData(String::number(hash()), *this);
}

PassRefPtr<SharedBuffer> FontPlatformData::openTypeTable(uint32_t table) const
{
#if USE(CAIRO)
    FT_Face freeTypeFace = cairo_ft_scaled_font_lock_face(m_scaledFont);
#elif USE(TYGL)
    FT_Face freeTypeFace = m_scaledFont->fontFace();
#endif
    if (!freeTypeFace)
        return 0;

    FT_ULong tableSize = 0;
    // Tag bytes need to be reversed because OT_MAKE_TAG uses big-endian order.
    uint32_t tag = FT_MAKE_TAG((table & 0xff), (table & 0xff00) >> 8, (table & 0xff0000) >> 16, table >> 24);
    if (FT_Load_Sfnt_Table(freeTypeFace, tag, 0, 0, &tableSize))
        return 0;

    RefPtr<SharedBuffer> buffer = SharedBuffer::create(tableSize);
    FT_ULong expectedTableSize = tableSize;
    if (buffer->size() != tableSize)
        return 0;

    FT_Error error = FT_Load_Sfnt_Table(freeTypeFace, tag, 0, reinterpret_cast<FT_Byte*>(const_cast<char*>(buffer->data())), &tableSize);
    if (error || tableSize != expectedTableSize)
        return 0;

#if USE(CAIRO)
    cairo_ft_scaled_font_unlock_face(m_scaledFont);
#endif

    return buffer.release();
}

void FontPlatformData::setOrientation(FontOrientation orientation)
{
    ASSERT(m_scaledFont);

    if (!m_scaledFont || (m_orientation == orientation))
        return;

#if USE(CAIRO)
    cairo_matrix_t transformationMatrix;
    cairo_matrix_init_identity(&transformationMatrix);

    cairo_matrix_t fontMatrix;
    cairo_scaled_font_get_font_matrix(m_scaledFont, &fontMatrix);

    cairo_font_options_t* options = getDefaultFontOptions();

    // In case of vertical orientation, rotate the transformation matrix.
    // Otherwise restore the horizontal orientation matrix.
    if (orientation == Vertical)
        rotateCairoMatrixForVerticalOrientation(&fontMatrix);
    else
        fontMatrix = m_horizontalOrientationMatrix;

    cairo_font_face_t* fontFace = cairo_scaled_font_get_font_face(m_scaledFont);
    cairo_scaled_font_destroy(m_scaledFont);
    m_scaledFont = cairo_scaled_font_create(fontFace, &fontMatrix, &transformationMatrix, options);
    cairo_font_options_destroy(options);
#elif USE(TYGL)
    m_textureFont->setTransform(m_horizontalOrientationTransform);
    if (orientation == Vertical) {
        TyGL::AffineTransform transform = m_horizontalOrientationTransform;
        TyGL::AffineTransform::Transform matrix = { { cos(-M_PI_2), sin(-M_PI_2), -sin(-M_PI_2), cos(-M_PI_2), -1, 0 } };
        transform.multiply(matrix);
        m_textureFont->setTransform(transform);
    }
#endif
    m_orientation = orientation;
}

}
