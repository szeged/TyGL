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
#include "PlatformContextTyGL.h"

#include "Font.h"
#include "GlyphBuffer.h"
#include "ShaderCommonTyGL.h"
#include "TextureFontTyGL.h"

namespace WebCore {

using TyGL::min;
using TyGL::max;

#define FONT_SHADER_ARGUMENTS(argument) \
    argument(u, texture, Texture, 1) \
    argument(a, position, Position, 4)

DEFINE_SHADER(
    FontShader,
    FONT_SHADER_ARGUMENTS,
    PlatformContextTyGL::kUniformSolidColoring | PlatformContextTyGL::kComplexFill,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_position;

        varying vec2 v_texturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_texturePosition = a_position.zw;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_texture;
        varying vec2 v_texturePosition;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec4 resultColor = vec4(0.0, 0.0, 0.0, texture2D(u_texture, v_texturePosition).a);
    )
)

GLfloat* PlatformContextTyGL::fontSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setPrimaryTexture(shaderArguments[FontShader::uTexture]);
    glVertexAttribPointer(shaderArguments[FontShader::aPosition], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 4;
}

#define TRANSFORMED_FONT_SHADER_ARGUMENTS(argument) \
    argument(u, texture, Texture, 1) \
    argument(a, position, Position, 4) \
    argument(a, texturePosition, TexturePosition, 4)

DEFINE_SHADER(
    TransformedFontShader,
    TRANSFORMED_FONT_SHADER_ARGUMENTS,
    PlatformContextTyGL::kUniformSolidColoring,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_position;
        attribute vec4 a_texturePosition;

        varying vec2 v_relativePosition;
        varying vec4 v_texturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_relativePosition = a_position.zw;
        v_texturePosition = a_texturePosition;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_texture;
        varying vec2 v_relativePosition;
        varying vec4 v_texturePosition;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        float s = v_relativePosition[0];
        float t = v_relativePosition[1];

        vec4 temporaryColor = vec4(0.0, 0.0, 0.0, 0.0);
        if (s == clamp(s, 0.0, 1.0) && t == clamp(t, 0.0, 1.0))
            temporaryColor.a = texture2D(u_texture, vec2(v_texturePosition[0] + s * v_texturePosition[2], v_texturePosition[1] + t * v_texturePosition[3])).a;
        vec4 resultColor = temporaryColor;
    )
)

bool PlatformContextTyGL::fontBeforeFlushAction(PlatformContextTyGL* context)
{
    reinterpret_cast<TyGL::FontTextureAtlasTyGL*>(context->pipelineBuffer()->beforeFlushAction.data)->update();
    return true;
}

GLfloat* PlatformContextTyGL::transformedFontSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    GLsizei strideLength = context->pipelineBuffer()->strideLength;
    context->setPrimaryTexture(shaderArguments[TransformedFontShader::uTexture]);
    glVertexAttribPointer(shaderArguments[TransformedFontShader::aPosition], 4, GL_FLOAT, GL_FALSE, strideLength, attributeStart);
    glVertexAttribPointer(shaderArguments[TransformedFontShader::aTexturePosition], 4, GL_FLOAT, GL_FALSE, strideLength, attributeStart + 4);
    return attributeStart + 8;
}

