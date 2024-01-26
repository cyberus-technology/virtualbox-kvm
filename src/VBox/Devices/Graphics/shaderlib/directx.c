/*
 * IWineD3D implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#include "config.h"
#include <stdio.h>
#include "wined3d_private.h"

#ifdef VBOX_WITH_WDDM
# include <VBoxCrHgsmi.h>
#endif

#ifdef VBOX_WITH_VMSVGA
# ifdef RT_OS_WINDOWS
DECLIMPORT(void) APIENTRY glFinish(void);
# else
void glFinish(void);
# endif
#endif

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

#define GLINFO_LOCATION (*gl_info)
#define WINE_DEFAULT_VIDMEM (64 * 1024 * 1024)

/* The d3d device ID */
#if 0 /* VBox: unused */
static const GUID IID_D3DDEVICE_D3DUID = { 0xaeb2cdd4, 0x6e41, 0x43ea, { 0x94,0x1c,0x83,0x61,0xcc,0x76,0x07,0x81 } };
#endif


/* Extension detection */
static const struct {
    const char *extension_string;
    GL_SupportedExt extension;
    DWORD version;
} EXTENSION_MAP[] = {
    /* APPLE */
    {"GL_APPLE_client_storage",             APPLE_CLIENT_STORAGE,           0                           },
    {"GL_APPLE_fence",                      APPLE_FENCE,                    0                           },
    {"GL_APPLE_float_pixels",               APPLE_FLOAT_PIXELS,             0                           },
    {"GL_APPLE_flush_buffer_range",         APPLE_FLUSH_BUFFER_RANGE,       0                           },
    {"GL_APPLE_flush_render",               APPLE_FLUSH_RENDER,             0                           },
    {"GL_APPLE_ycbcr_422",                  APPLE_YCBCR_422,                0                           },

    /* ARB */
    {"GL_ARB_color_buffer_float",           ARB_COLOR_BUFFER_FLOAT,         0                           },
    {"GL_ARB_depth_buffer_float",           ARB_DEPTH_BUFFER_FLOAT,         0                           },
    {"GL_ARB_depth_clamp",                  ARB_DEPTH_CLAMP,                0                           },
    {"GL_ARB_depth_texture",                ARB_DEPTH_TEXTURE,              0                           },
    {"GL_ARB_draw_buffers",                 ARB_DRAW_BUFFERS,               0                           },
    {"GL_ARB_fragment_program",             ARB_FRAGMENT_PROGRAM,           0                           },
    {"GL_ARB_fragment_shader",              ARB_FRAGMENT_SHADER,            0                           },
    {"GL_ARB_framebuffer_object",           ARB_FRAMEBUFFER_OBJECT,         0                           },
    {"GL_ARB_geometry_shader4",             ARB_GEOMETRY_SHADER4,           0                           },
    {"GL_ARB_half_float_pixel",             ARB_HALF_FLOAT_PIXEL,           0                           },
    {"GL_ARB_half_float_vertex",            ARB_HALF_FLOAT_VERTEX,          0                           },
    {"GL_ARB_imaging",                      ARB_IMAGING,                    0                           },
    {"GL_ARB_map_buffer_range",             ARB_MAP_BUFFER_RANGE,           0                           },
    {"GL_ARB_multisample",                  ARB_MULTISAMPLE,                0                           }, /* needs GLX_ARB_MULTISAMPLE as well */
    {"GL_ARB_multitexture",                 ARB_MULTITEXTURE,               0                           },
    {"GL_ARB_occlusion_query",              ARB_OCCLUSION_QUERY,            0                           },
    {"GL_ARB_pixel_buffer_object",          ARB_PIXEL_BUFFER_OBJECT,        0                           },
    {"GL_ARB_point_parameters",             ARB_POINT_PARAMETERS,           0                           },
    {"GL_ARB_point_sprite",                 ARB_POINT_SPRITE,               0                           },
    {"GL_ARB_provoking_vertex",             ARB_PROVOKING_VERTEX,           0                           },
    {"GL_ARB_shader_objects",               ARB_SHADER_OBJECTS,             0                           },
    {"GL_ARB_shader_texture_lod",           ARB_SHADER_TEXTURE_LOD,         0                           },
    {"GL_ARB_shading_language_100",         ARB_SHADING_LANGUAGE_100,       0                           },
    {"GL_ARB_sync",                         ARB_SYNC,                       0                           },
    {"GL_ARB_texture_border_clamp",         ARB_TEXTURE_BORDER_CLAMP,       0                           },
    {"GL_ARB_texture_compression",          ARB_TEXTURE_COMPRESSION,        0                           },
    {"GL_ARB_texture_cube_map",             ARB_TEXTURE_CUBE_MAP,           0                           },
    {"GL_ARB_texture_env_add",              ARB_TEXTURE_ENV_ADD,            0                           },
    {"GL_ARB_texture_env_combine",          ARB_TEXTURE_ENV_COMBINE,        0                           },
    {"GL_ARB_texture_env_dot3",             ARB_TEXTURE_ENV_DOT3,           0                           },
    {"GL_ARB_texture_float",                ARB_TEXTURE_FLOAT,              0                           },
    {"GL_ARB_texture_mirrored_repeat",      ARB_TEXTURE_MIRRORED_REPEAT,    0                           },
    {"GL_IBM_texture_mirrored_repeat",      ARB_TEXTURE_MIRRORED_REPEAT,    0                           },
    {"GL_ARB_texture_non_power_of_two",     ARB_TEXTURE_NON_POWER_OF_TWO,   MAKEDWORD_VERSION(2, 0)     },
    {"GL_ARB_texture_rectangle",            ARB_TEXTURE_RECTANGLE,          0                           },
    {"GL_ARB_texture_rg",                   ARB_TEXTURE_RG,                 0                           },
    {"GL_ARB_vertex_array_bgra",            ARB_VERTEX_ARRAY_BGRA,          0                           },
    {"GL_ARB_vertex_blend",                 ARB_VERTEX_BLEND,               0                           },
    {"GL_ARB_vertex_buffer_object",         ARB_VERTEX_BUFFER_OBJECT,       0                           },
    {"GL_ARB_vertex_program",               ARB_VERTEX_PROGRAM,             0                           },
    {"GL_ARB_vertex_shader",                ARB_VERTEX_SHADER,              0                           },

    /* ATI */
    {"GL_ATI_fragment_shader",              ATI_FRAGMENT_SHADER,            0                           },
    {"GL_ATI_separate_stencil",             ATI_SEPARATE_STENCIL,           0                           },
    {"GL_ATI_texture_compression_3dc",      ATI_TEXTURE_COMPRESSION_3DC,    0                           },
    {"GL_ATI_texture_env_combine3",         ATI_TEXTURE_ENV_COMBINE3,       0                           },
    {"GL_ATI_texture_mirror_once",          ATI_TEXTURE_MIRROR_ONCE,        0                           },

    /* EXT */
    {"GL_EXT_blend_color",                  EXT_BLEND_COLOR,                0                           },
    {"GL_EXT_blend_equation_separate",      EXT_BLEND_EQUATION_SEPARATE,    0                           },
    {"GL_EXT_blend_func_separate",          EXT_BLEND_FUNC_SEPARATE,        0                           },
    {"GL_EXT_blend_minmax",                 EXT_BLEND_MINMAX,               0                           },
    {"GL_EXT_draw_buffers2",                EXT_DRAW_BUFFERS2,              0                           },
    {"GL_EXT_fog_coord",                    EXT_FOG_COORD,                  0                           },
    {"GL_EXT_framebuffer_blit",             EXT_FRAMEBUFFER_BLIT,           0                           },
    {"GL_EXT_framebuffer_multisample",      EXT_FRAMEBUFFER_MULTISAMPLE,    0                           },
    {"GL_EXT_framebuffer_object",           EXT_FRAMEBUFFER_OBJECT,         0                           },
    {"GL_EXT_gpu_program_parameters",       EXT_GPU_PROGRAM_PARAMETERS,     0                           },
    {"GL_EXT_gpu_shader4",                  EXT_GPU_SHADER4,                0                           },
    {"GL_EXT_packed_depth_stencil",         EXT_PACKED_DEPTH_STENCIL,       0                           },
    {"GL_EXT_paletted_texture",             EXT_PALETTED_TEXTURE,           0                           },
    {"GL_EXT_point_parameters",             EXT_POINT_PARAMETERS,           0                           },
    {"GL_EXT_provoking_vertex",             EXT_PROVOKING_VERTEX,           0                           },
    {"GL_EXT_secondary_color",              EXT_SECONDARY_COLOR,            0                           },
    {"GL_EXT_stencil_two_side",             EXT_STENCIL_TWO_SIDE,           0                           },
    {"GL_EXT_stencil_wrap",                 EXT_STENCIL_WRAP,               0                           },
    {"GL_EXT_texture3D",                    EXT_TEXTURE3D,                  MAKEDWORD_VERSION(1, 2)     },
    {"GL_EXT_texture_compression_rgtc",     EXT_TEXTURE_COMPRESSION_RGTC,   0                           },
    {"GL_EXT_texture_compression_s3tc",     EXT_TEXTURE_COMPRESSION_S3TC,   0                           },
    {"GL_EXT_texture_env_add",              EXT_TEXTURE_ENV_ADD,            0                           },
    {"GL_EXT_texture_env_combine",          EXT_TEXTURE_ENV_COMBINE,        0                           },
    {"GL_EXT_texture_env_dot3",             EXT_TEXTURE_ENV_DOT3,           0                           },
    {"GL_EXT_texture_filter_anisotropic",   EXT_TEXTURE_FILTER_ANISOTROPIC, 0                           },
    {"GL_EXT_texture_lod_bias",             EXT_TEXTURE_LOD_BIAS,           0                           },
    {"GL_EXT_texture_sRGB",                 EXT_TEXTURE_SRGB,               0                           },
    {"GL_EXT_vertex_array_bgra",            EXT_VERTEX_ARRAY_BGRA,          0                           },

    /* NV */
    {"GL_NV_depth_clamp",                   NV_DEPTH_CLAMP,                 0                           },
    {"GL_NV_fence",                         NV_FENCE,                       0                           },
    {"GL_NV_fog_distance",                  NV_FOG_DISTANCE,                0                           },
    {"GL_NV_fragment_program",              NV_FRAGMENT_PROGRAM,            0                           },
    {"GL_NV_fragment_program2",             NV_FRAGMENT_PROGRAM2,           0                           },
    {"GL_NV_fragment_program_option",       NV_FRAGMENT_PROGRAM_OPTION,     0                           },
    {"GL_NV_half_float",                    NV_HALF_FLOAT,                  0                           },
    {"GL_NV_light_max_exponent",            NV_LIGHT_MAX_EXPONENT,          0                           },
    {"GL_NV_register_combiners",            NV_REGISTER_COMBINERS,          0                           },
    {"GL_NV_register_combiners2",           NV_REGISTER_COMBINERS2,         0                           },
    {"GL_NV_texgen_reflection",             NV_TEXGEN_REFLECTION,           0                           },
    {"GL_NV_texture_env_combine4",          NV_TEXTURE_ENV_COMBINE4,        0                           },
    {"GL_NV_texture_shader",                NV_TEXTURE_SHADER,              0                           },
    {"GL_NV_texture_shader2",               NV_TEXTURE_SHADER2,             0                           },
    {"GL_NV_vertex_program",                NV_VERTEX_PROGRAM,              0                           },
    {"GL_NV_vertex_program1_1",             NV_VERTEX_PROGRAM1_1,           0                           },
    {"GL_NV_vertex_program2",               NV_VERTEX_PROGRAM2,             0                           },
    {"GL_NV_vertex_program2_option",        NV_VERTEX_PROGRAM2_OPTION,      0                           },
    {"GL_NV_vertex_program3",               NV_VERTEX_PROGRAM3,             0                           },

    /* SGI */
    {"GL_SGIS_generate_mipmap",             SGIS_GENERATE_MIPMAP,           0                           },
};

/**********************************************************
 * Utility functions follow
 **********************************************************/

const struct min_lookup minMipLookup[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST_MIPMAP_NEAREST,  GL_NEAREST_MIPMAP_LINEAR}}, /* POINT*/
    {{GL_LINEAR,    GL_LINEAR_MIPMAP_NEAREST,   GL_LINEAR_MIPMAP_LINEAR}},  /* LINEAR */
};

const struct min_lookup minMipLookup_noFilter[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* POINT */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* LINEAR */
};

const struct min_lookup minMipLookup_noMip[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* POINT */
    {{GL_LINEAR,    GL_LINEAR,                  GL_LINEAR }},               /* LINEAR */
};

const GLenum magLookup[] =
{
    /* NONE     POINT       LINEAR */
    GL_NEAREST, GL_NEAREST, GL_LINEAR,
};

const GLenum magLookup_noFilter[] =
{
    /* NONE     POINT       LINEAR */
    GL_NEAREST, GL_NEAREST, GL_NEAREST,
};

/* drawStridedSlow attributes */
glAttribFunc position_funcs[WINED3D_FFP_EMIT_COUNT];
glAttribFunc diffuse_funcs[WINED3D_FFP_EMIT_COUNT];
glAttribFunc specular_func_3ubv;
glAttribFunc specular_funcs[WINED3D_FFP_EMIT_COUNT];
glAttribFunc normal_funcs[WINED3D_FFP_EMIT_COUNT];
glMultiTexCoordFunc multi_texcoord_funcs[WINED3D_FFP_EMIT_COUNT];


/**********************************************************
 * IWineD3D parts follows
 **********************************************************/

