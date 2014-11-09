/*
 * Copyright (C) 2002-2012 Carsten Haitzler, Dan Sinclair, Mike Blumenkrantz,
 * Samsung Electronics and various contributors (see AUTHORS)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Evas_TyGL_h
#define Evas_TyGL_h

#define GL_GLEXT_PROTOTYPES 1

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _Evas_GL_Color_Format
{
    EVAS_GL_RGB_888   = 0,
    EVAS_GL_RGBA_8888 = 1
} Evas_GL_Color_Format;

typedef enum _Evas_GL_Depth_Bits
{
    EVAS_GL_DEPTH_NONE   = 0,
    EVAS_GL_DEPTH_BIT_8  = 1,
    EVAS_GL_DEPTH_BIT_16 = 2,
    EVAS_GL_DEPTH_BIT_24 = 3,
    EVAS_GL_DEPTH_BIT_32 = 4
} Evas_GL_Depth_Bits;

typedef enum _Evas_GL_Stencil_Bits
{
    EVAS_GL_STENCIL_NONE   = 0,
    EVAS_GL_STENCIL_BIT_1  = 1,
    EVAS_GL_STENCIL_BIT_2  = 2,
    EVAS_GL_STENCIL_BIT_4  = 3,
    EVAS_GL_STENCIL_BIT_8  = 4,
    EVAS_GL_STENCIL_BIT_16 = 5
} Evas_GL_Stencil_Bits;

typedef enum _Evas_GL_Options_Bits
{
   EVAS_GL_OPTIONS_NONE    = 0,     /**< No extra options */
   EVAS_GL_OPTIONS_DIRECT  = (1<<0) /**< Optional hint to allow rendering directly to evas' window when possible */
} Evas_GL_Options_Bits;


typedef enum _Evas_GL_Multisample_Bits
{
   EVAS_GL_MULTISAMPLE_NONE = 0, /**< No multisample rendering */
   EVAS_GL_MULTISAMPLE_LOW  = 1, /**< MSAA with mininum number of samples */
   EVAS_GL_MULTISAMPLE_MED  = 2, /**< MSAA with half the number of max samples */
   EVAS_GL_MULTISAMPLE_HIGH = 3  /**< MSAA with maximum allowed samples */
} Evas_GL_Multisample_Bits;

typedef struct _Evas_GL               Evas_GL;
typedef struct _Evas_GL_API           Evas_GL_API;
typedef struct _Evas_GL_Config        Evas_GL_Config;
typedef struct _Evas_GL_Context       Evas_GL_Context;
typedef struct _Evas_GL_Surface       Evas_GL_Surface;

struct _Evas_GL_Config
{
   Evas_GL_Color_Format       color_format;     /**< Surface Color Format */
   Evas_GL_Depth_Bits         depth_bits;       /**< Surface Depth Bits */
   Evas_GL_Stencil_Bits       stencil_bits;     /**< Surface Stencil Bits */
   Evas_GL_Options_Bits       options_bits;     /**< Extra Surface Options */
   Evas_GL_Multisample_Bits   multisample_bits; /**< Optional Surface MSAA Bits */
};

EAPI Evas_GL                 *evas_gl_new                (Evas *e) EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1);
EAPI void                     evas_gl_free               (Evas_GL *evas_gl) EINA_ARG_NONNULL(1);

EAPI Eina_Bool                evas_gl_make_current       (Evas_GL *evas_gl, Evas_GL_Surface *surf, Evas_GL_Context *ctx) EINA_ARG_NONNULL(1,2);
EAPI Eina_Bool                evas_gl_native_surface_get (Evas_GL *evas_gl, Evas_GL_Surface *surf, Evas_Native_Surface *ns) EINA_ARG_NONNULL(1,2,3);
EAPI Evas_GL_API             *evas_gl_api_get            (Evas_GL *evas_gl) EINA_ARG_NONNULL(1);

