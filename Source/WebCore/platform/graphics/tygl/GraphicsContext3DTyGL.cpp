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
#include "GraphicsContext3D.h"

#if USE(3D_GRAPHICS)

#include "GraphicsContext3DPrivate.h"
#include <wtf/OwnPtr.h>

#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#else
#include "Extensions3DOpenGL.h"
#include "OpenGLShims.h"
#endif

namespace WebCore {

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attributes, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    ASSERT(renderStyle != RenderDirectlyToHostWindow);

    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attributes, hostWindow, renderStyle));
    return context->m_private ? context.release() : 0;
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attributes, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_compiler(isGLES2Compliant() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT)
    , m_attrs(attributes)
    , m_renderStyle(renderStyle)
    , m_texture(0)
    , m_compositorTexture(0)
    , m_fbo(0)
#if USE(OPENGL_ES_2)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
#endif
    , m_depthStencilBuffer(0)
    , m_layerComposited(false)
    , m_internalColorFormat(0)
    , m_multisampleFBO(0)
    , m_multisampleDepthStencilBuffer(0)
    , m_multisampleColorBuffer(0)
    , m_private(GraphicsContext3DPrivate::create(this, hostWindow))
{
    validateAttributes();

    // ANGLE initialization.
    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;
    m_compiler.setResources(ANGLEResources);

#if !USE(OPENGL_ES_2)
    ::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    ::glEnable(GL_POINT_SPRITE);
#endif
}

GraphicsContext3D::~GraphicsContext3D()
{
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D()
{
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight, PlatformContextTyGL* context)
{
}

//#if !USE(OPENGL_ES_2)
void GraphicsContext3D::createGraphicsSurfaces(const IntSize& size)
{
}
//#endif

bool GraphicsContext3D::makeContextCurrent()
{
    return true;
}

void GraphicsContext3D::setContextLostCallback(std::unique_ptr<ContextLostCallback>)
{
}

void GraphicsContext3D::setErrorMessageCallback(std::unique_ptr<ErrorMessageCallback>)
{
}

bool GraphicsContext3D::isGLES2Compliant() const
{
#if USE(OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
}

PlatformLayer* GraphicsContext3D::platformLayer() const
{
}

}

#endif //USE(3D_GRAPHICS)
