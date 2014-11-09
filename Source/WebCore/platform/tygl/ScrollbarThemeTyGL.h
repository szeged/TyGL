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

#ifndef ScrollbarThemeTyGL_h
#define ScrollbarThemeTyGL_h

#include "ScrollbarThemeComposite.h"

namespace WebCore {

class ScrollbarThemeTyGL : public ScrollbarThemeComposite {
public:
    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar) { return 13; }

protected:
    virtual bool hasButtons(ScrollbarThemeClient*) { return false; }
    virtual bool hasThumb(ScrollbarThemeClient*) { return true; }

    virtual IntRect backButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool /*painting*/ = false) { return IntRect(); }
    virtual IntRect forwardButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool /*painting*/ = false) { return IntRect(); }
    virtual IntRect trackRect(ScrollbarThemeClient*, bool painting = false);

    virtual void paintTrackBackground(GraphicsContext*, ScrollbarThemeClient*, const IntRect&);
    virtual void paintThumb(GraphicsContext*, ScrollbarThemeClient*, const IntRect&);
    virtual void paintButton(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, ScrollbarPart);
};

}
#endif // ScrollbarThemeTyGL_h