EAPI Evas_GL_Surface         *evas_gl_surface_create     (Evas_GL *evas_gl, Evas_GL_Config *cfg, int w, int h) EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1,2);
EAPI void                     evas_gl_surface_destroy    (Evas_GL *evas_gl, Evas_GL_Surface *surf) EINA_ARG_NONNULL(1,2);
EAPI Evas_GL_Context         *evas_gl_context_create     (Evas_GL *evas_gl, Evas_GL_Context *share_ctx) EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1);
EAPI void                     evas_gl_context_destroy    (Evas_GL *evas_gl, Evas_GL_Context *ctx) EINA_ARG_NONNULL(1,2);

typedef void                         *EvasGLImage;

struct _Evas_GL_API
{
   int            version;

   /* version 1: */
   /*------- GLES 2.0 -------*/
   void         (*glActiveTexture) (GLenum texture);
   void         (*glAttachShader) (GLuint program, GLuint shader);
   void         (*glBindAttribLocation) (GLuint program, GLuint index, const char* name);
   void         (*glBindBuffer) (GLenum target, GLuint buffer);
   void         (*glBindFramebuffer) (GLenum target, GLuint framebuffer);
   void         (*glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
   void         (*glBindTexture) (GLenum target, GLuint texture);
   void         (*glBlendColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
   void         (*glBlendEquation) ( GLenum mode );
   void         (*glBlendEquationSeparate) (GLenum modeRGB, GLenum modeAlpha);
   void         (*glBlendFunc) (GLenum sfactor, GLenum dfactor);
   void         (*glBlendFuncSeparate) (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
   void         (*glBufferData) (GLenum target, GLsizeiptr size, const void* data, GLenum usage);
   void         (*glBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
   GLenum       (*glCheckFramebufferStatus) (GLenum target);
   void         (*glClear) (GLbitfield mask);
   void         (*glClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
   void         (*glClearDepthf) (GLclampf depth);
   void         (*glClearStencil) (GLint s);
   void         (*glColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
   void         (*glCompileShader) (GLuint shader);
   void         (*glCompressedTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
   void         (*glCompressedTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
   void         (*glCopyTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
   void         (*glCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
   GLuint       (*glCreateProgram) (void);
   GLuint       (*glCreateShader) (GLenum type);
   void         (*glCullFace) (GLenum mode);
   void         (*glDeleteBuffers) (GLsizei n, const GLuint* buffers);
   void         (*glDeleteFramebuffers) (GLsizei n, const GLuint* framebuffers);
   void         (*glDeleteProgram) (GLuint program);
   void         (*glDeleteRenderbuffers) (GLsizei n, const GLuint* renderbuffers);
   void         (*glDeleteShader) (GLuint shader);
   void         (*glDeleteTextures) (GLsizei n, const GLuint* textures);
   void         (*glDepthFunc) (GLenum func);
   void         (*glDepthMask) (GLboolean flag);
   void         (*glDepthRangef) (GLclampf zNear, GLclampf zFar);
   void         (*glDetachShader) (GLuint program, GLuint shader);
   void         (*glDisable) (GLenum cap);
   void         (*glDisableVertexAttribArray) (GLuint index);
   void         (*glDrawArrays) (GLenum mode, GLint first, GLsizei count);
   void         (*glDrawElements) (GLenum mode, GLsizei count, GLenum type, const void* indices);
   void         (*glEnable) (GLenum cap);
   void         (*glEnableVertexAttribArray) (GLuint index);
   void         (*glFinish) (void);
   void         (*glFlush) (void);
   void         (*glFramebufferRenderbuffer) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
   void         (*glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
   void         (*glFrontFace) (GLenum mode);
   void         (*glGenBuffers) (GLsizei n, GLuint* buffers);
   void         (*glGenerateMipmap) (GLenum target);
   void         (*glGenFramebuffers) (GLsizei n, GLuint* framebuffers);
   void         (*glGenRenderbuffers) (GLsizei n, GLuint* renderbuffers);
   void         (*glGenTextures) (GLsizei n, GLuint* textures);
   void         (*glGetActiveAttrib) (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
   void         (*glGetActiveUniform) (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
   void         (*glGetAttachedShaders) (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
   int          (*glGetAttribLocation) (GLuint program, const char* name);
   void         (*glGetBooleanv) (GLenum pname, GLboolean* params);
   void         (*glGetBufferParameteriv) (GLenum target, GLenum pname, GLint* params);
   GLenum       (*glGetError) (void);
   void         (*glGetFloatv) (GLenum pname, GLfloat* params);
   void         (*glGetFramebufferAttachmentParameteriv) (GLenum target, GLenum attachment, GLenum pname, GLint* params);
   void         (*glGetIntegerv) (GLenum pname, GLint* params);
   void         (*glGetProgramiv) (GLuint program, GLenum pname, GLint* params);
   void         (*glGetProgramInfoLog) (GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
   void         (*glGetRenderbufferParameteriv) (GLenum target, GLenum pname, GLint* params);
   void         (*glGetShaderiv) (GLuint shader, GLenum pname, GLint* params);
   void         (*glGetShaderInfoLog) (GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
   void         (*glGetShaderPrecisionFormat) (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
   void         (*glGetShaderSource) (GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
   const GLubyte* (*glGetString) (GLenum name);
   void         (*glGetTexParameterfv) (GLenum target, GLenum pname, GLfloat* params);
   void         (*glGetTexParameteriv) (GLenum target, GLenum pname, GLint* params);
   void         (*glGetUniformfv) (GLuint program, GLint location, GLfloat* params);
   void         (*glGetUniformiv) (GLuint program, GLint location, GLint* params);
   int          (*glGetUniformLocation) (GLuint program, const char* name);
   void         (*glGetVertexAttribfv) (GLuint index, GLenum pname, GLfloat* params);
   void         (*glGetVertexAttribiv) (GLuint index, GLenum pname, GLint* params);
   void         (*glGetVertexAttribPointerv) (GLuint index, GLenum pname, void** pointer);
   void         (*glHint) (GLenum target, GLenum mode);
   GLboolean    (*glIsBuffer) (GLuint buffer);
   GLboolean    (*glIsEnabled) (GLenum cap);
   GLboolean    (*glIsFramebuffer) (GLuint framebuffer);
   GLboolean    (*glIsProgram) (GLuint program);
   GLboolean    (*glIsRenderbuffer) (GLuint renderbuffer);
   GLboolean    (*glIsShader) (GLuint shader);
   GLboolean    (*glIsTexture) (GLuint texture);
   void         (*glLineWidth) (GLfloat width);
   void         (*glLinkProgram) (GLuint program);
   void         (*glPixelStorei) (GLenum pname, GLint param);
   void         (*glPolygonOffset) (GLfloat factor, GLfloat units);
   void         (*glReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
   void         (*glReleaseShaderCompiler) (void);
   void         (*glRenderbufferStorage) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
   void         (*glSampleCoverage) (GLclampf value, GLboolean invert);
   void         (*glScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
   void         (*glShaderBinary) (GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length);
   void         (*glShaderSource) (GLuint shader, GLsizei count, const char* const * string, const GLint* length);
   void         (*glStencilFunc) (GLenum func, GLint ref, GLuint mask);
   void         (*glStencilFuncSeparate) (GLenum face, GLenum func, GLint ref, GLuint mask);
   void         (*glStencilMask) (GLuint mask);
   void         (*glStencilMaskSeparate) (GLenum face, GLuint mask);
   void         (*glStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
   void         (*glStencilOpSeparate) (GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
   void         (*glTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
   void         (*glTexParameterf) (GLenum target, GLenum pname, GLfloat param);
   void         (*glTexParameterfv) (GLenum target, GLenum pname, const GLfloat* params);
   void         (*glTexParameteri) (GLenum target, GLenum pname, GLint param);
   void         (*glTexParameteriv) (GLenum target, GLenum pname, const GLint* params);
   void         (*glTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
   void         (*glUniform1f) (GLint location, GLfloat x);
   void         (*glUniform1fv) (GLint location, GLsizei count, const GLfloat* v);
   void         (*glUniform1i) (GLint location, GLint x);
   void         (*glUniform1iv) (GLint location, GLsizei count, const GLint* v);
   void         (*glUniform2f) (GLint location, GLfloat x, GLfloat y);
   void         (*glUniform2fv) (GLint location, GLsizei count, const GLfloat* v);
   void         (*glUniform2i) (GLint location, GLint x, GLint y);
   void         (*glUniform2iv) (GLint location, GLsizei count, const GLint* v);
   void         (*glUniform3f) (GLint location, GLfloat x, GLfloat y, GLfloat z);
   void         (*glUniform3fv) (GLint location, GLsizei count, const GLfloat* v);
   void         (*glUniform3i) (GLint location, GLint x, GLint y, GLint z);
   void         (*glUniform3iv) (GLint location, GLsizei count, const GLint* v);
   void         (*glUniform4f) (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
   void         (*glUniform4fv) (GLint location, GLsizei count, const GLfloat* v);
   void         (*glUniform4i) (GLint location, GLint x, GLint y, GLint z, GLint w);
   void         (*glUniform4iv) (GLint location, GLsizei count, const GLint* v);
   void         (*glUniformMatrix2fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
   void         (*glUniformMatrix3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
   void         (*glUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
   void         (*glUseProgram) (GLuint program);
   void         (*glValidateProgram) (GLuint program);
   void         (*glVertexAttrib1f) (GLuint indx, GLfloat x);
   void         (*glVertexAttrib1fv) (GLuint indx, const GLfloat* values);
   void         (*glVertexAttrib2f) (GLuint indx, GLfloat x, GLfloat y);
   void         (*glVertexAttrib2fv) (GLuint indx, const GLfloat* values);
   void         (*glVertexAttrib3f) (GLuint indx, GLfloat x, GLfloat y, GLfloat z);
   void         (*glVertexAttrib3fv) (GLuint indx, const GLfloat* values);
   void         (*glVertexAttrib4f) (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
   void         (*glVertexAttrib4fv) (GLuint indx, const GLfloat* values);
   void         (*glVertexAttribPointer) (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);
   void         (*glViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

   /*------- GLES 2.0 Extensions -------*/
   // Notice these two names have been changed to fit Evas GL and not EGL!
   /* GL_OES_EvasGL_image */
   void         (*glEvasGLImageTargetTexture2DOES) (GLenum target, EvasGLImage image);
   void         (*glEvasGLImageTargetRenderbufferStorageOES) (GLenum target, EvasGLImage image);

   /* GL_OES_get_program_binary */
   void     (*glGetProgramBinaryOES) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
   void     (*glProgramBinaryOES) (GLuint program, GLenum binaryFormat, const void *binary, GLint length);
   /* GL_OES_mapbuffer */
   void*    (*glMapBufferOES) (GLenum target, GLenum access);
   GLboolean    (*glUnmapBufferOES) (GLenum target);
   void     (*glGetBufferPointervOES) (GLenum target, GLenum pname, void** params);
   /* GL_OES_texture_3D */
   void     (*glTexImage3DOES) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
   void     (*glTexSubImage3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
   void     (*glCopyTexSubImage3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
   void     (*glCompressedTexImage3DOES) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data);
   void     (*glCompressedTexSubImage3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data);
   void     (*glFramebufferTexture3DOES) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);

   /* AMD_performance_monitor */
   void     (*glGetPerfMonitorGroupsAMD) (GLint* numGroups, GLsizei groupsSize, GLuint* groups);
   void     (*glGetPerfMonitorCountersAMD) (GLuint group, GLint* numCounters, GLint* maxActiveCounters, GLsizei counterSize, GLuint* counters);
   void     (*glGetPerfMonitorGroupStringAMD) (GLuint group, GLsizei bufSize, GLsizei* length, char* groupString);
   void     (*glGetPerfMonitorCounterStringAMD) (GLuint group, GLuint counter, GLsizei bufSize, GLsizei* length, char* counterString);
   void     (*glGetPerfMonitorCounterInfoAMD) (GLuint group, GLuint counter, GLenum pname, void* data);
   void     (*glGenPerfMonitorsAMD) (GLsizei n, GLuint* monitors);
   void     (*glDeletePerfMonitorsAMD) (GLsizei n, GLuint* monitors);
   void     (*glSelectPerfMonitorCountersAMD) (GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint* countersList);
   void     (*glBeginPerfMonitorAMD) (GLuint monitor);
   void     (*glEndPerfMonitorAMD) (GLuint monitor);
   void     (*glGetPerfMonitorCounterDataAMD) (GLuint monitor, GLenum pname, GLsizei dataSize, GLuint* data, GLint* bytesWritten);

   /* GL_EXT_discard_framebuffer */
   void     (*glDiscardFramebufferEXT) (GLenum target, GLsizei numAttachments, const GLenum* attachments);

   /* GL_EXT_multi_draw_arrays */
   void     (*glMultiDrawArraysEXT) (GLenum mode, GLint* first, GLsizei* count, GLsizei primcount);
   void     (*glMultiDrawElementsEXT) (GLenum mode, const GLsizei* count, GLenum type, const GLvoid** indices, GLsizei primcount);

   /* GL_NV_fence */
   void     (*glDeleteFencesNV) (GLsizei n, const GLuint* fences);
   void     (*glGenFencesNV) (GLsizei n, GLuint* fences);
   GLboolean    (*glIsFenceNV) (GLuint fence);
   GLboolean    (*glTestFenceNV) (GLuint fence);
   void     (*glGetFenceivNV) (GLuint fence, GLenum pname, GLint* params);
   void     (*glFinishFenceNV) (GLuint fence);
   void     (*glSetFenceNV) (GLuint, GLenum);

   /* GL_QCOM_driver_control */
   void     (*glGetDriverControlsQCOM) (GLint* num, GLsizei size, GLuint* driverControls);
   void     (*glGetDriverControlStringQCOM) (GLuint driverControl, GLsizei bufSize, GLsizei* length, char* driverControlString);
   void     (*glEnableDriverControlQCOM) (GLuint driverControl);
   void     (*glDisableDriverControlQCOM) (GLuint driverControl);

   /* GL_QCOM_extended_get */
   void     (*glExtGetTexturesQCOM) (GLuint* textures, GLint maxTextures, GLint* numTextures);
   void     (*glExtGetBuffersQCOM) (GLuint* buffers, GLint maxBuffers, GLint* numBuffers);
   void     (*glExtGetRenderbuffersQCOM) (GLuint* renderbuffers, GLint maxRenderbuffers, GLint* numRenderbuffers);
   void     (*glExtGetFramebuffersQCOM) (GLuint* framebuffers, GLint maxFramebuffers, GLint* numFramebuffers);
   void     (*glExtGetTexLevelParameterivQCOM) (GLuint texture, GLenum face, GLint level, GLenum pname, GLint* params);
   void     (*glExtTexObjectStateOverrideiQCOM) (GLenum target, GLenum pname, GLint param);
   void     (*glExtGetTexSubImageQCOM) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, void* texels);
   void     (*glExtGetBufferPointervQCOM) (GLenum target, void** params);


   /* GL_QCOM_extended_get2 */
   void     (*glExtGetShadersQCOM) (GLuint* shaders, GLint maxShaders, GLint* numShaders);
   void     (*glExtGetProgramsQCOM) (GLuint* programs, GLint maxPrograms, GLint* numPrograms);
   GLboolean    (*glExtIsProgramBinaryQCOM) (GLuint program);
   void     (*glExtGetProgramBinarySourceQCOM) (GLuint program, GLenum shadertype, char* source, GLint* length);

   //------- EGL Related Extensions -------//
   /* EvasGL_KHR_image */
   EvasGLImage  (*evasglCreateImage) (int target, void* buffer, int* attrib_list);
   void         (*evasglDestroyImage) (EvasGLImage image);

   /* future calls will be added down here for expansion */
   /* version 2: */
};

#ifdef __cplusplus
}
#endif

#endif // Evas_TyGL_h