/* GL locking is done by the caller */
static inline BOOL test_arb_vs_offset_limit(const struct wined3d_gl_info *gl_info)
{
    GLuint prog;
    BOOL ret = FALSE;
    const char *testcode =
        "!!ARBvp1.0\n"
        "PARAM C[66] = { program.env[0..65] };\n"
        "ADDRESS A0;"
        "PARAM zero = {0.0, 0.0, 0.0, 0.0};\n"
        "ARL A0.x, zero.x;\n"
        "MOV result.position, C[A0.x + 65];\n"
        "END\n";

    while(glGetError());
    GL_EXTCALL(glGenProgramsARB(1, &prog));
    if(!prog) {
        ERR("Failed to create an ARB offset limit test program\n");
    }
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  (GLsizei)strlen(testcode), testcode));
    if(glGetError() != 0) {
        TRACE("OpenGL implementation does not allow indirect addressing offsets > 63\n");
        TRACE("error: %s\n", debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
    } else TRACE("OpenGL implementation allows offsets > 63\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("ARB vp offset limit test cleanup");

    return ret;
}

static DWORD ver_for_ext(GL_SupportedExt ext)
{
    unsigned int i;
    for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i) {
        if(EXTENSION_MAP[i].extension == ext) {
            return EXTENSION_MAP[i].version;
        }
    }
    return 0;
}

static BOOL match_ati_r300_to_500(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_ATI) return FALSE;
    if (device == CARD_ATI_RADEON_9500) return TRUE;
    if (device == CARD_ATI_RADEON_X700) return TRUE;
    if (device == CARD_ATI_RADEON_X1600) return TRUE;
    return FALSE;
}

static BOOL match_geforce5(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor == HW_VENDOR_NVIDIA)
    {
        if (device == CARD_NVIDIA_GEFORCEFX_5800 || device == CARD_NVIDIA_GEFORCEFX_5600)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL match_apple(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* MacOS has various specialities in the extensions it advertises. Some have to be loaded from
     * the opengl 1.2+ core, while other extensions are advertised, but software emulated. So try to
     * detect the Apple OpenGL implementation to apply some extension fixups afterwards.
     *
     * Detecting this isn't really easy. The vendor string doesn't mention Apple. Compile-time checks
     * aren't sufficient either because a Linux binary may display on a macos X server via remote X11.
     * So try to detect the GL implementation by looking at certain Apple extensions. Some extensions
     * like client storage might be supported on other implementations too, but GL_APPLE_flush_render
     * is specific to the Mac OS X window management, and GL_APPLE_ycbcr_422 is QuickTime specific. So
     * the chance that other implementations support them is rather small since Win32 QuickTime uses
     * DirectDraw, not OpenGL.
     *
     * This test has been moved into wined3d_guess_gl_vendor()
     */
    if (gl_vendor == GL_VENDOR_APPLE)
    {
        return TRUE;
    }
    return FALSE;
}

/* Context activation is done by the caller. */
static void test_pbo_functionality(struct wined3d_gl_info *gl_info)
{
    /* Some OpenGL implementations, namely Apple's Geforce 8 driver, advertises PBOs,
     * but glTexSubImage from a PBO fails miserably, with the first line repeated over
     * all the texture. This function detects this bug by its symptom and disables PBOs
     * if the test fails.
     *
     * The test uploads a 4x4 texture via the PBO in the "native" format GL_BGRA,
     * GL_UNSIGNED_INT_8_8_8_8_REV. This format triggers the bug, and it is what we use
     * for D3DFMT_A8R8G8B8. Then the texture is read back without any PBO and the data
     * read back is compared to the original. If they are equal PBOs are assumed to work,
     * otherwise the PBO extension is disabled. */
    GLuint texture, pbo;
    static const unsigned int pattern[] =
    {
        0x00000000, 0x000000ff, 0x0000ff00, 0x40ff0000,
        0x80ffffff, 0x40ffff00, 0x00ff00ff, 0x0000ffff,
        0x00ffff00, 0x00ff00ff, 0x0000ffff, 0x000000ff,
        0x80ff00ff, 0x0000ffff, 0x00ff00ff, 0x40ff00ff
    };
    unsigned int check[sizeof(pattern) / sizeof(pattern[0])];

    /* No PBO -> No point in testing them. */
    if (!gl_info->supported[ARB_PIXEL_BUFFER_OBJECT]) return;

    ENTER_GL();

    while (glGetError());
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
    checkGLcall("Specifying the PBO test texture");

    GL_EXTCALL(glGenBuffersARB(1, &pbo));
    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo));
    GL_EXTCALL(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, sizeof(pattern), pattern, GL_STREAM_DRAW_ARB));
    checkGLcall("Specifying the PBO test pbo");

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("Loading the PBO test texture");

    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
#ifdef VBOX_WITH_VMSVGA
    glFinish();
#else
    wglFinish(); /* just to be sure */
#endif
    memset(check, 0, sizeof(check));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, check);
    checkGLcall("Reading back the PBO test texture");

    glDeleteTextures(1, &texture);
    GL_EXTCALL(glDeleteBuffersARB(1, &pbo));
    checkGLcall("PBO test cleanup");

    LEAVE_GL();

    if (memcmp(check, pattern, sizeof(check)))
    {
        WARN_(d3d_caps)("PBO test failed, read back data doesn't match original.\n");
        WARN_(d3d_caps)("Disabling PBOs. This may result in slower performance.\n");
        gl_info->supported[ARB_PIXEL_BUFFER_OBJECT] = FALSE;
    }
    else
    {
        TRACE_(d3d_caps)("PBO test successful.\n");
    }
}

static BOOL match_apple_intel(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return (card_vendor == HW_VENDOR_INTEL) && (gl_vendor == GL_VENDOR_APPLE);
}

static BOOL match_apple_nonr500ati(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (gl_vendor != GL_VENDOR_APPLE) return FALSE;
    if (card_vendor != HW_VENDOR_ATI) return FALSE;
    if (device == CARD_ATI_RADEON_X1600) return FALSE;
    return TRUE;
}

static BOOL match_fglrx(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    return gl_vendor == GL_VENDOR_FGLRX;

}

static BOOL match_dx10_capable(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    /* DX9 cards support 40 single float varyings in hardware, most drivers report 32. ATI misreports
     * 44 varyings. So assume that if we have more than 44 varyings we have a dx10 card.
     * This detection is for the gl_ClipPos varying quirk. If a d3d9 card really supports more than 44
     * varyings and we subtract one in dx9 shaders its not going to hurt us because the dx9 limit is
     * hardcoded
     *
     * dx10 cards usually have 64 varyings */
    return gl_info->limits.glsl_varyings > 44;
}

/* A GL context is provided by the caller */
static BOOL match_allows_spec_alpha(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    GLenum error;
    DWORD data[16];

    if (!gl_info->supported[EXT_SECONDARY_COLOR]) return FALSE;

    ENTER_GL();
    while(glGetError());
    GL_EXTCALL(glSecondaryColorPointerEXT)(4, GL_UNSIGNED_BYTE, 4, data);
    error = glGetError();
    LEAVE_GL();

    if(error == GL_NO_ERROR)
    {
        TRACE("GL Implementation accepts 4 component specular color pointers\n");
        return TRUE;
    }
    else
    {
        TRACE("GL implementation does not accept 4 component specular colors, error %s\n",
              debug_glerror(error));
        return FALSE;
    }
}

static BOOL match_apple_nvts(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (!match_apple(gl_info, gl_renderer, gl_vendor, card_vendor, device)) return FALSE;
    return gl_info->supported[NV_TEXTURE_SHADER];
}

/* A GL context is provided by the caller */
static BOOL match_broken_nv_clip(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    GLuint prog;
    BOOL ret = FALSE;
    GLint pos;
    const char *testcode =
        "!!ARBvp1.0\n"
        "OPTION NV_vertex_program2;\n"
        "MOV result.clip[0], 0.0;\n"
        "MOV result.position, 0.0;\n"
        "END\n";

    if (!gl_info->supported[NV_VERTEX_PROGRAM2_OPTION]) return FALSE;

    ENTER_GL();
    while(glGetError());

    GL_EXTCALL(glGenProgramsARB(1, &prog));
    if(!prog)
    {
        ERR("Failed to create the NVvp clip test program\n");
        LEAVE_GL();
        return FALSE;
    }
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prog));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  (GLsizei)strlen(testcode), testcode));
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
    if(pos != -1)
    {
        WARN("GL_NV_vertex_program2_option result.clip[] test failed\n");
        TRACE("error: %s\n", debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        ret = TRUE;
        while(glGetError());
    }
    else TRACE("GL_NV_vertex_program2_option result.clip[] test passed\n");

    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0));
    GL_EXTCALL(glDeleteProgramsARB(1, &prog));
    checkGLcall("GL_NV_vertex_program2_option result.clip[] test cleanup");

    LEAVE_GL();
    return ret;
}

/* Context activation is done by the caller. */
static BOOL match_fbo_tex_update(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    char data[4 * 4 * 4];
    GLuint tex, fbo;
    GLenum status;

#ifndef VBOX_WITH_VMSVGA
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return FALSE;
#endif
    memset(data, 0xcc, sizeof(data));

    ENTER_GL();

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    checkGLcall("glTexImage2D");

    gl_info->fbo_ops.glGenFramebuffers(1, &fbo);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    checkGLcall("glFramebufferTexture2D");

    status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) ERR("FBO status %#x\n", status);
    checkGLcall("glCheckFramebufferStatus");

    memset(data, 0x11, sizeof(data));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glTexSubImage2D");

    glClearColor(0.996, 0.729, 0.745, 0.792);
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLcall("glClear");

    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    checkGLcall("glGetTexImage");

    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    gl_info->fbo_ops.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLcall("glBindTexture");

    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    checkGLcall("glDeleteTextures");

    LEAVE_GL();

    return *(DWORD *)data == 0x11111111;
}

static void quirk_arb_constants(struct wined3d_gl_info *gl_info)
{
    TRACE_(d3d_caps)("Using ARB vs constant limit(=%u) for GLSL.\n", gl_info->limits.arb_vs_native_constants);
    gl_info->limits.glsl_vs_float_constants = gl_info->limits.arb_vs_native_constants;
    TRACE_(d3d_caps)("Using ARB ps constant limit(=%u) for GLSL.\n", gl_info->limits.arb_ps_native_constants);
    gl_info->limits.glsl_ps_float_constants = gl_info->limits.arb_ps_native_constants;
}

static void quirk_apple_glsl_constants(struct wined3d_gl_info *gl_info)
{
    quirk_arb_constants(gl_info);
    /* MacOS needs uniforms for relative addressing offsets. This can accumulate to quite a few uniforms.
     * Beyond that the general uniform isn't optimal, so reserve a number of uniforms. 12 vec4's should
     * allow 48 different offsets or other helper immediate values. */
    TRACE_(d3d_caps)("Reserving 12 GLSL constants for compiler private use.\n");
    gl_info->reserved_glsl_constants = max(gl_info->reserved_glsl_constants, 12);
}

/* fglrx crashes with a very bad kernel panic if GL_POINT_SPRITE_ARB is set to GL_COORD_REPLACE_ARB
 * on more than one texture unit. This means that the d3d9 visual point size test will cause a
 * kernel panic on any machine running fglrx 9.3(latest that supports r300 to r500 cards). This
 * quirk only enables point sprites on the first texture unit. This keeps point sprites working in
 * most games, but avoids the crash
 *
 * A more sophisticated way would be to find all units that need texture coordinates and enable
 * point sprites for one if only one is found, and software emulate point sprites in drawStridedSlow
 * if more than one unit needs texture coordinates(This requires software ffp and vertex shaders though)
 *
 * Note that disabling the extension entirely does not gain predictability because there is no point
 * sprite capability flag in d3d, so the potential rendering bugs are the same if we disable the extension. */
static void quirk_one_point_sprite(struct wined3d_gl_info *gl_info)
{
    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        TRACE("Limiting point sprites to one texture unit.\n");
        gl_info->limits.point_sprite_units = 1;
    }
}

static void quirk_ati_dx9(struct wined3d_gl_info *gl_info)
{
    quirk_arb_constants(gl_info);

    /* MacOS advertises GL_ARB_texture_non_power_of_two on ATI r500 and earlier cards, although
     * these cards only support GL_ARB_texture_rectangle(D3DPTEXTURECAPS_NONPOW2CONDITIONAL).
     * If real NP2 textures are used, the driver falls back to software. We could just remove the
     * extension and use GL_ARB_texture_rectangle instead, but texture_rectangle is inconventient
     * due to the non-normalized texture coordinates. Thus set an internal extension flag,
     * GL_WINE_normalized_texrect, which signals the code that it can use non power of two textures
     * as per GL_ARB_texture_non_power_of_two, but has to stick to the texture_rectangle limits.
     *
     * fglrx doesn't advertise GL_ARB_texture_non_power_of_two, but it advertises opengl 2.0 which
     * has this extension promoted to core. The extension loading code sets this extension supported
     * due to that, so this code works on fglrx as well. */
    if(gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        TRACE("GL_ARB_texture_non_power_of_two advertised on R500 or earlier card, removing.\n");
        gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
        gl_info->supported[WINE_NORMALIZED_TEXRECT] = TRUE;
    }

    /* fglrx has the same structural issues as the one described in quirk_apple_glsl_constants, although
     * it is generally more efficient. Reserve just 8 constants. */
    TRACE_(d3d_caps)("Reserving 8 GLSL constants for compiler private use.\n");
    gl_info->reserved_glsl_constants = max(gl_info->reserved_glsl_constants, 8);
}

static void quirk_no_np2(struct wined3d_gl_info *gl_info)
{
    /*  The nVidia GeForceFX series reports OpenGL 2.0 capabilities with the latest drivers versions, but
     *  doesn't explicitly advertise the ARB_tex_npot extension in the GL extension string.
     *  This usually means that ARB_tex_npot is supported in hardware as long as the application is staying
     *  within the limits enforced by the ARB_texture_rectangle extension. This however is not true for the
     *  FX series, which instantly falls back to a slower software path as soon as ARB_tex_npot is used.
     *  We therefore completely remove ARB_tex_npot from the list of supported extensions.
     *
     *  Note that wine_normalized_texrect can't be used in this case because internally it uses ARB_tex_npot,
     *  triggering the software fallback. There is not much we can do here apart from disabling the
     *  software-emulated extension and reenable ARB_tex_rect (which was previously disabled
     *  in IWineD3DImpl_FillGLCaps).
     *  This fixup removes performance problems on both the FX 5900 and FX 5700 (e.g. for framebuffer
     *  post-processing effects in the game "Max Payne 2").
     *  The behaviour can be verified through a simple test app attached in bugreport #14724. */
    TRACE("GL_ARB_texture_non_power_of_two advertised through OpenGL 2.0 on NV FX card, removing.\n");
    gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] = FALSE;
    gl_info->supported[ARB_TEXTURE_RECTANGLE] = TRUE;
}

