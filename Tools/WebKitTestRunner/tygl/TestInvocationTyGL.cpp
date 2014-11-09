/*
 * Copyright (C) 2013, 2014 University of Szeged
 * Copyright (C) 2013 Samsung Electronics Ltd.
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
#include "TestInvocation.h"

#include "PixelDumpSupport.h"
#include "PngUtilsTyGL.h"
#include <WKImageTyGL.h>
#include <wtf/MD5.h>
#include <wtf/StringExtras.h>

namespace WTR {

void computeMD5HashStringForImageTyGL(ImageTyGL* image, char hashString[33])
{
    MD5 md5Context;
    md5Context.addBytes(image->data, image->width * image->height * 4);

    MD5::Digest hash;
    md5Context.checksum(hash);

    snprintf(hashString, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7],
        hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);
}

static void dumpBitmap(ImageTyGL* image, const char* checksum)
{
    Vector<unsigned char> png;
    imageTyGLToPng(image, &png);

    printPNG(png.data(), png.size(), checksum);
}

void TestInvocation::dumpPixelsAndCompareWithExpected(WKImageRef wkImage, WKArrayRef repaintRects)
{
    ImageTyGL* image = WKImageToData(wkImage);

    // FIXME: A non-null repaintRects array means we're doing a repaint test.
    char actualHash[33];
    computeMD5HashStringForImageTyGL(image, actualHash);

    if(!compareActualHashToExpectedAndDumpResults(actualHash))
        dumpBitmap(image, actualHash);

    delete image;
}

} // namespace WTR
