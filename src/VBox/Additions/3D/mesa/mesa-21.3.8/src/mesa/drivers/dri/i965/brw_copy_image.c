/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 Intel Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Jason Ekstrand <jason.ekstrand@intel.com>
 */

#include "brw_blorp.h"
#include "brw_fbo.h"
#include "brw_tex.h"
#include "brw_blit.h"
#include "brw_mipmap_tree.h"
#include "main/formats.h"
#include "main/teximage.h"
#include "drivers/common/meta.h"

static void
copy_miptrees(struct brw_context *brw,
              struct brw_mipmap_tree *src_mt,
              int src_x, int src_y, int src_z, unsigned src_level,
              struct brw_mipmap_tree *dst_mt,
              int dst_x, int dst_y, int dst_z, unsigned dst_level,
              int src_width, int src_height)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->ver <= 5) {
      /* On gfx4-5, try BLT first.
       *
       * Gfx4-5 have a single ring for both 3D and BLT operations, so there's
       * no inter-ring synchronization issues like on Gfx6+.  It is apparently
       * faster than using the 3D pipeline.  Original Gfx4 also has to rebase
       * and copy miptree slices in order to render to unaligned locations.
       */
      if (brw_miptree_copy(brw, src_mt, src_level, src_z, src_x, src_y,
                             dst_mt, dst_level, dst_z, dst_x, dst_y,
                             src_width, src_height))
         return;
   }

   brw_blorp_copy_miptrees(brw,
                           src_mt, src_level, src_z,
                           dst_mt, dst_level, dst_z,
                           src_x, src_y, dst_x, dst_y,
                           src_width, src_height);
}

static void
brw_copy_image_sub_data(struct gl_context *ctx,
                        struct gl_texture_image *src_image,
                        struct gl_renderbuffer *src_renderbuffer,
                        int src_x, int src_y, int src_z,
                        struct gl_texture_image *dst_image,
                        struct gl_renderbuffer *dst_renderbuffer,
                        int dst_x, int dst_y, int dst_z,
                        int src_width, int src_height)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_mipmap_tree *src_mt, *dst_mt;
   unsigned src_level, dst_level;

   if (src_image) {
      src_mt = brw_texture_image(src_image)->mt;
      src_level = src_image->Level + src_image->TexObject->Attrib.MinLevel;

      /* Cube maps actually have different images per face */
      if (src_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
         src_z = src_image->Face;

      src_z += src_image->TexObject->Attrib.MinLayer;
   } else {
      assert(src_renderbuffer);
      src_mt = brw_renderbuffer(src_renderbuffer)->mt;
      src_image = src_renderbuffer->TexImage;
      src_level = 0;
   }

   if (dst_image) {
      dst_mt = brw_texture_image(dst_image)->mt;

      dst_level = dst_image->Level + dst_image->TexObject->Attrib.MinLevel;

      /* Cube maps actually have different images per face */
      if (dst_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
         dst_z = dst_image->Face;

      dst_z += dst_image->TexObject->Attrib.MinLayer;
   } else {
      assert(dst_renderbuffer);
      dst_mt = brw_renderbuffer(dst_renderbuffer)->mt;
      dst_image = dst_renderbuffer->TexImage;
      dst_level = 0;
   }

   copy_miptrees(brw, src_mt, src_x, src_y, src_z, src_level,
                 dst_mt, dst_x, dst_y, dst_z, dst_level,
                 src_width, src_height);

   /* CopyImage only works for equal formats, texture view equivalence
    * classes, and a couple special cases for compressed textures.
    *
    * Notably, GL_DEPTH_STENCIL does not appear in any equivalence
    * classes, so we know the formats must be the same, and thus both
    * will either have stencil, or not.  They can't be mismatched.
    */
   assert((src_mt->stencil_mt != NULL) == (dst_mt->stencil_mt != NULL));

   if (dst_mt->stencil_mt) {
      copy_miptrees(brw, src_mt->stencil_mt, src_x, src_y, src_z, src_level,
                    dst_mt->stencil_mt, dst_x, dst_y, dst_z, dst_level,
                    src_width, src_height);
   }
}

void
brw_init_copy_image_functions(struct dd_function_table *functions)
{
   functions->CopyImageSubData = brw_copy_image_sub_data;
}
