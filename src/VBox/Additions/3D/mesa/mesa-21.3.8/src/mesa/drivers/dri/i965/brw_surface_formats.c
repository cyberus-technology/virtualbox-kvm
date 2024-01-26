/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "main/mtypes.h"

#include "isl/isl.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

enum isl_format
brw_isl_format_for_mesa_format(mesa_format mesa_format)
{
   /* This table is ordered according to the enum ordering in formats.h.  We do
    * expect that enum to be extended without our explicit initialization
    * staying in sync, so we initialize to 0 even though
    * ISL_FORMAT_R32G32B32A32_FLOAT happens to also be 0.
    */
   static const enum isl_format table[MESA_FORMAT_COUNT] = {
      [0 ... MESA_FORMAT_COUNT-1] = ISL_FORMAT_UNSUPPORTED,

      [MESA_FORMAT_R8G8B8A8_UNORM] = ISL_FORMAT_R8G8B8A8_UNORM,
      [MESA_FORMAT_B8G8R8A8_UNORM] = ISL_FORMAT_B8G8R8A8_UNORM,
      [MESA_FORMAT_R8G8B8X8_UNORM] = ISL_FORMAT_R8G8B8X8_UNORM,
      [MESA_FORMAT_B8G8R8X8_UNORM] = ISL_FORMAT_B8G8R8X8_UNORM,
      [MESA_FORMAT_RGB_UNORM8] = ISL_FORMAT_R8G8B8_UNORM,
      [MESA_FORMAT_B5G6R5_UNORM] = ISL_FORMAT_B5G6R5_UNORM,
      [MESA_FORMAT_B4G4R4A4_UNORM] = ISL_FORMAT_B4G4R4A4_UNORM,
      [MESA_FORMAT_B5G5R5A1_UNORM] = ISL_FORMAT_B5G5R5A1_UNORM,
      [MESA_FORMAT_LA_UNORM8] = ISL_FORMAT_L8A8_UNORM,
      [MESA_FORMAT_LA_UNORM16] = ISL_FORMAT_L16A16_UNORM,
      [MESA_FORMAT_A_UNORM8] = ISL_FORMAT_A8_UNORM,
      [MESA_FORMAT_A_UNORM16] = ISL_FORMAT_A16_UNORM,
      [MESA_FORMAT_L_UNORM8] = ISL_FORMAT_L8_UNORM,
      [MESA_FORMAT_L_UNORM16] = ISL_FORMAT_L16_UNORM,
      [MESA_FORMAT_I_UNORM8] = ISL_FORMAT_I8_UNORM,
      [MESA_FORMAT_I_UNORM16] = ISL_FORMAT_I16_UNORM,
      [MESA_FORMAT_YCBCR_REV] = ISL_FORMAT_YCRCB_NORMAL,
      [MESA_FORMAT_YCBCR] = ISL_FORMAT_YCRCB_SWAPUVY,
      [MESA_FORMAT_R_UNORM8] = ISL_FORMAT_R8_UNORM,
      [MESA_FORMAT_RG_UNORM8] = ISL_FORMAT_R8G8_UNORM,
      [MESA_FORMAT_R_UNORM16] = ISL_FORMAT_R16_UNORM,
      [MESA_FORMAT_RG_UNORM16] = ISL_FORMAT_R16G16_UNORM,
      [MESA_FORMAT_B10G10R10A2_UNORM] = ISL_FORMAT_B10G10R10A2_UNORM,
      [MESA_FORMAT_S_UINT8] = ISL_FORMAT_R8_UINT,

      [MESA_FORMAT_B8G8R8A8_SRGB] = ISL_FORMAT_B8G8R8A8_UNORM_SRGB,
      [MESA_FORMAT_R8G8B8A8_SRGB] = ISL_FORMAT_R8G8B8A8_UNORM_SRGB,
      [MESA_FORMAT_B8G8R8X8_SRGB] = ISL_FORMAT_B8G8R8X8_UNORM_SRGB,
      [MESA_FORMAT_R_SRGB8] = ISL_FORMAT_L8_UNORM_SRGB,
      [MESA_FORMAT_L_SRGB8] = ISL_FORMAT_L8_UNORM_SRGB,
      [MESA_FORMAT_LA_SRGB8] = ISL_FORMAT_L8A8_UNORM_SRGB,
      [MESA_FORMAT_SRGB_DXT1] = ISL_FORMAT_BC1_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT1] = ISL_FORMAT_BC1_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT3] = ISL_FORMAT_BC2_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT5] = ISL_FORMAT_BC3_UNORM_SRGB,

      [MESA_FORMAT_RGB_FXT1] = ISL_FORMAT_FXT1,
      [MESA_FORMAT_RGBA_FXT1] = ISL_FORMAT_FXT1,
      [MESA_FORMAT_RGB_DXT1] = ISL_FORMAT_BC1_UNORM,
      [MESA_FORMAT_RGBA_DXT1] = ISL_FORMAT_BC1_UNORM,
      [MESA_FORMAT_RGBA_DXT3] = ISL_FORMAT_BC2_UNORM,
      [MESA_FORMAT_RGBA_DXT5] = ISL_FORMAT_BC3_UNORM,

      [MESA_FORMAT_RGBA_FLOAT32] = ISL_FORMAT_R32G32B32A32_FLOAT,
      [MESA_FORMAT_RGBA_FLOAT16] = ISL_FORMAT_R16G16B16A16_FLOAT,
      [MESA_FORMAT_RGB_FLOAT32] = ISL_FORMAT_R32G32B32_FLOAT,
      [MESA_FORMAT_A_FLOAT32] = ISL_FORMAT_A32_FLOAT,
      [MESA_FORMAT_A_FLOAT16] = ISL_FORMAT_A16_FLOAT,
      [MESA_FORMAT_L_FLOAT32] = ISL_FORMAT_L32_FLOAT,
      [MESA_FORMAT_L_FLOAT16] = ISL_FORMAT_L16_FLOAT,
      [MESA_FORMAT_LA_FLOAT32] = ISL_FORMAT_L32A32_FLOAT,
      [MESA_FORMAT_LA_FLOAT16] = ISL_FORMAT_L16A16_FLOAT,
      [MESA_FORMAT_I_FLOAT32] = ISL_FORMAT_I32_FLOAT,
      [MESA_FORMAT_I_FLOAT16] = ISL_FORMAT_I16_FLOAT,
      [MESA_FORMAT_R_FLOAT32] = ISL_FORMAT_R32_FLOAT,
      [MESA_FORMAT_R_FLOAT16] = ISL_FORMAT_R16_FLOAT,
      [MESA_FORMAT_RG_FLOAT32] = ISL_FORMAT_R32G32_FLOAT,
      [MESA_FORMAT_RG_FLOAT16] = ISL_FORMAT_R16G16_FLOAT,

      [MESA_FORMAT_R_SINT8] = ISL_FORMAT_R8_SINT,
      [MESA_FORMAT_RG_SINT8] = ISL_FORMAT_R8G8_SINT,
      [MESA_FORMAT_RGB_SINT8] = ISL_FORMAT_R8G8B8_SINT,
      [MESA_FORMAT_RGBA_SINT8] = ISL_FORMAT_R8G8B8A8_SINT,
      [MESA_FORMAT_R_SINT16] = ISL_FORMAT_R16_SINT,
      [MESA_FORMAT_RG_SINT16] = ISL_FORMAT_R16G16_SINT,
      [MESA_FORMAT_RGB_SINT16] = ISL_FORMAT_R16G16B16_SINT,
      [MESA_FORMAT_RGBA_SINT16] = ISL_FORMAT_R16G16B16A16_SINT,
      [MESA_FORMAT_R_SINT32] = ISL_FORMAT_R32_SINT,
      [MESA_FORMAT_RG_SINT32] = ISL_FORMAT_R32G32_SINT,
      [MESA_FORMAT_RGB_SINT32] = ISL_FORMAT_R32G32B32_SINT,
      [MESA_FORMAT_RGBA_SINT32] = ISL_FORMAT_R32G32B32A32_SINT,

      [MESA_FORMAT_R_UINT8] = ISL_FORMAT_R8_UINT,
      [MESA_FORMAT_RG_UINT8] = ISL_FORMAT_R8G8_UINT,
      [MESA_FORMAT_RGB_UINT8] = ISL_FORMAT_R8G8B8_UINT,
      [MESA_FORMAT_RGBA_UINT8] = ISL_FORMAT_R8G8B8A8_UINT,
      [MESA_FORMAT_R_UINT16] = ISL_FORMAT_R16_UINT,
      [MESA_FORMAT_RG_UINT16] = ISL_FORMAT_R16G16_UINT,
      [MESA_FORMAT_RGB_UINT16] = ISL_FORMAT_R16G16B16_UINT,
      [MESA_FORMAT_RGBA_UINT16] = ISL_FORMAT_R16G16B16A16_UINT,
      [MESA_FORMAT_R_UINT32] = ISL_FORMAT_R32_UINT,
      [MESA_FORMAT_RG_UINT32] = ISL_FORMAT_R32G32_UINT,
      [MESA_FORMAT_RGB_UINT32] = ISL_FORMAT_R32G32B32_UINT,
      [MESA_FORMAT_RGBA_UINT32] = ISL_FORMAT_R32G32B32A32_UINT,

      [MESA_FORMAT_R_SNORM8] = ISL_FORMAT_R8_SNORM,
      [MESA_FORMAT_RG_SNORM8] = ISL_FORMAT_R8G8_SNORM,
      [MESA_FORMAT_R8G8B8A8_SNORM] = ISL_FORMAT_R8G8B8A8_SNORM,
      [MESA_FORMAT_R_SNORM16] = ISL_FORMAT_R16_SNORM,
      [MESA_FORMAT_RG_SNORM16] = ISL_FORMAT_R16G16_SNORM,
      [MESA_FORMAT_RGB_SNORM16] = ISL_FORMAT_R16G16B16_SNORM,
      [MESA_FORMAT_RGBA_SNORM16] = ISL_FORMAT_R16G16B16A16_SNORM,
      [MESA_FORMAT_RGBA_UNORM16] = ISL_FORMAT_R16G16B16A16_UNORM,

      [MESA_FORMAT_R_RGTC1_UNORM] = ISL_FORMAT_BC4_UNORM,
      [MESA_FORMAT_R_RGTC1_SNORM] = ISL_FORMAT_BC4_SNORM,
      [MESA_FORMAT_RG_RGTC2_UNORM] = ISL_FORMAT_BC5_UNORM,
      [MESA_FORMAT_RG_RGTC2_SNORM] = ISL_FORMAT_BC5_SNORM,

      [MESA_FORMAT_ETC1_RGB8] = ISL_FORMAT_ETC1_RGB8,
      [MESA_FORMAT_ETC2_RGB8] = ISL_FORMAT_ETC2_RGB8,
      [MESA_FORMAT_ETC2_SRGB8] = ISL_FORMAT_ETC2_SRGB8,
      [MESA_FORMAT_ETC2_RGBA8_EAC] = ISL_FORMAT_ETC2_EAC_RGBA8,
      [MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = ISL_FORMAT_ETC2_EAC_SRGB8_A8,
      [MESA_FORMAT_ETC2_R11_EAC] = ISL_FORMAT_EAC_R11,
      [MESA_FORMAT_ETC2_RG11_EAC] = ISL_FORMAT_EAC_RG11,
      [MESA_FORMAT_ETC2_SIGNED_R11_EAC] = ISL_FORMAT_EAC_SIGNED_R11,
      [MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = ISL_FORMAT_EAC_SIGNED_RG11,
      [MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] = ISL_FORMAT_ETC2_RGB8_PTA,
      [MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] = ISL_FORMAT_ETC2_SRGB8_PTA,

      [MESA_FORMAT_BPTC_RGBA_UNORM] = ISL_FORMAT_BC7_UNORM,
      [MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM] = ISL_FORMAT_BC7_UNORM_SRGB,
      [MESA_FORMAT_BPTC_RGB_SIGNED_FLOAT] = ISL_FORMAT_BC6H_SF16,
      [MESA_FORMAT_BPTC_RGB_UNSIGNED_FLOAT] = ISL_FORMAT_BC6H_UF16,

      [MESA_FORMAT_RGBA_ASTC_4x4]           = ISL_FORMAT_ASTC_LDR_2D_4X4_FLT16,
      [MESA_FORMAT_RGBA_ASTC_5x4]           = ISL_FORMAT_ASTC_LDR_2D_5X4_FLT16,
      [MESA_FORMAT_RGBA_ASTC_5x5]           = ISL_FORMAT_ASTC_LDR_2D_5X5_FLT16,
      [MESA_FORMAT_RGBA_ASTC_6x5]           = ISL_FORMAT_ASTC_LDR_2D_6X5_FLT16,
      [MESA_FORMAT_RGBA_ASTC_6x6]           = ISL_FORMAT_ASTC_LDR_2D_6X6_FLT16,
      [MESA_FORMAT_RGBA_ASTC_8x5]           = ISL_FORMAT_ASTC_LDR_2D_8X5_FLT16,
      [MESA_FORMAT_RGBA_ASTC_8x6]           = ISL_FORMAT_ASTC_LDR_2D_8X6_FLT16,
      [MESA_FORMAT_RGBA_ASTC_8x8]           = ISL_FORMAT_ASTC_LDR_2D_8X8_FLT16,
      [MESA_FORMAT_RGBA_ASTC_10x5]          = ISL_FORMAT_ASTC_LDR_2D_10X5_FLT16,
      [MESA_FORMAT_RGBA_ASTC_10x6]          = ISL_FORMAT_ASTC_LDR_2D_10X6_FLT16,
      [MESA_FORMAT_RGBA_ASTC_10x8]          = ISL_FORMAT_ASTC_LDR_2D_10X8_FLT16,
      [MESA_FORMAT_RGBA_ASTC_10x10]         = ISL_FORMAT_ASTC_LDR_2D_10X10_FLT16,
      [MESA_FORMAT_RGBA_ASTC_12x10]         = ISL_FORMAT_ASTC_LDR_2D_12X10_FLT16,
      [MESA_FORMAT_RGBA_ASTC_12x12]         = ISL_FORMAT_ASTC_LDR_2D_12X12_FLT16,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_4x4]   = ISL_FORMAT_ASTC_LDR_2D_4X4_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_5x4]   = ISL_FORMAT_ASTC_LDR_2D_5X4_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_5x5]   = ISL_FORMAT_ASTC_LDR_2D_5X5_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_6x5]   = ISL_FORMAT_ASTC_LDR_2D_6X5_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_6x6]   = ISL_FORMAT_ASTC_LDR_2D_6X6_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_8x5]   = ISL_FORMAT_ASTC_LDR_2D_8X5_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_8x6]   = ISL_FORMAT_ASTC_LDR_2D_8X6_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_8x8]   = ISL_FORMAT_ASTC_LDR_2D_8X8_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_10x5]  = ISL_FORMAT_ASTC_LDR_2D_10X5_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_10x6]  = ISL_FORMAT_ASTC_LDR_2D_10X6_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_10x8]  = ISL_FORMAT_ASTC_LDR_2D_10X8_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_10x10] = ISL_FORMAT_ASTC_LDR_2D_10X10_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_12x10] = ISL_FORMAT_ASTC_LDR_2D_12X10_U8SRGB,
      [MESA_FORMAT_SRGB8_ALPHA8_ASTC_12x12] = ISL_FORMAT_ASTC_LDR_2D_12X12_U8SRGB,

      [MESA_FORMAT_R9G9B9E5_FLOAT] = ISL_FORMAT_R9G9B9E5_SHAREDEXP,
      [MESA_FORMAT_R11G11B10_FLOAT] = ISL_FORMAT_R11G11B10_FLOAT,

      [MESA_FORMAT_R10G10B10A2_UNORM] = ISL_FORMAT_R10G10B10A2_UNORM,
      [MESA_FORMAT_B10G10R10A2_UINT] = ISL_FORMAT_B10G10R10A2_UINT,
      [MESA_FORMAT_R10G10B10A2_UINT] = ISL_FORMAT_R10G10B10A2_UINT,

      [MESA_FORMAT_B5G5R5X1_UNORM] = ISL_FORMAT_B5G5R5X1_UNORM,
      [MESA_FORMAT_R8G8B8X8_SRGB] = ISL_FORMAT_R8G8B8X8_UNORM_SRGB,
      [MESA_FORMAT_B10G10R10X2_UNORM] = ISL_FORMAT_B10G10R10X2_UNORM,
      [MESA_FORMAT_RGBX_UNORM16] = ISL_FORMAT_R16G16B16X16_UNORM,
      [MESA_FORMAT_RGBX_FLOAT16] = ISL_FORMAT_R16G16B16X16_FLOAT,
      [MESA_FORMAT_RGBX_FLOAT32] = ISL_FORMAT_R32G32B32X32_FLOAT,
   };

   assert(mesa_format < MESA_FORMAT_COUNT);
   return table[mesa_format];
}

