/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __RADEON_TEX_H__
#define __RADEON_TEX_H__

extern void radeonSetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv);
extern void radeonSetTexBuffer2(__DRIcontext *pDRICtx, GLint target, GLint glx_texture_format,
			       __DRIdrawable *dPriv);

extern void radeonUpdateTextureState( struct gl_context *ctx );

extern int radeonUploadTexImages( r100ContextPtr rmesa, radeonTexObjPtr t,
				  GLuint face );

extern void radeonDestroyTexObj( r100ContextPtr rmesa, radeonTexObjPtr t );
extern void radeonTexUpdateParameters(struct gl_context *ctx, GLuint unit);

extern void radeonInitTextureFuncs( radeonContextPtr radeon, struct dd_function_table *functions );

struct tx_table {
   GLuint format, filter;
};

/* XXX verify this table against MESA_FORMAT_x values */
static const struct tx_table tx_table[] =
{
   [ MESA_FORMAT_NONE ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_A8B8G8R8_UNORM ] = { RADEON_TXFORMAT_RGBA8888 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_R8G8B8A8_UNORM ] = { RADEON_TXFORMAT_RGBA8888 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B8G8R8A8_UNORM ] = { RADEON_TXFORMAT_ARGB8888 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A8R8G8B8_UNORM ] = { RADEON_TXFORMAT_ARGB8888 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B8G8R8X8_UNORM ] = { RADEON_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_X8R8G8B8_UNORM ] = { RADEON_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_BGR_UNORM8 ] = { RADEON_TXFORMAT_ARGB8888, 0 },
   [ MESA_FORMAT_B5G6R5_UNORM ] = { RADEON_TXFORMAT_RGB565, 0 },
   [ MESA_FORMAT_R5G6B5_UNORM ] = { RADEON_TXFORMAT_RGB565, 0 },
   [ MESA_FORMAT_B4G4R4A4_UNORM ] = { RADEON_TXFORMAT_ARGB4444 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A4R4G4B4_UNORM ] = { RADEON_TXFORMAT_ARGB4444 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_B5G5R5A1_UNORM ] = { RADEON_TXFORMAT_ARGB1555 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A1R5G5B5_UNORM ] = { RADEON_TXFORMAT_ARGB1555 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_LA_UNORM8 ] = { RADEON_TXFORMAT_AI88 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_A_UNORM8 ] = { RADEON_TXFORMAT_I8 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_L_UNORM8 ] = { RADEON_TXFORMAT_I8, 0 },
   [ MESA_FORMAT_I_UNORM8 ] = { RADEON_TXFORMAT_I8 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_YCBCR ] = { RADEON_TXFORMAT_YVYU422, RADEON_YUV_TO_RGB },
   [ MESA_FORMAT_YCBCR_REV ] = { RADEON_TXFORMAT_VYUY422, RADEON_YUV_TO_RGB },
   [ MESA_FORMAT_RGB_FXT1 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_RGBA_FXT1 ] = { 0xffffffff, 0 },
   [ MESA_FORMAT_RGB_DXT1 ] = { RADEON_TXFORMAT_DXT1, 0 },
   [ MESA_FORMAT_RGBA_DXT1 ] = { RADEON_TXFORMAT_DXT1 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_RGBA_DXT3 ] = { RADEON_TXFORMAT_DXT23 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
   [ MESA_FORMAT_RGBA_DXT5 ] = { RADEON_TXFORMAT_DXT45 | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 },
};


#endif /* __RADEON_TEX_H__ */
