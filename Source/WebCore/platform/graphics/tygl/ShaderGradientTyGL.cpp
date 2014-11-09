/*
 * Copyright (C) 2013, 2014 University of Szeged
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
#include "PlatformContextTyGL.h"

#include "AffineTransform.h"
#include "Gradient.h"
#include "NativeImageTyGL.h"
#include "PathTyGL.h"
#include "ShaderCommonTyGL.h"

namespace WebCore {

using TyGL::min;
using TyGL::max;

static void prepareStopPointInfoTexture(RefPtr<NativeImageTyGL>& currentGradientStopInfoTexture, const Vector<float>& stopPointDistances, const Vector<float>& stopPointColors  )
{
    const int numStops = stopPointDistances.size();
    const int textureWidth = numStops;
    const int textureHeight = 2;
    // Ensure the cached texture is large enough.
    if (!currentGradientStopInfoTexture || (currentGradientStopInfoTexture->width() < textureWidth || currentGradientStopInfoTexture->height() < textureHeight)) {
        // Old texture is not big enough to accommodate new number of stop points; use a new one and dispose of old.
        currentGradientStopInfoTexture = NativeImageTyGL::create(textureWidth, textureHeight);
    }
    const int pixelDataSize = 4; // BGRA.
    Vector<GLubyte> gradientStopInfoTextureData(textureWidth * textureHeight * pixelDataSize);

    for (int i = 0; i < numStops; i++) {
        gradientStopInfoTextureData[i * pixelDataSize + 0] = stopPointDistances[i] * 255.0;
        gradientStopInfoTextureData[i * pixelDataSize + 1] = stopPointDistances[i] * 255.0;
        gradientStopInfoTextureData[i * pixelDataSize + 2] = stopPointDistances[i] * 255.0;
        gradientStopInfoTextureData[i * pixelDataSize + 3] = 255;

        gradientStopInfoTextureData[i * pixelDataSize + textureWidth * pixelDataSize + 0] = stopPointColors[i * pixelDataSize + 2] * 255.0;
        gradientStopInfoTextureData[i * pixelDataSize + textureWidth * pixelDataSize + 1] = stopPointColors[i * pixelDataSize + 1] * 255.0;
        gradientStopInfoTextureData[i * pixelDataSize + textureWidth * pixelDataSize + 2] = stopPointColors[i * pixelDataSize + 0] * 255.0;
        gradientStopInfoTextureData[i * pixelDataSize + textureWidth * pixelDataSize + 3] = stopPointColors[i * pixelDataSize + 3] * 255.0;
    }
    // Use tightly packed data.
    glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  textureWidth, textureHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, gradientStopInfoTextureData.data());
    // Set the filtering mode
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

// See below for the meanings of "m" and "d".
// The stop point info (each stop point is a "distance" from 0.0 to 1.0 together with an RGB value) is encoded in a texture so that we can
// have (more-or-less) an arbitrary number of them.  The texture is generally numStops x 2, although we will re-use textures that can accommodate
// the current number of stop points that we wish to use, so can be larger.  The first row of pixels represents the distances; the
// second the colours.
// The "t1" is basically the progress along the line from start to end points (ranging from 0.0 to 1.0)
// of the current pixel when we project it onto this line for linear gradients, and the progress between start and end radius for radial gradients.
#define GRADIENT_SHADER_ARGUMENTS(argument) \
    argument(u, commonData, CommonData, 4) \
    argument(u, gradientStopInfoTexture, GradientStopInfoTexture, 1) \
    argument(u, textureWidth, TextureWidth, 1) \
    argument(u, linearData, LinearData, 4) \
    argument(u, radialData, RadialData, 4) \
    argument(u, transform, Transform, 4)

DEFINE_SHADER(
    GradientShader,
    GRADIENT_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_EMPTY_PROGRAM,
    // Vertex shader
    TyGL_EMPTY_PROGRAM,
    // Fragment variables
    TyGL_PROGRAM(
        uniform vec4 u_commonData;
        uniform sampler2D u_gradientStopInfoTexture;
        // u_textureWidth might not match u_numStops if we are re-using a
        // texture that was previously used for a greater number of stops.
        uniform int u_textureWidth;
        uniform vec4 u_linearData;
        uniform vec4 u_radialData;
        uniform vec4 u_transform;

        void GetStopDistance(in int stopPointIndex, out float stopDistance)
        {
            // We are storing the stop distances in the first row of the texture, and we want to read them back without any distortion caused by e.g. interpolation.
            // To ensure this, we read from the *centre* of each pixel e.g. 3rd pixel on first row -> read from (2.5, 0.25).
            stopDistance = texture2D( u_gradientStopInfoTexture, vec2((2.0 * float(stopPointIndex) + 1.0) / 2.0 / float(u_textureWidth), 0.25))[0];
        }

        void GetStopColor(in int stopPointIndex, out vec4 stopColor)
        {
            // As with GetStopDistance, but we are reading from the second row.
            stopColor = texture2D( u_gradientStopInfoTexture, vec2((2.0 * float(stopPointIndex) + 1.0) / 2.0 / float(u_textureWidth), 0.75));
        }
    ),
    // Fragment shader
    TyGL_PROGRAM(
        float t1 = 0.0;
        // TODO - this is just multiplication of a 2D-vector by a 2x2 "matrix" - find built-in way of doing this.
        float u = gl_FragCoord.x * u_transform[0] + gl_FragCoord.y * u_transform[2];
        float v = gl_FragCoord.x * u_transform[1] + gl_FragCoord.y * u_transform[3];
        vec2 startPoint = vec2(u_commonData[0], u_commonData[1]);
        int isRadial = int(u_commonData[2]);
        vec4 color;
        if (isRadial == 0) {
            vec2 endPoint = vec2(u_linearData[0], u_linearData[1]);
            float m = u_linearData[2];
            float d = u_linearData[3];
            if (startPoint[1] != endPoint[1]) {
                // Storing "u_d squared" might seem more sensible, here, but we're dealing with 16-bit floats and they can
                // easily overflow.  For much the same reason, we alternate divisions and multiplications.
                t1 = (v + m * (startPoint[0] - u) - startPoint[1]) / d * (endPoint[1] - startPoint[1]) / d;
            } else {
                // Horizontal gradients need to be special-cased ("m" would be undefined).
                t1 = (u - startPoint[0]) / (endPoint[0] - startPoint[0]);
            }
        } else {
            // Radial ellipses/ circles.
            float startRadius = u_radialData[0];
            float endRadius = u_radialData[1];
            float aspectRatio = u_radialData[2];
            t1 = (length(vec2(u - startPoint[0], (v - startPoint[1]) * aspectRatio)) - startRadius) / (endRadius - startRadius);
        }
        GetStopColor(0, color);
        int numStops = int(u_commonData[3]);
        if (t1 >= 0.0) {
            if (t1 >= 1.0)
                GetStopColor(numStops - 1, color);
            else {
                // Find which stop point our t1 is at, and interpolate colors between that and the previous one.
                int stopPointIndex = 0;
                while (true) {
                    float stopPointDistance;
                    GetStopDistance(stopPointIndex, stopPointDistance);
                    if (t1 <= stopPointDistance)
                        break;

                    ++stopPointIndex;
                }
                if (stopPointIndex == 0) {
                    // A fairly rare occurrence - without this, we will have occasional missing
                    // pixels (our tests show the issue).
                    stopPointIndex = 1;
                }
                vec4 stopPointStartColor;
                GetStopColor(stopPointIndex - 1, stopPointStartColor);
                vec4 stopPointEndColor;
                GetStopColor(stopPointIndex, stopPointEndColor);
                float stopPointDistanceBegin;
                GetStopDistance(stopPointIndex - 1, stopPointDistanceBegin);
                float stopPointDistanceEnd;
                GetStopDistance(stopPointIndex, stopPointDistanceEnd);
                color = mix(stopPointStartColor, stopPointEndColor, (t1 - stopPointDistanceBegin) / (stopPointDistanceEnd - stopPointDistanceBegin));
            }
        }
        resultColor = color * resultColor.a;
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_gradientShaderProgram = &GradientShader::s_program;

GLfloat* PlatformContextTyGL::gradientSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    RefPtr<Gradient> gradient = context->pipelineBuffer()->gradient;

    ASSERT(gradient);

    ASSERT(gradient->getStops().size() >= 1);

    Vector<Gradient::ColorStop> colorStops = gradient->getStops();
    // If the first stop is not at 0.0, we must add a "pseudo-stop" at the beginning, at distance 0.0
    // with color equal to the first color.
    if (colorStops[0].stop != 0.0)
        colorStops.insert(0, Gradient::ColorStop(0.0, colorStops[0].red, colorStops[0].green, colorStops[0].blue, colorStops[0].alpha));

    // If the last color stop is not 1.0, append one with the same color as the previous stop - this ensures that the while loop inside
    // the shader will terminate, and also fixes a corner case where just one stop is provided.
    if (colorStops.last().stop != 1.0) {
        Gradient::ColorStop originalLastColorStop = colorStops.last();
        colorStops.append(Gradient::ColorStop(1.0, originalLastColorStop.red, originalLastColorStop.green, originalLastColorStop.blue, originalLastColorStop.alpha));
    }
    const int numStops = colorStops.size();

    Vector<float> stopPointDistances(numStops);
    Vector<float> stopPointColors(numStops * 4);

    for (int i = 0; i < numStops; i++) {
        // Store the stop data in a way that is convenient to upload to the texture.
        Gradient::ColorStop colorStop = colorStops[i];
        stopPointDistances[i] = colorStop.stop;
        stopPointColors[i * 4 + 0] = colorStop.red;
        stopPointColors[i * 4 + 1] = colorStop.green;
        stopPointColors[i * 4 + 2] = colorStop.blue;
        stopPointColors[i * 4 + 3] = colorStop.alpha;
    }

    // Calling setUniformTexture has the effect of setting that texture as the active texture; creating a NativeImageTyGL (which can happen in
    // prepareStopPointInfoTexture(...)) binds that texture.  The result of  this is that if a previous shader has called
    // setUniformTexture, and we then create a NativeImageTyGL for our gradient stop info texture, our gradient stop info texture can
    // *replace* the uniform texture that the earlier shader set, which is almost certainly not what we want!
    // Call setUniformTexture first, even if we end up creating and using a new texture: this ensures that if we do create a new texture,
    // it doesn't overwrite earlier textures.
    context->setUniformTexture(shaderArguments[GradientShader::uGradientStopInfoTexture], context->m_gradientStopInfoImage.get());
    prepareStopPointInfoTexture(context->m_gradientStopInfoImage, stopPointDistances, stopPointColors);

    // We handle the transform within the shader itself, rather than let OpenGL ES interpolate for us: this is because gradients
    // can have their own transform that differs from the GraphicsContext's transform (via SVG's "gradientTransform" attribute), and we
    // can't guarantee that the shape to fill has set the texture coordinates appropriately.
    // Note that the shader maps *rendered* coordinates (i.e. pixels) back to the "gradient" coordinate-space i.e. the inverse of what
    // we usually do: hence the usage of "inverse" below.
    const TyGL::AffineTransform globalTransformInverse = context->pipelineBuffer()->complexFillTransform.inverse();
    const WebCore::AffineTransform gradientTransformWebCore = gradient->gradientSpaceTransform().inverse();
    const TyGL::AffineTransform::Transform gradientTransform = {gradientTransformWebCore.a(), gradientTransformWebCore.b(), gradientTransformWebCore.c(), gradientTransformWebCore.d(), gradientTransformWebCore.e(), gradientTransformWebCore.f() };
    TyGL::AffineTransform finalTransform = globalTransformInverse;
    finalTransform.multiply(gradientTransform);

    // Radial gradients must compute distances between pixels which, internally, uses a square of the x and y differences: if either of these
    // differences exceeds 256, we get an overflow.  Therefore, for radial gradients, we scale everything by a factor to avoid this.
    // The factor is currently largely arbitrary, based on a "sensible" "maximum" rendered gradient size - if the gradient "cuts out" at a certain
    // distance, then you should increase this.
    const float antiOverFlowScale = gradient->isRadial() ?  2.0 : 1.0;
    finalTransform.scale(1.0 / antiOverFlowScale, 1.0 / antiOverFlowScale);

    // Pre-translate the points: this way, we can pass the remaining transform to the shader in just
    // 4 floats, instead of 6.
    const float translateX = -finalTransform.xTranslation();
    const float translateY = -finalTransform.yTranslation();

    const FloatPoint startPoint = FloatPoint((gradient->p0().x() + translateX) / antiOverFlowScale, (gradient->p0().y() + translateY) / antiOverFlowScale);

    glUniform4f(shaderArguments[GradientShader::uCommonData], startPoint.x(), startPoint.y(), gradient->isRadial() ? 1 : 0, numStops);
    glUniform1i(shaderArguments[GradientShader::uTextureWidth], static_cast<GLint>(context->m_gradientStopInfoImage->width()));

    glUniform4f(shaderArguments[GradientShader::uTransform], finalTransform.transform().m_matrix[0], finalTransform.transform().m_matrix[1], finalTransform.transform().m_matrix[2], finalTransform.transform().m_matrix[3]);

    if (!gradient->isRadial()) {
        // Linear-specific stuff.
        const FloatPoint endPoint = FloatPoint(gradient->p1().x() + translateX, gradient->p1().y() + translateY);
        // "m" is the gradient of the line at right angles to the line from (startX, startY) - (endX, endPoint.y()),
        // or "0" if the gradient is horizontal, in which case it ends up not being used by the shader.
        const GLfloat m =  (startPoint.y() != endPoint.y()) ?  (startPoint.x() - endPoint.x()) / (endPoint.y() - startPoint.y()) : 0.0;
        // "d" is the length of the line from (startX, startY) - (endPoint.x(), endPoint.y()).
        const GLfloat d = sqrt((endPoint.x() - startPoint.x()) * (endPoint.x() - startPoint.x()) + (endPoint.y() - startPoint.y()) * (endPoint.y() - startPoint.y()));
        glUniform4f(shaderArguments[GradientShader::uLinearData], endPoint.x(), endPoint.y(), m, d);
    } else {
        // Radial-specific stuff.
        glUniform4f(shaderArguments[GradientShader::uRadialData], gradient->startRadius() * 1.0 / antiOverFlowScale, gradient->endRadius() * 1.0 / antiOverFlowScale, gradient->aspectRatio(), 0.0);
    }

    return attributeStart;
}
} // namespace WebCore
