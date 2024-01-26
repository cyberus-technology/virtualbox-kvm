/*
 * Copyright 2016 Intel Corporation
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#include <stdint.h>

#define __gen_address_type uint64_t
#define __gen_user_data void

static uint64_t
__gen_combine_address(__attribute__((unused)) void *data,
                      __attribute__((unused)) void *loc, uint64_t addr,
                      uint32_t delta)
{
   return addr + delta;
}

#include "genxml/gen_macros.h"
#include "genxml/genX_pack.h"

#include "isl_priv.h"

#if GFX_VER >= 7
static const uint8_t isl_encode_halign(uint8_t halign)
{
   switch (halign) {
#if GFX_VERx10 >= 125
   case  16: return HALIGN_16;
   case  32: return HALIGN_32;
   case  64: return HALIGN_64;
   case 128: return HALIGN_128;
#elif GFX_VER >= 8
   case   4: return HALIGN_4;
   case   8: return HALIGN_8;
   case  16: return HALIGN_16;
#else
   case   4: return HALIGN_4;
   case   8: return HALIGN_8;
#endif
   default: unreachable("Invalid halign");
   }
}
#endif

#if GFX_VER >= 6
static const uint8_t isl_encode_valign(uint8_t valign)
{
   switch (valign) {
#if GFX_VER >= 8
   case   4: return VALIGN_4;
   case   8: return VALIGN_8;
   case  16: return VALIGN_16;
#else
   case   2: return VALIGN_2;
   case   4: return VALIGN_4;
#endif
   default: unreachable("Invalid valign");
   }
}
#endif

#if GFX_VER >= 8
static const uint8_t isl_encode_tiling[] = {
   [ISL_TILING_LINEAR]  = LINEAR,
   [ISL_TILING_X]       = XMAJOR,
#if GFX_VERx10 >= 125
   [ISL_TILING_4]       = TILE4,
   [ISL_TILING_64]      = TILE64,
#else
   [ISL_TILING_Y0]      = YMAJOR,
   [ISL_TILING_Yf]      = YMAJOR,
   [ISL_TILING_Ys]      = YMAJOR,
#endif
#if GFX_VER <= 11
   [ISL_TILING_W]       = WMAJOR,
#endif
};
#endif

#if GFX_VER >= 7
static const uint32_t isl_encode_multisample_layout[] = {
   [ISL_MSAA_LAYOUT_NONE]           = MSFMT_MSS,
   [ISL_MSAA_LAYOUT_INTERLEAVED]    = MSFMT_DEPTH_STENCIL,
   [ISL_MSAA_LAYOUT_ARRAY]          = MSFMT_MSS,
};
#endif

#if GFX_VER >= 12
static const uint32_t isl_encode_aux_mode[] = {
   [ISL_AUX_USAGE_NONE] = AUX_NONE,
   [ISL_AUX_USAGE_MC] = AUX_NONE,
   [ISL_AUX_USAGE_MCS] = AUX_CCS_E,
   [ISL_AUX_USAGE_GFX12_CCS_E] = AUX_CCS_E,
   [ISL_AUX_USAGE_CCS_E] = AUX_CCS_E,
   [ISL_AUX_USAGE_HIZ_CCS_WT] = AUX_CCS_E,
   [ISL_AUX_USAGE_MCS_CCS] = AUX_MCS_LCE,
   [ISL_AUX_USAGE_STC_CCS] = AUX_CCS_E,
};
#elif GFX_VER >= 9
static const uint32_t isl_encode_aux_mode[] = {
   [ISL_AUX_USAGE_NONE] = AUX_NONE,
   [ISL_AUX_USAGE_HIZ] = AUX_HIZ,
   [ISL_AUX_USAGE_MCS] = AUX_CCS_D,
   [ISL_AUX_USAGE_CCS_D] = AUX_CCS_D,
   [ISL_AUX_USAGE_CCS_E] = AUX_CCS_E,
};
#elif GFX_VER >= 8
static const uint32_t isl_encode_aux_mode[] = {
   [ISL_AUX_USAGE_NONE] = AUX_NONE,
   [ISL_AUX_USAGE_HIZ] = AUX_HIZ,
   [ISL_AUX_USAGE_MCS] = AUX_MCS,
   [ISL_AUX_USAGE_CCS_D] = AUX_MCS,
};
#endif

static uint8_t
get_surftype(enum isl_surf_dim dim, isl_surf_usage_flags_t usage)
{
   switch (dim) {
   default:
      unreachable("bad isl_surf_dim");
   case ISL_SURF_DIM_1D:
      assert(!(usage & ISL_SURF_USAGE_CUBE_BIT));
      return SURFTYPE_1D;
   case ISL_SURF_DIM_2D:
      if ((usage & ISL_SURF_USAGE_CUBE_BIT) &&
          (usage & ISL_SURF_USAGE_TEXTURE_BIT)) {
         /* We need SURFTYPE_CUBE to make cube sampling work */
         return SURFTYPE_CUBE;
      } else {
         /* Everything else (render and storage) treat cubes as plain
          * 2D array textures
          */
         return SURFTYPE_2D;
      }
   case ISL_SURF_DIM_3D:
      assert(!(usage & ISL_SURF_USAGE_CUBE_BIT));
      return SURFTYPE_3D;
   }
}