static void quirk_texcoord_w(struct wined3d_gl_info *gl_info)
{
    /* The Intel GPUs on MacOS set the .w register of texcoords to 0.0 by default, which causes problems
     * with fixed function fragment processing. Ideally this flag should be detected with a test shader
     * and OpenGL feedback mode, but some GL implementations (MacOS ATI at least, probably all MacOS ones)
     * do not like vertex shaders in feedback mode and return an error, even though it should be valid
     * according to the spec.
     *
     * We don't want to enable this on all cards, as it adds an extra instruction per texcoord used. This
     * makes the shader slower and eats instruction slots which should be available to the d3d app.
     *
     * ATI Radeon HD 2xxx cards on MacOS have the issue. Instead of checking for the buggy cards, blacklist
     * all radeon cards on Macs and whitelist the good ones. That way we're prepared for the future. If
     * this workaround is activated on cards that do not need it, it won't break things, just affect
     * performance negatively. */
    TRACE("Enabling vertex texture coord fixes in vertex shaders.\n");
    gl_info->quirks |= WINED3D_QUIRK_SET_TEXCOORD_W;
}

static void quirk_clip_varying(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_GLSL_CLIP_VARYING;
}

static void quirk_allows_specular_alpha(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_ALLOWS_SPECULAR_ALPHA;
}

static void quirk_apple_nvts(struct wined3d_gl_info *gl_info)
{
    gl_info->supported[NV_TEXTURE_SHADER] = FALSE;
    gl_info->supported[NV_TEXTURE_SHADER2] = FALSE;
}

static void quirk_disable_nvvp_clip(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_NV_CLIP_BROKEN;
}

static void quirk_fbo_tex_update(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_FBO_TEX_UPDATE;
}

static BOOL match_ati_hd4800(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_ATI) return FALSE;
    if (device == CARD_ATI_RADEON_HD4800) return TRUE;
    return FALSE;
}

static void quirk_fullsize_blit(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_FULLSIZE_BLIT;
}

#ifdef VBOX_WITH_WDDM
static BOOL match_mesa_nvidia(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor != HW_VENDOR_NVIDIA) return FALSE;
    if (gl_vendor != GL_VENDOR_MESA) return FALSE;
    return TRUE;
}

static void quirk_no_shader_3(struct wined3d_gl_info *gl_info)
{
    int vs_selected_mode, ps_selected_mode;
    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if (vs_selected_mode != SHADER_GLSL && ps_selected_mode != SHADER_GLSL)
        return;

    gl_info->limits.arb_ps_instructions = 512;
}
#endif

static BOOL match_intel(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    if (card_vendor == HW_VENDOR_INTEL) return TRUE;
    if (gl_vendor == GL_VENDOR_INTEL) return TRUE;
    return FALSE;
}

static void quirk_force_blit(struct wined3d_gl_info *gl_info)
{
    gl_info->quirks |= WINED3D_QUIRK_FORCE_BLIT;
}

struct driver_quirk
{
    BOOL (*match)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
            enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device);
    void (*apply)(struct wined3d_gl_info *gl_info);
    const char *description;
};

static const struct driver_quirk quirk_table[] =
{
    {
        match_ati_r300_to_500,
        quirk_ati_dx9,
        "ATI GLSL constant and normalized texrect quirk"
    },
    /* MacOS advertises more GLSL vertex shader uniforms than supported by the hardware, and if more are
     * used it falls back to software. While the compiler can detect if the shader uses all declared
     * uniforms, the optimization fails if the shader uses relative addressing. So any GLSL shader
     * using relative addressing falls back to software.
     *
     * ARB vp gives the correct amount of uniforms, so use it instead of GLSL. */
    {
        match_apple,
        quirk_apple_glsl_constants,
        "Apple GLSL uniform override"
    },
    {
        match_geforce5,
        quirk_no_np2,
        "Geforce 5 NP2 disable"
    },
    {
        match_apple_intel,
        quirk_texcoord_w,
        "Init texcoord .w for Apple Intel GPU driver"
    },
    {
        match_apple_nonr500ati,
        quirk_texcoord_w,
        "Init texcoord .w for Apple ATI >= r600 GPU driver"
    },
    {
        match_fglrx,
        quirk_one_point_sprite,
        "Fglrx point sprite crash workaround"
    },
    {
        match_dx10_capable,
        quirk_clip_varying,
        "Reserved varying for gl_ClipPos"
    },
    {
        /* GL_EXT_secondary_color does not allow 4 component secondary colors, but most
         * GL implementations accept it. The Mac GL is the only implementation known to
         * reject it.
         *
         * If we can pass 4 component specular colors, do it, because (a) we don't have
         * to screw around with the data, and (b) the D3D fixed function vertex pipeline
         * passes specular alpha to the pixel shader if any is used. Otherwise the
         * specular alpha is used to pass the fog coordinate, which we pass to opengl
         * via GL_EXT_fog_coord.
         */
        match_allows_spec_alpha,
        quirk_allows_specular_alpha,
        "Allow specular alpha quirk"
    },
    {
        /* The pixel formats provided by GL_NV_texture_shader are broken on OSX
         * (rdar://5682521).
         */
        match_apple_nvts,
        quirk_apple_nvts,
        "Apple NV_texture_shader disable"
    },
#ifndef VBOX_WITH_VMSVGA
    {
        match_broken_nv_clip,
        quirk_disable_nvvp_clip,
        "Apple NV_vertex_program clip bug quirk"
    },
#endif
    {
        match_fbo_tex_update,
        quirk_fbo_tex_update,
        "FBO rebind for attachment updates"
    },
    {
        match_ati_hd4800,
        quirk_fullsize_blit,
        "Fullsize blit"
    },
#ifdef VBOX_WITH_WDDM
    {
        match_mesa_nvidia,
        quirk_no_shader_3,
        "disable shader 3 support"
    },
#endif
    {
        match_intel,
        quirk_force_blit,
        "force framebuffer blit when possible"
    }
};

/* Context activation is done by the caller. */
static void fixup_extensions(struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor gl_vendor, enum wined3d_pci_vendor card_vendor, enum wined3d_pci_device device)
{
    unsigned int i;

    for (i = 0; i < (sizeof(quirk_table) / sizeof(*quirk_table)); ++i)
    {
        if (!quirk_table[i].match(gl_info, gl_renderer, gl_vendor, card_vendor, device)) continue;
        TRACE_(d3d_caps)("Applying driver quirk \"%s\".\n", quirk_table[i].description);
        quirk_table[i].apply(gl_info);
    }

    /* Find out if PBOs work as they are supposed to. */
    test_pbo_functionality(gl_info);
}


/* Certain applications (Steam) complain if we report an outdated driver version. In general,
 * reporting a driver version is moot because we are not the Windows driver, and we have different
 * bugs, features, etc.
 *
 * The driver version has the form "x.y.z.w".
 *
 * "x" is the Windows version the driver is meant for:
 * 4 -> 95/98/NT4
 * 5 -> 2000
 * 6 -> 2000/XP
 * 7 -> Vista
 * 8 -> Win 7
 *
 * "y" is the Direct3D level the driver supports:
 * 11 -> d3d6
 * 12 -> d3d7
 * 13 -> d3d8
 * 14 -> d3d9
 * 15 -> d3d10
 *
 * "z" is unknown, possibly vendor specific.
 *
 * "w" is the vendor specific driver version.
 */
struct driver_version_information
{
    WORD vendor;                    /* reported PCI card vendor ID  */
    WORD card;                      /* reported PCI card device ID  */
    const char *description;        /* Description of the card e.g. NVIDIA RIVA TNT */
    WORD d3d_level;                 /* driver hiword to report      */
    WORD lopart_hi, lopart_lo;      /* driver loword to report      */
};

#if 0 /* VBox: unused */
static const struct driver_version_information driver_version_table[] =
{
    /* Nvidia drivers. Geforce6 and newer cards are supported by the current driver (180.x)
     * GeforceFX support is up to 173.x, - driver uses numbering x.y.11.7341 for 173.41 where x is the windows revision (6=2000/xp, 7=vista), y is unknown
     * Geforce2MX/3/4 up to 96.x - driver uses numbering 9.6.8.9 for 96.89
     * TNT/Geforce1/2 up to 71.x - driver uses numbering 7.1.8.6 for 71.86
     *
     * All version numbers used below are from the Linux nvidia drivers. */
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT,           "NVIDIA RIVA TNT",                  1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT2,          "NVIDIA RIVA TNT2/TNT2 Pro",        1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE,            "NVIDIA GeForce 256",               1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2_MX,        "NVIDIA GeForce2 MX/MX 400",        6,  4,  3      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2,           "NVIDIA GeForce2 GTS/GeForce2 Pro", 1,  8,  6      },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE3,           "NVIDIA GeForce3",                  6,  10, 9371   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_MX,        "NVIDIA GeForce4 MX 460",           6,  10, 9371   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_TI4200,    "NVIDIA GeForce4 Ti 4200",          6,  10, 9371   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5200,     "NVIDIA GeForce FX 5200",           15, 11, 7516   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5600,     "NVIDIA GeForce FX 5600",           15, 11, 7516   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5800,     "NVIDIA GeForce FX 5800",           15, 11, 7516   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6200,       "NVIDIA GeForce 6200",              15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6600GT,     "NVIDIA GeForce 6600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6800,       "NVIDIA GeForce 6800",              15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7300,       "NVIDIA GeForce Go 7300",           15, 11, 8585   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7400,       "NVIDIA GeForce Go 7400",           15, 11, 8585   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7600,       "NVIDIA GeForce 7600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7800GT,     "NVIDIA GeForce 7800 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     "NVIDIA GeForce 8300 GS",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     "NVIDIA GeForce 8600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    "NVIDIA GeForce 8600M GT",          15, 11, 8585   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    "NVIDIA GeForce 8800 GTS",          15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9200,       "NVIDIA GeForce 9200",              15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400GT,     "NVIDIA GeForce 9400 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9500GT,     "NVIDIA GeForce 9500 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     "NVIDIA GeForce 9600 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     "NVIDIA GeForce 9800 GT",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX260,     "NVIDIA GeForce GTX 260",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX275,     "NVIDIA GeForce GTX 275",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX280,     "NVIDIA GeForce GTX 280",           15, 11, 8618   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT240,      "NVIDIA GeForce GT 240",            15, 11, 8618   },

    /* ATI cards. The driver versions are somewhat similar, but not quite the same. Let's hardcode. */
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_9500,           "ATI Radeon 9500",                  14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_X700,           "ATI Radeon X700 SE",               14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_X1600,          "ATI Radeon X1600 Series",          14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD2350,         "ATI Mobility Radeon HD 2350",      14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD2600,         "ATI Mobility Radeon HD 2600",      14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD2900,         "ATI Radeon HD 2900 XT",            14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4350,         "ATI Radeon HD 4350",               14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4600,         "ATI Radeon HD 4600 Series",        14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4700,         "ATI Radeon HD 4700 Series",        14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD4800,         "ATI Radeon HD 4800 Series",        14, 10, 6764    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD5700,         "ATI Radeon HD 5700 Series",        14, 10, 8681    },
    {HW_VENDOR_ATI,        CARD_ATI_RADEON_HD5800,         "ATI Radeon HD 5800 Series",        14, 10, 8681    },

    /* TODO: Add information about legacy ATI hardware, Intel and other cards. */
};
#endif /* VBox: unused */


static DWORD wined3d_parse_gl_version(const char *gl_version)
{
    const char *ptr = gl_version;
    int major, minor;

    major = atoi(ptr);
    if (major <= 0) ERR_(d3d_caps)("Invalid opengl major version: %d.\n", major);

    while (isdigit(*ptr)) ++ptr;
    if (*ptr++ != '.') ERR_(d3d_caps)("Invalid opengl version string: %s.\n", debugstr_a(gl_version));

    minor = atoi(ptr);

    TRACE_(d3d_caps)("Found OpenGL version: %d.%d.\n", major, minor);

    return MAKEDWORD_VERSION(major, minor);
}

static enum wined3d_gl_vendor wined3d_guess_gl_vendor(struct wined3d_gl_info *gl_info, const char *gl_vendor_string, const char *gl_renderer)
{

    /* MacOS has various specialities in the extensions it advertises. Some have to be loaded from
     * the opengl 1.2+ core, while other extensions are advertised, but software emulated. So try to
     * detect the Apple OpenGL implementation to apply some extension fixups afterwards.
     *
     * Detecting this isn't really easy. The vendor string doesn't mention Apple. Compile-time checks
     * aren't sufficient either because a Linux binary may display on a macos X server via remote X11.
     * So try to detect the GL implementation by looking at certain Apple extensions. Some extensions
     * like client storage might be supported on other implementations too, but GL_APPLE_flush_render
     * is specific to the Mac OS X window management, and GL_APPLE_ycbcr_422 is QuickTime specific. So
     * the chance that other implementations support them is rather small since Win32 QuickTime uses
     * DirectDraw, not OpenGL. */
    if (gl_info->supported[APPLE_FENCE]
            && gl_info->supported[APPLE_CLIENT_STORAGE]
            && gl_info->supported[APPLE_FLUSH_RENDER]
            && gl_info->supported[APPLE_YCBCR_422])
        return GL_VENDOR_APPLE;

    if (strstr(gl_vendor_string, "NVIDIA"))
        return GL_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI"))
        return GL_VENDOR_FGLRX;

    if (strstr(gl_vendor_string, "Intel(R)")
            || strstr(gl_renderer, "Intel(R)")
            || strstr(gl_vendor_string, "Intel Inc."))
    {
        if (strstr(gl_renderer, "Mesa"))
            return GL_VENDOR_MESA;
        return GL_VENDOR_INTEL;
    }

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "DRI R300 Project")
            || strstr(gl_vendor_string, "X.Org R300 Project")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc.")
            || strstr(gl_renderer, "Mesa")
            || strstr(gl_renderer, "Gallium"))
        return GL_VENDOR_MESA;

    FIXME_(d3d_caps)("Received unrecognized GL_VENDOR %s. Returning GL_VENDOR_UNKNOWN.\n",
            debugstr_a(gl_vendor_string));

    return GL_VENDOR_UNKNOWN;
}

