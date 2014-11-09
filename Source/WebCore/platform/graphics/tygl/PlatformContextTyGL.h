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

#ifndef PlatformContexTyGL_h
#define PlatformContexTyGL_h

#include "ClipRectTyGL.h"
#include "Color.h"
#include "FloatRect.h"
#include "TyGLDefs.h"
#include "Gradient.h"
#include "GraphicsTypes.h"
#include "GraphicsContext.h"
#include "IntPoint.h"
#include "IntRect.h"
#include "IntSize.h"
#include "NativeImageTyGL.h"
#include "Path.h"
#include "Pattern.h"
#include "TransformTyGL.h"
#include "TrapezoidListTyGL.h"
#include "WindRule.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class Color;
class GLPlatformContext;
class GLPlatformSurface;
class GlyphBuffer;
class Image;
class NativeImageTyGL;
class RenderObject;
class RenderTheme;

namespace TyGL {
class GlyphBufferFontData;
class PathData;
class TrapezoidList;
}

class PlatformContextTyGL : public RefCounted<PlatformContextTyGL> {
public:
    static const int kMaximumShaderDescriptorLength = 16;

    static const int kNoShaderOptions = 0;
    static const int kAttributeSolidColoring = (1 << 0);
    static const int kUniformSolidColoring = (1 << 1);
    static const int kComplexFill = (1 << 2);

    struct ShaderProgram {
        static const int kLocalShaderId = 0;

        // List all defined shaders here as ClassName + Id.
        enum ShaderID {
            ResetId = 0,

            // Primary shaders.
            BorderLineShaderId,
            BulletPointShaderId,
            FillColorShaderId,
            FontShaderId,
            ImageShaderId,
            RectShaderId,
            TransformedFontShaderId,
            TransformedImageShaderId,

            // Secondary shaders: Coloring.
            NoSolidColorShaderId = 0,
            AttributeSolidColorShaderId = 1,
            UniformSolidColorShaderId = 2,
            GradientShaderId = 3,
            FastPatternShaderId = 4,
            TransformedPatternShaderId = 5,

            // Secondary shaders: Clipping.
            NoClipId = 0,
            ClipShaderId = 1,

            // Secondary shaders: Blending.
            NoBlendId = 0,
            BasicBlendShaderId = 1,
            ComplexBlendShaderId = 2,
            NonseparableBlendShaderId = 3,

            // Local shaders:
            CopyShaderId = kLocalShaderId,
            FillPathShaderId = kLocalShaderId,
        };

        int options;
        ShaderID shaderId;
        const GLchar* vertexVariables;
        const GLchar* vertexShader;
        const GLchar* fragmentVariables;
        const GLchar* fragmentShader;
        const GLchar** variables;
        const int* components;
    };

    enum UIElement {
        Button,
        Checkbox,
        RadioButton,
        EditBox,
        ScrollbarThumb
    };

    class Coloring {
    public:
        enum Type {
            SolidColoring,
            GradientColoring,
            PatternColoring
        };

        Coloring()
            : m_type(SolidColoring)
            , m_color(Color::black)
        {
        }

        Coloring(RGBA32 color)
            : m_type(SolidColoring)
            , m_color(color)
        {
        }

        Coloring(const Color& color)
            : m_type(SolidColoring)
            , m_color(premultipliedARGBFromColor(color.rgb()))
        {
        }

        Coloring(RefPtr<Gradient> gradient)
            : m_type(GradientColoring)
            , m_color(Color::black)
            , m_gradient(gradient)
        {
        }

        Coloring(RefPtr<Pattern> pattern)
            : m_type(PatternColoring)
            , m_color(Color::black)
            , m_pattern(pattern)
        {
        }

        void set(RGBA32 color)
        {
            m_type = SolidColoring;
            m_color = color;
            m_gradient.clear();
        }

        void set(RefPtr<Gradient> gradient)
        {
            m_type = GradientColoring;
            m_color = 0;
            m_gradient = gradient;
            m_pattern.clear();
        }

        void set(RefPtr<Pattern> pattern)
        {
            m_type = PatternColoring;
            m_color = 0;
            m_gradient.clear();
            m_pattern = pattern;
        }

        void changeColor(RGBA32 color)
        {
            ASSERT(m_type == SolidColoring);
            m_color = color;
        }

        Type type() { return m_type; }

