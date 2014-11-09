/*
 * Copyright (C) 2013 University of Szeged
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

#ifndef ScaledFontTyGL_h
#define ScaledFontTyGL_h

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#include <wtf/RefPtr.h>
#include <wtf/RefCounted.h>

typedef struct _FcPattern FcPattern;

namespace WTF {
template<> void refIfNotNull(FcPattern*);
template<> void derefIfNotNull(FcPattern*);
}

namespace WebCore {

struct FontFaceCacheEntry;

class ScaledFont : public RefCounted<ScaledFont> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ScaledFont> create(const FcPattern* pattern) { return adoptRef(new ScaledFont(pattern)); }

    ~ScaledFont();

    FT_Face fontFace();

    float size() const { return m_size; }
    void setSize(float size) { m_size = size; }

    bool operator==(const ScaledFont&) const;

private:
    ScaledFont(const FcPattern*);
    bool getFaceFromFcPattern(const FcPattern*, FT_Face*);
    bool newFaceFromFcPattern(const FcPattern*, FT_Face*);

    float m_size;
    RefPtr<FontFaceCacheEntry> m_cachedFace;
    unsigned m_patternHash;
};

} // namespace WebCore

#endif // ScaledFontTyGL_h