void PlatformContextTyGL::addTextAttributes(TyGL::GlyphBufferFontData& fontData, PassRefPtr<NativeImageTyGL> texture, const FloatRect& boundingBox, const IntRect& paintRect)
{
    int fromIndex = fontData.fromIndex();
    int numGlyphs = fontData.numberOfGlyphs();

    TyGL::TextureFont* textureFont = fontData.textureFont();
    const GlyphBuffer* glyphBuffer = fontData.glyphBuffer();
    GlyphBufferGlyph* glyphs = const_cast<GlyphBufferGlyph*>(glyphBuffer->glyphs(fromIndex));
    const FontMetrics& fontMetrics = textureFont->font()->fontMetrics();
    RefPtr<NativeImageTyGL> atlasTexture = texture;

    float xOffset = boundingBox.x();
    float yOffset = boundingBox.y() + fontMetrics.ascent();

    bool glyphTransformIsNeeded = !fontData.textureFont()->transform().isIdentity();
    bool textTransformIsNeeded = fontData.textureFont()->transform().type() != TyGL::AffineTransform::Move || transform().type() != TyGL::AffineTransform::Move;
    ShaderProgram* primaryShader = !textTransformIsNeeded ? &FontShader::s_program : &TransformedFontShader::s_program;
    ShaderVariableSetter primaryShaderVariableSetter = !textTransformIsNeeded ? fontSetShaderArguments : transformedFontSetShaderArguments;

    BeforeFlushAction beforeFlushAction(fontBeforeFlushAction, textureFont->fontTextureAtlas());
    beginAttributeAdding(primaryShader, atlasTexture, primaryShaderVariableSetter, paintRect, &beforeFlushAction);

    int i = 0;
    while (i < numGlyphs) {
        FT_Error errorCode = 0;
        const TyGL::TextureFont::GlyphMetrics& glyph = textureFont->glyph(glyphs[i], errorCode);
        TyGL::AffineTransform glyphTransform = fontData.textureFont()->transform();
        if (glyphTransformIsNeeded) {
            TyGL::AffineTransform::Transform matrix = glyphTransform.transform();
            matrix.m_matrix[4] *= glyph.size.width();
            matrix.m_matrix[5] *= glyph.size.height();
            glyphTransform = matrix;
        }

        if (!errorCode)
            addGlyphAttributes(FloatRect(FloatPoint(xOffset + glyph.offset.x(), yOffset - glyph.offset.y()), glyph.size), glyph, glyphTransform);
        else if (errorCode == TyGL::TextureFont::AtlasFullError) {
            // We have to flush current attribute buffer because after the atlas expansion relative atlas coordinates
            // in the buffer will become invalid.
            flushBatchingQueueIfNeeded();
            if (!textureFont->expandAtlas())
                return;

            // Texture atlas has changed.
            beforeFlushAction.data = textureFont->fontTextureAtlas();
            beginAttributeAdding(primaryShader, atlasTexture, primaryShaderVariableSetter, paintRect, &beforeFlushAction);
            continue;
        } else if (errorCode == TyGL::TextureFont::OutOfMemoryError)
            CRASH();

        // We get here if there is no error or error loading the glyph or other FreeType error. Move to the next glyph.
        xOffset += glyphBuffer->advanceAt(fromIndex + i).width();
        ++i;
    }
}