        RGBA32 color()
        {
            ASSERT(m_type == SolidColoring);
            return m_color;
        }

        PassRefPtr<Gradient> gradient() const { ASSERT(m_gradient); return m_gradient; }
        PassRefPtr<Pattern> pattern() const { ASSERT(m_pattern); return m_pattern; }

    private:
        Type m_type;
        RGBA32 m_color;
        RefPtr<Gradient> m_gradient;
        RefPtr<Pattern> m_pattern;
    };

    static PassRefPtr<PlatformContextTyGL> create(PassRefPtr<NativeImageTyGL> targetTexture)
    {
        return adoptRef(new PlatformContextTyGL(targetTexture));
    }

    ~PlatformContextTyGL();

    static PlatformContextTyGL* release(PassRefPtr<PlatformContextTyGL> context) { return context.leakRef(); }
    static void createGLContextIfNeed()
    {
        if (!s_offScreenContext)
            createGLContext();
    }
    static GLPlatformContext* offScreenContext() { return s_offScreenContext; }
    static GLPlatformSurface* offScreenSurface() { return s_offScreenSurface; }
    PassRefPtr<NativeImageTyGL> targetTexture() const { return m_targetTexture; }

    void save();
    void restore();

    void setStrokeThickness(float);
    void setFillColor(const Color&);
    void setStrokeColor(const Color&);
    void setCompositeOperation(CompositeOperator, BlendMode);
    void setLineJoin(const LineJoin);
    void setLineCap(const LineCap);
    void setMiterLimit(float);

    void translate(float x, float y);
    void scale(float, float);
    void rotate(float);
    const TyGL::AffineTransform& transform() const { return m_state.transform; }
    const TyGL::ClipRect& boundingClipRect() { return m_state.boundingClipRect; }
    void setTransform(TyGL::AffineTransform::Transform transform) { m_state.transform = transform; }
    void multiplyTransform(TyGL::AffineTransform::Transform transform) { m_state.transform *= transform; }

    void clip(const FloatRect&);
    void clip(const TyGL::PathData*, WindRule);
    void clipOut(const FloatRect& rect);
    void clipOut(const TyGL::PathData*);

    void clipToImageBuffer(PassRefPtr<NativeImageTyGL> image, const FloatRect& rect)
    {
        m_state.clipMaskImage = image;
        m_state.boundingClipRect = TyGL::ClipRect::createRect(IntRect(rect));
    }

    void copyImage(const FloatRect&, PassRefPtr<NativeImageTyGL>, const FloatRect&, CompositeOperator, BlendMode);
    void fillRect(const FloatRect&, Coloring);
    void drawBulletPoint(const FloatRect&, const Color&);
    void clearRect(const FloatRect&);
    void drawBorderLine(const WebCore::FloatPoint&, const WebCore::FloatPoint&, const WebCore::Color&, float, float, float);
    void drawUIElement(UIElement, const IntRect&, RenderTheme* = nullptr, const RenderObject* = nullptr);
    void drawUIArrow(const IntRect&, bool);

    void addGlyphAttributes(const FloatRect&, const TyGL::TextureFont::GlyphMetrics&, const TyGL::AffineTransform&);
    void fillText(TyGL::GlyphBufferFontData&);
    void fillText(TyGL::TextureFont*, const GlyphBuffer*, int, int, const FloatPoint&, Coloring);
    void fillPath(const TyGL::PathData*, WindRule = RULE_NONZERO);
    void fillPath(const TyGL::PathData*, Coloring, WindRule = RULE_NONZERO);
    void strokePath(const TyGL::PathData*);
    void strokePath(const TyGL::PathData*, Coloring);

    static void flushPendingDraws() { flushBatchingQueueIfNeeded(); }
    static void premultiplyUnmultipliedImageData(NativeImageTyGL*);

    Coloring parseFillColoring(const GraphicsContextState& state)
    {
        // Color is set when setPlatformFillColor is called.
        if (state.fillGradient)
            return Coloring(state.fillGradient);
        else if (state.fillPattern) {
            // Make sure pattern image is cached.
            state.fillPattern->tileImage()->nativeImageForCurrentFrame();
            return Coloring(state.fillPattern);
        }
        return m_state.fillColoring;
    }

