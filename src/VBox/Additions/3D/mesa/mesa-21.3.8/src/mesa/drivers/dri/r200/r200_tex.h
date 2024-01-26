/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@vmware.com>
 */

#ifndef __R200_TEX_H__
#define __R200_TEX_H__

extern void r200SetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv);
extern void r200SetTexBuffer2(__DRIcontext *pDRICtx, GLint target, GLint glx_texture_format,
			      __DRIdrawable *dPriv);

extern void r200UpdateTextureState( struct gl_context *ctx );

extern int r200UploadTexImages( r200ContextPtr rmesa, radeonTexObjPtr t, GLuint face );

extern void r200DestroyTexObj( r200ContextPtr rmesa, radeonTexObjPtr t );

extern void r200InitTextureFuncs( radeonContextPtr radeon, struct dd_function_table *functions );

extern void r200UpdateFragmentShader( struct gl_context *ctx );
extern void r200TexUpdateParameters(struct gl_context *ctx, GLuint unit);

extern void set_re_cntl_d3d( struct gl_context *ctx, int unit, GLboolean use_d3d );

struct tx_table {
   GLuint format, filter;
};

/* Note the tables (have to) contain invalid entries (if they are only valid
 * for either be/le) */
static const struct tx_table tx_table_be[] =
{
   [ MESA_FORMAT_A8B8G8R8_UNORM ] = { R200_TXFORMAT_ABGR8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_R8G8B8A8_UNORM ] = { R200_TXFORMAT_RGBA8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B8G8R8A8_UNORM ] = { R200_TXFORMAT_ARGB8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B8G8R8X8_UNORM ] = { R200_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_A8R8G8B8_UNORM ] = { R200_TXFORMAT_ARGB8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_X8R8G8B8_UNORM ] = { R200_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_BGR_UNORM8 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_B5G6R5_UNORM ] = { R200_TXFORMAT_RGB565, 0 },
   [ MESA_FORMAT_R5G6B5_UNORM ] = { R200_TXFORMAT_RGB565, 0 },
   [ MESA_FORMAT_B4G4R4A4_UNORM ] = { R200_TXFORMAT_ARGB4444 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A4R4G4B4_UNORM ] = { R200_TXFORMAT_ARGB4444 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B5G5R5A1_UNORM ] = { R200_TXFORMAT_ARGB1555 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A1R5G5B5_UNORM ] = { R200_TXFORMAT_ARGB1555 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A_UNORM8 ] = { R200_TXFORMAT_I8 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_L_UNORM8 ] = { R200_TXFORMAT_I8, 0 },
   [ MESA_FORMAT_LA_UNORM8 ] = { R200_TXFORMAT_AI88 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_I_UNORM8 ] = { R200_TXFORMAT_I8 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_YCBCR ] = { R200_TXFORMAT_YVYU422, R200_YUV_TO_RGB },
   [ MESA_FORMAT_YCBCR_REV ] = { R200_TXFORMAT_VYUY422, R200_YUV_TO_RGB },
   [ MESA_FORMAT_RGB_FXT1 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_RGBA_FXT1 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_RGB_DXT1 ] = { R200_TXFORMAT_DXT1, 0 },
   [ MESA_FORMAT_RGBA_DXT1 ] = { R200_TXFORMAT_DXT1 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_RGBA_DXT3 ] = { R200_TXFORMAT_DXT23 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_RGBA_DXT5 ] = { R200_TXFORMAT_DXT45 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
};

static const struct tx_table tx_table_le[] =
{
   [ MESA_FORMAT_A8B8G8R8_UNORM ] = { R200_TXFORMAT_RGBA8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_R8G8B8A8_UNORM ] = { R200_TXFORMAT_ABGR8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B8G8R8A8_UNORM ] = { R200_TXFORMAT_ARGB8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B8G8R8X8_UNORM ] = { R200_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_A8R8G8B8_UNORM ] = { R200_TXFORMAT_ARGB8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_X8R8G8B8_UNORM ] = { R200_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_BGR_UNORM8 ] = { R200_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_B5G6R5_UNORM ] = { R200_TXFORMAT_RGB565, 0 },
   [ MESA_FORMAT_R5G6B5_UNORM ] = { R200_TXFORMAT_RGB565, 0 },
   [ MESA_FORMAT_B4G4R4A4_UNORM ] = { R200_TXFORMAT_ARGB4444 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A4R4G4B4_UNORM ] = { R200_TXFORMAT_ARGB4444 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B5G5R5A1_UNORM ] = { R200_TXFORMAT_ARGB1555 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A1R5G5B5_UNORM ] = { R200_TXFORMAT_ARGB1555 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A_UNORM8 ] = { R200_TXFORMAT_I8 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_L_UNORM8 ] = { R200_TXFORMAT_I8, 0 },
   [ MESA_FORMAT_LA_UNORM8 ] = { R200_TXFORMAT_AI88 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_I_UNORM8 ] = { R200_TXFORMAT_I8 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_YCBCR ] = { R200_TXFORMAT_YVYU422, R200_YUV_TO_RGB },
   [ MESA_FORMAT_YCBCR_REV ] = { R200_TXFORMAT_VYUY422, R200_YUV_TO_RGB },
   [ MESA_FORMAT_RGB_FXT1 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_RGBA_FXT1 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_RGB_DXT1 ] = { R200_TXFORMAT_DXT1, 0 },
   [ MESA_FORMAT_RGBA_DXT1 ] = { R200_TXFORMAT_DXT1 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_RGBA_DXT3 ] = { R200_TXFORMAT_DXT23 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_RGBA_DXT5 ] = { R200_TXFORMAT_DXT45 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
};



#endif /* __R200_TEX_H__ */