/**
 * Get the horizontal and vertical alignment in the units expected by the
 * hardware.  Note that this does NOT give you the actual hardware enum values
 * but an index into the isl_encode_[hv]align arrays above.
 */
UNUSED static struct isl_extent3d
get_image_alignment(const struct isl_surf *surf)
{
   if (GFX_VERx10 >= 125) {
      if (surf->tiling == ISL_TILING_64) {
         /* The hardware ignores the alignment values. Anyway, the surface's
          * true alignment is likely outside the enum range of HALIGN* and
          * VALIGN*.
          */
         return isl_extent3d(128, 4, 1);
      } else if (isl_format_get_layout(surf->format)->bpb % 3 == 0) {
         /* On XeHP, RENDER_SURFACE_STATE.SurfaceHorizontalAlignment is in
          * units of elements for 24, 48, and 96 bpb formats.
          */
         return isl_surf_get_image_alignment_el(surf);
      } else {
         /* On XeHP, RENDER_SURFACE_STATE.SurfaceHorizontalAlignment is in
          * units of bytes for formats that are powers of two.
          */
         const uint32_t bs = isl_format_get_layout(surf->format)->bpb / 8;
         return isl_extent3d(surf->image_alignment_el.w * bs,
                             surf->image_alignment_el.h,
                             surf->image_alignment_el.d);
      }
   } else if (GFX_VER >= 9) {
      if (isl_tiling_is_std_y(surf->tiling) ||
          surf->dim_layout == ISL_DIM_LAYOUT_GFX9_1D) {
         /* The hardware ignores the alignment values. Anyway, the surface's
          * true alignment is likely outside the enum range of HALIGN* and
          * VALIGN*.
          */
         return isl_extent3d(4, 4, 1);
      } else {
         /* In Skylake, RENDER_SUFFACE_STATE.SurfaceVerticalAlignment is in units
          * of surface elements (not pixels nor samples). For compressed formats,
          * a "surface element" is defined as a compression block.  For example,
          * if SurfaceVerticalAlignment is VALIGN_4 and SurfaceFormat is an ETC2
          * format (ETC2 has a block height of 4), then the vertical alignment is
          * 4 compression blocks or, equivalently, 16 pixels.
          */
         return isl_surf_get_image_alignment_el(surf);
      }
   } else {
      /* Pre-Skylake, RENDER_SUFFACE_STATE.SurfaceVerticalAlignment is in
       * units of surface samples.  For example, if SurfaceVerticalAlignment
       * is VALIGN_4 and the surface is singlesampled, then for any surface
       * format (compressed or not) the vertical alignment is
       * 4 pixels.
       */
      return isl_surf_get_image_alignment_sa(surf);
   }
}

#if GFX_VER >= 8
static uint32_t
get_qpitch(const struct isl_surf *surf)
{
   switch (surf->dim_layout) {
   default:
      unreachable("Bad isl_surf_dim");
   case ISL_DIM_LAYOUT_GFX4_2D:
      if (GFX_VER >= 9) {
         if (surf->dim == ISL_SURF_DIM_3D && surf->tiling == ISL_TILING_W) {
            /* This is rather annoying and completely undocumented.  It
             * appears that the hardware has a bug (or undocumented feature)
             * regarding stencil buffers most likely related to the way
             * W-tiling is handled as modified Y-tiling.  If you bind a 3-D
             * stencil buffer normally, and use texelFetch on it, the z or
             * array index will get implicitly multiplied by 2 for no obvious
             * reason.  The fix appears to be to divide qpitch by 2 for
             * W-tiled surfaces.
             */
            return isl_surf_get_array_pitch_el_rows(surf) / 2;
         } else {
            return isl_surf_get_array_pitch_el_rows(surf);
         }
      } else {
         /* From the Broadwell PRM for RENDER_SURFACE_STATE.QPitch
          *
          *    "This field must be set to an integer multiple of the Surface
          *    Vertical Alignment. For compressed textures (BC*, FXT1,
          *    ETC*, and EAC* Surface Formats), this field is in units of
          *    rows in the uncompressed surface, and must be set to an
          *    integer multiple of the vertical alignment parameter "j"
          *    defined in the Common Surface Formats section."
          */
         return isl_surf_get_array_pitch_sa_rows(surf);
      }
   case ISL_DIM_LAYOUT_GFX9_1D:
      /* QPitch is usually expressed as rows of surface elements (where
       * a surface element is an compression block or a single surface
       * sample). Skylake 1D is an outlier.
       *
       * From the Skylake BSpec >> Memory Views >> Common Surface
       * Formats >> Surface Layout and Tiling >> 1D Surfaces:
       *
       *    Surface QPitch specifies the distance in pixels between array
       *    slices.
       */
      return isl_surf_get_array_pitch_el(surf);
   case ISL_DIM_LAYOUT_GFX4_3D:
      /* QPitch doesn't make sense for ISL_DIM_LAYOUT_GFX4_3D since it uses a
       * different pitch at each LOD.  Also, the QPitch field is ignored for
       * these surfaces.  From the Broadwell PRM documentation for QPitch:
       *
       *    This field specifies the distance in rows between array slices. It
       *    is used only in the following cases:
       *     - Surface Array is enabled OR
       *     - Number of Mulitsamples is not NUMSAMPLES_1 and Multisampled
       *       Surface Storage Format set to MSFMT_MSS OR
       *     - Surface Type is SURFTYPE_CUBE
       *
       * None of the three conditions above can possibly apply to a 3D surface
       * so it is safe to just set QPitch to 0.
       */
      return 0;
   }
}
#endif /* GFX_VER >= 8 */