    Coloring parseStrokeColoring(const GraphicsContextState& state)
    {
        // Color is set when setPlatformStrokeColor is called.
        if (state.strokeGradient)
            return Coloring(state.strokeGradient);
        else if (state.strokePattern) {
            // Make sure pattern image is cached.
            state.strokePattern->tileImage()->nativeImageForCurrentFrame();
            return Coloring(state.strokePattern);
        }
        return m_state.strokeColoring;
    }

private:

    struct TyGLState {
        TyGL::AffineTransform transform;
        // In absoulte coordinates inside targetTexture().
        TyGL::ClipRect boundingClipRect;
        RefPtr<NativeImageTyGL> clipMaskImage;
        Coloring fillColoring;
        Coloring strokeColoring;
        float strokeWidth;
        LineCap lineCapMode;
        LineJoin lineJoinMode;
        float miterLimit;
        CompositeOperator compositeOperator;
        BlendMode blendMode;
    };

    typedef GLfloat* (*ShaderVariableSetter)(PlatformContextTyGL*, GLfloat*, GLuint* shaderArguments);

    struct BeforeFlushAction {
        typedef bool (*Callback)(PlatformContextTyGL*);

        BeforeFlushAction(Callback callback, void *data)
            : callback(callback)
            , data(data)
        {
        }

        Callback callback;
        void *data;
    };

    struct PathAttribute {
        OwnPtr<TyGL::TrapezoidList> path;
    };

    struct PipelineBuffer {
        static const int kMaximumNumberOfShaderPieces = 8;
        static const int kMaximumNumberOfAttributes = 65536;
        static const int kMaximumNumberOfUshortQuads = 65536 / 6;
        static const int kMaximumNumberOfPathAttributes = 16;

        static GLuint s_indexBufferObject;

        PipelineBuffer()
            : context(nullptr)
            , beforeFlushAction(nullptr, nullptr)
        {
        }

        static void initQuadIndexes();

        void beginAttributeAdding(PlatformContextTyGL*, ShaderProgram*, PassRefPtr<NativeImageTyGL>, ShaderVariableSetter, const IntRect&, BeforeFlushAction*);

        void flushBatchingQueue(bool = true);
        void flushBatchingQueueIfNeeded(bool reset = true)
        {
            if (context)
                flushBatchingQueue(reset);
        }

        void endAttributeAdding();

        inline bool hasSameBeforeFlushAction(BeforeFlushAction* newBeforeFlushAction)
        {
            if (!beforeFlushAction.callback)
                return !newBeforeFlushAction;
            if (!newBeforeFlushAction)
                return false;
            ASSERT(beforeFlushAction.callback == newBeforeFlushAction->callback);
            return beforeFlushAction.data == newBeforeFlushAction->data;
        }

        inline bool hasSameColor()
        {
            if (context->m_coloring->type() != coloring.type())
                return false;

            switch (coloring.type()) {
            case Coloring::SolidColoring:
                return !(primaryShader->options & PlatformContextTyGL::kUniformSolidColoring)
                    || (coloring.color() == context->m_coloring->color());
            case Coloring::GradientColoring:
                return coloring.gradient() == context->m_coloring->gradient();
            case Coloring::PatternColoring:
                return coloring.pattern() == context->m_coloring->pattern();
            default:
                ASSERT_NOT_REACHED();
                return false;
            }
        }

        inline bool hasFreeSpace(int neededSpace = 0)
        {
            return nextAttribute + (numberOfAttributes * 4) * (neededSpace + 1) <= attributes + kMaximumNumberOfAttributes
                && quadCount + neededSpace < kMaximumNumberOfUshortQuads;
        }

        inline void addAttribute(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
            GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3)
        {
            GLfloat* attribute = nextAttribute;
            attribute[0] = x0;
            attribute[1] = y0;
            attribute += numberOfAttributes;
            attribute[0] = x1;
            attribute[1] = y1;
            attribute += numberOfAttributes;
            attribute[0] = x2;
            attribute[1] = y2;
            attribute += numberOfAttributes;
            attribute[0] = x3;
            attribute[1] = y3;
            nextAttribute += 2;
        }