static enum wined3d_pci_vendor wined3d_guess_card_vendor(const char *gl_vendor_string, const char *gl_renderer)
{
    if (strstr(gl_vendor_string, "NVIDIA"))
        return HW_VENDOR_NVIDIA;

    if (strstr(gl_vendor_string, "ATI")
            || strstr(gl_vendor_string, "Advanced Micro Devices, Inc.")
            || strstr(gl_vendor_string, "X.Org R300 Project")
            || strstr(gl_vendor_string, "DRI R300 Project"))
        return HW_VENDOR_ATI;

    if (strstr(gl_vendor_string, "Intel(R)")
            || strstr(gl_renderer, "Intel(R)")
            || strstr(gl_vendor_string, "Intel Inc."))
        return HW_VENDOR_INTEL;

    if (strstr(gl_vendor_string, "Mesa")
            || strstr(gl_vendor_string, "Tungsten Graphics, Inc")
            || strstr(gl_vendor_string, "VMware, Inc."))
        return HW_VENDOR_SOFTWARE;

    FIXME_(d3d_caps)("Received unrecognized GL_VENDOR %s. Returning HW_VENDOR_NVIDIA.\n", debugstr_a(gl_vendor_string));

    return HW_VENDOR_NVIDIA;
}



static enum wined3d_pci_device select_card_nvidia_binary(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
#ifndef VBOX_WITH_WDDM
    if (WINE_D3D10_CAPABLE(gl_info))
#endif
    {
        /* Geforce 200 - highend */
        if (strstr(gl_renderer, "GTX 280")
                || strstr(gl_renderer, "GTX 285")
                || strstr(gl_renderer, "GTX 295"))
        {
            *vidmem = 1024;
            return CARD_NVIDIA_GEFORCE_GTX280;
        }

        /* Geforce 200 - midend high */
        if (strstr(gl_renderer, "GTX 275"))
        {
            *vidmem = 896;
            return CARD_NVIDIA_GEFORCE_GTX275;
        }

        /* Geforce 200 - midend */
        if (strstr(gl_renderer, "GTX 260"))
        {
            *vidmem = 1024;
            return CARD_NVIDIA_GEFORCE_GTX260;
        }
        /* Geforce 200 - midend */
        if (strstr(gl_renderer, "GT 240"))
        {
           *vidmem = 512;
           return CARD_NVIDIA_GEFORCE_GT240;
        }

        /* Geforce9 - highend / Geforce 200 - midend (GTS 150/250 are based on the same core) */
        if (strstr(gl_renderer, "9800")
                || strstr(gl_renderer, "GTS 150")
                || strstr(gl_renderer, "GTS 250"))
        {
            *vidmem = 512;
            return CARD_NVIDIA_GEFORCE_9800GT;
        }

        /* Geforce9 - midend */
        if (strstr(gl_renderer, "9600"))
        {
            *vidmem = 384; /* The 9600GSO has 384MB, the 9600GT has 512-1024MB */
            return CARD_NVIDIA_GEFORCE_9600GT;
        }

        /* Geforce9 - midend low / Geforce 200 - low */
        if (strstr(gl_renderer, "9500")
                || strstr(gl_renderer, "GT 120")
                || strstr(gl_renderer, "GT 130"))
        {
            *vidmem = 256; /* The 9500GT has 256-1024MB */
            return CARD_NVIDIA_GEFORCE_9500GT;
        }

        /* Geforce9 - lowend */
        if (strstr(gl_renderer, "9400"))
        {
            *vidmem = 256; /* The 9400GT has 256-1024MB */
            return CARD_NVIDIA_GEFORCE_9400GT;
        }

        /* Geforce9 - lowend low */
        if (strstr(gl_renderer, "9100")
                || strstr(gl_renderer, "9200")
                || strstr(gl_renderer, "9300")
                || strstr(gl_renderer, "G 100"))
        {
            *vidmem = 256; /* The 9100-9300 cards have 256MB */
            return CARD_NVIDIA_GEFORCE_9200;
        }

        /* Geforce8 - highend */
        if (strstr(gl_renderer, "8800"))
        {
            *vidmem = 320; /* The 8800GTS uses 320MB, a 8800GTX can have 768MB */
            return CARD_NVIDIA_GEFORCE_8800GTS;
        }

        /* Geforce8 - midend mobile */
        if (strstr(gl_renderer, "8600 M"))
        {
            *vidmem = 512;
            return CARD_NVIDIA_GEFORCE_8600MGT;
        }

        /* Geforce8 - midend */
        if (strstr(gl_renderer, "8600")
                || strstr(gl_renderer, "8700"))
        {
            *vidmem = 256;
            return CARD_NVIDIA_GEFORCE_8600GT;
        }

        /* Geforce8 - lowend */
        if (strstr(gl_renderer, "8100")
                || strstr(gl_renderer, "8200")
                || strstr(gl_renderer, "8300")
                || strstr(gl_renderer, "8400")
                || strstr(gl_renderer, "8500"))
        {
            *vidmem = 128; /* 128-256MB for a 8300, 256-512MB for a 8400 */
            return CARD_NVIDIA_GEFORCE_8300GS;
        }

        /* Geforce8-compatible fall back if the GPU is not in the list yet */
        *vidmem = 128;
        return CARD_NVIDIA_GEFORCE_8300GS;
    }

    /* Both the GeforceFX, 6xxx and 7xxx series support D3D9. The last two types have more
     * shader capabilities, so we use the shader capabilities to distinguish between FX and 6xxx/7xxx.
     */
    if (WINE_D3D9_CAPABLE(gl_info) && gl_info->supported[NV_VERTEX_PROGRAM3])
    {
        /* Geforce7 - highend */
        if (strstr(gl_renderer, "7800")
                || strstr(gl_renderer, "7900")
                || strstr(gl_renderer, "7950")
                || strstr(gl_renderer, "Quadro FX 4")
                || strstr(gl_renderer, "Quadro FX 5"))
        {
            *vidmem = 256; /* A 7800GT uses 256MB while highend 7900 cards can use 512MB */
            return CARD_NVIDIA_GEFORCE_7800GT;
        }

        /* Geforce7 midend */
        if (strstr(gl_renderer, "7600")
                || strstr(gl_renderer, "7700"))
        {
            *vidmem = 256; /* The 7600 uses 256-512MB */
            return CARD_NVIDIA_GEFORCE_7600;
        }

        /* Geforce7 lower medium */
        if (strstr(gl_renderer, "7400"))
        {
            *vidmem = 256; /* The 7400 uses 256-512MB */
            return CARD_NVIDIA_GEFORCE_7400;
        }

        /* Geforce7 lowend */
        if (strstr(gl_renderer, "7300"))
        {
            *vidmem = 256; /* Mac Pros with this card have 256 MB */
            return CARD_NVIDIA_GEFORCE_7300;
        }

        /* Geforce6 highend */
        if (strstr(gl_renderer, "6800"))
        {
            *vidmem = 128; /* The 6800 uses 128-256MB, the 7600 uses 256-512MB */
            return CARD_NVIDIA_GEFORCE_6800;
        }

        /* Geforce6 - midend */
        if (strstr(gl_renderer, "6600")
                || strstr(gl_renderer, "6610")
                || strstr(gl_renderer, "6700"))
        {
            *vidmem = 128; /* A 6600GT has 128-256MB */
            return CARD_NVIDIA_GEFORCE_6600GT;
        }

        /* Geforce6/7 lowend */
        *vidmem = 64; /* */
        return CARD_NVIDIA_GEFORCE_6200; /* Geforce 6100/6150/6200/7300/7400/7500 */
    }

    if (WINE_D3D9_CAPABLE(gl_info))
    {
        /* GeforceFX - highend */
        if (strstr(gl_renderer, "5800")
                || strstr(gl_renderer, "5900")
                || strstr(gl_renderer, "5950")
                || strstr(gl_renderer, "Quadro FX"))
        {
            *vidmem = 256; /* 5800-5900 cards use 256MB */
            return CARD_NVIDIA_GEFORCEFX_5800;
        }

        /* GeforceFX - midend */
        if (strstr(gl_renderer, "5600")
                || strstr(gl_renderer, "5650")
                || strstr(gl_renderer, "5700")
                || strstr(gl_renderer, "5750"))
        {
            *vidmem = 128; /* A 5600 uses 128-256MB */
            return CARD_NVIDIA_GEFORCEFX_5600;
        }

        /* GeforceFX - lowend */
        *vidmem = 64; /* Normal FX5200 cards use 64-256MB; laptop (non-standard) can have less */
        return CARD_NVIDIA_GEFORCEFX_5200; /* GeforceFX 5100/5200/5250/5300/5500 */
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        if (strstr(gl_renderer, "GeForce4 Ti") || strstr(gl_renderer, "Quadro4"))
        {
            *vidmem = 64; /* Geforce4 Ti cards have 64-128MB */
            return CARD_NVIDIA_GEFORCE4_TI4200; /* Geforce4 Ti4200/Ti4400/Ti4600/Ti4800, Quadro4 */
        }

        *vidmem = 64; /* Geforce3 cards have 64-128MB */
        return CARD_NVIDIA_GEFORCE3; /* Geforce3 standard/Ti200/Ti500, Quadro DCC */
    }

    if (WINE_D3D7_CAPABLE(gl_info))
    {
        if (strstr(gl_renderer, "GeForce4 MX"))
        {
            /* Most Geforce4MX GPUs have at least 64MB of memory, some
             * early models had 32MB but most have 64MB or even 128MB. */
            *vidmem = 64;
            return CARD_NVIDIA_GEFORCE4_MX; /* MX420/MX440/MX460/MX4000 */
        }

        if (strstr(gl_renderer, "GeForce2 MX") || strstr(gl_renderer, "Quadro2 MXR"))
        {
            *vidmem = 32; /* Geforce2MX GPUs have 32-64MB of video memory */
            return CARD_NVIDIA_GEFORCE2_MX; /* Geforce2 standard/MX100/MX200/MX400, Quadro2 MXR */
        }

        if (strstr(gl_renderer, "GeForce2") || strstr(gl_renderer, "Quadro2"))
        {
            *vidmem = 32; /* Geforce2 GPUs have 32-64MB of video memory */
            return CARD_NVIDIA_GEFORCE2; /* Geforce2 GTS/Pro/Ti/Ultra, Quadro2 */
        }

        /* Most Geforce1 cards have 32MB, there are also some rare 16
         * and 64MB (Dell) models. */
        *vidmem = 32;
        return CARD_NVIDIA_GEFORCE; /* Geforce 256/DDR, Quadro */
    }

    if (strstr(gl_renderer, "TNT2"))
    {
        *vidmem = 32; /* Most TNT2 boards have 32MB, though there are 16MB boards too */
        return CARD_NVIDIA_RIVA_TNT2; /* Riva TNT2 standard/M64/Pro/Ultra */
    }

    *vidmem = 16; /* Most TNT boards have 16MB, some rare models have 8MB */
    return CARD_NVIDIA_RIVA_TNT; /* Riva TNT, Vanta */

}

