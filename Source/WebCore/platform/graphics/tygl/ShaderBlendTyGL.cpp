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

#include "GraphicsTypes.h"
#include "ShaderCommonTyGL.h"

namespace WebCore {

#define BLEND_SHADER_ARGUMENTS(argument) \
    argument(u, blendOriginalDestinationPixelsTexture, BlendOriginalDestinationPixelsTexture, 1) \
    argument(u, blendMode, BlendMode, 1) \
    argument(a, blendTexturePosition, BlendTexturePosition, 2)

#define BLEND_MODE_MULTIPLY 1
#define BLEND_MODE_SCREEN 2
#define BLEND_MODE_OVERLAY 3
#define BLEND_MODE_DARKEN 4
#define BLEND_MODE_LIGHTEN 5
#define BLEND_MODE_COLOR_DODGE 6
#define BLEND_MODE_COLOR_BURN 7
#define BLEND_MODE_HARD_LIGHT 8
#define BLEND_MODE_SOFT_LIGHT 9
#define BLEND_MODE_DIFFERENCE 10
#define BLEND_MODE_EXCLUSION 11
#define BLEND_MODE_HUE 12
#define BLEND_MODE_SATURATION 13
#define BLEND_MODE_COLOR 14
#define BLEND_MODE_LUMINOSITY 15

DEFINE_SHADER(
    BasicBlendShader,
    BLEND_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec2 a_blendTexturePosition;

        varying vec2 v_destTexturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_destTexturePosition = a_blendTexturePosition;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_blendOriginalDestinationPixelsTexture;
        uniform int u_blendMode;

        varying vec2 v_destTexturePosition;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec4 blendDestinationColor = texture2D(u_blendOriginalDestinationPixelsTexture, v_destTexturePosition);

        if (blendDestinationColor.a != 0.0 && resultColor.a != 0.0) {
            blendDestinationColor.rgb = blendDestinationColor.rgb / blendDestinationColor.a;
            resultColor.rgb = resultColor.rgb / resultColor.a;

            vec3 blendResult;

            if (u_blendMode == BLEND_MODE_MULTIPLY)
                blendResult = blendDestinationColor.rgb * resultColor.rgb;
            else if (u_blendMode == BLEND_MODE_SCREEN)
                blendResult = blendDestinationColor.rgb + resultColor.rgb - blendDestinationColor.rgb * resultColor.rgb;
            else if (u_blendMode == BLEND_MODE_DARKEN)
                blendResult = min(blendDestinationColor.rgb, resultColor.rgb);
            else if (u_blendMode == BLEND_MODE_LIGHTEN)
                blendResult = max(blendDestinationColor.rgb, resultColor.rgb);
            else if (u_blendMode == BLEND_MODE_DIFFERENCE)
                blendResult = abs(blendDestinationColor.rgb - resultColor.rgb);
            else if (u_blendMode == BLEND_MODE_EXCLUSION)
                blendResult = blendDestinationColor.rgb + resultColor.rgb - 2.0 * blendDestinationColor.rgb * resultColor.rgb;

            resultColor.rgb = (1.0 - blendDestinationColor.a) * resultColor.rgb + blendDestinationColor.a * blendResult.rgb;
            resultColor.rgb *= resultColor.a;
        }
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_basicBlendModeShaderProgram = &BasicBlendShader::s_program;

GLfloat* PlatformContextTyGL::blendModeSetBasicShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setUniformTexture(shaderArguments[BasicBlendShader::uBlendOriginalDestinationPixelsTexture], context->pipelineBuffer()->originalDestinationImage.get());
    context->setUniformInt(shaderArguments[BasicBlendShader::uBlendMode], context->pipelineBuffer()->blendMode);
    glVertexAttribPointer(shaderArguments[BasicBlendShader::aBlendTexturePosition], 2, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 2;
}

DEFINE_SHADER(
    ComplexBlendShader,
    BLEND_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec2 a_blendTexturePosition;

        varying vec2 v_destTexturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_destTexturePosition = a_blendTexturePosition;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_blendOriginalDestinationPixelsTexture;
        uniform int u_blendMode;

        varying vec2 v_destTexturePosition;

        float blendModeOverlayCore(float resultColor, float blendDestinationColor) {
            if (blendDestinationColor <= 0.5)
                return resultColor * 2.0 * blendDestinationColor;
            else
                return resultColor + (2.0 * blendDestinationColor - 1.0) - resultColor * (2.0 * blendDestinationColor - 1.0);
        }

        vec3 blendModeOverlay(vec3 resultColor, vec3 blendDestinationColor) {
            vec3 result;

            result.r = blendModeOverlayCore(resultColor.r, blendDestinationColor.r);
            result.g = blendModeOverlayCore(resultColor.g, blendDestinationColor.g);
            result.b = blendModeOverlayCore(resultColor.b, blendDestinationColor.b);

            return result;
        }

        float blendModeColorDodgeCore(float resultColor, float blendDestinationColor) {
            if (blendDestinationColor == 0.0)
                return 0.0;
            else if (resultColor == 1.0)
                return 1.0;
            else
                return min(1.0, blendDestinationColor / (1.0 - resultColor));
        }

        vec3 blendModeColorDodge(vec3 resultColor, vec3 blendDestinationColor) {
            vec3 result;

            result.r = blendModeColorDodgeCore(resultColor.r, blendDestinationColor.r);
            result.g = blendModeColorDodgeCore(resultColor.g, blendDestinationColor.g);
            result.b = blendModeColorDodgeCore(resultColor.b, blendDestinationColor.b);

            return result;
        }

        float blendModeColorBurnCore(float resultColor, float blendDestinationColor) {
            if (blendDestinationColor == 1.0)
                return 1.0;
            else if (resultColor == 0.0)
                return 0.0;
            else
                return 1.0 - min(1.0, (1.0 - blendDestinationColor) / resultColor);
        }

        vec3 blendModeColorBurn(vec3 resultColor, vec3 blendDestinationColor) {
            vec3 result;

            result.r = blendModeColorBurnCore(resultColor.r, blendDestinationColor.r);
            result.g = blendModeColorBurnCore(resultColor.g, blendDestinationColor.g);
            result.b = blendModeColorBurnCore(resultColor.b, blendDestinationColor.b);

            return result;
        }

        float blendModeHardLightCore(float resultColor, float blendDestinationColor) {
            if (resultColor <= 0.5)
                return blendDestinationColor * 2.0 * resultColor;
            else
                return blendDestinationColor + (2.0 * resultColor - 1.0) - blendDestinationColor * (2.0 * resultColor - 1.0);
        }

        vec3 blendModeHardLight(vec3 resultColor, vec3 blendDestinationColor) {
            vec3 result;

            result.r = blendModeHardLightCore(resultColor.r, blendDestinationColor.r);
            result.g = blendModeHardLightCore(resultColor.g, blendDestinationColor.g);
            result.b = blendModeHardLightCore(resultColor.b, blendDestinationColor.b);

            return result;
        }

        float blendModeSoftLightCore(float resultColor, float blendDestinationColor) {
            if (resultColor <= 0.5)
                return blendDestinationColor - (1.0 - 2.0 * resultColor) * blendDestinationColor * (1.0 - blendDestinationColor);
            else {
                float d;

                if (blendDestinationColor <= 0.25)
                    d = ((16.0 * blendDestinationColor - 12.0) * blendDestinationColor + 4.0) * blendDestinationColor;
                else
                    d = sqrt(blendDestinationColor);

                return blendDestinationColor + (2.0 * resultColor - 1.0) * (d - blendDestinationColor);
            }
        }

        vec3 blendModeSoftLight(vec3 resultColor, vec3 blendDestinationColor) {
            vec3 result;

            result.r = blendModeSoftLightCore(resultColor.r, blendDestinationColor.r);
            result.g = blendModeSoftLightCore(resultColor.g, blendDestinationColor.g);
            result.b = blendModeSoftLightCore(resultColor.b, blendDestinationColor.b);

            return result;
        }
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec4 blendDestinationColor = texture2D(u_blendOriginalDestinationPixelsTexture, v_destTexturePosition);

        if (blendDestinationColor.a != 0.0 && resultColor.a != 0.0) {
            blendDestinationColor.rgb = blendDestinationColor.rgb / blendDestinationColor.a;
            resultColor.rgb = resultColor.rgb / resultColor.a;

            vec3 blendResult;

            if (u_blendMode == BLEND_MODE_OVERLAY)
                blendResult = blendModeOverlay(resultColor.rgb, blendDestinationColor.rgb);
            else if (u_blendMode == BLEND_MODE_COLOR_DODGE)
                blendResult = blendModeColorDodge(resultColor.rgb, blendDestinationColor.rgb);
            else if (u_blendMode == BLEND_MODE_COLOR_BURN)
                blendResult = blendModeColorBurn(resultColor.rgb, blendDestinationColor.rgb);
            else if (u_blendMode == BLEND_MODE_HARD_LIGHT)
                blendResult = blendModeHardLight(resultColor.rgb, blendDestinationColor.rgb);
            else if (u_blendMode == BLEND_MODE_SOFT_LIGHT)
                blendResult = blendModeSoftLight(resultColor.rgb, blendDestinationColor.rgb);

            resultColor.rgb = (1.0 - blendDestinationColor.a) * resultColor.rgb + blendDestinationColor.a * blendResult.rgb;
            resultColor.rgb *= resultColor.a;
        }
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_complexBlendModeShaderProgram = &ComplexBlendShader::s_program;

GLfloat* PlatformContextTyGL::blendModeSetComplexShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setUniformTexture(shaderArguments[ComplexBlendShader::uBlendOriginalDestinationPixelsTexture], context->pipelineBuffer()->originalDestinationImage.get());
    context->setUniformInt(shaderArguments[ComplexBlendShader::uBlendMode], context->pipelineBuffer()->blendMode);
    glVertexAttribPointer(shaderArguments[ComplexBlendShader::aBlendTexturePosition], 2, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 2;
}

DEFINE_SHADER(
    NonseparableBlendShader,
    BLEND_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec2 a_blendTexturePosition;

        varying vec2 v_destTexturePosition;
    ),
    // Vertex shader
    TyGL_PROGRAM(
        v_destTexturePosition = a_blendTexturePosition;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        uniform sampler2D u_blendOriginalDestinationPixelsTexture;
        uniform int u_blendMode;

        varying vec2 v_destTexturePosition;

        float luminance(vec3 color)
        {
            return 0.3 * color.r + 0.59 * color.g + 0.11 * color.b;
        }

        vec3 clipColor(vec3 color)
        {
            float luminance = luminance(color);
            float min = min(color.r, min(color.g, color.b));
            float max = max(color.r, max(color.g, color.b));

            if (min < 0.0)
                color = vec3(luminance) + (((color - vec3(luminance)) * vec3(luminance)) / vec3(luminance - min));

            if (max > 1.0)
                color = vec3(luminance) + (((color - vec3(luminance)) * vec3(1.0 - luminance)) / vec3(max - luminance));

            return color;
        }

        vec3 setLuminance(vec3 color, float luminanceToSet)
        {
            float difference = luminanceToSet - luminance(color);

            color += difference;

            return clipColor(color);
        }

        float saturation(vec3 color)
        {
            return max(color.r, max(color.g, color.b)) - min(color.r, min(color.g, color.b));
        }

        vec3 setSaturation(vec3 color, float saturationToSet)
        {
            int min;
            int mid;
            int max;

            if(color[0] <= color[1]) {
                if(color[1] <= color[2]) {
                    min = 0;
                    mid = 1;
                    max = 2;
                } else if(color[0] <= color[2]) {
                    min = 0;
                    mid = 2;
                    max = 1;
                } else {
                    min = 2;
                    mid = 0;
                    max = 1;
                }
            } else if(color[0] <= color[2]) {
                min = 1;
                mid = 0;
                max = 2;
            } else if(color[1] <= color[2]) {
                min = 1;
                mid = 2;
                max = 0;
            } else {
                min = 2;
                mid = 1;
                max = 0;
            }

            if (color[max] > color[min]) {
                color[mid] = (((color[mid] - color[min]) * saturationToSet) / (color[max] - color[min]));
                color[max] = saturationToSet;
            } else {
                color[mid] = 0.0;
                color[max] = 0.0;
            }

            color[min] = 0.0;
            return color;
        }
    ),
    // Fragment shader
    TyGL_PROGRAM(
        vec4 blendDestinationColor = texture2D(u_blendOriginalDestinationPixelsTexture, v_destTexturePosition);

        if (blendDestinationColor.a != 0.0 && resultColor.a != 0.0) {
            blendDestinationColor.rgb = blendDestinationColor.rgb / blendDestinationColor.a;
            resultColor.rgb = resultColor.rgb / resultColor.a;

            vec3 blendResult;

            if (u_blendMode == BLEND_MODE_HUE)
                blendResult = setLuminance(setSaturation(resultColor.rgb, saturation(blendDestinationColor.rgb)), luminance(blendDestinationColor.rgb));
            else if (u_blendMode == BLEND_MODE_SATURATION)
                blendResult = setLuminance(setSaturation(blendDestinationColor.rgb, saturation(resultColor.rgb)), luminance(blendDestinationColor.rgb));
            else if (u_blendMode == BLEND_MODE_COLOR)
                blendResult = setLuminance(resultColor.rgb, luminance(blendDestinationColor.rgb));
            else if (u_blendMode == BLEND_MODE_LUMINOSITY)
                blendResult = setLuminance(blendDestinationColor.rgb, luminance(resultColor.rgb));

            resultColor.rgb = (1.0 - blendDestinationColor.a) * resultColor.rgb + blendDestinationColor.a * blendResult.rgb;
            resultColor.rgb *= resultColor.a;
        }
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_nonseparableBlendModeShaderProgram = &NonseparableBlendShader::s_program;

GLfloat* PlatformContextTyGL::blendModeSetNonseparableShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setUniformTexture(shaderArguments[NonseparableBlendShader::uBlendOriginalDestinationPixelsTexture], context->pipelineBuffer()->originalDestinationImage.get());
    context->setUniformInt(shaderArguments[NonseparableBlendShader::uBlendMode], context->pipelineBuffer()->blendMode);
    glVertexAttribPointer(shaderArguments[NonseparableBlendShader::aBlendTexturePosition], 2, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 2;
}

void PlatformContextTyGL::blendModeSetShaderAttributes()
{
    addAttributePosition(0.0, 0.0, 1.0, 1.0);
}

} // namespace WebCore