        void addAttribute(GLfloat x0, GLfloat y0, GLfloat z0, GLfloat w0,
            GLfloat x1, GLfloat y1, GLfloat z1, GLfloat w1,
            GLfloat x2, GLfloat y2, GLfloat z2, GLfloat w2,
            GLfloat x3, GLfloat y3, GLfloat z3, GLfloat w3)
        {
            GLfloat* attribute = nextAttribute;
            attribute[0] = x0;
            attribute[1] = y0;
            attribute[2] = z0;
            attribute[3] = w0;
            attribute += numberOfAttributes;
            attribute[0] = x1;
            attribute[1] = y1;
            attribute[2] = z1;
            attribute[3] = w1;
            attribute += numberOfAttributes;
            attribute[0] = x2;
            attribute[1] = y2;
            attribute[2] = z2;
            attribute[3] = w2;
            attribute += numberOfAttributes;
            attribute[0] = x3;
            attribute[1] = y3;
            attribute[2] = z3;
            attribute[3] = w3;
            nextAttribute += 4;
        }

        void addAttributeRepeated(GLfloat x, GLfloat y)
        {
            GLfloat* attribute = nextAttribute;
            for (int i = 0; i < 4; ++i) {
                attribute[0] = x;
                attribute[1] = y;
                attribute += numberOfAttributes;
            }
            nextAttribute += 2;
        }

        void addAttributeRepeated(GLfloat x, GLfloat y, GLfloat z)
        {
            GLfloat* attribute = nextAttribute;
            for (int i = 0; i < 4; ++i) {
                attribute[0] = x;
                attribute[1] = y;
                attribute[2] = z;
                attribute += numberOfAttributes;
            }
            nextAttribute += 3;
        }

        void addAttributeRepeated(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
        {
            GLfloat* attribute = nextAttribute;
            for (int i = 0; i < 4; ++i) {
                attribute[0] = x;
                attribute[1] = y;
                attribute[2] = z;
                attribute[3] = w;
                attribute += numberOfAttributes;
            }
            nextAttribute += 4;
        }

        void addTrapezoidList(PassOwnPtr<TyGL::TrapezoidList> trapezoidList)
        {
            pathAttributes[quadCount].path = trapezoidList;
        }

        // State arguments.
        // These members are checked by beginAttributeAdding
        // to decide whether batching is possible.
        PlatformContextTyGL* context;
        ShaderProgram* primaryShader;
        BeforeFlushAction beforeFlushAction;
        RefPtr<NativeImageTyGL> primaryShaderImage;
        Coloring coloring;
        RefPtr<Gradient> gradient;
        RefPtr<Pattern> pattern;
        // We need to cache global transform for gradients and patterns
        TyGL::AffineTransform complexFillTransform;
        RefPtr<NativeImageTyGL> clipMaskImage;
        CompositeOperator compositeOperator;
        BlendMode blendMode;

        // Pipeline related members.
        GLuint* compiledShader;
        int shaderProgramCount;
        ShaderVariableSetter shaderVariableSetters[kMaximumNumberOfShaderPieces];
        IntRect paintRect;
        int numberOfAttributes;
        GLsizei strideLength;
        int quadCount;
        GLfloat* nextAttribute;
        GLfloat attributes[kMaximumNumberOfAttributes];
        int textureIndex;

        PathAttribute pathAttributes[kMaximumNumberOfPathAttributes];

        RefPtr<NativeImageTyGL> originalDestinationImage;
    };

    static PipelineBuffer* pipelineBuffer() { return s_pipelineBuffer; }

    PlatformContextTyGL(PassRefPtr<NativeImageTyGL>);

    static void createGLContext();

    static bool compileShaderInternal(GLuint*, int, const ShaderProgram**);

    static bool compileShader(GLuint* compiledShader, int count, const ShaderProgram** programList)
    {
        if (compiledShader[0])
            return true;
        return compileShaderInternal(compiledShader, count, programList);
    }

    static bool compileAndUseShader(GLuint* compiledShader, int count, const ShaderProgram** programList)
    {
        if (compiledShader[0]) {
            glUseProgram(compiledShader[0]);
            return true;
        }
        return compileShaderInternal(compiledShader, count, programList);
    }

    static bool isInteger(GLfloat value) { return value == static_cast<GLfloat>(static_cast<int>(value)); }
    inline void resetColoring() { m_coloring = &m_state.fillColoring; }

    void fillPathAlphaTexture(TyGL::TrapezoidList*, PassRefPtr<NativeImageTyGL>);
    void paintPath(const TyGL::PathData*, RGBA32, WindRule = RULE_NONZERO);
    void paintTexture(GLfloat*, PassRefPtr<NativeImageTyGL>);