void
isl_genX(surf_fill_state_s)(const struct isl_device *dev, void *state,
                            const struct isl_surf_fill_state_info *restrict info)
{
   struct GENX(RENDER_SURFACE_STATE) s = { 0 };

   s.SurfaceType = get_surftype(info->surf->dim, info->view->usage);

   if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT)
      assert(isl_format_supports_rendering(dev->info, info->view->format));
   else if (info->view->usage & ISL_SURF_USAGE_TEXTURE_BIT)
      assert(isl_format_supports_sampling(dev->info, info->view->format));

   /* From the Sky Lake PRM Vol. 2d, RENDER_SURFACE_STATE::SurfaceFormat
    *
    *    This field cannot be a compressed (BC*, DXT*, FXT*, ETC*, EAC*)
    *    format if the Surface Type is SURFTYPE_1D
    */
   if (info->surf->dim == ISL_SURF_DIM_1D)
      assert(!isl_format_is_compressed(info->view->format));

   if (isl_format_is_compressed(info->surf->format)) {
      /* You're not allowed to make a view of a compressed format with any
       * format other than the surface format.  None of the userspace APIs
       * allow for this directly and doing so would mess up a number of
       * surface parameters such as Width, Height, and alignments.  Ideally,
       * we'd like to assert that the two formats match.  However, we have an
       * S3TC workaround that requires us to do reinterpretation.  So assert
       * that they're at least the same bpb and block size.
       */
      ASSERTED const struct isl_format_layout *surf_fmtl =
         isl_format_get_layout(info->surf->format);
      ASSERTED const struct isl_format_layout *view_fmtl =
         isl_format_get_layout(info->surf->format);
      assert(surf_fmtl->bpb == view_fmtl->bpb);
      assert(surf_fmtl->bw == view_fmtl->bw);
      assert(surf_fmtl->bh == view_fmtl->bh);
   }

   s.SurfaceFormat = info->view->format;

#if GFX_VER >= 12
   /* The BSpec description of this field says:
    *
    *    "This bit field, when set, indicates if the resource is created as
    *    Depth/Stencil resource."
    *
    *    "SW must set this bit for any resource that was created with
    *    Depth/Stencil resource flag. Setting this bit allows HW to properly
    *    interpret the data-layout for various cases. For any resource that's
    *    created without Depth/Stencil resource flag, it must be reset."
    *
    * Even though the docs for this bit seem to imply that it's required for
    * anything which might have been used for depth/stencil, empirical
    * evidence suggests that it only affects CCS compression usage.  There are
    * a few things which back this up:
    *
    *  1. The docs are also pretty clear that this bit was added as part
    *     of enabling Gfx12 depth/stencil lossless compression.
    *
    *  2. The only new difference between depth/stencil and color images on
    *     Gfx12 (where the bit was added) is how they treat CCS compression.
    *     All other differences such as alignment requirements and MSAA layout
    *     are already covered by other bits.
    *
    * Under these assumptions, it makes sense for ISL to model this bit as
    * being an extension of AuxiliarySurfaceMode where STC_CCS and HIZ_CCS_WT
    * are indicated by AuxiliarySurfaceMode == CCS_E and DepthStencilResource
    * == true.
    */
   s.DepthStencilResource = info->aux_usage == ISL_AUX_USAGE_HIZ_CCS_WT ||
                            info->aux_usage == ISL_AUX_USAGE_STC_CCS;
#endif

#if GFX_VER <= 5
   s.ColorBufferComponentWriteDisables = info->write_disables;
   s.ColorBlendEnable = info->blend_enable;
#else
   assert(info->write_disables == 0);
#endif

#if GFX_VERx10 == 75
   s.IntegerSurfaceFormat =
      isl_format_has_int_channel((enum isl_format) s.SurfaceFormat);
