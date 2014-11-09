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

#ifndef TyGLDefs_h
#define TyGLDefs_h

// GLDefs.h may define some symbols, which clashes with other WebKit symbols,
// such as None, Cursor. Therefore it cannot be included by TyGL header files.

#include <wtf/tygl/Evas_TyGL.h>

namespace WebCore {
namespace TyGL {

ALWAYS_INLINE int max(int a, int b) { return (a > b) ? a : b; }
ALWAYS_INLINE int min(int a, int b) { return (a < b) ? a : b; }

ALWAYS_INLINE GLfloat max(GLfloat a, GLfloat b) { return (a > b) ? a : b; }
ALWAYS_INLINE GLfloat min(GLfloat a, GLfloat b) { return (a < b) ? a : b; }

ALWAYS_INLINE int clamp(int value, int minValue, int maxValue) { return min(max(value, minValue), maxValue); }

typedef unsigned RGBA32; // ARGB quadruplet for colors

// These colors are the same as in the Color class
const RGBA32 black = 0xFF000000;
const RGBA32 white = 0xFFFFFFFF;
const RGBA32 darkGray = 0xFF808080;
const RGBA32 gray = 0xFFA0A0A0;
const RGBA32 lightGray = 0xFFC0C0C0;
const RGBA32 transparent = 0x00000000;

// Frequently used colors
const RGBA32 red = 0xFFFF0000;
const RGBA32 green = 0xFF008000;
const RGBA32 lightGreen = 0xFF00FF00;
const RGBA32 blue = 0xFF0000FF;
const RGBA32 yellow = 0xFFFFFF00;
const RGBA32 orange = 0xFFFF9632;

// These are used for the UI elements
const RGBA32 gray225 = 0xFFE1E1E1;
const RGBA32 gray240 = 0xFFF0F0F0;

} // namespace TyGL
} // namespace WebCore

#endif // TyGLDefs_h