void
brw_screen_init_surface_formats(struct brw_screen *screen)
{
   const struct intel_device_info *devinfo = &screen->devinfo;
   mesa_format format;

   memset(&screen->mesa_format_supports_texture, 0,
          sizeof(screen->mesa_format_supports_texture));

   for (format = MESA_FORMAT_NONE + 1; format < MESA_FORMAT_COUNT; format++) {
      if (!_mesa_get_format_name(format))
         continue;
      enum isl_format texture, render;
      bool is_integer = _mesa_is_format_integer_color(format);

      render = texture = brw_isl_format_for_mesa_format(format);

      /* Only exposed with EXT_memory_object_* support which
       * is not for older gens.
       */
      if (devinfo->ver < 7 && format == MESA_FORMAT_Z_UNORM16)
         continue;

      if (texture == ISL_FORMAT_UNSUPPORTED)
         continue;

      /* Don't advertise 8 and 16-bit RGB formats to core mesa.  This ensures
       * that they are renderable from an API perspective since core mesa will
       * fall back to RGBA or RGBX (we can't render to non-power-of-two
       * formats).  For 8-bit, formats, this also keeps us from hitting some
       * nasty corners in brw_miptree_map_blit if you ever try to map one.
       */
      int format_size = _mesa_get_format_bytes(format);
      if (format_size == 3 || format_size == 6)
         continue;

      if (isl_format_supports_sampling(devinfo, texture) &&
          (isl_format_supports_filtering(devinfo, texture) || is_integer))
         screen->mesa_format_supports_texture[format] = true;

      /* Re-map some render target formats to make them supported when they
       * wouldn't be using their format for texturing.
       */
      switch (render) {
         /* For these formats, we just need to read/write the first
          * channel into R, which is to say that we just treat them as
          * GL_RED.
          */
      case ISL_FORMAT_I32_FLOAT:
      case ISL_FORMAT_L32_FLOAT:
         render = ISL_FORMAT_R32_FLOAT;
         break;
      case ISL_FORMAT_I16_FLOAT:
      case ISL_FORMAT_L16_FLOAT:
         render = ISL_FORMAT_R16_FLOAT;
         break;
      case ISL_FORMAT_I8_UNORM:
      case ISL_FORMAT_L8_UNORM:
         render = ISL_FORMAT_R8_UNORM;
         break;
      case ISL_FORMAT_I16_UNORM:
      case ISL_FORMAT_L16_UNORM:
         render = ISL_FORMAT_R16_UNORM;
         break;
      case ISL_FORMAT_R16G16B16X16_UNORM:
         render = ISL_FORMAT_R16G16B16A16_UNORM;
         break;
      case ISL_FORMAT_R16G16B16X16_FLOAT:
         render = ISL_FORMAT_R16G16B16A16_FLOAT;
         break;
      case ISL_FORMAT_B8G8R8X8_UNORM:
         /* XRGB is handled as ARGB because the chips in this family
          * cannot render to XRGB targets.  This means that we have to
          * mask writes to alpha (ala glColorMask) and reconfigure the
          * alpha blending hardware to use GL_ONE (or GL_ZERO) for
          * cases where GL_DST_ALPHA (or GL_ONE_MINUS_DST_ALPHA) is
          * used. On Gfx8+ BGRX is actually allowed (but not RGBX).
          */
         if (!isl_format_supports_rendering(devinfo, texture))
            render = ISL_FORMAT_B8G8R8A8_UNORM;
         break;
      case ISL_FORMAT_B8G8R8X8_UNORM_SRGB:
         if (!isl_format_supports_rendering(devinfo, texture))
            render = ISL_FORMAT_B8G8R8A8_UNORM_SRGB;
         break;
      case ISL_FORMAT_R8G8B8X8_UNORM:
         render = ISL_FORMAT_R8G8B8A8_UNORM;
         break;
      case ISL_FORMAT_R8G8B8X8_UNORM_SRGB:
         render = ISL_FORMAT_R8G8B8A8_UNORM_SRGB;
         break;
      default:
         break;
      }

      /* Note that GL_EXT_texture_integer says that blending doesn't occur for
       * integer, so we don't need hardware support for blending on it.  Other
       * than that, GL in general requires alpha blending for render targets,
       * even though we don't support it for some formats.
       */
      if (isl_format_supports_rendering(devinfo, render) &&
          (isl_format_supports_alpha_blending(devinfo, render) || is_integer)) {
         screen->mesa_to_isl_render_format[format] = render;
         screen->mesa_format_supports_render[format] = true;
      }
   }

   /* We will check this table for FBO completeness, but the surface format
    * table above only covered color rendering.
    */
   screen->mesa_format_supports_render[MESA_FORMAT_Z24_UNORM_S8_UINT] = true;
   screen->mesa_format_supports_render[MESA_FORMAT_Z24_UNORM_X8_UINT] = true;
   screen->mesa_format_supports_render[MESA_FORMAT_S_UINT8] = true;
   screen->mesa_format_supports_render[MESA_FORMAT_Z_FLOAT32] = true;
   screen->mesa_format_supports_render[MESA_FORMAT_Z32_FLOAT_S8X24_UINT] = true;
   if (devinfo->ver >= 8)
      screen->mesa_format_supports_render[MESA_FORMAT_Z_UNORM16] = true;

   /* We remap depth formats to a supported texturing format in
    * translate_tex_format().
    */
   screen->mesa_format_supports_texture[MESA_FORMAT_Z24_UNORM_S8_UINT] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_Z24_UNORM_X8_UINT] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_Z_FLOAT32] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_Z32_FLOAT_S8X24_UINT] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_S_UINT8] = true;

   /* Benchmarking shows that Z16 is slower than Z24, so there's no reason to
    * use it unless you're under memory (not memory bandwidth) pressure.
    *
    * Apparently, the GPU's depth scoreboarding works on a 32-bit granularity,
    * which corresponds to one pixel in the depth buffer for Z24 or Z32 formats.
    * However, it corresponds to two pixels with Z16, which means both need to
    * hit the early depth case in order for it to happen.
    *
    * Other speculation is that we may be hitting increased fragment shader
    * execution from GL_LEQUAL/GL_EQUAL depth tests at reduced precision.
    *
    * With the PMA stall workaround in place, Z16 is faster than Z24, as it
    * should be.
    */
   if (devinfo->ver >= 8)
      screen->mesa_format_supports_texture[MESA_FORMAT_Z_UNORM16] = true;

   /* The RGBX formats are not renderable. Normally these get mapped
    * internally to RGBA formats when rendering. However on Gfx9+ when this
    * internal override is used fast clears don't work so they are disabled in
    * brw_meta_fast_clear. To avoid this problem we can just pretend not to
    * support RGBX formats at all. This will cause the upper layers of Mesa to
    * pick the RGBA formats instead. This works fine because when it is used
    * as a texture source the swizzle state is programmed to force the alpha
    * channel to 1.0 anyway. We could also do this for all gens except that
    * it's a bit more difficult when the hardware doesn't support texture
    * swizzling. Gens using the blorp have further problems because that
    * doesn't implement this swizzle override. We don't need to do this for
    * BGRX because that actually is supported natively on Gfx8+.
    */
   if (devinfo->ver >= 9) {
      static const mesa_format rgbx_formats[] = {
         MESA_FORMAT_R8G8B8X8_UNORM,
         MESA_FORMAT_R8G8B8X8_SRGB,
         MESA_FORMAT_RGBX_UNORM16,
         MESA_FORMAT_RGBX_FLOAT16,
         MESA_FORMAT_RGBX_FLOAT32
      };

      for (int i = 0; i < ARRAY_SIZE(rgbx_formats); i++) {
         screen->mesa_format_supports_texture[rgbx_formats[i]] = false;
         screen->mesa_format_supports_render[rgbx_formats[i]] = false;
      }
   }

   /* On hardware that lacks support for ETC1, we map ETC1 to RGBX
    * during glCompressedTexImage2D(). See brw_mipmap_tree::wraps_etc1.
    */
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC1_RGB8] = true;

   /* On hardware that lacks support for ETC2, we map ETC2 to a suitable
    * MESA_FORMAT during glCompressedTexImage2D().
    * See brw_mipmap_tree::wraps_etc2.
    */
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_RGB8] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_SRGB8] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_RGBA8_EAC] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_R11_EAC] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_RG11_EAC] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_SIGNED_R11_EAC] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] = true;
   screen->mesa_format_supports_texture[MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] = true;
}