#endif

   assert(info->surf->logical_level0_px.width > 0 &&
          info->surf->logical_level0_px.height > 0);

   s.Width = info->surf->logical_level0_px.width - 1;
   s.Height = info->surf->logical_level0_px.height - 1;

   /* In the gfx6 PRM Volume 1 Part 1: Graphics Core, Section 7.18.3.7.1
    * (Surface Arrays For all surfaces other than separate stencil buffer):
    *
    * "[DevSNB] Errata: Sampler MSAA Qpitch will be 4 greater than the value
    *  calculated in the equation above , for every other odd Surface Height
    *  starting from 1 i.e. 1,5,9,13"
    *
    * Since this Qpitch errata only impacts the sampler, we have to adjust the
    * input for the rendering surface to achieve the same qpitch. For the
    * affected heights, we increment the height by 1 for the rendering
    * surface.
    */
   if (GFX_VER == 6 && (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) &&
       info->surf->samples > 1 &&
       (info->surf->logical_level0_px.height % 4) == 1)
      s.Height++;

   switch (s.SurfaceType) {
   case SURFTYPE_1D:
   case SURFTYPE_2D:
      /* From the Ivy Bridge PRM >> RENDER_SURFACE_STATE::MinimumArrayElement:
       *
       *    "If Number of Multisamples is not MULTISAMPLECOUNT_1, this field
       *    must be set to zero if this surface is used with sampling engine
       *    messages."
       *
       * This restriction appears to exist only on Ivy Bridge.
       */
      if (GFX_VERx10 == 70 && !ISL_DEV_IS_BAYTRAIL(dev) &&
          (info->view->usage & ISL_SURF_USAGE_TEXTURE_BIT) &&
          info->surf->samples > 1)
         assert(info->view->base_array_layer == 0);

      s.MinimumArrayElement = info->view->base_array_layer;

      /* From the Broadwell PRM >> RENDER_SURFACE_STATE::Depth:
       *
       *    For SURFTYPE_1D, 2D, and CUBE: The range of this field is reduced
       *    by one for each increase from zero of Minimum Array Element. For
       *    example, if Minimum Array Element is set to 1024 on a 2D surface,
       *    the range of this field is reduced to [0,1023].
       *
       * In other words, 'Depth' is the number of array layers.
       */
      s.Depth = info->view->array_len - 1;

      /* From the Broadwell PRM >> RENDER_SURFACE_STATE::RenderTargetViewExtent:
       *
       *    For Render Target and Typed Dataport 1D and 2D Surfaces:
       *    This field must be set to the same value as the Depth field.
       */
      if (info->view->usage & (ISL_SURF_USAGE_RENDER_TARGET_BIT |
                               ISL_SURF_USAGE_STORAGE_BIT))
         s.RenderTargetViewExtent = s.Depth;
      break;
   case SURFTYPE_CUBE:
      s.MinimumArrayElement = info->view->base_array_layer;
      /* Same as SURFTYPE_2D, but divided by 6 */
      s.Depth = info->view->array_len / 6 - 1;
      if (info->view->usage & (ISL_SURF_USAGE_RENDER_TARGET_BIT |
                               ISL_SURF_USAGE_STORAGE_BIT))
         s.RenderTargetViewExtent = s.Depth;
      break;
   case SURFTYPE_3D:
      /* From the Broadwell PRM >> RENDER_SURFACE_STATE::Depth:
       *
       *    If the volume texture is MIP-mapped, this field specifies the
       *    depth of the base MIP level.
       */
      s.Depth = info->surf->logical_level0_px.depth - 1;

      /* From the Broadwell PRM >> RENDER_SURFACE_STATE::RenderTargetViewExtent:
       *
       *    For Render Target and Typed Dataport 3D Surfaces: This field
       *    indicates the extent of the accessible 'R' coordinates minus 1 on
       *    the LOD currently being rendered to.
       *
       * The docs specify that this only matters for render targets and
       * surfaces used with typed dataport messages.  Prior to Ivy Bridge, the
       * Depth field has more bits than RenderTargetViewExtent so we can have
       * textures with more levels than we can render to.  In order to prevent
       * assert-failures in the packing function below, we only set the field
       * when it's actually going to be used by the hardware.
       *
       * Similaraly, the MinimumArrayElement field is ignored by all hardware
       * prior to Sky Lake when texturing and we want it set to 0 anyway.
       * Since it's already initialized to 0, we can just leave it alone for
       * texture surfaces.
       */
      if (info->view->usage & (ISL_SURF_USAGE_RENDER_TARGET_BIT |
                               ISL_SURF_USAGE_STORAGE_BIT)) {
         s.MinimumArrayElement = info->view->base_array_layer;
         s.RenderTargetViewExtent = info->view->array_len - 1;
      }
      break;
   default:
      unreachable("bad SurfaceType");
   }

#if GFX_VER >= 12
   /* Wa_1806565034: Only set SurfaceArray if arrayed surface is > 1. */
   s.SurfaceArray = info->surf->dim != ISL_SURF_DIM_3D &&
      info->view->array_len > 1;
#elif GFX_VER >= 7
   s.SurfaceArray = info->surf->dim != ISL_SURF_DIM_3D;
#endif

   if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT) {
      /* For render target surfaces, the hardware interprets field
       * MIPCount/LOD as LOD. The Broadwell PRM says:
       *
       *    MIPCountLOD defines the LOD that will be rendered into.
       *    SurfaceMinLOD is ignored.
       */
      s.MIPCountLOD = info->view->base_level;
      s.SurfaceMinLOD = 0;
   } else {
      /* For non render target surfaces, the hardware interprets field
       * MIPCount/LOD as MIPCount.  The range of levels accessible by the
       * sampler engine is [SurfaceMinLOD, SurfaceMinLOD + MIPCountLOD].
       */
      s.SurfaceMinLOD = info->view->base_level;
      s.MIPCountLOD = MAX(info->view->levels, 1) - 1;
   }

#if GFX_VER >= 9
   /* We don't use miptails yet.  The PRM recommends that you set "Mip Tail
    * Start LOD" to 15 to prevent the hardware from trying to use them.
    */
   s.TiledResourceMode = NONE;
   s.MipTailStartLOD = 15;
#endif

#if GFX_VER >= 6
   const struct isl_extent3d image_align = get_image_alignment(info->surf);
   s.SurfaceVerticalAlignment = isl_encode_valign(image_align.height);
#if GFX_VER >= 7
   s.SurfaceHorizontalAlignment = isl_encode_halign(image_align.width);
#endif
#endif

   if (info->surf->dim_layout == ISL_DIM_LAYOUT_GFX9_1D) {
      /* For gfx9 1-D textures, surface pitch is ignored */
      s.SurfacePitch = 0;
   } else {
      s.SurfacePitch = info->surf->row_pitch_B - 1;
   }

