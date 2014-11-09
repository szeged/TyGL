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

#ifndef NativeImageTyGL_h
#define NativeImageTyGL_h

#include "TyGLDefs.h"
#include "IntSize.h"
#include <wtf/Assertions.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class NativeImageTyGL : public RefCounted<NativeImageTyGL> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static const unsigned NoOptions = 0;
    static const unsigned HasAlphaChannel = 1 << 0;
    static const unsigned AlphaImage = 1 << 1;
    static const unsigned PremultipliedAlpha = 1 << 2;

    static PassRefPtr<NativeImageTyGL> create(int width, int height, const void* buffer = 0, unsigned options = NoOptions)
    {
        return adoptRef(new NativeImageTyGL(width, height, buffer, options));
    }

    static PassRefPtr<NativeImageTyGL> create(const IntSize& size, const void* buffer = 0, unsigned options = NoOptions)
    {
        return create(size.width(), size.height(), buffer, options);
    }

    PassRefPtr<NativeImageTyGL> clone();

    static PassRefPtr<NativeImageTyGL> createShared(int width, int height, uintptr_t sharedImageHandle = 0)
    {
        return adoptRef(new NativeImageTyGL(width, height, sharedImageHandle));
    }

    static PassRefPtr<NativeImageTyGL> createShared(const IntSize& size, uintptr_t sharedImageHandle = 0)
    {
        return createShared(size.width(), size.height(), sharedImageHandle);
    }

    ~NativeImageTyGL();

    const IntSize& size() const { return m_size; }
    int width() const { return m_size.width(); }
    int height() const { return m_size.height(); }
    uintptr_t sharedImageHandle() { return m_sharedImageHandle; }
    GLuint texture() const { return m_texture; }
    size_t sizeInBytes() const { return width() * height() * 4; }
    bool hasAlphaChannel() const { return m_options & HasAlphaChannel; }
    GLuint fbo() const { return m_fbo; }
    uint32_t pixelColor() const { return m_pixelColor; }

    void bindTexture() { glBindTexture(GL_TEXTURE_2D, m_texture); }
    void bindFbo();

protected:
    NativeImageTyGL(int, int, const void* = 0, unsigned options = NoOptions);
    NativeImageTyGL(int, int, uintptr_t);

private:
    IntSize m_size;
    // m_sharedImageHandle and m_texture cannot be non 0 in the same time.
    uintptr_t m_sharedImageHandle;
    void* m_sharedDisplayHandle;
    uintptr_t m_privateImageHandle;
    GLuint m_texture;
    GLuint m_fbo;
    GLuint m_renderBuffer;
    unsigned m_options;
    uint32_t m_pixelColor;
};

}
#endif // NativeImageTyGL_h