void
brw_init_surface_formats(struct brw_context *brw)
{
   struct brw_screen *screen = brw->screen;
   struct gl_context *ctx = &brw->ctx;

   brw->mesa_format_supports_render = screen->mesa_format_supports_render;
   brw->mesa_to_isl_render_format = screen->mesa_to_isl_render_format;

   STATIC_ASSERT(ARRAY_SIZE(ctx->TextureFormatSupported) ==
                 ARRAY_SIZE(screen->mesa_format_supports_texture));

   for (unsigned i = 0; i < ARRAY_SIZE(ctx->TextureFormatSupported); ++i) {
      ctx->TextureFormatSupported[i] = screen->mesa_format_supports_texture[i];
   }
}

bool
brw_render_target_supported(struct brw_context *brw,
                            struct gl_renderbuffer *rb)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   mesa_format format = rb->Format;

   /* Many integer formats are promoted to RGBA (like XRGB8888 is), which means
    * we would consider them renderable even though we don't have surface
    * support for their alpha behavior and don't have the blending unit
    * available to fake it like we do for XRGB8888.  Force them to being
    * unsupported.
    */
   if (_mesa_is_format_integer_color(format) &&
       rb->_BaseFormat != GL_RGBA &&
       rb->_BaseFormat != GL_RG &&
       rb->_BaseFormat != GL_RED)
      return false;

   /* Under some conditions, MSAA is not supported for formats whose width is
    * more than 64 bits.
    */
   if (devinfo->ver < 8 &&
       rb->NumSamples > 0 && _mesa_get_format_bytes(format) > 8) {
      /* Gfx6: MSAA on >64 bit formats is unsupported. */
      if (devinfo->ver <= 6)
         return false;

      /* Gfx7: 8x MSAA on >64 bit formats is unsupported. */
      if (rb->NumSamples >= 8)
         return false;
   }

   return brw->mesa_format_supports_render[format];
}

