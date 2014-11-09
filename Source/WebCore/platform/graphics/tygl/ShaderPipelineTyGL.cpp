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

#include "config.h"
#include "PlatformContextTyGL.h"

#include "NativeImageTyGL.h"
#include "NotImplemented.h"
#include "PathTyGL.h"
#include "ShaderCommonTyGL.h"
#include "TrapezoidListTyGL.h"

namespace WebCore {

typedef struct ShaderHash
{
    union {
        uint32_t hash;
        struct {
            uint32_t blending : 2;
            uint32_t clipping : 1;
            uint32_t coloring : 4;
            uint32_t primary  : 25;
        };
    };
} ShaderHash;

class ShaderCache {
public:
    static const int kMaximumCachedShaders = 16;

    struct ShaderData
    {
        bool isCompiled() { return shaderData[0]; }

        GLuint shaderData[PlatformContextTyGL::kMaximumShaderDescriptorLength];
        int numberOfAttributes;
    };

    ShaderCache()
    {
        m_head = m_shaderCacheItems;

        m_shaderCacheItems[0].prev = nullptr;
        m_shaderCacheItems[0].next = m_shaderCacheItems + 1;
        // 0 is not a valid shader hash, it's safe to initialize with.
        m_shaderCacheItems[0].hash = 0;
        m_shaderCacheItems[0].data.shaderData[0] = 0;

        for (int i = 1; i < kMaximumCachedShaders; ++i) {
            m_shaderCacheItems[i].prev = m_shaderCacheItems + i - 1;
            m_shaderCacheItems[i].next = m_shaderCacheItems + i + 1;
        }

        m_shaderCacheItems[kMaximumCachedShaders].prev = m_shaderCacheItems + kMaximumCachedShaders - 1;
        m_shaderCacheItems[kMaximumCachedShaders].next = nullptr;
    }

    ShaderData* operator[](uint32_t hash)
    {
        ASSERT(hash != 0);

        WTF::HashMap<uint32_t, ShaderCacheItem*>::iterator it = m_shaderProgramCache.find(hash);
        ShaderCacheItem* cacheItem;

        if (it != m_shaderProgramCache.end()) {
            cacheItem = (*it).value;
        } else {
            cacheItem = m_shaderCacheItems[kMaximumCachedShaders].prev;
            if (cacheItem->data.isCompiled())
                glDeleteProgram(cacheItem->data.shaderData[0]);
            if (cacheItem->hash != 0)
                m_shaderProgramCache.remove(cacheItem->hash);
            m_shaderProgramCache.add(hash, cacheItem);
            cacheItem->hash = hash;
            // Set shaderData[0] to 0 to indicate that the shader is not compiled.
            cacheItem->data.shaderData[0] = 0;
        }

        moveToFront(cacheItem) ;
        return &(cacheItem->data);
    }

private:
    struct ShaderCacheItem {
        uint32_t hash;
        ShaderData data;
        ShaderCacheItem* next;
        ShaderCacheItem* prev;
    };

    void moveToFront(ShaderCacheItem* item)
    {
        if (m_head == item)
            return;
        item->prev->next = item->next;
        item->next->prev = item->prev;
        item->next = m_head;
        m_head->prev = item;
        item->prev = nullptr;
        m_head = item;
    }