static enum wined3d_pci_device select_card_ati_binary(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
    /* See http://developer.amd.com/drivers/pc_vendor_id/Pages/default.aspx
     *
     * Beware: renderer string do not match exact card model,
     * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
#ifndef VBOX_WITH_WDDM
    if (WINE_D3D10_CAPABLE(gl_info))
#endif
    {
        /* Radeon EG CYPRESS XT / PRO HD5800 - highend */
        if (strstr(gl_renderer, "HD 5800")          /* Radeon EG CYPRESS HD58xx generic renderer string */
                || strstr(gl_renderer, "HD 5850")   /* Radeon EG CYPRESS XT */
                || strstr(gl_renderer, "HD 5870"))  /* Radeon EG CYPRESS PRO */
        {
            *vidmem = 1024; /* note: HD58xx cards use 1024MB  */
            return CARD_ATI_RADEON_HD5800;
        }

        /* Radeon EG JUNIPER XT / LE HD5700 - midend */
        if (strstr(gl_renderer, "HD 5700")          /* Radeon EG JUNIPER HD57xx generic renderer string */
                || strstr(gl_renderer, "HD 5750")   /* Radeon EG JUNIPER LE */
                || strstr(gl_renderer, "HD 5770"))  /* Radeon EG JUNIPER XT */
        {
            *vidmem = 512; /* note: HD5770 cards use 1024MB and HD5750 cards use 512MB or 1024MB  */
            return CARD_ATI_RADEON_HD5700;
        }

        /* Radeon R7xx HD4800 - highend */
        if (strstr(gl_renderer, "HD 4800")          /* Radeon RV7xx HD48xx generic renderer string */
                || strstr(gl_renderer, "HD 4830")   /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4850")   /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4870")   /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4890"))  /* Radeon RV790 */
        {
            *vidmem = 512; /* note: HD4890 cards use 1024MB */
            return CARD_ATI_RADEON_HD4800;
        }

        /* Radeon R740 HD4700 - midend */
        if (strstr(gl_renderer, "HD 4700")          /* Radeon RV770 */
                || strstr(gl_renderer, "HD 4770"))  /* Radeon RV740 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4700;
        }

        /* Radeon R730 HD4600 - midend */
        if (strstr(gl_renderer, "HD 4600")          /* Radeon RV730 */
                || strstr(gl_renderer, "HD 4650")   /* Radeon RV730 */
                || strstr(gl_renderer, "HD 4670"))  /* Radeon RV730 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4600;
        }

        /* Radeon R710 HD4500/HD4350 - lowend */
        if (strstr(gl_renderer, "HD 4350")          /* Radeon RV710 */
                || strstr(gl_renderer, "HD 4550"))  /* Radeon RV710 */
        {
            *vidmem = 256;
            return CARD_ATI_RADEON_HD4350;
        }

        /* Radeon R6xx HD2900/HD3800 - highend */
        if (strstr(gl_renderer, "HD 2900")
                || strstr(gl_renderer, "HD 3870")
                || strstr(gl_renderer, "HD 3850"))
        {
            *vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
            return CARD_ATI_RADEON_HD2900;
        }

        /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
        if (strstr(gl_renderer, "HD 2600")
                || strstr(gl_renderer, "HD 3830")
                || strstr(gl_renderer, "HD 3690")
                || strstr(gl_renderer, "HD 3650"))
        {
            *vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
            return CARD_ATI_RADEON_HD2600;
        }

        /* Radeon R6xx HD2350/HD2400/HD3400 - lowend
         * Note HD2300=DX9, HD2350=DX10 */
        if (strstr(gl_renderer, "HD 2350")
                || strstr(gl_renderer, "HD 2400")
                || strstr(gl_renderer, "HD 3470")
                || strstr(gl_renderer, "HD 3450")
                || strstr(gl_renderer, "HD 3430")
                || strstr(gl_renderer, "HD 3400"))
        {
            *vidmem = 256; /* HD2350/2400 use 256MB, HD34xx use 256-512MB */
            return CARD_ATI_RADEON_HD2350;
        }

        /* Radeon R6xx/R7xx integrated */
        if (strstr(gl_renderer, "HD 3100")
                || strstr(gl_renderer, "HD 3200")
                || strstr(gl_renderer, "HD 3300"))
        {
            *vidmem = 128; /* 128MB */
            return CARD_ATI_RADEON_HD3200;
        }

        /* Default for when no GPU has been found */
        *vidmem = 128; /* 128MB */
        return CARD_ATI_RADEON_HD3200;
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        /* Radeon R5xx */
        if (strstr(gl_renderer, "X1600")
                || strstr(gl_renderer, "X1650")
                || strstr(gl_renderer, "X1800")
                || strstr(gl_renderer, "X1900")
                || strstr(gl_renderer, "X1950"))
        {
            *vidmem = 128; /* X1600 uses 128-256MB, >=X1800 uses 256MB */
            return CARD_ATI_RADEON_X1600;
        }

        /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300/X2500/HD2300 (lowend R5xx)
         * Note X2300/X2500/HD2300 are R5xx GPUs with a 2xxx naming but they are still DX9-only */
        if (strstr(gl_renderer, "X700")
                || strstr(gl_renderer, "X800")
                || strstr(gl_renderer, "X850")
                || strstr(gl_renderer, "X1300")
                || strstr(gl_renderer, "X1400")
                || strstr(gl_renderer, "X1450")
                || strstr(gl_renderer, "X1550")
                || strstr(gl_renderer, "X2300")
                || strstr(gl_renderer, "X2500")
                || strstr(gl_renderer, "HD 2300")
                )
        {
            *vidmem = 128; /* x700/x8*0 use 128-256MB, >=x1300 128-512MB */
            return CARD_ATI_RADEON_X700;
        }

        /* Radeon Xpress Series - onboard, DX9b, Shader 2.0, 300-400MHz */
        if (strstr(gl_renderer, "Radeon Xpress"))
        {
            *vidmem = 64; /* Shared RAM, BIOS configurable, 64-256M */
            return CARD_ATI_RADEON_XPRESS_200M;
        }

        /* Radeon R3xx */
        *vidmem = 64; /* Radeon 9500 uses 64MB, higher models use up to 256MB */
        return CARD_ATI_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        *vidmem = 64; /* 8500/9000 cards use mostly 64MB, though there are 32MB and 128MB models */
        return CARD_ATI_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */
    }

    if (WINE_D3D7_CAPABLE(gl_info))
    {
        *vidmem = 32; /* There are models with up to 64MB */
        return CARD_ATI_RADEON_7200; /* Radeon 7000/7100/7200/7500 */
    }

    *vidmem = 16; /* There are 16-32MB models */
    return CARD_ATI_RAGE_128PRO;

}

static enum wined3d_pci_device select_card_intel_binary(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
    if (strstr(gl_renderer, "X3100"))
    {
        /* MacOS calls the card GMA X3100, Google findings also suggest the name GM965 */
        *vidmem = 128;
        return CARD_INTEL_X3100;
    }

    if (strstr(gl_renderer, "GMA 950") || strstr(gl_renderer, "945GM"))
    {
        /* MacOS calls the card GMA 950, but everywhere else the PCI ID is named 945GM */
        *vidmem = 64;
        return CARD_INTEL_I945GM;
    }

    if (strstr(gl_renderer, "915GM")) return CARD_INTEL_I915GM;
    if (strstr(gl_renderer, "915G")) return CARD_INTEL_I915G;
    if (strstr(gl_renderer, "865G")) return CARD_INTEL_I865G;
    if (strstr(gl_renderer, "855G")) return CARD_INTEL_I855G;
    if (strstr(gl_renderer, "830G")) return CARD_INTEL_I830G;
    return CARD_INTEL_I915G;

}

static enum wined3d_pci_device select_card_ati_mesa(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
    /* See http://developer.amd.com/drivers/pc_vendor_id/Pages/default.aspx
     *
     * Beware: renderer string do not match exact card model,
     * eg HD 4800 is returned for multiple cards, even for RV790 based ones. */
    if (strstr(gl_renderer, "Gallium"))
    {
        /* Radeon R7xx HD4800 - highend */
        if (strstr(gl_renderer, "R700")          /* Radeon R7xx HD48xx generic renderer string */
                || strstr(gl_renderer, "RV770")  /* Radeon RV770 */
                || strstr(gl_renderer, "RV790"))  /* Radeon RV790 */
        {
            *vidmem = 512; /* note: HD4890 cards use 1024MB */
            return CARD_ATI_RADEON_HD4800;
        }

        /* Radeon R740 HD4700 - midend */
        if (strstr(gl_renderer, "RV740"))          /* Radeon RV740 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4700;
        }

        /* Radeon R730 HD4600 - midend */
        if (strstr(gl_renderer, "RV730"))        /* Radeon RV730 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4600;
        }

        /* Radeon R710 HD4500/HD4350 - lowend */
        if (strstr(gl_renderer, "RV710"))          /* Radeon RV710 */
        {
            *vidmem = 256;
            return CARD_ATI_RADEON_HD4350;
        }

        /* Radeon R6xx HD2900/HD3800 - highend */
        if (strstr(gl_renderer, "R600")
                || strstr(gl_renderer, "RV670")
                || strstr(gl_renderer, "R680"))
        {
            *vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
            return CARD_ATI_RADEON_HD2900;
        }

        /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
        if (strstr(gl_renderer, "RV630")
                || strstr(gl_renderer, "RV635"))
        {
            *vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
            return CARD_ATI_RADEON_HD2600;
        }

        /* Radeon R6xx HD2350/HD2400/HD3400 - lowend */
        if (strstr(gl_renderer, "RV610")
                || strstr(gl_renderer, "RV620"))
        {
            *vidmem = 256; /* HD2350/2400 use 256MB, HD34xx use 256-512MB */
            return CARD_ATI_RADEON_HD2350;
        }

        /* Radeon R6xx/R7xx integrated */
        if (strstr(gl_renderer, "RS780")
                || strstr(gl_renderer, "RS880"))
        {
            *vidmem = 128; /* 128MB */
            return CARD_ATI_RADEON_HD3200;
        }

        /* Radeon R5xx */
        if (strstr(gl_renderer, "RV530")
                || strstr(gl_renderer, "RV535")
                || strstr(gl_renderer, "RV560")
                || strstr(gl_renderer, "R520")
                || strstr(gl_renderer, "RV570")
                || strstr(gl_renderer, "R580"))
        {
            *vidmem = 128; /* X1600 uses 128-256MB, >=X1800 uses 256MB */
            return CARD_ATI_RADEON_X1600;
        }

        /* Radeon R4xx + X1300/X1400/X1450/X1550/X2300 (lowend R5xx) */
        if (strstr(gl_renderer, "R410")
                || strstr(gl_renderer, "R420")
                || strstr(gl_renderer, "R423")
                || strstr(gl_renderer, "R430")
                || strstr(gl_renderer, "R480")
                || strstr(gl_renderer, "R481")
                || strstr(gl_renderer, "RV410")
                || strstr(gl_renderer, "RV515")
                || strstr(gl_renderer, "RV516"))
        {
            *vidmem = 128; /* x700/x8*0 use 128-256MB, >=x1300 128-512MB */
            return CARD_ATI_RADEON_X700;
        }

        /* Radeon Xpress Series - onboard, DX9b, Shader 2.0, 300-400MHz */
        if (strstr(gl_renderer, "RS400")
                || strstr(gl_renderer, "RS480")
                || strstr(gl_renderer, "RS482")
                || strstr(gl_renderer, "RS485")
                || strstr(gl_renderer, "RS600")
                || strstr(gl_renderer, "RS690")
                || strstr(gl_renderer, "RS740"))
        {
            *vidmem = 64; /* Shared RAM, BIOS configurable, 64-256M */
            return CARD_ATI_RADEON_XPRESS_200M;
        }

        /* Radeon R3xx */
        if (strstr(gl_renderer, "R300")
                || strstr(gl_renderer, "RV350")
                || strstr(gl_renderer, "RV351")
                || strstr(gl_renderer, "RV360")
                || strstr(gl_renderer, "RV370")
                || strstr(gl_renderer, "R350")
                || strstr(gl_renderer, "R360"))
        {
            *vidmem = 64; /* Radeon 9500 uses 64MB, higher models use up to 256MB */
            return CARD_ATI_RADEON_9500; /* Radeon 9500/9550/9600/9700/9800/X300/X550/X600 */
        }
    }

    if (WINE_D3D9_CAPABLE(gl_info))
    {
        /* Radeon R7xx HD4800 - highend */
        if (strstr(gl_renderer, "(R700")          /* Radeon R7xx HD48xx generic renderer string */
                || strstr(gl_renderer, "(RV770")  /* Radeon RV770 */
                || strstr(gl_renderer, "(RV790"))  /* Radeon RV790 */
        {
            *vidmem = 512; /* note: HD4890 cards use 1024MB */
            return CARD_ATI_RADEON_HD4800;
        }

        /* Radeon R740 HD4700 - midend */
        if (strstr(gl_renderer, "(RV740"))          /* Radeon RV740 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4700;
        }

        /* Radeon R730 HD4600 - midend */
        if (strstr(gl_renderer, "(RV730"))        /* Radeon RV730 */
        {
            *vidmem = 512;
            return CARD_ATI_RADEON_HD4600;
        }

        /* Radeon R710 HD4500/HD4350 - lowend */
        if (strstr(gl_renderer, "(RV710"))          /* Radeon RV710 */
        {
            *vidmem = 256;
            return CARD_ATI_RADEON_HD4350;
        }

        /* Radeon R6xx HD2900/HD3800 - highend */
        if (strstr(gl_renderer, "(R600")
                || strstr(gl_renderer, "(RV670")
                || strstr(gl_renderer, "(R680"))
        {
            *vidmem = 512; /* HD2900/HD3800 uses 256-1024MB */
            return CARD_ATI_RADEON_HD2900;
        }

        /* Radeon R6xx HD2600/HD3600 - midend; HD3830 is China-only midend */
        if (strstr(gl_renderer, "(RV630")
                || strstr(gl_renderer, "(RV635"))
        {
            *vidmem = 256; /* HD2600/HD3600 uses 256-512MB */
            return CARD_ATI_RADEON_HD2600;
        }

        /* Radeon R6xx HD2300/HD2400/HD3400 - lowend */
        if (strstr(gl_renderer, "(RV610")
                || strstr(gl_renderer, "(RV620"))
        {
            *vidmem = 256; /* HD2350/2400 use 256MB, HD34xx use 256-512MB */
            return CARD_ATI_RADEON_HD2350;
        }

        /* Radeon R6xx/R7xx integrated */
        if (strstr(gl_renderer, "(RS780")
                || strstr(gl_renderer, "(RS880"))
        {
            *vidmem = 128; /* 128MB */
            return CARD_ATI_RADEON_HD3200;
        }
    }

    if (WINE_D3D8_CAPABLE(gl_info))
    {
        *vidmem = 64; /* 8500/9000 cards use mostly 64MB, though there are 32MB and 128MB models */
        return CARD_ATI_RADEON_8500; /* Radeon 8500/9000/9100/9200/9300 */
    }

    if (WINE_D3D7_CAPABLE(gl_info))
    {
        *vidmem = 32; /* There are models with up to 64MB */
        return CARD_ATI_RADEON_7200; /* Radeon 7000/7100/7200/7500 */
    }

    *vidmem = 16; /* There are 16-32MB models */
    return CARD_ATI_RAGE_128PRO;

}