#if GFX_VER >= 8
   s.SurfaceQPitch = get_qpitch(info->surf) >> 2;
#elif GFX_VER == 7
   s.SurfaceArraySpacing = info->surf->array_pitch_span ==
                           ISL_ARRAY_PITCH_SPAN_COMPACT;
#endif

#if GFX_VER >= 8
   assert(GFX_VER < 12 || info->surf->tiling != ISL_TILING_W);
   s.TileMode = isl_encode_tiling[info->surf->tiling];
#else
   s.TiledSurface = info->surf->tiling != ISL_TILING_LINEAR,
   s.TileWalk = info->surf->tiling == ISL_TILING_Y0 ? TILEWALK_YMAJOR :
                                                      TILEWALK_XMAJOR,
#endif

#if GFX_VER >= 8
   s.RenderCacheReadWriteMode = WriteOnlyCache;
#else
   s.RenderCacheReadWriteMode = 0;
#endif

#if GFX_VER >= 11
   /* We've seen dEQP failures when enabling this bit with UINT formats,
    * which particularly affects blorp_copy() operations.  It shouldn't
    * have any effect on UINT textures anyway, so disable it for them.
    */
   s.EnableUnormPathInColorPipe =
      !isl_format_has_int_channel(info->view->format);
#endif

   s.CubeFaceEnablePositiveZ = 1;
   s.CubeFaceEnableNegativeZ = 1;
   s.CubeFaceEnablePositiveY = 1;
   s.CubeFaceEnableNegativeY = 1;
   s.CubeFaceEnablePositiveX = 1;
   s.CubeFaceEnableNegativeX = 1;

#if GFX_VER >= 6
   s.NumberofMultisamples = ffs(info->surf->samples) - 1;
#if GFX_VER >= 7
   s.MultisampledSurfaceStorageFormat =
      isl_encode_multisample_layout[info->surf->msaa_layout];
#endif
#endif

#if (GFX_VERx10 >= 75)
   if (info->view->usage & ISL_SURF_USAGE_RENDER_TARGET_BIT)
      assert(isl_swizzle_supports_rendering(dev->info, info->view->swizzle));

   s.ShaderChannelSelectRed = (enum GENX(ShaderChannelSelect)) info->view->swizzle.r;
   s.ShaderChannelSelectGreen = (enum GENX(ShaderChannelSelect)) info->view->swizzle.g;
   s.ShaderChannelSelectBlue = (enum GENX(ShaderChannelSelect)) info->view->swizzle.b;
   s.ShaderChannelSelectAlpha = (enum GENX(ShaderChannelSelect)) info->view->swizzle.a;
#else
   assert(isl_swizzle_is_identity(info->view->swizzle));
#endif

   s.SurfaceBaseAddress = info->address;

#if GFX_VER >= 6
   s.MOCS = info->mocs;
#endif

#if GFX_VERx10 >= 45
   if (info->x_offset_sa != 0 || info->y_offset_sa != 0) {
      /* There are fairly strict rules about when the offsets can be used.
       * These are mostly taken from the Sky Lake PRM documentation for
       * RENDER_SURFACE_STATE.
       */
      assert(info->surf->tiling != ISL_TILING_LINEAR);
      assert(info->surf->dim == ISL_SURF_DIM_2D);
      assert(isl_is_pow2(isl_format_get_layout(info->view->format)->bpb));
      assert(info->surf->levels == 1);
      assert(info->surf->logical_level0_px.array_len == 1);
      assert(info->aux_usage == ISL_AUX_USAGE_NONE);

      if (GFX_VER >= 8) {
         /* Broadwell added more rules. */
         assert(info->surf->samples == 1);
         if (isl_format_get_layout(info->view->format)->bpb == 8)
            assert(info->x_offset_sa % 16 == 0);
         if (isl_format_get_layout(info->view->format)->bpb == 16)
            assert(info->x_offset_sa % 8 == 0);
      }

#if GFX_VER >= 7
      s.SurfaceArray = false;
#endif
   }

   const unsigned x_div = 4;
   const unsigned y_div = GFX_VER >= 8 ? 4 : 2;
   assert(info->x_offset_sa % x_div == 0);
   assert(info->y_offset_sa % y_div == 0);
   s.XOffset = info->x_offset_sa / x_div;
   s.YOffset = info->y_offset_sa / y_div;
#else
   assert(info->x_offset_sa == 0);
   assert(info->y_offset_sa == 0);
#endif