    inline bool fillRectFastPath(const FloatRect&);
    void fillAbsoluteRect(GLfloat, GLfloat, GLfloat, GLfloat);
    inline void copyImageFastPath(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat,
        PassRefPtr<NativeImageTyGL>, const FloatRect&, CompositeOperator, BlendMode);

    inline void beginAttributeAdding(ShaderProgram* primaryShader, PassRefPtr<NativeImageTyGL> primaryShaderImage,
        ShaderVariableSetter primaryShaderVariableSetter, const IntRect& paintRect, BeforeFlushAction* beforeFlushAction = nullptr)
    {
        pipelineBuffer()->beginAttributeAdding(this,  primaryShader, primaryShaderImage, primaryShaderVariableSetter, paintRect, beforeFlushAction);
    }

    inline void beginAttributeAdding(ShaderProgram* primaryShader, PassRefPtr<NativeImageTyGL> primaryShaderImage,
        ShaderVariableSetter primaryShaderVariableSetter, GLfloat left, GLfloat bottom, GLfloat right, GLfloat top, BeforeFlushAction* beforeFlushAction = nullptr)
    {
        IntRect paintRect(left, bottom, right - left, top - bottom);
        pipelineBuffer()->beginAttributeAdding(this,  primaryShader, primaryShaderImage, primaryShaderVariableSetter, paintRect, beforeFlushAction);
    }

    inline void endAttributeAdding() { pipelineBuffer()->endAttributeAdding(); }
    inline bool hasFreeSpace(int neededSpace = 0) { return pipelineBuffer()->hasFreeSpace(neededSpace); }
    inline static void flushBatchingQueueIfNeeded(bool reset = true) { pipelineBuffer()->flushBatchingQueueIfNeeded(reset); }

    void paintButton(const IntRect&, bool);
    void paintCheckbox(const IntRect&, bool);
    void paintRadioButton(const IntRect&, bool);
    void paintScrollbarThumb(const IntRect&);
    void paintEditBox(const IntRect&, bool);
    void paintTexturedQuads(int, PassRefPtr<NativeImageTyGL>, float*, float*, const IntRect&);
    void loadUITexture();

    inline void setUniformTexture(GLuint textureUniformPosition, PassRefPtr<NativeImageTyGL> texture)
    {
        glActiveTexture(GL_TEXTURE0 + pipelineBuffer()->textureIndex);
        texture->bindTexture();
        glUniform1i(textureUniformPosition, pipelineBuffer()->textureIndex);
        ++pipelineBuffer()->textureIndex;
    }

    inline void setUniformInt(GLuint intUniformPosition, GLuint value)
    {
        glUniform1i(intUniformPosition, value);
    }

    inline void setPrimaryTexture(GLuint id)
    {
        setUniformTexture(id, pipelineBuffer()->primaryShaderImage);
    }

    inline static void setUniformColor(GLint shaderArguments, RGBA32 color)
    {
        GLfloat r = static_cast<GLfloat>(redChannel(color)) / 255;
        GLfloat g = static_cast<GLfloat>(greenChannel(color)) / 255;
        GLfloat b = static_cast<GLfloat>(blueChannel(color)) / 255;
        GLfloat a = static_cast<GLfloat>(alphaChannel(color)) / 255;
        glUniform4f(shaderArguments, r, g, b, a);
    }

    void addAttribute(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1,
        GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3)
    {
        pipelineBuffer()->addAttribute(x0, y0, x1, y1, x2, y2, x3, y3);
    }

    void addAttribute(GLfloat x0, GLfloat y0, GLfloat z0, GLfloat w0,
        GLfloat x1, GLfloat y1, GLfloat z1, GLfloat w1,
        GLfloat x2, GLfloat y2, GLfloat z2, GLfloat w2,
        GLfloat x3, GLfloat y3, GLfloat z3, GLfloat w3)
    {
        pipelineBuffer()->addAttribute(x0, y0, z0, w0, x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3);
    }

    void addAttributeRepeated(GLfloat x, GLfloat y)
    {
        pipelineBuffer()->addAttributeRepeated(x, y);
    }

    void addAttributeRepeated(GLfloat x, GLfloat y, GLfloat z)
    {
        pipelineBuffer()->addAttributeRepeated(x, y, z);
    }