static enum wined3d_pci_device select_card_nvidia_mesa(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
    FIXME_(d3d_caps)("Card selection not handled for Mesa Nouveau driver\n");
#ifndef VBOX_WITH_WDDM
    if (WINE_D3D9_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCEFX_5600;
#else
    /* tmp work around to disable quirk_no_np2 quirk for mesa drivers */
    if (WINE_D3D9_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE_6200;
#endif
    if (WINE_D3D8_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE3;
    if (WINE_D3D7_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE;
    if (WINE_D3D6_CAPABLE(gl_info)) return CARD_NVIDIA_RIVA_TNT;
    return CARD_NVIDIA_RIVA_128;
}

static enum wined3d_pci_device select_card_intel_cmn(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
    if (strstr(gl_renderer, "HD Graphics")
            || strstr(gl_renderer, "Sandybridge"))
        return CARD_INTEL_SBHD;
    FIXME_(d3d_caps)("Card selection not handled for Windows Intel driver\n");
    return CARD_INTEL_I915G;
}

static enum wined3d_pci_device select_card_intel_mesa(const struct wined3d_gl_info *gl_info,
        const char *gl_renderer, unsigned int *vidmem)
{
    return select_card_intel_cmn(gl_info, gl_renderer, vidmem);
}

struct vendor_card_selection
{
    enum wined3d_gl_vendor gl_vendor;
    enum wined3d_pci_vendor card_vendor;
    const char *description;        /* Description of the card selector i.e. Apple OS/X Intel */
    enum wined3d_pci_device (*select_card)(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
            unsigned int *vidmem );
};

static const struct vendor_card_selection vendor_card_select_table[] =
{
    {GL_VENDOR_NVIDIA, HW_VENDOR_NVIDIA,  "Nvidia binary driver",     select_card_nvidia_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_NVIDIA,  "Apple OSX NVidia binary driver",   select_card_nvidia_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_ATI,     "Apple OSX AMD/ATI binary driver",  select_card_ati_binary},
    {GL_VENDOR_APPLE,  HW_VENDOR_INTEL,   "Apple OSX Intel binary driver",    select_card_intel_binary},
    {GL_VENDOR_FGLRX,  HW_VENDOR_ATI,     "AMD/ATI binary driver",    select_card_ati_binary},
    {GL_VENDOR_MESA,   HW_VENDOR_ATI,     "Mesa AMD/ATI driver",      select_card_ati_mesa},
    {GL_VENDOR_MESA,   HW_VENDOR_NVIDIA,  "Mesa Nouveau driver",      select_card_nvidia_mesa},
    {GL_VENDOR_MESA,   HW_VENDOR_INTEL,   "Mesa Intel driver",        select_card_intel_mesa},
    {GL_VENDOR_INTEL,  HW_VENDOR_INTEL,   "Windows Intel binary driver",  select_card_intel_cmn}
};


static enum wined3d_pci_device wined3d_guess_card(const struct wined3d_gl_info *gl_info, const char *gl_renderer,
        enum wined3d_gl_vendor *gl_vendor, enum wined3d_pci_vendor *card_vendor, unsigned int *vidmem)
{
    /* Above is a list of Nvidia and ATI GPUs. Both vendors have dozens of
     * different GPUs with roughly the same features. In most cases GPUs from a
     * certain family differ in clockspeeds, the amount of video memory and the
     * number of shader pipelines.
     *
     * A Direct3D device object contains the PCI id (vendor + device) of the
     * videocard which is used for rendering. Various applications use this
     * information to get a rough estimation of the features of the card and
     * some might use it for enabling 3d effects only on certain types of
     * videocards. In some cases games might even use it to work around bugs
     * which happen on certain videocards/driver combinations. The problem is
     * that OpenGL only exposes a rendering string containing the name of the
     * videocard and not the PCI id.
     *
     * Various games depend on the PCI id, so somehow we need to provide one.
     * A simple option is to parse the renderer string and translate this to
     * the right PCI id. This is a lot of work because there are more than 200
     * GPUs just for Nvidia. Various cards share the same renderer string, so
     * the amount of code might be 'small' but there are quite a number of
     * exceptions which would make this a pain to maintain. Another way would
     * be to query the PCI id from the operating system (assuming this is the
     * videocard which is used for rendering which is not always the case).
     * This would work but it is not very portable. Second it would not work
     * well in, let's say, a remote X situation in which the amount of 3d
     * features which can be used is limited.
     *
     * As said most games only use the PCI id to get an indication of the
     * capabilities of the card. It doesn't really matter if the given id is
     * the correct one if we return the id of a card with similar 3d features.
     *
     * The code below checks the OpenGL capabilities of a videocard and matches
     * that to a certain level of Direct3D functionality. Once a card passes
     * the Direct3D9 check, we know that the card (in case of Nvidia) is at
     * least a GeforceFX. To give a better estimate we do a basic check on the
     * renderer string but if that won't pass we return a default card. This
     * way is better than maintaining a full card database as even without a
     * full database we can return a card with similar features. Second the
     * size of the database can be made quite small because when you know what
     * type of 3d functionality a card has, you know to which GPU family the
     * GPU must belong. Because of this you only have to check a small part of
     * the renderer string to distinguishes between different models from that
     * family.
     *
     * The code also selects a default amount of video memory which we will
     * use for an estimation of the amount of free texture memory. In case of
     * real D3D the amount of texture memory includes video memory and system
     * memory (to be specific AGP memory or in case of PCIE TurboCache /
     * HyperMemory). We don't know how much system memory can be addressed by
     * the system but we can make a reasonable estimation about the amount of
     * video memory. If the value is slightly wrong it doesn't matter as we
     * didn't include AGP-like memory which makes the amount of addressable
     * memory higher and second OpenGL isn't that critical it moves to system
     * memory behind our backs if really needed. Note that the amount of video
     * memory can be overruled using a registry setting. */

#ifndef VBOX
    int i;
#else
    size_t i;
#endif

    for (i = 0; i < (sizeof(vendor_card_select_table) / sizeof(*vendor_card_select_table)); ++i)
    {
        if ((vendor_card_select_table[i].gl_vendor != *gl_vendor)
            || (vendor_card_select_table[i].card_vendor != *card_vendor))
                continue;
        TRACE_(d3d_caps)("Applying card_selector \"%s\".\n", vendor_card_select_table[i].description);
        return vendor_card_select_table[i].select_card(gl_info, gl_renderer, vidmem);
    }

    FIXME_(d3d_caps)("No card selector available for GL vendor %d and card vendor %04x.\n",
                     *gl_vendor, *card_vendor);

    /* Default to generic Nvidia hardware based on the supported OpenGL extensions. The choice
     * for Nvidia was because the hardware and drivers they make are of good quality. This makes
     * them a good generic choice. */
    *card_vendor = HW_VENDOR_NVIDIA;
#ifndef VBOX_WITH_WDDM
    if (WINE_D3D9_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCEFX_5600;
#else
    /* tmp work around to disable quirk_no_np2 quirk for not-recognized drivers */
    if (WINE_D3D9_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE_6200;
#endif

    if (WINE_D3D8_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE3;
    if (WINE_D3D7_CAPABLE(gl_info)) return CARD_NVIDIA_GEFORCE;
    if (WINE_D3D6_CAPABLE(gl_info)) return CARD_NVIDIA_RIVA_TNT;
    return CARD_NVIDIA_RIVA_128;
}

#ifndef VBOX_WITH_VMSVGA
static const struct fragment_pipeline *select_fragment_implementation(struct wined3d_adapter *adapter)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if ((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL)
            && gl_info->supported[ARB_FRAGMENT_PROGRAM]) return &arbfp_fragment_pipeline;
    else if (ps_selected_mode == SHADER_ATI) return &atifs_fragment_pipeline;
    else if (gl_info->supported[NV_REGISTER_COMBINERS]
            && gl_info->supported[NV_TEXTURE_SHADER2]) return &nvts_fragment_pipeline;
    else if (gl_info->supported[NV_REGISTER_COMBINERS]) return &nvrc_fragment_pipeline;
    else return &ffp_fragment_pipeline;
}
#endif

static const shader_backend_t *select_shader_backend(struct wined3d_adapter *adapter)
{
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(&adapter->gl_info, &ps_selected_mode, &vs_selected_mode);
    if (vs_selected_mode == SHADER_GLSL || ps_selected_mode == SHADER_GLSL) return &glsl_shader_backend;
#ifndef VBOX_WITH_VMSVGA
    if (vs_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_ARB) return &arb_program_shader_backend;
#endif
    return &none_shader_backend;
}

#ifndef VBOX_WITH_VMSVGA
static const struct blit_shader *select_blit_implementation(struct wined3d_adapter *adapter)
{
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    int vs_selected_mode, ps_selected_mode;

    select_shader_mode(gl_info, &ps_selected_mode, &vs_selected_mode);
    if ((ps_selected_mode == SHADER_ARB || ps_selected_mode == SHADER_GLSL)
            && gl_info->supported[ARB_FRAGMENT_PROGRAM]) return &arbfp_blit;
    else return &ffp_blit;
}
#endif

#ifdef VBOX_WITH_VMSVGA
/** Checks if @a pszExtension is one of the extensions we're looking for and
 *  updates @a pGlInfo->supported accordingly. */
static void check_gl_extension(struct wined3d_gl_info *pGlInfo, const char *pszExtension)
{
    size_t i;
    TRACE_(d3d_caps)("- %s\n", debugstr_a(pszExtension));
    for (i = 0; i < RT_ELEMENTS(EXTENSION_MAP); i++)
        if (!strcmp(pszExtension, EXTENSION_MAP[i].extension_string))
        {
            TRACE_(d3d_caps)(" FOUND: %s support.\n", EXTENSION_MAP[i].extension_string);
            pGlInfo->supported[EXTENSION_MAP[i].extension] = TRUE;
            return;
        }
}
#endif

/* Context activation is done by the caller. */
BOOL IWineD3DImpl_FillGLCaps(struct wined3d_adapter *adapter, struct VBOXVMSVGASHADERIF *pVBoxShaderIf)
{
#ifndef VBOX_WITH_VMSVGA
    struct wined3d_driver_info *driver_info = &adapter->driver_info;
#endif
    struct wined3d_gl_info *gl_info = &adapter->gl_info;
#ifndef VBOX_WITH_VMSVGA
    const char *GL_Extensions    = NULL;
    const char *WGL_Extensions   = NULL;
#endif
    const char *gl_vendor_str, *gl_renderer_str, *gl_version_str;
#ifndef VBOX_WITH_VMSVGA
    struct fragment_caps fragment_caps;
#endif
    enum wined3d_gl_vendor gl_vendor;
    enum wined3d_pci_vendor card_vendor;
    enum wined3d_pci_device device;
    GLint       gl_max;
    GLfloat     gl_floatv[2];
    unsigned    i;
#ifndef VBOX_WITH_VMSVGA
    HDC         hdc;
#endif
    unsigned int vidmem=0;
    DWORD gl_version;
#ifndef VBOX_WITH_VMSVGA
    size_t len;
#endif

    TRACE_(d3d_caps)("(%p)\n", gl_info);

    ENTER_GL();

    VBOX_CHECK_GL_CALL(gl_renderer_str = (const char *)glGetString(GL_RENDERER));
    TRACE_(d3d_caps)("GL_RENDERER: %s.\n", debugstr_a(gl_renderer_str));
    if (!gl_renderer_str)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_RENDERER.\n");
        return FALSE;
    }

    VBOX_CHECK_GL_CALL(gl_vendor_str = (const char *)glGetString(GL_VENDOR));
    TRACE_(d3d_caps)("GL_VENDOR: %s.\n", debugstr_a(gl_vendor_str));
    if (!gl_vendor_str)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_VENDOR.\n");
        return FALSE;
    }

    /* Parse the GL_VERSION field into major and minor information */
    VBOX_CHECK_GL_CALL(gl_version_str = (const char *)glGetString(GL_VERSION));
    TRACE_(d3d_caps)("GL_VERSION: %s.\n", debugstr_a(gl_version_str));
    if (!gl_version_str)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_VERSION.\n");
        return FALSE;
    }
    gl_version = wined3d_parse_gl_version(gl_version_str);

    /*
     * Initialize openGL extension related variables
     *  with Default values
     */
    memset(gl_info->supported, 0, sizeof(gl_info->supported));
    gl_info->limits.blends = 1;
    gl_info->limits.buffers = 1;
    gl_info->limits.textures = 1;
    gl_info->limits.fragment_samplers = 1;
    gl_info->limits.vertex_samplers = 0;
    gl_info->limits.combined_samplers = gl_info->limits.fragment_samplers + gl_info->limits.vertex_samplers;
    gl_info->limits.sampler_stages = 1;
    gl_info->limits.glsl_vs_float_constants = 0;
    gl_info->limits.glsl_ps_float_constants = 0;
    gl_info->limits.arb_vs_float_constants = 0;
    gl_info->limits.arb_vs_native_constants = 0;
    gl_info->limits.arb_vs_instructions = 0;
    gl_info->limits.arb_vs_temps = 0;
    gl_info->limits.arb_ps_float_constants = 0;
    gl_info->limits.arb_ps_local_constants = 0;
    gl_info->limits.arb_ps_instructions = 0;
    gl_info->limits.arb_ps_temps = 0;

    /* Retrieve opengl defaults */
    VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max));
    gl_info->limits.clipplanes = min(WINED3DMAXUSERCLIPPLANES, gl_max);
    TRACE_(d3d_caps)("ClipPlanes support - num Planes=%d\n", gl_max);

#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    if (glGetError() != GL_NO_ERROR)
    {
        pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_LIGHTS, &gl_max));
        pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
    }
#else
    VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_LIGHTS, &gl_max));
#endif
    gl_info->limits.lights = gl_max;
    TRACE_(d3d_caps)("Lights support - max lights=%d\n", gl_max);

    VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max));
    gl_info->limits.texture_size = gl_max;
    TRACE_(d3d_caps)("Maximum texture size support - max texture size=%d\n", gl_max);

#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv);
    if (glGetError() != GL_NO_ERROR)
    {
        pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
        VBOX_CHECK_GL_CALL(glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv));
        if (glGetError() != GL_NO_ERROR)
            gl_floatv[0] = gl_floatv[1] = 1;
        pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
    }
#else
    VBOX_CHECK_GL_CALL(glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, gl_floatv));
#endif
    gl_info->limits.pointsize_min = gl_floatv[0];
    gl_info->limits.pointsize_max = gl_floatv[1];
    TRACE_(d3d_caps)("Maximum point size support - max point size=%f\n", gl_floatv[1]);

    /* Parse the gl supported features, in theory enabling parts of our code appropriately. */
#ifndef VBOX_WITH_VMSVGA
    GL_Extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (!GL_Extensions)
    {
        LEAVE_GL();
        ERR_(d3d_caps)("Received a NULL GL_EXTENSIONS.\n");
        return FALSE;
    }

    LEAVE_GL();

    TRACE_(d3d_caps)("GL_Extensions reported:\n");
#endif

    gl_info->supported[WINED3D_GL_EXT_NONE] = TRUE;

    gl_info->supported[VBOX_SHARED_CONTEXTS] = TRUE;

#ifdef VBOX_WITH_VMSVGA
    {
        void *pvEnumCtx = NULL;
        char  szCurExt[256];
        while (pVBoxShaderIf->pfnGetNextExtension(pVBoxShaderIf, &pvEnumCtx, szCurExt, sizeof(szCurExt), false /*fOtherProfile*/))
            check_gl_extension(gl_info, szCurExt);

        /* The cheap way out. */
        pvEnumCtx = NULL;
        while (pVBoxShaderIf->pfnGetNextExtension(pVBoxShaderIf, &pvEnumCtx, szCurExt, sizeof(szCurExt), true /*fOtherProfile*/))
            check_gl_extension(gl_info, szCurExt);
    }
