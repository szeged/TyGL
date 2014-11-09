/*
 * Copyright (C) 2010, 2011 Apple Inc.
 * Copyright (C) 2013 University of Szeged
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ShareableBitmap_h
#define ShareableBitmap_h

#include "SharedMemory.h"
#include <WebCore/IntRect.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if USE(CG)
#include <wtf/RetainPtr.h>
#endif

#if USE(CAIRO)
#include <WebCore/RefPtrCairo.h>
#endif

#if USE(TYGL)
#include <WebCore/NativeImageTyGL.h>
#endif

namespace WebCore {
    class Image;
    class GraphicsContext;
}

namespace WebKit {
    
class ShareableBitmap : public RefCounted<ShareableBitmap> {
public:
    enum Flag {
        NoFlags = 0,
        SupportsAlpha = 1 << 0,
    };
    typedef unsigned Flags;

    class Handle {
        WTF_MAKE_NONCOPYABLE(Handle);
    public:
        Handle();

#if USE(TYGL)
        bool isNull() const { return m_imageHandle; }
#else
        bool isNull() const { return m_handle.isNull(); }
        void clear();
#endif

        void encode(IPC::ArgumentEncoder&) const;
        static bool decode(IPC::ArgumentDecoder&, Handle&);

    private:
        friend class ShareableBitmap;

#if USE(TYGL)
        uintptr_t m_imageHandle;
#else
        mutable SharedMemory::Handle m_handle;
#endif
        WebCore::IntSize m_size;
        Flags m_flags;
    };

    // Create a shareable bitmap that uses malloced memory.
    static PassRefPtr<ShareableBitmap> create(const WebCore::IntSize&, Flags);

    // Create a shareable bitmap whose backing memory can be shared with another process.
    static PassRefPtr<ShareableBitmap> createShareable(const WebCore::IntSize&, Flags);

#if !USE(TYGL)
    // Create a shareable bitmap from an already existing shared memory block.
    static PassRefPtr<ShareableBitmap> create(const WebCore::IntSize&, Flags, PassRefPtr<SharedMemory>);
#endif

    // Create a shareable bitmap from a handle.
    static PassRefPtr<ShareableBitmap> create(const Handle&, SharedMemory::Protection = SharedMemory::ReadWrite);

    // Create a handle.
    bool createHandle(Handle&, SharedMemory::Protection = SharedMemory::ReadWrite);

    ~ShareableBitmap();

    const WebCore::IntSize& size() const { return m_size; }
    WebCore::IntRect bounds() const { return WebCore::IntRect(WebCore::IntPoint(), size()); }

#if !USE(TYGL)
    bool resize(const WebCore::IntSize& size);
#endif

    // Create a graphics context that can be used to paint into the backing store.
    std::unique_ptr<WebCore::GraphicsContext> createGraphicsContext();

    // Paint the backing store into the given context.
    void paint(WebCore::GraphicsContext&, const WebCore::IntPoint& destination, const WebCore::IntRect& source);
    void paint(WebCore::GraphicsContext&, float scaleFactor, const WebCore::IntPoint& destination, const WebCore::IntRect& source);

#if USE(TYGL)
    bool isBackedBySharedMemory() const { return m_sharedImage->sharedImageHandle(); }
#else
    bool isBackedBySharedMemory() const { return m_sharedMemory; }
#endif

    // This creates a bitmap image that directly references the shared bitmap data.
    // This is only safe to use when we know that the contents of the shareable bitmap won't change.
    PassRefPtr<WebCore::Image> createImage();

#if USE(CG)
    // This creates a copied CGImageRef (most likely a copy-on-write) of the shareable bitmap.
    RetainPtr<CGImageRef> makeCGImageCopy();

    // This creates a CGImageRef that directly references the shared bitmap data.
    // This is only safe to use when we know that the contents of the shareable bitmap won't change.
    RetainPtr<CGImageRef> makeCGImage();
#elif USE(CAIRO)
    // This creates a BitmapImage that directly references the shared bitmap data.
    // This is only safe to use when we know that the contents of the shareable bitmap won't change.
    PassRefPtr<cairo_surface_t> createCairoSurface();
#elif USE(TYGL)
    PassRefPtr<WebCore::NativeImageTyGL> sharedImage() { return m_sharedImage; }
#endif

private:
#if USE(TYGL)
    ShareableBitmap(const WebCore::IntSize&, Flags, PassRefPtr<WebCore::NativeImageTyGL>);
#else
    ShareableBitmap(const WebCore::IntSize&, Flags, void*);
    ShareableBitmap(const WebCore::IntSize&, Flags, PassRefPtr<SharedMemory>);
#endif

#if USE(CAIRO)
    static size_t numBytesForSize(const WebCore::IntSize&);
#else
    static size_t numBytesForSize(const WebCore::IntSize& size) { return size.width() * size.height() * 4; }
#endif

#if USE(CG)
    RetainPtr<CGImageRef> createCGImage(CGDataProviderRef) const;
    static void releaseBitmapContextData(void* typelessBitmap, void* typelessData);
    static void releaseDataProviderData(void* typelessBitmap, const void* typelessData, size_t);
#endif

#if USE(CAIRO)
    static void releaseSurfaceData(void* typelessBitmap);
#endif

#if !USE(TYGL)
    void* data() const;
#endif
    size_t sizeInBytes() const { return numBytesForSize(m_size); }

    WebCore::IntSize m_size;
    Flags m_flags;

    // If the shareable bitmap is backed by shared memory, this points to the shared memory object.
    RefPtr<SharedMemory> m_sharedMemory;

    // If the shareable bitmap is backed by fastMalloced memory, this points to the data.
    void* m_data;

#if USE(TYGL)
    RefPtr<WebCore::NativeImageTyGL> m_sharedImage;
#endif
};

} // namespace WebKit

#endif // ShareableBitmap_h
