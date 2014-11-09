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
#include "PlatformContextTyGL.h"

#include "Color.h"
#include "GLDefs.h"
#include "GLPlatformContext.h"
#include "GraphicsContext3D.h"
#include "ShaderCommonTyGL.h"

namespace WebCore {

using TyGL::min;
using TyGL::max;

GLPlatformContext* PlatformContextTyGL::s_offScreenContext = 0;
GLPlatformSurface* PlatformContextTyGL::s_offScreenSurface = 0;
PlatformContextTyGL::PipelineBuffer* PlatformContextTyGL::s_pipelineBuffer = 0;
GLuint PlatformContextTyGL::PipelineBuffer::s_indexBufferObject;

PlatformContextTyGL::PlatformContextTyGL(PassRefPtr<NativeImageTyGL> targetBuffer)
{
    m_targetTexture = targetBuffer;
    createGLContextIfNeed();

    // Remember to initialize non trivial members in TyGLState
    m_state.boundingClipRect.setDimension(m_targetTexture->width(), m_targetTexture->height());
    m_state.fillColoring.set(Color::black);
    m_state.strokeColoring.set(Color::black);
    m_state.strokeWidth = 1;
    m_state.lineCapMode = ButtCap;
    m_state.lineJoinMode = MiterJoin;
    m_state.miterLimit = 0;
    m_state.compositeOperator = CompositeSourceOver;
    m_state.blendMode = BlendModeNormal;
    resetColoring();
    m_gradientStopInfoImage = NativeImageTyGL::create(2, 2);
}

PlatformContextTyGL::~PlatformContextTyGL()
{
    if (pipelineBuffer()->context == this)
        pipelineBuffer()->flushBatchingQueue();
}

void PlatformContextTyGL::createGLContext()
{
    s_offScreenContext = GLPlatformContext::createContext(GraphicsContext3D::RenderOffscreen).release();
    // FIXME: Some targets might not support alpha, we should learn that from an EGL config.
    s_offScreenSurface = GLPlatformSurface::createOffScreenSurface(GLPlatformSurface::SupportAlpha).release();
    s_offScreenContext->initialize(s_offScreenSurface);
    glViewport(0, 0, VIEWPORT_SIZE, VIEWPORT_SIZE);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_BLEND);

    s_pipelineBuffer = new PipelineBuffer();
    PipelineBuffer::initQuadIndexes();
}

void PlatformContextTyGL::save()
{
    m_stack.append(m_state);
}

void PlatformContextTyGL::restore()
{
    m_state = m_stack.last();
    m_stack.removeLast();
}

void PlatformContextTyGL::setStrokeThickness(float thickness)
{
    m_state.strokeWidth = thickness;
}

void PlatformContextTyGL::setFillColor(const Color& color)
{
    m_state.fillColoring.set(premultipliedARGBFromColor(color));
}

void PlatformContextTyGL::setStrokeColor(const Color& color)
{
    m_state.strokeColoring.set(premultipliedARGBFromColor(color));
}

void PlatformContextTyGL::setCompositeOperation(CompositeOperator op, BlendMode blendMode)
{
    m_state.compositeOperator = op;
    m_state.blendMode = blendMode;
}

void PlatformContextTyGL::setLineJoin(const LineJoin joinMode)
{
    m_state.lineJoinMode = joinMode;
}

void PlatformContextTyGL::setLineCap(const LineCap capMode)
{
    m_state.lineCapMode = capMode;
}

void PlatformContextTyGL::setMiterLimit(float miterLimit)
{
    m_state.miterLimit = miterLimit;
}

void PlatformContextTyGL::translate(float x, float y)
{
    m_state.transform.translate(x, y);
}

void PlatformContextTyGL::scale(float sx, float sy)
{
    m_state.transform.scale(sx, sy);
}

void PlatformContextTyGL::rotate(float angle)
{
    m_state.transform.rotate(angle);
}