#if GFX_VER >= 7
   if (info->aux_usage != ISL_AUX_USAGE_NONE) {
      /* Check valid aux usages per-gen */
      if (GFX_VER >= 12) {
         assert(info->aux_usage == ISL_AUX_USAGE_MCS ||
                info->aux_usage == ISL_AUX_USAGE_CCS_E ||
                info->aux_usage == ISL_AUX_USAGE_GFX12_CCS_E ||
                info->aux_usage == ISL_AUX_USAGE_MC ||
                info->aux_usage == ISL_AUX_USAGE_HIZ_CCS_WT ||
                info->aux_usage == ISL_AUX_USAGE_MCS_CCS ||
                info->aux_usage == ISL_AUX_USAGE_STC_CCS);
      } else if (GFX_VER >= 9) {
         assert(info->aux_usage == ISL_AUX_USAGE_HIZ ||
                info->aux_usage == ISL_AUX_USAGE_MCS ||
                info->aux_usage == ISL_AUX_USAGE_CCS_D ||
                info->aux_usage == ISL_AUX_USAGE_CCS_E);
      } else if (GFX_VER >= 8) {
         assert(info->aux_usage == ISL_AUX_USAGE_HIZ ||
                info->aux_usage == ISL_AUX_USAGE_MCS ||
                info->aux_usage == ISL_AUX_USAGE_CCS_D);
      } else if (GFX_VER >= 7) {
         assert(info->aux_usage == ISL_AUX_USAGE_MCS ||
                info->aux_usage == ISL_AUX_USAGE_CCS_D);
      }

      /* The docs don't appear to say anything whatsoever about compression
       * and the data port.  Testing seems to indicate that the data port
       * completely ignores the AuxiliarySurfaceMode field.
       *
       * On gfx12 HDC supports compression.
       */
      if (GFX_VER < 12)
         assert(!(info->view->usage & ISL_SURF_USAGE_STORAGE_BIT));

      if (isl_surf_usage_is_depth(info->surf->usage))
         assert(isl_aux_usage_has_hiz(info->aux_usage));

      if (isl_surf_usage_is_stencil(info->surf->usage))
         assert(info->aux_usage == ISL_AUX_USAGE_STC_CCS);

      if (isl_aux_usage_has_hiz(info->aux_usage)) {
         /* For Gfx8-10, there are some restrictions around sampling from HiZ.
          * The Skylake PRM docs for RENDER_SURFACE_STATE::AuxiliarySurfaceMode
          * say:
          *
          *    "If this field is set to AUX_HIZ, Number of Multisamples must
          *    be MULTISAMPLECOUNT_1, and Surface Type cannot be SURFTYPE_3D."
          *
          * On Gfx12, the docs are a bit less obvious but the restriction is
          * the same.  The limitation isn't called out explicitly but the docs
          * for the CCS_E value of RENDER_SURFACE_STATE::AuxiliarySurfaceMode
          * say:
          *
          *    "If Number of multisamples > 1, programming this value means
          *    MSAA compression is enabled for that surface. Auxillary surface
          *    is MSC with tile y."
          *
          * Since this interpretation ignores whether the surface is
          * depth/stencil or not and since multisampled depth buffers use
          * ISL_MSAA_LAYOUT_INTERLEAVED which is incompatible with MCS
          * compression, this means that we can't even specify MSAA depth CCS
          * in RENDER_SURFACE_STATE::AuxiliarySurfaceMode.
          */
         assert(info->surf->samples == 1);

         /* The dimension must not be 3D */
         assert(info->surf->dim != ISL_SURF_DIM_3D);

         /* The format must be one of the following: */
         switch (info->view->format) {
         case ISL_FORMAT_R32_FLOAT:
         case ISL_FORMAT_R24_UNORM_X8_TYPELESS:
         case ISL_FORMAT_R16_UNORM:
            break;
         default:
            assert(!"Incompatible HiZ Sampling format");
            break;
         }
      }

#if GFX_VERx10 >= 125
      s.RenderCompressionFormat =
         isl_get_render_compression_format(info->surf->format);
#endif
#if GFX_VER >= 12
      s.MemoryCompressionEnable = info->aux_usage == ISL_AUX_USAGE_MC;
#endif
#if GFX_VER >= 8
      s.AuxiliarySurfaceMode = isl_encode_aux_mode[info->aux_usage];
#else
      s.MCSEnable = true;
#endif
   }

   /* The auxiliary buffer info is filled when it's useable by the HW.
    *
    * Starting with Gfx12, the only form of compression that can be used
    * with RENDER_SURFACE_STATE which requires an aux surface is MCS.
    * HiZ still requires a surface but the HiZ surface can only be
    * accessed through 3DSTATE_HIER_DEPTH_BUFFER.
    *
    * On all earlier hardware, an aux surface is required for all forms
    * of compression.
    */
   if ((GFX_VER < 12 && info->aux_usage != ISL_AUX_USAGE_NONE) ||
       (GFX_VER >= 12 && isl_aux_usage_has_mcs(info->aux_usage))) {

      assert(info->aux_surf != NULL);

      struct isl_tile_info tile_info;
      isl_surf_get_tile_info(info->aux_surf, &tile_info);
      uint32_t pitch_in_tiles =
         info->aux_surf->row_pitch_B / tile_info.phys_extent_B.width;

      s.AuxiliarySurfaceBaseAddress = info->aux_address;
      s.AuxiliarySurfacePitch = pitch_in_tiles - 1;

#if GFX_VER >= 8
      /* Auxiliary surfaces in ISL have compressed formats but the hardware
       * doesn't expect our definition of the compression, it expects qpitch
       * in units of samples on the main surface.
       */
      s.AuxiliarySurfaceQPitch =
         isl_surf_get_array_pitch_sa_rows(info->aux_surf) >> 2;
#endif
   }
#endif