    void addAttributeRepeated(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
    {
        pipelineBuffer()->addAttributeRepeated(x, y, z, w);
    }

    void addAttributePosition(GLfloat left, GLfloat bottom, GLfloat right, GLfloat top)
    {
        addAttribute(left, bottom, right, bottom, left, top, right, top);
    }

    void addAttributeAbsolutePosition()
    {
        GLfloat left, bottom, right, top;
        IntRect& paintRect = pipelineBuffer()->paintRect;

        left = paintRect.x();
        bottom = paintRect.y();
        right = paintRect.maxX();
        top = paintRect.maxY();
        addAttributePosition(left, bottom, right, top);
    }

    void addAttributeColorRepeated(RGBA32 color)
    {
        GLfloat r = static_cast<GLfloat>(redChannel(color)) / 255;
        GLfloat g = static_cast<GLfloat>(greenChannel(color)) / 255;
        GLfloat b = static_cast<GLfloat>(blueChannel(color)) / 255;
        GLfloat a = static_cast<GLfloat>(alphaChannel(color)) / 255;
        addAttributeRepeated(r, g, b, a);
    }

    void addTrapezoidList(PassOwnPtr<TyGL::TrapezoidList> trapezoidList)
    {
        pipelineBuffer()->addTrapezoidList(trapezoidList);
    }

    void blendModeSetShaderAttributes();
    void clipSetShaderAttributes();
    void addTextAttributes(TyGL::GlyphBufferFontData&, PassRefPtr<NativeImageTyGL>, const FloatRect&, const IntRect&);

    static GLfloat* attributeSolidColorSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* blendModeSetBasicShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* blendModeSetComplexShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* blendModeSetNonseparableShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* clipSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* fontSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* transformedFontSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* imageSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* pathSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* rectSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* transformedImageSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* uniformSolidColorSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* gradientSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* bulletPointSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* drawBorderLineSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* fastPatternSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);
    static GLfloat* transformedPatternSetShaderArguments(PlatformContextTyGL*, GLfloat*, GLuint*);

    static bool fontBeforeFlushAction(PlatformContextTyGL*);
    static bool pathBeforeFlushAction(PlatformContextTyGL*);

    class ClippedTransformedQuad {
    public:
        ClippedTransformedQuad(const FloatPoint preTransformCoords[4], const TyGL::AffineTransform&, const TyGL::ClipRect& boundingClipRect);
        ClippedTransformedQuad(const FloatRect& preTransformRect, const TyGL::AffineTransform&, const TyGL::ClipRect& boundingClipRect);
        void addToPipeline(PipelineBuffer*);
        const FloatPoint& transformedCoords(int i) const { return m_transformedCoords[i]; };
        bool hasVisiblePixels() const { return m_hasVisiblePixels; };

        GLfloat minX() const { return m_minX; };
        GLfloat minY() const { return m_minY; };
        GLfloat maxX() const { return m_maxX; };
        GLfloat maxY() const { return m_maxY; };
    private:
        void compute(const TyGL::AffineTransform&, const TyGL::ClipRect& boundingClipRect);
        bool m_hasVisiblePixels;
        GLfloat m_minX, m_minY, m_maxX, m_maxY;
        GLfloat m_v0X, m_v0Y, m_v1X, m_v1Y;
        FloatPoint m_bottomLeftTransformed;
        FloatPoint m_transformedCoords[4];
    };

    RefPtr<NativeImageTyGL> m_targetTexture;
    TyGLState m_state;
    Vector<TyGLState> m_stack;
    Coloring* m_coloring;

    static PipelineBuffer* s_pipelineBuffer;
    static ShaderProgram* s_attributeSolidColorShaderProgram;
    static ShaderProgram* s_uniformSolidColorShaderProgram;
    static ShaderProgram* s_basicBlendModeShaderProgram;
    static ShaderProgram* s_complexBlendModeShaderProgram;
    static ShaderProgram* s_nonseparableBlendModeShaderProgram;
    static ShaderProgram* s_clipShaderProgram;
    static ShaderProgram* s_gradientShaderProgram;
    static ShaderProgram* s_fastPatternShaderProgram;
    static ShaderProgram* s_transformedPatternShaderProgram;
    static RefPtr<Image> s_uiElements;

    static GLPlatformContext* s_offScreenContext;
    static GLPlatformSurface* s_offScreenSurface;

    RefPtr<NativeImageTyGL> m_gradientStopInfoImage;
};

} //namespace WebCore

#endif // PlatformContexTyGL_h
