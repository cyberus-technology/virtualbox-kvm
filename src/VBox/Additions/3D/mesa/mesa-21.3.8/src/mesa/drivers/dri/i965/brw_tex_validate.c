/*
 * Copyright © 2013 Intel Corporation
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
#include "main/macros.h"
#include "main/samplerobj.h"
#include "main/teximage.h"
#include "main/texobj.h"

#include "brw_context.h"
#include "brw_mipmap_tree.h"
#include "brw_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * Sets our driver-specific variant of tObj->_MaxLevel for later surface state
 * upload.
 *
 * If we're only ensuring that there is storage for the first miplevel of a
 * texture, then in texture setup we're going to have to make sure we don't
 * allow sampling beyond level 0.
 */
static void
brw_update_max_level(struct gl_texture_object *tObj,
                     struct gl_sampler_object *sampler)
{
   struct brw_texture_object *brw_obj = brw_texture_object(tObj);

   if (!tObj->_MipmapComplete ||
       (tObj->_RenderToTexture &&
        (sampler->Attrib.MinFilter == GL_NEAREST ||
         sampler->Attrib.MinFilter == GL_LINEAR))) {
      brw_obj->_MaxLevel = tObj->Attrib.BaseLevel;
   } else {
      brw_obj->_MaxLevel = tObj->_MaxLevel;
   }
}

/**
 * At rendering-from-a-texture time, make sure that the texture object has a
 * miptree that can hold the entire texture based on
 * BaseLevel/MaxLevel/filtering, and copy in any texture images that are
 * stored in other miptrees.
 */
void
brw_finalize_mipmap_tree(struct brw_context *brw,
                           struct gl_texture_object *tObj)
{
   struct brw_texture_object *brw_obj = brw_texture_object(tObj);
   GLuint face, i;
   GLuint nr_faces = 0;
   struct brw_texture_image *firstImage;
   int width, height, depth;

   /* TBOs require no validation -- they always just point to their BO. */
   if (tObj->Target == GL_TEXTURE_BUFFER)
      return;

   /* What levels does this validated texture image require? */
   int validate_first_level = tObj->Attrib.BaseLevel;
   int validate_last_level = brw_obj->_MaxLevel;

   /* Skip the loop over images in the common case of no images having
    * changed.  But if the GL_BASE_LEVEL or GL_MAX_LEVEL change to something we
    * haven't looked at, then we do need to look at those new images.
    */
   if (!brw_obj->needs_validate &&
       validate_first_level >= brw_obj->validated_first_level &&
       validate_last_level <= brw_obj->validated_last_level) {
      return;
   }

   /* On recent generations, immutable textures should not get this far
    * -- they should have been created in a validated state, and nothing
    * can invalidate them.
    *
    * Unfortunately, this is not true on pre-Sandybridge hardware -- when
    * rendering into an immutable-format depth texture we may have to rebase
    * the rendered levels to meet alignment requirements.
    *
    * FINISHME: Avoid doing this.
    */
   assert(!tObj->Immutable || brw->screen->devinfo.ver < 6);

   firstImage = brw_texture_image(tObj->Image[0][tObj->Attrib.BaseLevel]);
   if (!firstImage)
      return;

   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    */
   if (brw_obj->mt &&
       (!brw_miptree_match_image(brw_obj->mt, &firstImage->base.Base) ||
        validate_first_level < brw_obj->mt->first_level ||
        validate_last_level > brw_obj->mt->last_level)) {
      brw_miptree_release(&brw_obj->mt);
   }


   /* May need to create a new tree:
    */
   if (!brw_obj->mt) {
      const unsigned level = firstImage->base.Base.Level;
      brw_get_image_dims(&firstImage->base.Base, &width, &height, &depth);
      /* Figure out image dimensions at start level. */
      switch(brw_obj->base.Target) {
      case GL_TEXTURE_2D_MULTISAMPLE:
      case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      case GL_TEXTURE_RECTANGLE:
      case GL_TEXTURE_EXTERNAL_OES:
          assert(level == 0);
          break;
      case GL_TEXTURE_3D:
          depth = depth << level;
          FALLTHROUGH;
      case GL_TEXTURE_2D:
      case GL_TEXTURE_2D_ARRAY:
      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_ARRAY:
          height = height << level;
          FALLTHROUGH;
      case GL_TEXTURE_1D:
      case GL_TEXTURE_1D_ARRAY:
          width = width << level;
          break;
      default:
          unreachable("Unexpected target");
      }
      perf_debug("Creating new %s %dx%dx%d %d-level miptree to handle "
                 "finalized texture miptree.\n",
                 _mesa_get_format_name(firstImage->base.Base.TexFormat),
                 width, height, depth, validate_last_level + 1);

      brw_obj->mt = brw_miptree_create(brw,
                                       brw_obj->base.Target,
                                       firstImage->base.Base.TexFormat,
                                       0, /* first_level */
                                       validate_last_level,
                                       width,
                                       height,
                                       depth,
                                       1 /* num_samples */,
                                       MIPTREE_CREATE_BUSY);
      if (!brw_obj->mt)
         return;
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = _mesa_num_tex_faces(brw_obj->base.Target);
   for (face = 0; face < nr_faces; face++) {
      for (i = validate_first_level; i <= validate_last_level; i++) {
         struct brw_texture_image *brw_image =
            brw_texture_image(brw_obj->base.Image[face][i]);
         /* skip too small size mipmap */
         if (brw_image == NULL)
            break;

         if (brw_obj->mt != brw_image->mt)
            brw_miptree_copy_teximage(brw, brw_image, brw_obj->mt);

         /* After we're done, we'd better agree that our layout is
          * appropriate, or we'll end up hitting this function again on the
          * next draw
          */
         assert(brw_miptree_match_image(brw_obj->mt, &brw_image->base.Base));
      }
   }

   brw_obj->validated_first_level = validate_first_level;
   brw_obj->validated_last_level = validate_last_level;
   brw_obj->_Format = firstImage->base.Base.TexFormat,
   brw_obj->needs_validate = false;
}

/**
 * Finalizes all textures, completing any rendering that needs to be done
 * to prepare them.
 */
void
brw_validate_textures(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const int max_enabled_unit = ctx->Texture._MaxEnabledTexImageUnit;

   for (int unit = 0; unit <= max_enabled_unit; unit++) {
      struct gl_texture_object *tex_obj = ctx->Texture.Unit[unit]._Current;

      if (!tex_obj)
         continue;

      struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);

      /* We know that this is true by now, and if it wasn't, we might have
       * mismatched level sizes and the copies would fail.
       */
      assert(tex_obj->_BaseComplete);

      brw_update_max_level(tex_obj, sampler);
      brw_finalize_mipmap_tree(brw, tex_obj);
   }
}