    ShaderCacheItem* m_head;
    ShaderCacheItem m_shaderCacheItems[kMaximumCachedShaders + 1];
    WTF::HashMap<uint32_t, ShaderCacheItem*> m_shaderProgramCache;
};

void PlatformContextTyGL::PipelineBuffer::beginAttributeAdding(PlatformContextTyGL* newContext, ShaderProgram* newPrimaryShader,
    PassRefPtr<NativeImageTyGL> newPrimaryShaderImage, ShaderVariableSetter newPrimaryShaderVariableSetter, const IntRect& newPaintRect, BeforeFlushAction* newBeforeFlushAction)
{
    static ShaderCache shaderCache;

    ASSERT(newContext);
    ASSERT(!newBeforeFlushAction || newBeforeFlushAction->callback);

    // This includes fast checks for secondary shaders.
    if (context == newContext
        && primaryShader == newPrimaryShader
        && primaryShaderImage == newPrimaryShaderImage
        && clipMaskImage == newContext->m_state.clipMaskImage
        && compositeOperator == newContext->m_state.compositeOperator
        && newContext->m_state.blendMode == BlendMode::BlendModeNormal
        && hasSameBeforeFlushAction(newBeforeFlushAction)
        && hasSameColor()
        && hasFreeSpace()) {
        if (coloring.type() == Coloring::SolidColoring && (primaryShader->options & PlatformContextTyGL::kAttributeSolidColoring))
            coloring.changeColor(context->m_coloring->color());
        paintRect = newPaintRect;
        return;
    }

    flushBatchingQueueIfNeeded();

    context = newContext;
    primaryShader = newPrimaryShader;
    primaryShaderImage = newPrimaryShaderImage;
    if (newBeforeFlushAction)
        beforeFlushAction = *newBeforeFlushAction;
    else
        beforeFlushAction.callback = nullptr;
    coloring = *(context->m_coloring);
    if (coloring.type() == Coloring::GradientColoring)
        gradient = coloring.gradient();
    else if (coloring.type() == Coloring::PatternColoring)
        pattern = coloring.pattern();
    if (coloring.type() == Coloring::GradientColoring || coloring.type() == Coloring::PatternColoring)
        complexFillTransform = context->transform();
    clipMaskImage = context->m_state.clipMaskImage;
    compositeOperator = context->m_state.compositeOperator;
    blendMode = context->m_state.blendMode;

    paintRect = newPaintRect;
    quadCount = 0;
    nextAttribute = attributes;
    textureIndex = 0;

    // Now construct the shader from the pieces.
    ShaderHash shaderHash;
    shaderHash.hash = 0;

    shaderProgramCount = 1;
    shaderVariableSetters[0] = newPrimaryShaderVariableSetter;

    const ShaderProgram* combinedShader[kMaximumNumberOfShaderPieces] = { primaryShader };
    shaderHash.primary = primaryShader->shaderId;
    ASSERT(!((primaryShader->options & PlatformContextTyGL::kAttributeSolidColoring) && (primaryShader->options & PlatformContextTyGL::kUniformSolidColoring)));

    if (coloring.type() == Coloring::SolidColoring) {
        if (primaryShader->options & PlatformContextTyGL::kAttributeSolidColoring) {
            combinedShader[shaderProgramCount] = PlatformContextTyGL::s_attributeSolidColorShaderProgram;
            shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::attributeSolidColorSetShaderArguments;
            ++shaderProgramCount;
            shaderHash.coloring = ShaderProgram::AttributeSolidColorShaderId;
        }

        if (primaryShader->options & PlatformContextTyGL::kUniformSolidColoring) {
            combinedShader[shaderProgramCount] = PlatformContextTyGL::s_uniformSolidColorShaderProgram;
            shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::uniformSolidColorSetShaderArguments;
            ++shaderProgramCount;
            shaderHash.coloring = ShaderProgram::UniformSolidColorShaderId;
        }
    } else if (coloring.type() == Coloring::GradientColoring) {
        if (primaryShader->options & PlatformContextTyGL::kComplexFill) {
            combinedShader[shaderProgramCount] = PlatformContextTyGL::s_gradientShaderProgram;
            shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::gradientSetShaderArguments;
            ++shaderProgramCount;
            shaderHash.coloring = ShaderProgram::GradientShaderId;
        }
    } else if (coloring.type() == Coloring::PatternColoring) {
        if (primaryShader->options & PlatformContextTyGL::kComplexFill) {
            const bool useFastPatternShader = (complexFillTransform.type() == TyGL::AffineTransform::Move)
                && isInteger(complexFillTransform.xTranslation())
                && isInteger(complexFillTransform.yTranslation())
                && pattern->getPatternSpaceTransform().isIdentityOrTranslation()
                && isInteger(pattern->getPatternSpaceTransform().e())
                && isInteger(pattern->getPatternSpaceTransform().f());

            if (useFastPatternShader) {
                combinedShader[shaderProgramCount] = PlatformContextTyGL::s_fastPatternShaderProgram;
                shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::fastPatternSetShaderArguments;
                shaderHash.coloring = ShaderProgram::FastPatternShaderId;
            } else {
                combinedShader[shaderProgramCount] = PlatformContextTyGL::s_transformedPatternShaderProgram;
                shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::transformedPatternSetShaderArguments;
                shaderHash.coloring = ShaderProgram::TransformedPatternShaderId;
            }
            ++shaderProgramCount;
        }
    }

    if (clipMaskImage.get()) {
        combinedShader[shaderProgramCount] = PlatformContextTyGL::s_clipShaderProgram;
        shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::clipSetShaderArguments;
        ++shaderProgramCount;
        shaderHash.clipping = ShaderProgram::ClipShaderId;
    }

    // NOTE The current W3C standard http://www.w3.org/TR/2014/CR-compositing-1-20140220/ doesn't describe Difference Compositing mode.
    // As other ports use Difference Blending Mode in this case, let's also do the same!
    switch (compositeOperator == CompositeDifference ? BlendMode::BlendModeDifference : blendMode) {
    case BlendMode::BlendModeNormal:
        break;
    case BlendMode::BlendModeMultiply:
    case BlendMode::BlendModeScreen:
    case BlendMode::BlendModeDarken:
    case BlendMode::BlendModeLighten:
    case BlendMode::BlendModeDifference:
    case BlendMode::BlendModeExclusion:
        combinedShader[shaderProgramCount] = PlatformContextTyGL::s_basicBlendModeShaderProgram;
        shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::blendModeSetBasicShaderArguments;
        ++shaderProgramCount;
        shaderHash.blending = ShaderProgram::BasicBlendShaderId;
        break;
    case BlendMode::BlendModeOverlay:
    case BlendMode::BlendModeColorDodge:
    case BlendMode::BlendModeColorBurn:
    case BlendMode::BlendModeHardLight:
    case BlendMode::BlendModeSoftLight:
        combinedShader[shaderProgramCount] = PlatformContextTyGL::s_complexBlendModeShaderProgram;
        shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::blendModeSetComplexShaderArguments;
        ++shaderProgramCount;
        shaderHash.blending = ShaderProgram::ComplexBlendShaderId;
        break;
    case BlendMode::BlendModeHue:
    case BlendMode::BlendModeSaturation:
    case BlendMode::BlendModeColor:
    case BlendMode::BlendModeLuminosity:
        combinedShader[shaderProgramCount] = PlatformContextTyGL::s_nonseparableBlendModeShaderProgram;
        shaderVariableSetters[shaderProgramCount] = PlatformContextTyGL::blendModeSetNonseparableShaderArguments;
        ++shaderProgramCount;
        shaderHash.blending = ShaderProgram::NonseparableBlendShaderId;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    numberOfAttributes = 0;
    ShaderCache::ShaderData* currentShader;

    currentShader = shaderCache[shaderHash.hash];

    if (currentShader->isCompiled()) {
        numberOfAttributes = currentShader->numberOfAttributes;
    } else {
        compileShaderInternal(currentShader->shaderData, shaderProgramCount, combinedShader);

        for (int shader = 0; shader < shaderProgramCount; shader++) {
            const int* components = combinedShader[shader]->components;
            const GLchar** arguments = combinedShader[shader]->variables;
            for (int i = 0; components[i]; ++i) {
                ASSERT((arguments[i][0] == 'a' || arguments[i][0] == 'u') && arguments[i][1] == '_');
                if (arguments[i][0] == 'a')
                    numberOfAttributes += components[i];
            }
        }

        currentShader->numberOfAttributes = numberOfAttributes;
    }

    compiledShader = currentShader->shaderData;
    strideLength = numberOfAttributes * sizeof(GLfloat);
}

void PlatformContextTyGL::PipelineBuffer::endAttributeAdding()
{
    if (coloring.type() == Coloring::SolidColoring && (primaryShader->options & PlatformContextTyGL::kAttributeSolidColoring))
        context->addAttributeColorRepeated(coloring.color());

    if (clipMaskImage.get())
        context->clipSetShaderAttributes();

    // NOTE The current W3C standard http://www.w3.org/TR/2014/CR-compositing-1-20140220/ doesn't describe Difference Compositing mode.
    // As other ports use Difference Blending Mode in this case, let's also do the same!
    if (blendMode != BlendMode::BlendModeNormal || compositeOperator == CompositeDifference)
        context->blendModeSetShaderAttributes();

    nextAttribute += numberOfAttributes * 3;
    quadCount++;
    ASSERT((nextAttribute <= attributes + kMaximumNumberOfAttributes) && quadCount <= kMaximumNumberOfUshortQuads);
}

#define COPY_SHADER_ARGUMENTS(argument) \
    argument(u, texture, Texture, 1) \
    argument(a, position, Position, 4)

DEFINE_SHADER(
    CopyShader,
    COPY_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
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
        vec4 resultColor = texture2D(u_texture, v_texturePosition);
    )
)

void PlatformContextTyGL::paintTexture(GLfloat* position, PassRefPtr<NativeImageTyGL> texture)
{
    static GLuint g_CopyShader[4];
    const ShaderProgram* program[1] = { &CopyShader::s_program };

    if (!compileAndUseShader(g_CopyShader, 1, program))
        return;

    GLuint offset = g_CopyShader[1];

    glActiveTexture(GL_TEXTURE0);
    texture->bindTexture();
    glUniform1i(g_CopyShader[offset + CopyShader::uTexture], 0);

    glVertexAttribPointer(g_CopyShader[offset + CopyShader::aPosition], 4, GL_FLOAT, GL_FALSE, 0, position);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void PlatformContextTyGL::PipelineBuffer::flushBatchingQueue(bool reset)
{
    ASSERT(context && compiledShader[0]);

    GLfloat* attributeStart;

    if (beforeFlushAction.callback) {
        if (!beforeFlushAction.callback(context))
            goto finishFlush;
    }

    glUseProgram(compiledShader[0]);

    context->targetTexture()->bindFbo();

    // NOTE The current W3C standard http://www.w3.org/TR/2014/CR-compositing-1-20140220/ doesn't describe Difference Compositing mode.
    // As other ports use Difference Blending Mode in this case, let's also do the same!
    if (blendMode != BlendMode::BlendModeNormal || compositeOperator == CompositeDifference) {
        originalDestinationImage = NativeImageTyGL::create(paintRect.size());
        originalDestinationImage->bindTexture();
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paintRect.x(), paintRect.y(), paintRect.width(), paintRect.height());
    }

    attributeStart = attributes;
    for (int i = 0; i < shaderProgramCount; ++i)
        attributeStart = shaderVariableSetters[i](context, attributeStart, compiledShader + compiledShader[i + 1]);

    ASSERT(attributeStart - attributes == numberOfAttributes);

    switch (compositeOperator) {
    case CompositeCopy:
        glBlendFunc(GL_ONE, GL_ZERO);
        break;
    case CompositeClear:
        notImplemented();
        // Fall-through: Composite mode is not implemented,
        // so we fall back to the default mode.
    case CompositeSourceOver:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case CompositeSourceIn:
        glBlendFunc(GL_DST_ALPHA, GL_ZERO);
        break;
    case CompositeSourceOut:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);
        break;
    case CompositeSourceAtop:
        glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case CompositeDestinationOver:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
        break;
    case CompositeDestinationIn:
        glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
        break;
    case CompositeDestinationOut:
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case CompositeDestinationAtop:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA);
        break;
    case CompositeXOR:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case CompositePlusLighter:
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    // TODO: SubimageSupport.

    ASSERT(quadCount <= kMaximumNumberOfUshortQuads);
    if (quadCount == 1)
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    else
        glDrawElements(GL_TRIANGLES, quadCount * 6, GL_UNSIGNED_SHORT, nullptr);

finishFlush:
    if (reset) {
        context = 0;
        clipMaskImage.clear();
        return;
    }

    quadCount = 0;
    nextAttribute = attributes;
    textureIndex = 0;
}

} // namespace WebCore