void PlatformContextTyGL::addGlyphAttributes(const FloatRect& destRect, const TyGL::TextureFont::GlyphMetrics& glyphMetrics, const TyGL::AffineTransform& glyphTransform)
{
    // TODO - make use of ClippedTransformedQuad.
    bool transformIsNeeded = glyphTransform.type() != TyGL::AffineTransform::Move || transform().type() != TyGL::AffineTransform::Move;

    FloatPoint coords[4];
    FloatRect outboundRect = transform().apply(destRect, coords, transformIsNeeded);
    TyGL::TextureFont::AtlasRegion atlasRegion = glyphMetrics.atlasRegion;
    GLfloat textureLeft = atlasRegion.s0;
    GLfloat textureBottom = atlasRegion.t0;
    GLfloat textureRight = atlasRegion.s1;
    GLfloat textureTop = atlasRegion.t1;
    GLfloat width = textureRight - textureLeft;
    GLfloat height = textureTop - textureBottom;

    if (!transformIsNeeded) {
        GLfloat v0X = coords[1].x() - coords[0].x();
        GLfloat v0Y = coords[1].y() - coords[0].y();
        GLfloat v1X = coords[2].x() - coords[0].x();
        GLfloat v1Y = coords[2].y() - coords[0].y();

        GLfloat left = coords[0].x();
        GLfloat bottom = coords[0].y();
        GLfloat right = coords[1].x();
        GLfloat top = coords[2].y();

        if (!boundingClipRect().intersects(left, bottom, right, top))
            return;

        // Clipping.
        GLfloat limit = boundingClipRect().left();
        if (left < limit) {
            textureLeft += ((limit - left) / v0X) * width;
            left = limit;
        }
        limit = boundingClipRect().bottom();
        if (bottom < limit) {
            textureBottom += ((limit - bottom) / v1Y) * height;
            bottom = limit;
        }
        limit = boundingClipRect().right();
        if (right > limit) {
            textureRight -= ((right - limit) / v0X) * width;
            right = limit;
        }
        limit = boundingClipRect().top();
        if (top > limit) {
            textureTop -= ((top - limit) / v1Y) * height;
            top = limit;
        }
        if (left >= right || bottom >= top)
            return;

        if (!hasFreeSpace())
            flushBatchingQueueIfNeeded(false);

        addAttribute(left, bottom, textureLeft, textureBottom, right, bottom, textureRight, textureBottom, left, top, textureLeft, textureTop, right, top, textureRight, textureTop);
        endAttributeAdding();
        return;
    }

    if (glyphTransform.type() != TyGL::AffineTransform::Move) {
        FloatPoint glyphOrigin(coords[0]);
        coords[0] += -glyphOrigin;
        coords[1] += -glyphOrigin;
        coords[2] += -glyphOrigin;
        coords[3] += -glyphOrigin;
        coords[0] = glyphTransform.apply(coords[0]);
        coords[1] = glyphTransform.apply(coords[1]);
        coords[2] = glyphTransform.apply(coords[2]);
        coords[3] = glyphTransform.apply(coords[3]);
        coords[0] += glyphOrigin;
        coords[1] += glyphOrigin;
        coords[2] += glyphOrigin;
        coords[3] += glyphOrigin;
        outboundRect = TyGL::AffineTransform::outboundRect(coords);
    }

    if (!boundingClipRect().intersects(outboundRect))
        return;

    GLfloat v0X = coords[1].x() - coords[0].x();
    GLfloat v0Y = coords[1].y() - coords[0].y();
    GLfloat v1X = coords[2].x() - coords[0].x();
    GLfloat v1Y = coords[2].y() - coords[0].y();
    GLfloat discriminant = v1X * v0Y - v1Y * v0X;

    // Parallel or zero length vectors.
    if (!discriminant)
        return;

    GLfloat minX = outboundRect.x();
    GLfloat minY = outboundRect.y();
    GLfloat maxX = outboundRect.maxX();
    GLfloat maxY = outboundRect.maxY();
    minX = max(minX, boundingClipRect().left());
    maxX = min(maxX, boundingClipRect().right());
    minY = max(minY, boundingClipRect().bottom());
    maxY = min(maxY, boundingClipRect().top());

    v1X /= discriminant;
    v1Y /= discriminant;
    v0X /= -discriminant;
    v0Y /= -discriminant;

    if (!hasFreeSpace(3))
        flushBatchingQueueIfNeeded(false);

    addAttributePosition(minX, minY, maxX, maxY);

    minX -= coords[0].x();
    maxX -= coords[0].x();
    minY -= coords[0].y();
    maxY -= coords[0].y();

    addAttribute(minY * v1X - minX * v1Y, minY * v0X - minX * v0Y,
        minY * v1X - maxX * v1Y, minY * v0X - maxX * v0Y,
        maxY * v1X - minX * v1Y, maxY * v0X - minX * v0Y,
        maxY * v1X - maxX * v1Y, maxY * v0X - maxX * v0Y);

    addAttributeRepeated(textureLeft, textureBottom, width, height);
    endAttributeAdding();
}

void PlatformContextTyGL::fillText(TyGL::GlyphBufferFontData& fontData)
{
    FloatRect boundingBox = fontData.calculateBoundingBox();
    if (boundingBox.isEmpty())
        return;

    FloatPoint coords[4];
    FloatRect outboundRect = transform().apply(boundingBox, coords);
    if (!boundingClipRect().intersects(outboundRect))
        return;

    RefPtr<NativeImageTyGL> atlasTexture = fontData.atlasTexture();
    if (!atlasTexture)
        return;

    addTextAttributes(fontData, atlasTexture, boundingBox, IntRect(outboundRect));
}

void PlatformContextTyGL::fillText(TyGL::TextureFont* textureFont, const GlyphBuffer* glyphBuffer, int fromIndex, int numGlyphs, const FloatPoint& point, Coloring coloring)
{
    m_coloring = &coloring;
    TyGL::GlyphBufferFontData fontData(textureFont, glyphBuffer, fromIndex, numGlyphs, point);
    fillText(fontData);
    resetColoring();
}

} // namespace WebCore
