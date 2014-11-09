/*
 * Copyright (C) 2014 Samsung Electronics Ltd.
 * Copyright (C) 2014 Nicolas P. Rougier
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
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ``AS IS'' AND ANY
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
#ifndef FontTextureAtlasTyGL_h
#define FontTextureAtlasTyGL_h

#include "IntRect.h"
#include "NativeImageTyGL.h"
#include <wtf/Vector.h>

namespace WebCore {
namespace TyGL {

class FontTextureAtlasTyGL : public NativeImageTyGL {
public:
    static const int DefaultAtlasSide = 64;
    static const int MaximumAtlasSide = 4096;
    static PassRefPtr<FontTextureAtlasTyGL> create(const IntSize& size)
    {
        return adoptRef(new FontTextureAtlasTyGL(size.width(), size.height()));
    }

    IntRect region(int, int);
    void copyGlyphBitmapToRegion(const IntRect&, const unsigned char*, size_t, size_t);
    void update();

private:
    FontTextureAtlasTyGL()
        : NativeImageTyGL(0, 0)
        , m_usedMemory(0)
        , m_dirty(false)
    {
    }

    FontTextureAtlasTyGL(int width, int height)
        : NativeImageTyGL(width, height, 0, AlphaImage)
        , m_atlasBackingStore((width * height + 3) / 4)
        , m_usedMemory(0)
        , m_dirty(false)
    {
    }

    int fit(int, const int, const int) const;
    void mergeNodes();
    void shrinkNodes(int);

    // Backing store for the atlas. It stores one channel alpha images for the glyphs.
    // The store buffer is made aligned by 4 bytes boundary to speed up the atlas texture upload.
    Vector<uint32_t> m_atlasBackingStore;
    // Bottom row of allocated regions in the atlas.
    Vector<IntRect> m_nodes;

    int m_usedMemory;
    bool m_dirty;
};

} // namespace TyGL
} // namespace WebCore

#endif // FontTextureAtlasTyGL_h
