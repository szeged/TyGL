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

#ifndef ShaderCommonTyGL_h
#define ShaderCommonTyGL_h

// Global macros used by various shaders.
#define TyGL_PROGRAM_STR(...)  #__VA_ARGS__
#define TyGL_PROGRAM(...) TyGL_PROGRAM_STR(__VA_ARGS__)
#define TyGL_EMPTY_PROGRAM 0

#define SET_POSITION8(position, a0, a1, a2, a3, a4, a5, a6, a7) \
    position[0] = (a0); \
    position[1] = (a1); \
    position[2] = (a2); \
    position[3] = (a3); \
    position[4] = (a4); \
    position[5] = (a5); \
    position[6] = (a6); \
    position[7] = (a7);

#define SET_POSITION16(position, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
    position[0] = (a0); \
    position[1] = (a1); \
    position[2] = (a2); \
    position[3] = (a3); \
    position[4] = (a4); \
    position[5] = (a5); \
    position[6] = (a6); \
    position[7] = (a7); \
    position[8] = (a8); \
    position[9] = (a9); \
    position[10] = (a10); \
    position[11] = (a11); \
    position[12] = (a12); \
    position[13] = (a13); \
    position[14] = (a14); \
    position[15] = (a15);

// Can only be used in PlatformContexTyGL.
#define VIEWPORT_SIZE 2048

// The following macros are helpers:
#define UNDERSCORE_PREFIX(name) _ ## name
#define SHADER_ARGUMENT_NAME_NO_EXPAND(type, name) type ## name
#define SHADER_ARGUMENT_NAME(type, name) SHADER_ARGUMENT_NAME_NO_EXPAND(type, name)

#define SHADER_NAME_STRING_NO_EXPAND(name) #name
#define SHADER_NAME_STRING(name) SHADER_NAME_STRING_NO_EXPAND(name)

// This macro expects two identical names: one with capital, and the other with
// non-capital starting character. All other characters must be the same.
// example: argument(u, texture, Texture)
#define ARGUMENT_LIST_ENUM(type, name, Name, numberOfComponents) type ## Name,
#define ARGUMENT_LIST_STRING(type, name, Name, numberOfComponents) SHADER_NAME_STRING(SHADER_ARGUMENT_NAME(type, UNDERSCORE_PREFIX(name))),
#define ARGUMENT_LIST_COMPONENTS(type, name, Name, numberOfComponents) numberOfComponents,

// This is the only macro used elsewhere:
#define DEFINE_SHADER(className, arguments, options, vertexVariables, vertexShader, fragmentVariables, fragmentShader) \
    class className { \
    public: \
        enum { \
            arguments(ARGUMENT_LIST_ENUM) \
        }; \
        static const GLchar* s_argumentList[]; \
        static const int s_componentsList[]; \
        static PlatformContextTyGL::ShaderProgram s_program; \
    }; \
    \
    const GLchar* className::s_argumentList[] = { \
        arguments(ARGUMENT_LIST_STRING) \
        0 \
    }; \
    \
    const int className::s_componentsList[] = { \
        arguments(ARGUMENT_LIST_COMPONENTS) \
        0 \
    }; \
    \
    PlatformContextTyGL::ShaderProgram className::s_program = { \
        options, \
        PlatformContextTyGL::ShaderProgram::className ## Id, \
        vertexVariables, \
        vertexShader, \
        fragmentVariables, \
        fragmentShader, \
        className::s_argumentList, \
        className::s_componentsList, \
    };

// Shader coding guidelines
//    First shader is the source shader.
//       * Can specify kVec4Position.
//       * Fragment shader must specify "vec4 resultColor".
//       * No need variable prefixing.
//    Other shaders
//       * Must prefix all their variables with the type of the shader.

namespace WebCore{

} //namespace WebCore

#endif // ShaderCommonTyGL_h
