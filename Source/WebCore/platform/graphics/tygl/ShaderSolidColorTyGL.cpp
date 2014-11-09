/*
 * Copyright (C) 2014 University of Szeged. All rights reserved.
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

#include "ShaderCommonTyGL.h"

namespace WebCore {

#define ATTRIBUTE_SOLID_COLOR_SHADER_ARGUMENTS(argument) \
    argument(a, attributeSolidColorColor, AttributeSolidColorColor, 4)

DEFINE_SHADER(
    AttributeSolidColorShader,
    ATTRIBUTE_SOLID_COLOR_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_PROGRAM(
        attribute vec4 a_attributeSolidColorColor;

        varying vec4 v_attributeSolidColorColor;
    ),
    // Vertex shader
    TyGL_PROGRAM(
       v_attributeSolidColorColor = a_attributeSolidColorColor;
    ),
    // Fragment variables
    TyGL_PROGRAM(
        varying vec4 v_attributeSolidColorColor;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        resultColor = v_attributeSolidColorColor * resultColor.a;
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_attributeSolidColorShaderProgram = &AttributeSolidColorShader::s_program;

GLfloat* PlatformContextTyGL::attributeSolidColorSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    glVertexAttribPointer(shaderArguments[AttributeSolidColorShader::aAttributeSolidColorColor], 4, GL_FLOAT, GL_FALSE, context->pipelineBuffer()->strideLength, attributeStart);
    return attributeStart + 4;
}

#define UNIFORM_SOLID_COLOR_SHADER_ARGUMENTS(argument) \
    argument(u, uniformSolidColorColor, UniformSolidColorColor, 4)

DEFINE_SHADER(
    UniformSolidColorShader,
    UNIFORM_SOLID_COLOR_SHADER_ARGUMENTS,
    PlatformContextTyGL::kNoShaderOptions,
    // Vertex variables
    TyGL_EMPTY_PROGRAM,
    // Vertex shader
    TyGL_EMPTY_PROGRAM,
    // Fragment variables
    TyGL_PROGRAM(
        uniform vec4 u_uniformSolidColorColor;
    ),
    // Fragment shader
    TyGL_PROGRAM(
        resultColor = u_uniformSolidColorColor * resultColor.a;
    )
)

PlatformContextTyGL::ShaderProgram* PlatformContextTyGL::s_uniformSolidColorShaderProgram = &UniformSolidColorShader::s_program;

GLfloat* PlatformContextTyGL::uniformSolidColorSetShaderArguments(PlatformContextTyGL* context, GLfloat* attributeStart, GLuint* shaderArguments)
{
    context->setUniformColor(shaderArguments[UniformSolidColorShader::uUniformSolidColorColor], context->pipelineBuffer()->coloring.color());
    return attributeStart;
}

} // namespace WebCore
