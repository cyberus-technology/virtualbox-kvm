/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * OpenGL support info used for 2D support detection
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_VBoxGL2D_h
#define VBOX_INCLUDED_VBoxGL2D_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

typedef char GLchar;

#ifndef GL_COMPILE_STATUS
# define GL_COMPILE_STATUS 0x8b81
#endif
#ifndef GL_LINK_STATUS
# define GL_LINK_STATUS    0x8b82
#endif
#ifndef GL_FRAGMENT_SHADER
# define GL_FRAGMENT_SHADER 0x8b30
#endif
#ifndef GL_VERTEX_SHADER
# define GL_VERTEX_SHADER 0x8b31
#endif

/* GL_ARB_multitexture */
#ifndef GL_TEXTURE0
# define GL_TEXTURE0                    0x84c0
#endif
#ifndef GL_TEXTURE1
# define GL_TEXTURE1                    0x84c1
#endif
#ifndef GL_MAX_TEXTURE_COORDS
# define GL_MAX_TEXTURE_COORDS          0x8871
#endif
#ifndef GL_MAX_TEXTURE_IMAGE_UNITS
# define GL_MAX_TEXTURE_IMAGE_UNITS     0x8872
#endif

#ifndef APIENTRY
# define APIENTRY
#endif

typedef GLvoid (APIENTRY *PFNVBOXVHWA_ACTIVE_TEXTURE) (GLenum texture);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_MULTI_TEX_COORD2I) (GLenum texture, GLint v0, GLint v1);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_MULTI_TEX_COORD2F) (GLenum texture, GLfloat v0, GLfloat v1);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_MULTI_TEX_COORD2D) (GLenum texture, GLdouble v0, GLdouble v1);

/* GL_ARB_texture_rectangle */
#ifndef GL_TEXTURE_RECTANGLE
# define GL_TEXTURE_RECTANGLE 0x84F5
#endif

/* GL_ARB_shader_objects */
/* GL_ARB_fragment_shader */

typedef GLuint (APIENTRY *PFNVBOXVHWA_CREATE_SHADER)  (GLenum type);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_SHADER_SOURCE)  (GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_COMPILE_SHADER) (GLuint shader);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_DELETE_SHADER)  (GLuint shader);

typedef GLuint (APIENTRY *PFNVBOXVHWA_CREATE_PROGRAM) ();
typedef GLvoid (APIENTRY *PFNVBOXVHWA_ATTACH_SHADER)  (GLuint program, GLuint shader);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_DETACH_SHADER)  (GLuint program, GLuint shader);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_LINK_PROGRAM)   (GLuint program);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_USE_PROGRAM)    (GLuint program);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_DELETE_PROGRAM) (GLuint program);

typedef GLboolean (APIENTRY *PFNVBOXVHWA_IS_SHADER)   (GLuint shader);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GET_SHADERIV)   (GLuint shader, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY *PFNVBOXVHWA_IS_PROGRAM)  (GLuint program);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GET_PROGRAMIV)  (GLuint program, GLenum pname, GLint *params);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GET_ATTACHED_SHADERS) (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GET_SHADER_INFO_LOG)  (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GET_PROGRAM_INFO_LOG) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRY *PFNVBOXVHWA_GET_UNIFORM_LOCATION) (GLint programObj, const GLchar *name);

typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM1F)(GLint location, GLfloat v0);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM2F)(GLint location, GLfloat v0, GLfloat v1);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM3F)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM4F)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM1I)(GLint location, GLint v0);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM2I)(GLint location, GLint v0, GLint v1);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM3I)(GLint location, GLint v0, GLint v1, GLint v2);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_UNIFORM4I)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

/* GL_ARB_pixel_buffer_object*/
#ifndef Q_WS_MAC
/* apears to be defined on mac */
typedef ptrdiff_t GLsizeiptr;
#endif

