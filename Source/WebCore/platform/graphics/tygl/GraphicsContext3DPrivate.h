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

#ifndef GraphicsContext3DPrivate_h
#define GraphicsContext3DPrivate_h

#if USE(3D_GRAPHICS) || USE(ACCELERATED_COMPOSITING)

#include "GraphicsContext3D.h"

#if USE(TEXTURE_MAPPER_GL)
#include <texmap/TextureMapperPlatformLayer.h>
#endif

#include "GLPlatformContext.h"

class PageClientEfl;

namespace WebCore {
class GraphicsContext3DPrivate
#if USE(TEXTURE_MAPPER_GL)
        : public TextureMapperPlatformLayer
#endif
{
public:
    static std::unique_ptr<GraphicsContext3DPrivate> create(GraphicsContext3D*, HostWindow*);
    ~GraphicsContext3DPrivate();

    bool createSurface(PageClientEfl*, bool);
    PlatformGraphicsContext3D platformGraphicsContext3D() const;
    void setContextLostCallback(std::unique_ptr<GraphicsContext3D::ContextLostCallback>);
#if USE(TEXTURE_MAPPER_GL)
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float) override;
#endif
#if USE(GRAPHICS_SURFACE)
    virtual IntSize platformLayerSize() const override;
    virtual uint32_t copyToGraphicsSurface() override;
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const override;
    virtual GraphicsSurface::Flags graphicsSurfaceFlags() const override;
    void didResizeCanvas(const IntSize&);
#endif
    bool makeContextCurrent() const;

private:
#if USE(GRAPHICS_SURFACE)
    enum PendingOperation {
        Default = 0x00, // No Pending Operation.
        CreateSurface = 0x01,
        Resize = 0x02,
        DeletePreviousSurface = 0x04
    };

    typedef unsigned PendingSurfaceOperation;
#endif

    GraphicsContext3DPrivate(GraphicsContext3D*, HostWindow*);
    bool initialize();
    void createGraphicsSurface();
    bool prepareBuffer() const;
    void releaseResources();

    GraphicsContext3D* m_context;
    HostWindow* m_hostWindow;
    OwnPtr<GLPlatformContext> m_offScreenContext;
    OwnPtr<GLPlatformSurface> m_offScreenSurface;
#if USE(GRAPHICS_SURFACE)
    GraphicsSurfaceToken m_surfaceHandle;
    RefPtr<GraphicsSurface> m_graphicsSurface;
    RefPtr<GraphicsSurface> m_previousGraphicsSurface;
    PendingSurfaceOperation m_surfaceOperation : 3;
#endif
    OwnPtr<GraphicsContext3D::ContextLostCallback> m_contextLostCallback;
    ListHashSet<GC3Denum> m_syntheticErrors;
    IntSize m_size;
    IntRect m_targetRect;
};

} // namespace WebCore

#endif

#endif // GraphicsContext3DPrivate_h
