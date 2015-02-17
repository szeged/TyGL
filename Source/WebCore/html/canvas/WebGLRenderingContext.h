/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebGLRenderingContext_h
#define WebGLRenderingContext_h

#include "WebGLRenderingContextBase.h"

namespace WebCore {

class WebGLRenderingContext final : public WebGLRenderingContextBase {
public:
    WebGLRenderingContext(HTMLCanvasElement*, GraphicsContext3D::Attributes);
    WebGLRenderingContext(HTMLCanvasElement*, PassRefPtr<GraphicsContext3D>, GraphicsContext3D::Attributes);
    virtual bool isWebGL1() const { return true; }
    
    virtual WebGLExtension* getExtension(const String&) override;
    virtual WebGLGetInfo getParameter(GC3Denum pname, ExceptionCode&) override;
    virtual Vector<String> getSupportedExtensions() override;
    bool validateCapability(const char* functionName, GC3Denum cap) override;
};
    
} // namespace WebCore

#endif