#ifndef GL_READ_ONLY
# define GL_READ_ONLY                   0x88B8
#endif
#ifndef GL_WRITE_ONLY
# define GL_WRITE_ONLY                  0x88B9
#endif
#ifndef GL_READ_WRITE
# define GL_READ_WRITE                  0x88BA
#endif
#ifndef GL_STREAM_DRAW
# define GL_STREAM_DRAW                 0x88E0
#endif
#ifndef GL_STREAM_READ
# define GL_STREAM_READ                 0x88E1
#endif
#ifndef GL_STREAM_COPY
# define GL_STREAM_COPY                 0x88E2
#endif
#ifndef GL_DYNAMIC_DRAW
# define GL_DYNAMIC_DRAW                0x88E8
#endif

#ifndef GL_PIXEL_PACK_BUFFER
# define GL_PIXEL_PACK_BUFFER           0x88EB
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
# define GL_PIXEL_UNPACK_BUFFER         0x88EC
#endif
#ifndef GL_PIXEL_PACK_BUFFER_BINDING
# define GL_PIXEL_PACK_BUFFER_BINDING   0x88ED
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER_BINDING
# define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#endif

typedef GLvoid (APIENTRY *PFNVBOXVHWA_GEN_BUFFERS)(GLsizei n, GLuint *buffers);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_DELETE_BUFFERS)(GLsizei n, const GLuint *buffers);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_BIND_BUFFER)(GLenum target, GLuint buffer);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_BUFFER_DATA)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef GLvoid* (APIENTRY *PFNVBOXVHWA_MAP_BUFFER)(GLenum target, GLenum access);
typedef GLboolean (APIENTRY *PFNVBOXVHWA_UNMAP_BUFFER)(GLenum target);

/* GL_EXT_framebuffer_object */
#ifndef GL_FRAMEBUFFER
# define GL_FRAMEBUFFER                0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
# define GL_COLOR_ATTACHMENT0          0x8CE0
#endif

typedef GLboolean (APIENTRY *PFNVBOXVHWA_IS_FRAMEBUFFER)(GLuint framebuffer);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_BIND_FRAMEBUFFER)(GLenum target, GLuint framebuffer);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_DELETE_FRAMEBUFFERS)(GLsizei n, const GLuint *framebuffers);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GEN_FRAMEBUFFERS)(GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY *PFNVBOXVHWA_CHECK_FRAMEBUFFER_STATUS)(GLenum target);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_FRAMEBUFFER_TEXTURE1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_FRAMEBUFFER_TEXTURE2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_FRAMEBUFFER_TEXTURE3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef GLvoid (APIENTRY *PFNVBOXVHWA_GET_FRAMEBUFFER_ATTACHMENT_PARAMETRIV)(GLenum target, GLenum attachment, GLenum pname, GLint *params);


/*****************/

/* functions */

/* @todo: move those to VBoxGLInfo class instance members ??? */
extern PFNVBOXVHWA_ACTIVE_TEXTURE vboxglActiveTexture;
extern PFNVBOXVHWA_MULTI_TEX_COORD2I vboxglMultiTexCoord2i;
extern PFNVBOXVHWA_MULTI_TEX_COORD2D vboxglMultiTexCoord2d;
extern PFNVBOXVHWA_MULTI_TEX_COORD2F vboxglMultiTexCoord2f;


extern PFNVBOXVHWA_CREATE_SHADER   vboxglCreateShader;
extern PFNVBOXVHWA_SHADER_SOURCE   vboxglShaderSource;
extern PFNVBOXVHWA_COMPILE_SHADER  vboxglCompileShader;
extern PFNVBOXVHWA_DELETE_SHADER   vboxglDeleteShader;

extern PFNVBOXVHWA_CREATE_PROGRAM  vboxglCreateProgram;
extern PFNVBOXVHWA_ATTACH_SHADER   vboxglAttachShader;
extern PFNVBOXVHWA_DETACH_SHADER   vboxglDetachShader;
extern PFNVBOXVHWA_LINK_PROGRAM    vboxglLinkProgram;
extern PFNVBOXVHWA_USE_PROGRAM     vboxglUseProgram;
extern PFNVBOXVHWA_DELETE_PROGRAM  vboxglDeleteProgram;