#if GFX_VER >= 8 && GFX_VER < 11
   /* From the CHV PRM, Volume 2d, page 321 (RENDER_SURFACE_STATE dword 0
    * bit 9 "Sampler L2 Bypass Mode Disable" Programming Notes):
    *
    *    This bit must be set for the following surface types: BC2_UNORM
    *    BC3_UNORM BC5_UNORM BC5_SNORM BC7_UNORM
    */
   if (GFX_VER >= 9 || dev->info->is_cherryview) {
      switch (info->view->format) {
      case ISL_FORMAT_BC2_UNORM:
      case ISL_FORMAT_BC3_UNORM:
      case ISL_FORMAT_BC5_UNORM:
      case ISL_FORMAT_BC5_SNORM:
      case ISL_FORMAT_BC7_UNORM:
         s.SamplerL2BypassModeDisable = true;
         break;
      default:
         /* From the SKL PRM, Programming Note under Sampler Output Channel
          * Mapping:
          *
          *    If a surface has an associated HiZ Auxilliary surface, the
          *    Sampler L2 Bypass Mode Disable field in the RENDER_SURFACE_STATE
          *    must be set.
          */
         if (GFX_VER >= 9 && info->aux_usage == ISL_AUX_USAGE_HIZ)
            s.SamplerL2BypassModeDisable = true;
         break;
      }
   }
#endif

   if (isl_aux_usage_has_fast_clears(info->aux_usage)) {
      if (info->use_clear_address) {
#if GFX_VER >= 10
         s.ClearValueAddressEnable = true;
         s.ClearValueAddress = info->clear_address;
#else
         unreachable("Gfx9 and earlier do not support indirect clear colors");
#endif
      }

#if GFX_VER == 11
      /*
       * From BXML > GT > Shared Functions > vol5c Shared Functions >
       * [Structure] RENDER_SURFACE_STATE [BDW+] > ClearColorConversionEnable:
       *
       *   Project: Gfx11
       *
       *   "Enables Pixel backend hw to convert clear values into native format
       *    and write back to clear address, so that display and sampler can use
       *    the converted value for resolving fast cleared RTs."
       *
       * Summary:
       *   Clear color conversion must be enabled if the clear color is stored
       *   indirectly and fast color clears are enabled.
       */
      if (info->use_clear_address) {
         s.ClearColorConversionEnable = true;
      }
#endif

#if GFX_VER >= 12
      assert(info->use_clear_address);
#elif GFX_VER >= 9
      if (!info->use_clear_address) {
         s.RedClearColor = info->clear_color.u32[0];
         s.GreenClearColor = info->clear_color.u32[1];
         s.BlueClearColor = info->clear_color.u32[2];
         s.AlphaClearColor = info->clear_color.u32[3];
      }
#elif GFX_VER >= 7
      /* Prior to Sky Lake, we only have one bit for the clear color which
       * gives us 0 or 1 in whatever the surface's format happens to be.
       */
      if (isl_format_has_int_channel(info->view->format)) {
         for (unsigned i = 0; i < 4; i++) {
            assert(info->clear_color.u32[i] == 0 ||
                   info->clear_color.u32[i] == 1);
         }
         s.RedClearColor = info->clear_color.u32[0] != 0;
         s.GreenClearColor = info->clear_color.u32[1] != 0;
         s.BlueClearColor = info->clear_color.u32[2] != 0;
         s.AlphaClearColor = info->clear_color.u32[3] != 0;
      } else {
         for (unsigned i = 0; i < 4; i++) {
            assert(info->clear_color.f32[i] == 0.0f ||
                   info->clear_color.f32[i] == 1.0f);
         }
         s.RedClearColor = info->clear_color.f32[0] != 0.0f;
         s.GreenClearColor = info->clear_color.f32[1] != 0.0f;
         s.BlueClearColor = info->clear_color.f32[2] != 0.0f;
         s.AlphaClearColor = info->clear_color.f32[3] != 0.0f;
      }
#endif
   }

   GENX(RENDER_SURFACE_STATE_pack)(NULL, state, &s);
}

