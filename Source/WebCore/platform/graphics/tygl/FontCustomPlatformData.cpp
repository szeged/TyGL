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
#include "FontCustomPlatformData.h"

#include "FontPlatformData.h"
#include "SharedBuffer.h"

namespace WebCore {

FontCustomPlatformData::FontCustomPlatformData(FT_Face freeTypeFace, SharedBuffer& buffer)
    : m_freeTypeFace(freeTypeFace)
    , m_buffer(&buffer)
{
}

std::unique_ptr<FontCustomPlatformData> createFontCustomPlatformData(SharedBuffer& buffer)
{
    static FT_Library library;
    if (!library && FT_Init_FreeType(&library)) {
        library = nullptr;
        return nullptr;
    }

    FT_Face freeTypeFace;
    if (FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte*>(buffer.data()), buffer.size(), 0, &freeTypeFace))
        return nullptr;
    return std::make_unique<FontCustomPlatformData>(freeTypeFace, buffer);
}

FontCustomPlatformData::~FontCustomPlatformData()
{
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalIgnoringCase(format, "truetype") || equalIgnoringCase(format, "opentype") || equalIgnoringCase(format, "woff");
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic, FontOrientation orientation,
                                                          FontWidthVariant, FontRenderingMode mode)
{
    return FontPlatformData(m_freeTypeFace, size, bold, italic, orientation);
}
} //namespace WebCore