#else /* !VBOX_WITH_VMSVGA */
    while (*GL_Extensions)
    {
        const char *start;
        char current_ext[256];

        while (isspace(*GL_Extensions)) ++GL_Extensions;
        start = GL_Extensions;
        while (!isspace(*GL_Extensions) && *GL_Extensions) ++GL_Extensions;

        len = GL_Extensions - start;
        if (!len || len >= sizeof(current_ext)) continue;

        memcpy(current_ext, start, len);
        current_ext[len] = '\0';
        TRACE_(d3d_caps)("- %s\n", debugstr_a(current_ext));

        for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i)
        {
            if (!strcmp(current_ext, EXTENSION_MAP[i].extension_string))
            {
                TRACE_(d3d_caps)(" FOUND: %s support.\n", EXTENSION_MAP[i].extension_string);
                gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
                break;
            }
        }
    }
#endif /* !VBOX_WITH_VMSVGA */

#ifdef VBOX_WITH_VMSVGA
# ifdef RT_OS_WINDOWS
#  define OGLGETPROCADDRESS      wglGetProcAddress
# elif RT_OS_DARWIN
#  define OGLGETPROCADDRESS(x)   MyNSGLGetProcAddress(x)
# else
extern void (*glXGetProcAddress(const GLubyte *procname))( void );
#  define OGLGETPROCADDRESS(x)   glXGetProcAddress((const GLubyte *)x)
# endif
#endif

    /* Now work out what GL support this card really has */
#define USE_GL_FUNC(type, pfn, ext, replace) \
{ \
    DWORD ver = ver_for_ext(ext); \
    if (gl_info->supported[ext]) gl_info->pfn = (type)OGLGETPROCADDRESS(#pfn); \
    else if (ver && ver <= gl_version) gl_info->pfn = (type)OGLGETPROCADDRESS(#replace); \
    else gl_info->pfn = NULL; \
}
    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

#ifndef VBOX_WITH_VMSVGA
#define USE_GL_FUNC(type, pfn, ext, replace) gl_info->pfn = (type)OGLGETPROCADDRESS(#pfn);
    WGL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC
#endif

    ENTER_GL();

    /* Now mark all the extensions supported which are included in the opengl core version. Do this *after*
     * loading the functions, otherwise the code above will load the extension entry points instead of the
     * core functions, which may not work. */
    for (i = 0; i < (sizeof(EXTENSION_MAP) / sizeof(*EXTENSION_MAP)); ++i)
    {
        if (!gl_info->supported[EXTENSION_MAP[i].extension]
                && EXTENSION_MAP[i].version <= gl_version && EXTENSION_MAP[i].version)
        {
            TRACE_(d3d_caps)(" GL CORE: %s support.\n", EXTENSION_MAP[i].extension_string);
            gl_info->supported[EXTENSION_MAP[i].extension] = TRUE;
        }
    }

    if (gl_info->supported[APPLE_FENCE])
    {
        /* GL_NV_fence and GL_APPLE_fence provide the same functionality basically.
         * The apple extension interacts with some other apple exts. Disable the NV
         * extension if the apple one is support to prevent confusion in other parts
         * of the code. */
        gl_info->supported[NV_FENCE] = FALSE;
    }
    if (gl_info->supported[APPLE_FLOAT_PIXELS])
    {
        /* GL_APPLE_float_pixels == GL_ARB_texture_float + GL_ARB_half_float_pixel
         *
         * The enums are the same:
         * GL_RGBA16F_ARB     = GL_RGBA_FLOAT16_APPLE = 0x881A
         * GL_RGB16F_ARB      = GL_RGB_FLOAT16_APPLE  = 0x881B
         * GL_RGBA32F_ARB     = GL_RGBA_FLOAT32_APPLE = 0x8814
         * GL_RGB32F_ARB      = GL_RGB_FLOAT32_APPLE  = 0x8815
         * GL_HALF_FLOAT_ARB  = GL_HALF_APPLE         = 0x140B
         */
        if (!gl_info->supported[ARB_TEXTURE_FLOAT])
        {
            TRACE_(d3d_caps)(" IMPLIED: GL_ARB_texture_float support(from GL_APPLE_float_pixels.\n");
            gl_info->supported[ARB_TEXTURE_FLOAT] = TRUE;
        }
        if (!gl_info->supported[ARB_HALF_FLOAT_PIXEL])
        {
            TRACE_(d3d_caps)(" IMPLIED: GL_ARB_half_float_pixel support(from GL_APPLE_float_pixels.\n");
            gl_info->supported[ARB_HALF_FLOAT_PIXEL] = TRUE;
        }
    }
    if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
    {
        /* GL_ARB_map_buffer_range and GL_APPLE_flush_buffer_range provide the same
         * functionality. Prefer the ARB extension */
        gl_info->supported[APPLE_FLUSH_BUFFER_RANGE] = FALSE;
    }
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        TRACE_(d3d_caps)(" IMPLIED: NVIDIA (NV) Texture Gen Reflection support.\n");
        gl_info->supported[NV_TEXGEN_REFLECTION] = TRUE;
    }
    if (!gl_info->supported[ARB_DEPTH_CLAMP] && gl_info->supported[NV_DEPTH_CLAMP])
    {
        TRACE_(d3d_caps)(" IMPLIED: ARB_depth_clamp support (by NV_depth_clamp).\n");
        gl_info->supported[ARB_DEPTH_CLAMP] = TRUE;
    }
    if (!gl_info->supported[ARB_VERTEX_ARRAY_BGRA] && gl_info->supported[EXT_VERTEX_ARRAY_BGRA])
    {
        TRACE_(d3d_caps)(" IMPLIED: ARB_vertex_array_bgra support (by EXT_vertex_array_bgra).\n");
        gl_info->supported[ARB_VERTEX_ARRAY_BGRA] = TRUE;
    }
    if (gl_info->supported[NV_TEXTURE_SHADER2])
    {
        if (gl_info->supported[NV_REGISTER_COMBINERS])
        {
            /* Also disable ATI_FRAGMENT_SHADER if register combiners and texture_shader2
             * are supported. The nv extensions provide the same functionality as the
             * ATI one, and a bit more(signed pixelformats). */
            gl_info->supported[ATI_FRAGMENT_SHADER] = FALSE;
        }
    }

    if (gl_info->supported[NV_REGISTER_COMBINERS])
    {
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &gl_max));
        gl_info->limits.general_combiners = gl_max;
        TRACE_(d3d_caps)("Max general combiners: %d.\n", gl_max);
    }
    if (gl_info->supported[ARB_DRAW_BUFFERS])
    {
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &gl_max));
        gl_info->limits.buffers = gl_max;
        TRACE_(d3d_caps)("Max draw buffers: %u.\n", gl_max);
    }
    if (gl_info->supported[ARB_MULTITEXTURE])
    {
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
        if (glGetError() != GL_NO_ERROR)
            VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gl_max));
#else
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max));
#endif
        gl_info->limits.textures = min(MAX_TEXTURES, gl_max);
        TRACE_(d3d_caps)("Max textures: %d.\n", gl_info->limits.textures);

        if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
        {
            GLint tmp;
            VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &tmp));
            gl_info->limits.fragment_samplers = min(MAX_FRAGMENT_SAMPLERS, tmp);
        }
        else
        {
            gl_info->limits.fragment_samplers = max(gl_info->limits.fragment_samplers, (UINT)gl_max);
        }
        TRACE_(d3d_caps)("Max fragment samplers: %d.\n", gl_info->limits.fragment_samplers);

        if (gl_info->supported[ARB_VERTEX_SHADER])
        {
            GLint tmp;
            VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &tmp));
            gl_info->limits.vertex_samplers = tmp;
            VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &tmp));
            gl_info->limits.combined_samplers = tmp;

            /* Loading GLSL sampler uniforms is much simpler if we can assume that the sampler setup
             * is known at shader link time. In a vertex shader + pixel shader combination this isn't
             * an issue because then the sampler setup only depends on the two shaders. If a pixel
             * shader is used with fixed function vertex processing we're fine too because fixed function
             * vertex processing doesn't use any samplers. If fixed function fragment processing is
             * used we have to make sure that all vertex sampler setups are valid together with all
             * possible fixed function fragment processing setups. This is true if vsamplers + MAX_TEXTURES
             * <= max_samplers. This is true on all d3d9 cards that support vtf(gf 6 and gf7 cards).
             * dx9 radeon cards do not support vertex texture fetch. DX10 cards have 128 samplers, and
             * dx9 is limited to 8 fixed function texture stages and 4 vertex samplers. DX10 does not have
             * a fixed function pipeline anymore.
             *
             * So this is just a check to check that our assumption holds true. If not, write a warning
             * and reduce the number of vertex samplers or probably disable vertex texture fetch. */
            if (gl_info->limits.vertex_samplers && gl_info->limits.combined_samplers < 12
                    && MAX_TEXTURES + gl_info->limits.vertex_samplers > gl_info->limits.combined_samplers)
            {
                FIXME("OpenGL implementation supports %u vertex samplers and %u total samplers.\n",
                        gl_info->limits.vertex_samplers, gl_info->limits.combined_samplers);
                FIXME("Expected vertex samplers + MAX_TEXTURES(=8) > combined_samplers.\n");
                if (gl_info->limits.combined_samplers > MAX_TEXTURES)
                    gl_info->limits.vertex_samplers = gl_info->limits.combined_samplers - MAX_TEXTURES;
                else
                    gl_info->limits.vertex_samplers = 0;
            }
        }
        else
        {
            gl_info->limits.combined_samplers = gl_info->limits.fragment_samplers;
        }
        TRACE_(d3d_caps)("Max vertex samplers: %u.\n", gl_info->limits.vertex_samplers);
        TRACE_(d3d_caps)("Max combined samplers: %u.\n", gl_info->limits.combined_samplers);
    }
    if (gl_info->supported[ARB_VERTEX_BLEND])
    {
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
        if (glGetError() != GL_NO_ERROR)
        {
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
            VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max));
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
        }
#else
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max));
#endif
        gl_info->limits.blends = gl_max;
        TRACE_(d3d_caps)("Max blends: %u.\n", gl_info->limits.blends);
    }
    if (gl_info->supported[EXT_TEXTURE3D])
    {
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_EXT, &gl_max));
        gl_info->limits.texture3d_size = gl_max;
        TRACE_(d3d_caps)("Max texture3D size: %d.\n", gl_info->limits.texture3d_size);
    }
    if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
    {
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max));
        gl_info->limits.anisotropy = gl_max;
        TRACE_(d3d_caps)("Max anisotropy: %d.\n", gl_info->limits.anisotropy);
    }
    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
        if (glGetError() != GL_NO_ERROR)
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
#endif
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max)));
        gl_info->limits.arb_ps_float_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM float constants: %d.\n", gl_info->limits.arb_ps_float_constants);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max)));
        gl_info->limits.arb_ps_native_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native float constants: %d.\n",
                gl_info->limits.arb_ps_native_constants);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max)));
        gl_info->limits.arb_ps_temps = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native temporaries: %d.\n", gl_info->limits.arb_ps_temps);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max)));
        gl_info->limits.arb_ps_instructions = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM native instructions: %d.\n", gl_info->limits.arb_ps_instructions);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &gl_max)));
        gl_info->limits.arb_ps_local_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_PROGRAM local parameters: %d.\n", gl_info->limits.arb_ps_instructions);
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
#endif
    }
    if (gl_info->supported[ARB_VERTEX_PROGRAM])
    {
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max));
        if (glGetError() != GL_NO_ERROR)
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
#endif
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &gl_max)));
        gl_info->limits.arb_vs_float_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM float constants: %d.\n", gl_info->limits.arb_vs_float_constants);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &gl_max)));
        gl_info->limits.arb_vs_native_constants = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native float constants: %d.\n",
                gl_info->limits.arb_vs_native_constants);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &gl_max)));
        gl_info->limits.arb_vs_temps = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native temporaries: %d.\n", gl_info->limits.arb_vs_temps);
        VBOX_CHECK_GL_CALL(GL_EXTCALL(glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &gl_max)));
        gl_info->limits.arb_vs_instructions = gl_max;
        TRACE_(d3d_caps)("Max ARB_VERTEX_PROGRAM native instructions: %d.\n", gl_info->limits.arb_vs_instructions);
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
#endif
#ifndef VBOX_WITH_VMSVGA
        if (test_arb_vs_offset_limit(gl_info)) gl_info->quirks |= WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT;
#endif
    }
    if (gl_info->supported[ARB_VERTEX_SHADER])
    {
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &gl_max));
        gl_info->limits.glsl_vs_float_constants = gl_max / 4;
#ifdef VBOX_WITH_WDDM
        /* AFAICT the " / 4" here comes from that we're going to use the glsl_vs/ps_float_constants to create vec4 arrays,
         * thus each array element has 4 components, so the actual number of vec4 arrays is GL_MAX_VERTEX/FRAGMENT_UNIFORM_COMPONENTS_ARB / 4
         * win8 Aero won't properly work with this constant < 256 in any way,
         * while Intel drivers I've encountered this problem with supports vec4 arrays of size >  GL_MAX_VERTEX/FRAGMENT_UNIFORM_COMPONENTS_ARB / 4
         * so use it here.
         * @todo: add logging
         * @todo: perhaps should be movet to quirks?
         * */
        if (gl_info->limits.glsl_vs_float_constants < 256 && gl_max >= 256)
        {
            DWORD dwVersion = GetVersion();
            DWORD dwMajor = (DWORD)(LOBYTE(LOWORD(dwVersion)));
            DWORD dwMinor = (DWORD)(HIBYTE(LOWORD(dwVersion)));
            /* tmp workaround Win8 Aero requirement for 256 */
            if (dwMajor > 6 || dwMinor > 1)
            {
                gl_info->limits.glsl_vs_float_constants = 256;
            }
        }
