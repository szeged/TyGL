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

#include "config.h"
#include "ScaledFontTyGL.h"

#include <wtf/HashMap.h>

namespace WTF {
template<> void refIfNotNull(FcPattern* ptr)
{
    if (LIKELY(ptr != 0))
        FcPatternReference(ptr);
}

template<> void derefIfNotNull(FcPattern* ptr)
{
    if (LIKELY(ptr != 0))
        FcPatternDestroy(ptr);
}
}

namespace WebCore {

using namespace WTF;

struct FontFaceCacheEntry : public RefCounted<FontFaceCacheEntry> {
public:
    static PassRefPtr<FontFaceCacheEntry> create(FT_Face fontFace, bool isOwned) { return adoptRef(new FontFaceCacheEntry(fontFace, isOwned)); }

    ~FontFaceCacheEntry()
    {
        if (faceIsOwned)
            FT_Done_Face(face);
    }

    FT_Face face;
    bool faceIsOwned;

private:
    FontFaceCacheEntry(FT_Face fontFace, bool isOwned)
        : face(fontFace)
        , faceIsOwned(isOwned)
    {
    }
};

static FT_Library freeTypeLibrary()
{
    static FT_Library library;
    if (!library && FT_Init_FreeType(&library)) {
        library = nullptr;
        return nullptr;
    }
    return library;
}

typedef HashMap<unsigned, RefPtr<FontFaceCacheEntry>> FontFaceCache;

static FontFaceCache& fontFaceCache()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(FontFaceCache, fontCache, ());
    return fontCache;
}

ScaledFont::ScaledFont(const FcPattern* pattern)
    : m_size(0)
    , m_cachedFace(nullptr)
    , m_patternHash(FcPatternHash(pattern))
{
    FT_Face face = 0;
    bool faceIsOwned = false;

    FontFaceCache::iterator iterator = fontFaceCache().find(m_patternHash);
    if (iterator != fontFaceCache().end()) {
        m_cachedFace = iterator->value;
    } else {
        if (!getFaceFromFcPattern(pattern, &face)) {
            faceIsOwned = true;
            if (!newFaceFromFcPattern(pattern, &face))
                CRASH();
        }
        m_cachedFace = FontFaceCacheEntry::create(face, faceIsOwned);
        fontFaceCache().add(m_patternHash, m_cachedFace);
    }
}

bool ScaledFont::getFaceFromFcPattern(const FcPattern* pattern, FT_Face* face)
{
    return FcPatternGetFTFace(pattern, FC_FT_FACE, 0, face) == FcResultMatch;
}

bool ScaledFont::newFaceFromFcPattern(const FcPattern* pattern, FT_Face* face)
{
    FcChar8* fontFilename;
    if (FcPatternGetString(pattern, FC_FILE, 0, &fontFilename) == FcResultMatch) {
        if (!FT_New_Face(freeTypeLibrary(), reinterpret_cast<char*>(fontFilename), 0, face))
            return true;
    }
    LOG_ERROR("Failed to find a FreeType Face from the given pattern.");
    return false;
}

bool ScaledFont::operator==(const ScaledFont& other) const
{
    return m_patternHash == other.m_patternHash;
}

ScaledFont::~ScaledFont()
{
    ASSERT(m_cachedFace && m_cachedFace->face);
    // When the reference count is two at this point,
    // that means the current ScaledFont instance has one
    // reference and the cache itself has another. We can remove it
    // from the cache already, and it will be deleted by
    // the RefPtr destructor.
    if (m_cachedFace->refCount() == 2)
        fontFaceCache().remove(m_patternHash);
}

FT_Face ScaledFont::fontFace()
{
    return m_cachedFace->face;
}

}