void
isl_genX(buffer_fill_state_s)(const struct isl_device *dev, void *state,
                              const struct isl_buffer_fill_state_info *restrict info)
{
   uint64_t buffer_size = info->size_B;

   /* Uniform and Storage buffers need to have surface size not less that the
    * aligned 32-bit size of the buffer. To calculate the array lenght on
    * unsized arrays in StorageBuffer the last 2 bits store the padding size
    * added to the surface, so we can calculate latter the original buffer
    * size to know the number of elements.
    *
    *  surface_size = isl_align(buffer_size, 4) +
    *                 (isl_align(buffer_size) - buffer_size)
    *
    *  buffer_size = (surface_size & ~3) - (surface_size & 3)
    */
   if ((info->format == ISL_FORMAT_RAW  ||
        info->stride_B < isl_format_get_layout(info->format)->bpb / 8) &&
       !info->is_scratch) {
      assert(info->stride_B == 1);
      uint64_t aligned_size = isl_align(buffer_size, 4);
      buffer_size = aligned_size + (aligned_size - buffer_size);
   }

   uint32_t num_elements = buffer_size / info->stride_B;

   assert(num_elements > 0);
   if (info->format == ISL_FORMAT_RAW) {
      assert(num_elements <= dev->max_buffer_size);
   } else {
      /* From the IVB PRM, SURFACE_STATE::Height,
       *
       *    For typed buffer and structured buffer surfaces, the number
       *    of entries in the buffer ranges from 1 to 2^27.
       */
      assert(num_elements <= (1ull << 27));
   }

   struct GENX(RENDER_SURFACE_STATE) s = { 0, };

   s.SurfaceFormat = info->format;

   s.SurfaceType = SURFTYPE_BUFFER;
#if GFX_VERx10 >= 125
   if (info->is_scratch) {
      /* From the BSpec:
       *
       *    "For surfaces of type SURFTYPE_SCRATCH, valid range of pitch is:
       *    [63,262143] -> [64B, 256KB].  Also, for SURFTYPE_SCRATCH, the
       *    pitch must be a multiple of 64bytes."
       */
      assert(info->format == ISL_FORMAT_RAW);
      assert(info->stride_B % 64 == 0);
      assert(info->stride_B <= 256 * 1024);
      s.SurfaceType = SURFTYPE_SCRATCH;
   }
#else
   assert(!info->is_scratch);
#endif

   s.SurfacePitch = info->stride_B - 1;

#if GFX_VER >= 6
   s.SurfaceVerticalAlignment = isl_encode_valign(4);
#if GFX_VERx10 >= 125
   s.SurfaceHorizontalAlignment = isl_encode_halign(128);
#elif GFX_VER >= 7
   s.SurfaceHorizontalAlignment = isl_encode_halign(4);
   s.SurfaceArray = false;
#endif
#endif

#if GFX_VER >= 7
   s.Height = ((num_elements - 1) >> 7) & 0x3fff;
   s.Width = (num_elements - 1) & 0x7f;
   s.Depth = ((num_elements - 1) >> 21) & 0x3ff;
#else
   s.Height = ((num_elements - 1) >> 7) & 0x1fff;
   s.Width = (num_elements - 1) & 0x7f;
   s.Depth = ((num_elements - 1) >> 20) & 0x7f;
#endif

   if (GFX_VER == 12 && dev->info->revision == 0) {
      /* TGL-LP A0 has a HW bug (fixed in later HW) which causes buffer
       * textures with very close base addresses (delta < 64B) to corrupt each
       * other.  We can sort-of work around this by making small buffer
       * textures 1D textures instead.  This doesn't fix the problem for large
       * buffer textures but the liklihood of large, overlapping, and very
       * close buffer textures is fairly low and the point is to hack around
       * the bug so we can run apps and tests.
       */
       if (info->format != ISL_FORMAT_RAW &&
           info->stride_B == isl_format_get_layout(info->format)->bpb / 8 &&
           num_elements <= (1 << 14)) {
         s.SurfaceType = SURFTYPE_1D;
         s.Width = num_elements - 1;
         s.Height = 0;
         s.Depth = 0;
      }
   }

#if GFX_VER >= 6
   s.NumberofMultisamples = MULTISAMPLECOUNT_1;
#endif

#if (GFX_VER >= 8)
   s.TileMode = LINEAR;
#else
   s.TiledSurface = false;
#endif

#if (GFX_VER >= 8)
   s.RenderCacheReadWriteMode = WriteOnlyCache;
#else
   s.RenderCacheReadWriteMode = 0;
#endif

   s.SurfaceBaseAddress = info->address;
#if GFX_VER >= 6
   s.MOCS = info->mocs;
#endif

#if (GFX_VERx10 >= 75)
   s.ShaderChannelSelectRed = (enum GENX(ShaderChannelSelect)) info->swizzle.r;
   s.ShaderChannelSelectGreen = (enum GENX(ShaderChannelSelect)) info->swizzle.g;
   s.ShaderChannelSelectBlue = (enum GENX(ShaderChannelSelect)) info->swizzle.b;
   s.ShaderChannelSelectAlpha = (enum GENX(ShaderChannelSelect)) info->swizzle.a;
#endif

   GENX(RENDER_SURFACE_STATE_pack)(NULL, state, &s);
}

void
isl_genX(null_fill_state)(void *state,
                          const struct isl_null_fill_state_info *restrict info)
{
   struct GENX(RENDER_SURFACE_STATE) s = {
      .SurfaceType = SURFTYPE_NULL,
      /* We previously had this format set to B8G8R8A8_UNORM but ran into
       * hangs on IVB. R32_UINT seems to work for everybody.
       *
       * https://gitlab.freedesktop.org/mesa/mesa/-/issues/1872
       */
      .SurfaceFormat = ISL_FORMAT_R32_UINT,
#if GFX_VER >= 7
      .SurfaceArray = info->size.depth > 1,
#endif
#if GFX_VERx10 >= 125
      .TileMode = TILE4,
#elif GFX_VER >= 8
      .TileMode = YMAJOR,
#else
      .TiledSurface = true,
      .TileWalk = TILEWALK_YMAJOR,
#endif
#if GFX_VER == 7
      /* According to PRMs: "Volume 4 Part 1: Subsystem and Cores – Shared
       * Functions"
       *
       * RENDER_SURFACE_STATE::Surface Vertical Alignment
       *
       *    "This field must be set to VALIGN_4 for all tiled Y Render Target
       *     surfaces."
       *
       * Affect IVB, HSW.
       */
      .SurfaceVerticalAlignment = VALIGN_4,
#endif
      .MIPCountLOD = info->levels,
      .Width = info->size.width - 1,
      .Height = info->size.height - 1,
      .Depth = info->size.depth - 1,
      .RenderTargetViewExtent = info->size.depth - 1,
#if GFX_VER <= 5
      .MinimumArrayElement = info->minimum_array_element,
      .ColorBufferComponentWriteDisables = 0xf,
#endif
   };
   GENX(RENDER_SURFACE_STATE_pack)(NULL, state, &s);
}