extern PFNVBOXVHWA_IS_SHADER       vboxglIsShader;
extern PFNVBOXVHWA_GET_SHADERIV    vboxglGetShaderiv;
extern PFNVBOXVHWA_IS_PROGRAM      vboxglIsProgram;
extern PFNVBOXVHWA_GET_PROGRAMIV   vboxglGetProgramiv;
extern PFNVBOXVHWA_GET_ATTACHED_SHADERS vboxglGetAttachedShaders;
extern PFNVBOXVHWA_GET_SHADER_INFO_LOG  vboxglGetShaderInfoLog;
extern PFNVBOXVHWA_GET_PROGRAM_INFO_LOG vboxglGetProgramInfoLog;

extern PFNVBOXVHWA_GET_UNIFORM_LOCATION vboxglGetUniformLocation;

extern PFNVBOXVHWA_UNIFORM1F vboxglUniform1f;
extern PFNVBOXVHWA_UNIFORM2F vboxglUniform2f;
extern PFNVBOXVHWA_UNIFORM3F vboxglUniform3f;
extern PFNVBOXVHWA_UNIFORM4F vboxglUniform4f;

extern PFNVBOXVHWA_UNIFORM1I vboxglUniform1i;
extern PFNVBOXVHWA_UNIFORM2I vboxglUniform2i;
extern PFNVBOXVHWA_UNIFORM3I vboxglUniform3i;
extern PFNVBOXVHWA_UNIFORM4I vboxglUniform4i;

extern PFNVBOXVHWA_GEN_BUFFERS vboxglGenBuffers;
extern PFNVBOXVHWA_DELETE_BUFFERS vboxglDeleteBuffers;
extern PFNVBOXVHWA_BIND_BUFFER vboxglBindBuffer;
extern PFNVBOXVHWA_BUFFER_DATA vboxglBufferData;
extern PFNVBOXVHWA_MAP_BUFFER vboxglMapBuffer;
extern PFNVBOXVHWA_UNMAP_BUFFER vboxglUnmapBuffer;

extern PFNVBOXVHWA_IS_FRAMEBUFFER vboxglIsFramebuffer;
extern PFNVBOXVHWA_BIND_FRAMEBUFFER vboxglBindFramebuffer;
extern PFNVBOXVHWA_DELETE_FRAMEBUFFERS vboxglDeleteFramebuffers;
extern PFNVBOXVHWA_GEN_FRAMEBUFFERS vboxglGenFramebuffers;
extern PFNVBOXVHWA_CHECK_FRAMEBUFFER_STATUS vboxglCheckFramebufferStatus;
extern PFNVBOXVHWA_FRAMEBUFFER_TEXTURE1D vboxglFramebufferTexture1D;
extern PFNVBOXVHWA_FRAMEBUFFER_TEXTURE2D vboxglFramebufferTexture2D;
extern PFNVBOXVHWA_FRAMEBUFFER_TEXTURE3D vboxglFramebufferTexture3D;
extern PFNVBOXVHWA_GET_FRAMEBUFFER_ATTACHMENT_PARAMETRIV vboxglGetFramebufferAttachmentParameteriv;


/*
 * Glossing over qt 5 vs 6 differences.
 *
 * Note! We could use the qt6 classes in 5, but we probably don't have the
 *       necessary modules in our current qt builds.
 */
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QOpenGLWidget;
class QOpenGLContext;
# define MY_QOpenGLWidget   QOpenGLWidget
# define MY_QOpenGLContext  QOpenGLContext
#else
class QGLWidget;
class QGLContext;
# define MY_QOpenGLWidget   QGLWidget
# define MY_QOpenGLContext  QGLContext
#endif