enum isl_format
translate_tex_format(struct brw_context *brw,
                     mesa_format mesa_format,
                     GLenum srgb_decode)
{
   struct gl_context *ctx = &brw->ctx;
   if (srgb_decode == GL_SKIP_DECODE_EXT)
      mesa_format = _mesa_get_srgb_format_linear(mesa_format);

   switch( mesa_format ) {

   case MESA_FORMAT_Z_UNORM16:
      return ISL_FORMAT_R16_UNORM;

   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      return ISL_FORMAT_R24_UNORM_X8_TYPELESS;

   case MESA_FORMAT_Z_FLOAT32:
      return ISL_FORMAT_R32_FLOAT;

   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return ISL_FORMAT_R32_FLOAT_X8X24_TYPELESS;

   case MESA_FORMAT_RGBA_FLOAT32:
      /* The value of this ISL surface format is 0, which tricks the
       * assertion below.
       */
      return ISL_FORMAT_R32G32B32A32_FLOAT;

   case MESA_FORMAT_RGBA_ASTC_4x4:
   case MESA_FORMAT_RGBA_ASTC_5x4:
   case MESA_FORMAT_RGBA_ASTC_5x5:
   case MESA_FORMAT_RGBA_ASTC_6x5:
   case MESA_FORMAT_RGBA_ASTC_6x6:
   case MESA_FORMAT_RGBA_ASTC_8x5:
   case MESA_FORMAT_RGBA_ASTC_8x6:
   case MESA_FORMAT_RGBA_ASTC_8x8:
   case MESA_FORMAT_RGBA_ASTC_10x5:
   case MESA_FORMAT_RGBA_ASTC_10x6:
   case MESA_FORMAT_RGBA_ASTC_10x8:
   case MESA_FORMAT_RGBA_ASTC_10x10:
   case MESA_FORMAT_RGBA_ASTC_12x10:
   case MESA_FORMAT_RGBA_ASTC_12x12: {
      enum isl_format isl_fmt =
         brw_isl_format_for_mesa_format(mesa_format);

      /**
       * It is possible to process these formats using the LDR Profile
       * or the Full Profile mode of the hardware. Because, it isn't
       * possible to determine if an HDR or LDR texture is being rendered, we
       * can't determine which mode to enable in the hardware. Therefore, to
       * handle all cases, always default to Full profile unless we are
       * processing sRGBs, which are incompatible with this mode.
       */
      if (ctx->Extensions.KHR_texture_compression_astc_hdr)
         isl_fmt |= GFX9_SURFACE_ASTC_HDR_FORMAT_BIT;

      return isl_fmt;
   }

   default:
      return brw_isl_format_for_mesa_format(mesa_format);
   }
}