bool PlatformContextTyGL::compileShaderInternal(GLuint* compiledShader, int count, const ShaderProgram** programList)
{
    GLuint shaderProgram = 0;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint* offset;
    GLint intValue;

    ASSERT(!compiledShader[0] && count >= 1);
    // One define and one source part for each piece, and three extra strings.
    const GLchar* shaderSource[PlatformContextTyGL::PipelineBuffer::kMaximumNumberOfShaderPieces * 2 + 3];
    const GLchar** current = shaderSource;

    ASSERT(count <= PlatformContextTyGL::PipelineBuffer::kMaximumNumberOfShaderPieces);

    for (int i = 0; i < count; i++) {
        if (programList[i]->vertexVariables && programList[i]->vertexVariables[0])
            *current++ = programList[i]->vertexVariables;
    }
    *current++ = TyGL_PROGRAM( void main(void) { );
    for (int i = 0; i < count; i++) {
        if (programList[i]->vertexShader && programList[i]->vertexShader[0])
            *current++ = programList[i]->vertexShader;
    }
    *current++ = TyGL_PROGRAM( gl_Position = vec4(a_position.xy * (2.0 / VIEWPORT_SIZE.0) - 1.0, 0.0, 1.0); } );

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, current - shaderSource, shaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &intValue);
    if (intValue != GL_TRUE)
        goto error;

    current = shaderSource;
    *current++ = "#ifdef GL_ES\nprecision mediump float;\n#endif\n";
    for (int i = 0; i < count; i++) {
        if (programList[i]->fragmentVariables && programList[i]->fragmentVariables[0])
            *current++ = programList[i]->fragmentVariables;
    }
    *current++ = TyGL_PROGRAM( void main(void) { );
    for (int i = 0; i < count; i++) {
        if (programList[i]->fragmentShader && programList[i]->fragmentShader[0])
            *current++ = programList[i]->fragmentShader;
    }
    *current++ = TyGL_PROGRAM( gl_FragColor = resultColor; } );

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, current - shaderSource, shaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &intValue);
    if (intValue != GL_TRUE)
        goto error;

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &intValue);
    if (intValue != GL_TRUE)
        goto error;

    // According to the specification, the shaders are kept
    // around until the program object is freed (reference counted).
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(shaderProgram);
    offset = compiledShader + 1 + count;
    for (int i = 0; i < count; i++) {
        const GLchar** arguments = programList[i]->variables;
        compiledShader[i + 1] = offset - compiledShader;
        while (*arguments) {
            const GLchar* name = *arguments;
            ASSERT((name[0] == 'u' || name[0] == 'a') && name[1] == '_');
            if (name[0] == 'u') {
                *offset++ = glGetUniformLocation(shaderProgram, name);
            } else {
                *offset = glGetAttribLocation(shaderProgram, name);
                glEnableVertexAttribArray(*offset);
                offset++;
            }
            arguments++;
        }
    }

    ASSERT(offset - compiledShader < kMaximumShaderDescriptorLength);
    compiledShader[0] = shaderProgram;
    return true;

error:
    if (vertexShader)
        glDeleteShader(vertexShader);
    if (fragmentShader)
        glDeleteShader(fragmentShader);
    if (shaderProgram)
        glDeleteProgram(shaderProgram);
    return false;
}

void PlatformContextTyGL::PipelineBuffer::initQuadIndexes()
{
    GLushort* quadIndexes;
    int bufferSize = kMaximumNumberOfUshortQuads * 6;

    quadIndexes = reinterpret_cast<GLushort*>(fastMalloc(bufferSize * sizeof(GLushort)));

    GLushort* currentQuad = quadIndexes;
    GLushort* currentQuadEnd = quadIndexes + bufferSize;

    int index = 0;
    while (currentQuad < currentQuadEnd) {
        currentQuad[0] = index;
        currentQuad[1] = index + 1;
        currentQuad[2] = index + 2;
        currentQuad[3] = index + 1;
        currentQuad[4] = index + 2;
        currentQuad[5] = index + 3;
        currentQuad += 6;
        index += 4;
    }

    glGenBuffers(1, &PipelineBuffer::s_indexBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PipelineBuffer::s_indexBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize * sizeof(GLushort), quadIndexes, GL_STATIC_DRAW);

    fastFree(quadIndexes);
}

} // namespace WebCore
