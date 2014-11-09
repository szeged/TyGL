/*
 * Copyright (C) 2013, 2014 University of Szeged.
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

#ifndef PngUtilsTyGL_h
#define PngUtilsTyGL_h

#include "WKImageTyGL.h"
#include <wtf/Vector.h>

extern "C" {
#include <png.h>
}

using namespace WTF;

static void writeFunction(png_structp png, unsigned char* data, png_size_t length)
{
    Vector<unsigned char>* out = reinterpret_cast<Vector<unsigned char>*>(png_get_io_ptr(png));
    out->append(data, length);
}

static void pngSimpleOutputFlush(png_structp pngPtr)
{
}

void imageTyGLToPng(ImageTyGL* image, Vector<unsigned char>* png)
{
    int width = image->width ;
    int height = image->height;

    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;
    png_byte** rowPointers = nullptr;

    int pixelSize = 4;
    int depth = 8;
    int rowSize = sizeof(uint8_t) * width * pixelSize;

    pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!pngPtr) {
        LOG_ERROR("Cannot create pngPtr for png file writing.");
        return;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        LOG_ERROR("Cannot create infoPtr for png file writing.");
        return;
    }

    png_set_IHDR(pngPtr, infoPtr, width, height, depth, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    rowPointers = reinterpret_cast<png_byte**>(png_malloc(pngPtr, height * sizeof(png_byte*)));

    for (int y = 0; y < height; ++y) {
        unsigned char* source_row = image->data + (y * rowSize);
        png_byte* destination_row = reinterpret_cast<png_byte*>(png_malloc(pngPtr, rowSize));
        memcpy(destination_row, source_row, rowSize);
        rowPointers[y] = destination_row;
    }

    png_set_write_fn(pngPtr, png, writeFunction, pngSimpleOutputFlush);
    png_set_rows(pngPtr, infoPtr, rowPointers);
    png_write_png(pngPtr, infoPtr, PNG_TRANSFORM_IDENTITY, nullptr);

    for (int y = 0; y < height; ++y)
        png_free(pngPtr, rowPointers[y]);

    png_free(pngPtr, rowPointers);

    png_destroy_write_struct(&pngPtr, &infoPtr);
}

#endif