#endif
        TRACE_(d3d_caps)("Max ARB_VERTEX_SHADER float constants: %u.\n", gl_info->limits.glsl_vs_float_constants);
    }
    if (gl_info->supported[ARB_FRAGMENT_SHADER])
    {
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &gl_max));
        gl_info->limits.glsl_ps_float_constants = gl_max / 4;
#ifdef VBOX_WITH_WDDM
        /* AFAICT the " / 4" here comes from that we're going to use the glsl_vs/ps_float_constants to create vec4 arrays,
         * thus each array element has 4 components, so the actual number of vec4 arrays is GL_MAX_VERTEX/FRAGMENT_UNIFORM_COMPONENTS_ARB / 4
         * win8 Aero won't properly work with this constant < 256 in any way,
         * while Intel drivers I've encountered this problem with supports vec4 arrays of size >  GL_MAX_VERTEX/FRAGMENT_UNIFORM_COMPONENTS_ARB / 4
         * so use it here.
         * @todo: add logging
         * @todo: perhaps should be movet to quirks?
         * */
        if (gl_info->limits.glsl_ps_float_constants < 256 && gl_max >= 256)
        {
            DWORD dwVersion = GetVersion();
            DWORD dwMajor = (DWORD)(LOBYTE(LOWORD(dwVersion)));
            DWORD dwMinor = (DWORD)(HIBYTE(LOWORD(dwVersion)));
            /* tmp workaround Win8 Aero requirement for 256 */
            if (dwMajor > 6 || dwMinor > 1)
            {
                gl_info->limits.glsl_ps_float_constants = 256;
            }
        }
#endif
        TRACE_(d3d_caps)("Max ARB_FRAGMENT_SHADER float constants: %u.\n", gl_info->limits.glsl_ps_float_constants);
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max);
        if (glGetError() != GL_NO_ERROR)
        {
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
            VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max));
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
        }
#else
        VBOX_CHECK_GL_CALL(glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &gl_max));
#endif
        gl_info->limits.glsl_varyings = gl_max;
        TRACE_(d3d_caps)("Max GLSL varyings: %u (%u 4 component varyings).\n", gl_max, gl_max / 4);
    }
    if (gl_info->supported[ARB_SHADING_LANGUAGE_100])
    {
        const char *str = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION_ARB);
        unsigned int major, minor;

        TRACE_(d3d_caps)("GLSL version string: %s.\n", debugstr_a(str));

        /* The format of the GLSL version string is "major.minor[.release] [vendor info]". */
        sscanf(str, "%u.%u", &major, &minor);
        gl_info->glsl_version = MAKEDWORD_VERSION(major, minor);
    }
    if (gl_info->supported[NV_LIGHT_MAX_EXPONENT])
    {
#ifdef VBOX_VMSVGA3D_DUAL_OPENGL_PROFILE
        glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->limits.shininess);
        if (glGetError() != GL_NO_ERROR)
        {
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, true /*fOtherProfile*/);
            VBOX_CHECK_GL_CALL(glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->limits.shininess));
            pVBoxShaderIf->pfnSwitchInitProfile(pVBoxShaderIf, false /*fOtherProfile*/);
        }
#else
        VBOX_CHECK_GL_CALL(glGetFloatv(GL_MAX_SHININESS_NV, &gl_info->limits.shininess));
#endif
    }
    else
    {
        gl_info->limits.shininess = 128.0f;
    }
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        /* If we have full NP2 texture support, disable
         * GL_ARB_texture_rectangle because we will never use it.
         * This saves a few redundant glDisable calls. */
        gl_info->supported[ARB_TEXTURE_RECTANGLE] = FALSE;
    }
    if (gl_info->supported[ATI_FRAGMENT_SHADER])
    {
        /* Disable NV_register_combiners and fragment shader if this is supported.
         * generally the NV extensions are preferred over the ATI ones, and this
         * extension is disabled if register_combiners and texture_shader2 are both
         * supported. So we reach this place only if we have incomplete NV dxlevel 8
         * fragment processing support. */
        gl_info->supported[NV_REGISTER_COMBINERS] = FALSE;
        gl_info->supported[NV_REGISTER_COMBINERS2] = FALSE;
        gl_info->supported[NV_TEXTURE_SHADER] = FALSE;
        gl_info->supported[NV_TEXTURE_SHADER2] = FALSE;
    }
    if (gl_info->supported[NV_HALF_FLOAT])
    {
        /* GL_ARB_half_float_vertex is a subset of GL_NV_half_float. */
        gl_info->supported[ARB_HALF_FLOAT_VERTEX] = TRUE;
    }
    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        gl_info->limits.point_sprite_units = gl_info->limits.textures;
    }
    else
    {
        gl_info->limits.point_sprite_units = 0;
    }
#ifndef VBOX_WITH_VMSVGA
    checkGLcall("extension detection");
#endif
    LEAVE_GL();

#ifndef VBOX_WITH_VMSVGA
    adapter->fragment_pipe = select_fragment_implementation(adapter);
#endif
    adapter->shader_backend = select_shader_backend(adapter);
#ifndef VBOX_WITH_VMSVGA
    adapter->blitter = select_blit_implementation(adapter);

    adapter->fragment_pipe->get_caps(gl_info, &fragment_caps);
    gl_info->limits.texture_stages = fragment_caps.MaxTextureBlendStages;
    TRACE_(d3d_caps)("Max texture stages: %u.\n", gl_info->limits.texture_stages);

    /* In some cases the number of texture stages can be larger than the number
     * of samplers. The GF4 for example can use only 2 samplers (no fragment
     * shaders), but 8 texture stages (register combiners). */
    gl_info->limits.sampler_stages = max(gl_info->limits.fragment_samplers, gl_info->limits.texture_stages);
#endif

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
    {
        gl_info->fbo_ops.glIsRenderbuffer = gl_info->glIsRenderbuffer;
        gl_info->fbo_ops.glBindRenderbuffer = gl_info->glBindRenderbuffer;
        gl_info->fbo_ops.glDeleteRenderbuffers = gl_info->glDeleteRenderbuffers;
        gl_info->fbo_ops.glGenRenderbuffers = gl_info->glGenRenderbuffers;
        gl_info->fbo_ops.glRenderbufferStorage = gl_info->glRenderbufferStorage;
        gl_info->fbo_ops.glRenderbufferStorageMultisample = gl_info->glRenderbufferStorageMultisample;
        gl_info->fbo_ops.glGetRenderbufferParameteriv = gl_info->glGetRenderbufferParameteriv;
        gl_info->fbo_ops.glIsFramebuffer = gl_info->glIsFramebuffer;
        gl_info->fbo_ops.glBindFramebuffer = gl_info->glBindFramebuffer;
        gl_info->fbo_ops.glDeleteFramebuffers = gl_info->glDeleteFramebuffers;
        gl_info->fbo_ops.glGenFramebuffers = gl_info->glGenFramebuffers;
        gl_info->fbo_ops.glCheckFramebufferStatus = gl_info->glCheckFramebufferStatus;
        gl_info->fbo_ops.glFramebufferTexture1D = gl_info->glFramebufferTexture1D;
        gl_info->fbo_ops.glFramebufferTexture2D = gl_info->glFramebufferTexture2D;
        gl_info->fbo_ops.glFramebufferTexture3D = gl_info->glFramebufferTexture3D;
        gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->glFramebufferRenderbuffer;
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv = gl_info->glGetFramebufferAttachmentParameteriv;
        gl_info->fbo_ops.glBlitFramebuffer = gl_info->glBlitFramebuffer;
        gl_info->fbo_ops.glGenerateMipmap = gl_info->glGenerateMipmap;
    }
    else
    {
        if (gl_info->supported[EXT_FRAMEBUFFER_OBJECT])
        {
            gl_info->fbo_ops.glIsRenderbuffer = gl_info->glIsRenderbufferEXT;
            gl_info->fbo_ops.glBindRenderbuffer = gl_info->glBindRenderbufferEXT;
            gl_info->fbo_ops.glDeleteRenderbuffers = gl_info->glDeleteRenderbuffersEXT;
            gl_info->fbo_ops.glGenRenderbuffers = gl_info->glGenRenderbuffersEXT;
            gl_info->fbo_ops.glRenderbufferStorage = gl_info->glRenderbufferStorageEXT;
            gl_info->fbo_ops.glGetRenderbufferParameteriv = gl_info->glGetRenderbufferParameterivEXT;
            gl_info->fbo_ops.glIsFramebuffer = gl_info->glIsFramebufferEXT;
            gl_info->fbo_ops.glBindFramebuffer = gl_info->glBindFramebufferEXT;
            gl_info->fbo_ops.glDeleteFramebuffers = gl_info->glDeleteFramebuffersEXT;
            gl_info->fbo_ops.glGenFramebuffers = gl_info->glGenFramebuffersEXT;
            gl_info->fbo_ops.glCheckFramebufferStatus = gl_info->glCheckFramebufferStatusEXT;
            gl_info->fbo_ops.glFramebufferTexture1D = gl_info->glFramebufferTexture1DEXT;
            gl_info->fbo_ops.glFramebufferTexture2D = gl_info->glFramebufferTexture2DEXT;
            gl_info->fbo_ops.glFramebufferTexture3D = gl_info->glFramebufferTexture3DEXT;
            gl_info->fbo_ops.glFramebufferRenderbuffer = gl_info->glFramebufferRenderbufferEXT;
            gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv = gl_info->glGetFramebufferAttachmentParameterivEXT;
            gl_info->fbo_ops.glGenerateMipmap = gl_info->glGenerateMipmapEXT;
        }
#ifndef VBOX_WITH_VMSVGA
        else if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            WARN_(d3d_caps)("Framebuffer objects not supported, falling back to backbuffer offscreen rendering mode.\n");
            wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
        }
#endif
        if (gl_info->supported[EXT_FRAMEBUFFER_BLIT])
        {
            gl_info->fbo_ops.glBlitFramebuffer = gl_info->glBlitFramebufferEXT;
        }
        if (gl_info->supported[EXT_FRAMEBUFFER_MULTISAMPLE])
        {
            gl_info->fbo_ops.glRenderbufferStorageMultisample = gl_info->glRenderbufferStorageMultisampleEXT;
        }
    }

#ifndef VBOX_WITH_VMSVGA
    /* MRTs are currently only supported when FBOs are used. */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
    {
        gl_info->limits.buffers = 1;
    }
#endif
    gl_vendor = wined3d_guess_gl_vendor(gl_info, gl_vendor_str, gl_renderer_str);
    card_vendor = wined3d_guess_card_vendor(gl_vendor_str, gl_renderer_str);
    TRACE_(d3d_caps)("found GL_VENDOR (%s)->(0x%04x/0x%04x)\n", debugstr_a(gl_vendor_str), gl_vendor, card_vendor);

    device = wined3d_guess_card(gl_info, gl_renderer_str, &gl_vendor, &card_vendor, &vidmem);
    TRACE_(d3d_caps)("FOUND (fake) card: 0x%x (vendor id), 0x%x (device id)\n", card_vendor, device);

    /* If we have an estimate use it, else default to 64MB;  */
    if(vidmem)
        gl_info->vidmem = vidmem*1024*1024; /* convert from MBs to bytes */
    else
        gl_info->vidmem = WINE_DEFAULT_VIDMEM;

    gl_info->wrap_lookup[WINED3DTADDRESS_WRAP - WINED3DTADDRESS_WRAP] = GL_REPEAT;
    gl_info->wrap_lookup[WINED3DTADDRESS_MIRROR - WINED3DTADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT] ? GL_MIRRORED_REPEAT_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3DTADDRESS_CLAMP - WINED3DTADDRESS_WRAP] = GL_CLAMP_TO_EDGE;
    gl_info->wrap_lookup[WINED3DTADDRESS_BORDER - WINED3DTADDRESS_WRAP] =
            gl_info->supported[ARB_TEXTURE_BORDER_CLAMP] ? GL_CLAMP_TO_BORDER_ARB : GL_REPEAT;
    gl_info->wrap_lookup[WINED3DTADDRESS_MIRRORONCE - WINED3DTADDRESS_WRAP] =
            gl_info->supported[ATI_TEXTURE_MIRROR_ONCE] ? GL_MIRROR_CLAMP_TO_EDGE_ATI : GL_REPEAT;

#ifndef VBOX_WITH_VMSVGA
    /* Make sure there's an active HDC else the WGL extensions will fail */
    hdc = pwglGetCurrentDC();
    if (hdc) {
        /* Not all GL drivers might offer WGL extensions e.g. VirtualBox */
        if(GL_EXTCALL(wglGetExtensionsStringARB))
            WGL_Extensions = GL_EXTCALL(wglGetExtensionsStringARB(hdc));

        if (NULL == WGL_Extensions) {
            ERR("   WGL_Extensions returns NULL\n");
        } else {
            TRACE_(d3d_caps)("WGL_Extensions reported:\n");
            while (*WGL_Extensions != 0x00) {
                const char *Start;
                char ThisExtn[256];

                while (isspace(*WGL_Extensions)) WGL_Extensions++;
                Start = WGL_Extensions;
                while (!isspace(*WGL_Extensions) && *WGL_Extensions != 0x00) {
                    WGL_Extensions++;
                }

                len = WGL_Extensions - Start;
                if (len == 0 || len >= sizeof(ThisExtn))
                    continue;

                memcpy(ThisExtn, Start, len);
                ThisExtn[len] = '\0';
                TRACE_(d3d_caps)("- %s\n", debugstr_a(ThisExtn));

                if (!strcmp(ThisExtn, "WGL_ARB_pixel_format")) {
                    gl_info->supported[WGL_ARB_PIXEL_FORMAT] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_ARB_pixel_format support\n");
                }
                if (!strcmp(ThisExtn, "WGL_WINE_pixel_format_passthrough")) {
                    gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH] = TRUE;
                    TRACE_(d3d_caps)("FOUND: WGL_WINE_pixel_format_passthrough support\n");
                }
            }
        }
    }
#endif

    fixup_extensions(gl_info, gl_renderer_str, gl_vendor, card_vendor, device);
#ifndef VBOX_WITH_VMSVGA
    init_driver_info(driver_info, card_vendor, device);
    add_gl_compat_wrappers(gl_info);
#endif

    return TRUE;
}