/**
 * Convert a MESA_FORMAT to the corresponding BRW_DEPTHFORMAT enum.
 */
uint32_t
brw_depth_format(struct brw_context *brw, mesa_format format)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   switch (format) {
   case MESA_FORMAT_Z_UNORM16:
      return BRW_DEPTHFORMAT_D16_UNORM;
   case MESA_FORMAT_Z_FLOAT32:
      return BRW_DEPTHFORMAT_D32_FLOAT;
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      if (devinfo->ver >= 6) {
         return BRW_DEPTHFORMAT_D24_UNORM_X8_UINT;
      } else {
         /* Use D24_UNORM_S8, not D24_UNORM_X8.
          *
          * D24_UNORM_X8 was not introduced until Gfx5. (See the Ironlake PRM,
          * Volume 2, Part 1, Section 8.4.6 "Depth/Stencil Buffer State", Bits
          * 3DSTATE_DEPTH_BUFFER.Surface_Format).
          *
          * However, on Gfx5, D24_UNORM_X8 may be used only if separate
          * stencil is enabled, and we never enable it. From the Ironlake PRM,
          * same section as above, 3DSTATE_DEPTH_BUFFER's
          * "Separate Stencil Buffer Enable" bit:
          *
          * "If this field is disabled, the Surface Format of the depth
          *  buffer cannot be D24_UNORM_X8_UINT."
          */
         return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
      }
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT;
   default:
      unreachable("Unexpected depth format.");
   }
}