class VBoxGLInfo
{
public:
    VBoxGLInfo() :
        mGLVersion(0),
        mFragmentShaderSupported(false),
        mTextureRectangleSupported(false),
        mTextureNP2Supported(false),
        mPBOSupported(false),
        mFBOSupported(false),
        mMultiTexNumSupported(1), /* 1 would mean it is not supported */
        m_GL_ARB_multitexture(false),
        m_GL_ARB_shader_objects(false),
        m_GL_ARB_fragment_shader(false),
        m_GL_ARB_pixel_buffer_object(false),
        m_GL_ARB_texture_rectangle(false),
        m_GL_EXT_texture_rectangle(false),
        m_GL_NV_texture_rectangle(false),
        m_GL_ARB_texture_non_power_of_two(false),
        m_GL_EXT_framebuffer_object(false),
        mInitialized(false)
    {}

    void init(const MY_QOpenGLContext *pContext);

    bool isInitialized() const { return mInitialized; }

    int getGLVersion() const { return mGLVersion; }
    bool isFragmentShaderSupported() const { return mFragmentShaderSupported; }
    bool isTextureRectangleSupported() const { return mTextureRectangleSupported; }
    bool isTextureNP2Supported() const { return mTextureNP2Supported; }
    bool isPBOSupported() const { return mPBOSupported; }
    /* some ATI drivers do not seem to support non-zero offsets when dealing with PBOs
     * @todo: add a check for that, always unsupported currently */
    bool isPBOOffsetSupported() const { return false; }
    bool isFBOSupported() const { return mFBOSupported; }
    /* 1 would mean it is not supported */
    int getMultiTexNumSupported() const { return mMultiTexNumSupported; }

    static int parseVersion(const GLubyte * ver);
private:
    void initExtSupport(const MY_QOpenGLContext &context);

    int mGLVersion;
    bool mFragmentShaderSupported;
    bool mTextureRectangleSupported;
    bool mTextureNP2Supported;
    bool mPBOSupported;
    bool mFBOSupported;
    int mMultiTexNumSupported; /* 1 would mean it is not supported */

    bool m_GL_ARB_multitexture;
    bool m_GL_ARB_shader_objects;
    bool m_GL_ARB_fragment_shader;
    bool m_GL_ARB_pixel_buffer_object;
    bool m_GL_ARB_texture_rectangle;
    bool m_GL_EXT_texture_rectangle;
    bool m_GL_NV_texture_rectangle;
    bool m_GL_ARB_texture_non_power_of_two;
    bool m_GL_EXT_framebuffer_object;

    bool mInitialized;
};

class VBoxGLTmpContext
{
public:
    VBoxGLTmpContext();
    ~VBoxGLTmpContext();

    const MY_QOpenGLContext *makeCurrent();
private:
    MY_QOpenGLWidget *mWidget;
};


#define VBOXQGL_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define FOURCC_AYUV VBOXQGL_MAKEFOURCC('A', 'Y', 'U', 'V')
#define FOURCC_UYVY VBOXQGL_MAKEFOURCC('U', 'Y', 'V', 'Y')
#define FOURCC_YUY2 VBOXQGL_MAKEFOURCC('Y', 'U', 'Y', '2')
#define FOURCC_YV12 VBOXQGL_MAKEFOURCC('Y', 'V', '1', '2')
#define VBOXVHWA_NUMFOURCC 4

class VBoxVHWAInfo
{
public:
    VBoxVHWAInfo() :
        mFourccSupportedCount(0),
        mInitialized(false)
    {}

    VBoxVHWAInfo(const VBoxGLInfo & glInfo) :
        mglInfo(glInfo),
        mFourccSupportedCount(0),
        mInitialized(false)
    {}

    void init(const MY_QOpenGLContext *pContext);

    bool isInitialized() const { return mInitialized; }

    const VBoxGLInfo & getGlInfo() const { return mglInfo; }

    bool isVHWASupported() const;

    int getFourccSupportedCount() const { return mFourccSupportedCount; }
    const uint32_t * getFourccSupportedList() const { return mFourccSupportedList; }

    static bool checkVHWASupport();
private:
    VBoxGLInfo mglInfo;
    uint32_t mFourccSupportedList[VBOXVHWA_NUMFOURCC];
    int mFourccSupportedCount;

    bool mInitialized;
};

#endif /* !VBOX_INCLUDED_VBoxGL2D_h */
