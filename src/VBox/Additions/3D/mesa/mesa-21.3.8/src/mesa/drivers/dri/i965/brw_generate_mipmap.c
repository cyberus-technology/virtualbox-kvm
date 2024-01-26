/*
 * Copyright © 2016 Intel Corporation
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

#include "main/mipmap.h"
#include "main/teximage.h"
#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_tex.h"
#include "drivers/common/meta.h"

#define FILE_DEBUG_FLAG DEBUG_BLORP


/**
 * The GenerateMipmap() driver hook.
 */
void
brw_generate_mipmap(struct gl_context *ctx, GLenum target,
                    struct gl_texture_object *tex_obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_texture_object *intel_obj = brw_texture_object(tex_obj);
   const unsigned base_level = tex_obj->Attrib.BaseLevel;
   unsigned last_level, first_layer, last_layer;

   /* Blorp doesn't handle combined depth/stencil surfaces on Gfx4-5 yet. */
   if (devinfo->ver <= 5 &&
       (tex_obj->Image[0][base_level]->_BaseFormat == GL_DEPTH_COMPONENT ||
        tex_obj->Image[0][base_level]->_BaseFormat == GL_DEPTH_STENCIL)) {
      _mesa_meta_GenerateMipmap(ctx, target, tex_obj);
      return;
   }

   /* find expected last mipmap level to generate */
   last_level = _mesa_compute_num_levels(ctx, tex_obj, target) - 1;

   if (last_level == 0)
      return;

   /* The texture isn't in a "complete" state yet so set the expected
    * last_level here; we're not going through normal texture validation.
    */
   intel_obj->_MaxLevel = last_level;

   if (!tex_obj->Immutable) {
      _mesa_prepare_mipmap_levels(ctx, tex_obj, base_level, last_level);

      /* At this point, memory for all the texture levels has been
       * allocated.  However, the base level image may be in one resource
       * while the subsequent/smaller levels may be in another resource.
       * Finalizing the texture will copy the base images from the former
       * resource to the latter.
       *
       * After this, we'll have all mipmap levels in one resource.
       */
      brw_finalize_mipmap_tree(brw, tex_obj);
   }

   struct brw_mipmap_tree *mt = intel_obj->mt;
   if (!mt) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "mipmap generation");
      return;
   }

   const mesa_format format = intel_obj->_Format;

   /* Fall back to the CPU for non-renderable cases.
    *
    * TODO: 3D textures require blending data from multiple slices,
    * which means we need custom shaders.  For now, fall back.
    */
   if (!brw->mesa_format_supports_render[format] || target == GL_TEXTURE_3D) {
      _mesa_generate_mipmap(ctx, target, tex_obj);
      return;
   }

   const struct isl_extent4d *base_size = &mt->surf.logical_level0_px;

   if (mt->target == GL_TEXTURE_CUBE_MAP) {
      first_layer = _mesa_tex_target_to_face(target);
      last_layer = first_layer;
   } else {
      first_layer = 0;
      last_layer = base_size->array_len - 1;
   }

   /* The GL_EXT_texture_sRGB_decode extension's issues section says:
    *
    *    "10) How is mipmap generation of sRGB textures affected by the
    *     TEXTURE_SRGB_DECODE_EXT parameter?
    *
    *     RESOLVED:  When the TEXTURE_SRGB_DECODE parameter is DECODE_EXT
    *     for an sRGB texture, mipmap generation should decode sRGB texels
    *     to a linear RGB color space, perform downsampling, then encode
    *     back to an sRGB color space.  (Issue 24 in the EXT_texture_sRGB
    *     specification provides a rationale for why.)  When the parameter
    *     is SKIP_DECODE_EXT instead, mipmap generation skips the encode
    *     and decode steps during mipmap generation.  By skipping the
    *     encode and decode steps, sRGB mipmap generation should match
    *     the mipmap generation for a non-sRGB texture."
    */
   bool do_srgb = tex_obj->Sampler.Attrib.sRGBDecode == GL_DECODE_EXT;

   for (unsigned dst_level = base_level + 1;
        dst_level <= last_level;
        dst_level++) {

      const unsigned src_level = dst_level - 1;

      for (unsigned layer = first_layer; layer <= last_layer; layer++) {
         brw_blorp_blit_miptrees(brw, mt, src_level, layer, format,
                                 SWIZZLE_XYZW, mt, dst_level, layer, format,
                                 0, 0,
                                 minify(base_size->width, src_level),
                                 minify(base_size->height, src_level),
                                 0, 0,
                                 minify(base_size->width, dst_level),
                                 minify(base_size->height, dst_level),
                                 GL_LINEAR, false, false,
                                 do_srgb, do_srgb);
      }
   }
}
