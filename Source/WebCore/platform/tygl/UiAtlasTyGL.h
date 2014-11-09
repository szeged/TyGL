/*
 * Copyright (C) 2014 University of Szeged
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

#ifndef UiAtlasTyGL_h
#define UiAtlasTyGL_h

// The following constanst are pixel positions in the tygl/UIElements.png file
// for easier extending of the resource file please use pixel coordinates divided with the atlas size

namespace WebCore {
namespace TyGL {

    const int uiAtlasSize = 64;

    // Button
    constexpr float buttonPadding = 5.0;
    constexpr float buttonTexPadding = buttonPadding / uiAtlasSize;
    constexpr float buttonTexWidth = 32.0 / uiAtlasSize;
    constexpr float buttonTexX = 0.0 / uiAtlasSize;
    constexpr float buttonTexY = 0.0 / uiAtlasSize;
    constexpr float buttonTexXPressed = 32.0 / uiAtlasSize;
    constexpr float buttonTexYPressed = buttonTexY;

    // Radio button
    constexpr float radioTexWidth = 13.0 / uiAtlasSize;
    constexpr float radioTexX = 0.0 / uiAtlasSize;
    constexpr float radioTexXChecked = 13.0 / uiAtlasSize;
    constexpr float radioTexY = 32.0 / uiAtlasSize;
    constexpr float radioTexYChecked = radioTexY;

    // Checkbox button
    constexpr float checkboxTexWidth = 13.0 / uiAtlasSize;
    constexpr float checkboxTexX = 0.0 / uiAtlasSize;
    constexpr float checkboxTexXChecked = 13.0 / uiAtlasSize;
    constexpr float checkboxTexY = 45.0 / uiAtlasSize;
    constexpr float checkboxTexYChecked = checkboxTexY;

    // Scrollbar thumb
    constexpr float scrollbarThumbPadding = 5.0;
    constexpr float scrollbarThumbTexPadding = scrollbarThumbPadding / uiAtlasSize;
    constexpr float scrollbarThumbTexWidth = 15.0 / uiAtlasSize;
    constexpr float scrollbarThumbTexHeight = 32.0 / uiAtlasSize;
    constexpr float scrollbarThumbTexX = 32.0 / uiAtlasSize;
    constexpr float scrollbarThumbTexY = 32.0 / uiAtlasSize;

    // Spinner arrow
    float arrowTexWidth = 10.0 / uiAtlasSize;
    float arrowTexX = 51.0 / uiAtlasSize;
    float arrowTexY = 36.0 / uiAtlasSize;;

} // namespace TyGL
} // namespace WebCore

#endif
