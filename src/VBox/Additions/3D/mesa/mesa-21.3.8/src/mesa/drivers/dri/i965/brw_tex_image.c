
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/formats.h"
#include "main/glformats.h"
#include "main/image.h"
#include "main/pbo.h"
#include "main/renderbuffer.h"
#include "main/texcompress.h"
#include "main/texgetimage.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/texstore.h"
#include "main/glthread.h"

#include "drivers/common/meta.h"

#include "brw_mipmap_tree.h"
#include "brw_buffer_objects.h"
#include "brw_batch.h"
#include "brw_tex.h"
#include "brw_fbo.h"
#include "brw_image.h"
#include "brw_context.h"
#include "brw_blorp.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/* Make sure one doesn't end up shrinking base level zero unnecessarily.
 * Determining the base level dimension by shifting higher level dimension
 * ends up in off-by-one value in case base level has NPOT size (for example,
 * 293 != 146 << 1).
 * Choose the original base level dimension when shifted dimensions agree.
 * Otherwise assume real resize is intended and use the new shifted value.
 */
static unsigned
get_base_dim(unsigned old_base_dim, unsigned new_level_dim, unsigned level)
{
   const unsigned old_level_dim = old_base_dim >> level;
   const unsigned new_base_dim = new_level_dim << level;

   return old_level_dim == new_level_dim ? old_base_dim : new_base_dim;
}

/* Work back from the specified level of the image to the baselevel and create a
 * miptree of that size.
 */
struct brw_mipmap_tree *
brw_miptree_create_for_teximage(struct brw_context *brw,
                                struct brw_texture_object *brw_obj,
                                struct brw_texture_image *brw_image,
                                enum brw_miptree_create_flags flags)
{
   GLuint lastLevel;
   int width, height, depth;
   unsigned old_width = 0, old_height = 0, old_depth = 0;
   const struct brw_mipmap_tree *old_mt = brw_obj->mt;
   const unsigned level = brw_image->base.Base.Level;

   brw_get_image_dims(&brw_image->base.Base, &width, &height, &depth);

   if (old_mt) {
      old_width = old_mt->surf.logical_level0_px.width;
      old_height = old_mt->surf.logical_level0_px.height;
      old_depth = old_mt->surf.dim == ISL_SURF_DIM_3D ?
                     old_mt->surf.logical_level0_px.depth :
                     old_mt->surf.logical_level0_px.array_len;
   }

   DBG("%s\n", __func__);

   /* Figure out image dimensions at start level. */
   switch(brw_obj->base.Target) {
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_EXTERNAL_OES:
      assert(level == 0);
      break;
   case GL_TEXTURE_3D:
      depth = old_mt ? get_base_dim(old_depth, depth, level) :
                       depth << level;
      FALLTHROUGH;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      height = old_mt ? get_base_dim(old_height, height, level) :
                        height << level;
      FALLTHROUGH;
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      width = old_mt ? get_base_dim(old_width, width, level) :
                       width << level;
      break;
   default:
      unreachable("Unexpected target");
   }

   /* Guess a reasonable value for lastLevel.  This is probably going
    * to be wrong fairly often and might mean that we have to look at
    * resizable buffers, or require that buffers implement lazy
    * pagetable arrangements.
    */
   if ((brw_obj->base.Sampler.Attrib.MinFilter == GL_NEAREST ||
        brw_obj->base.Sampler.Attrib.MinFilter == GL_LINEAR) &&
       brw_image->base.Base.Level == 0 &&
       !brw_obj->base.Attrib.GenerateMipmap) {
      lastLevel = 0;
   } else {
      lastLevel = _mesa_get_tex_max_num_levels(brw_obj->base.Target,
                                               width, height, depth) - 1;
   }

   return brw_miptree_create(brw,
                             brw_obj->base.Target,
                             brw_image->base.Base.TexFormat,
                             0,
                             lastLevel,
                             width,
                             height,
                             depth,
                             MAX2(brw_image->base.Base.NumSamples, 1),
                             flags);
}

static bool
brw_texsubimage_blorp(struct brw_context *brw, GLuint dims,
                      struct gl_texture_image *tex_image,
                      unsigned x, unsigned y, unsigned z,
                      unsigned width, unsigned height, unsigned depth,
                      GLenum format, GLenum type, const void *pixels,
                      const struct gl_pixelstore_attrib *packing)
{
   struct brw_texture_image *intel_image = brw_texture_image(tex_image);
   const unsigned mt_level = tex_image->Level + tex_image->TexObject->Attrib.MinLevel;
   const unsigned mt_z = tex_image->TexObject->Attrib.MinLayer + tex_image->Face + z;

   /* The blorp path can't understand crazy format hackery */
   if (_mesa_base_tex_format(&brw->ctx, tex_image->InternalFormat) !=
       _mesa_get_format_base_format(tex_image->TexFormat))
      return false;

   return brw_blorp_upload_miptree(brw, intel_image->mt, tex_image->TexFormat,
                                   mt_level, x, y, mt_z, width, height, depth,
                                   tex_image->TexObject->Target, format, type,
                                   pixels, packing);
}

/**
 * \brief A fast path for glTexImage and glTexSubImage.
 *
 * This fast path is taken when the texture format is BGRA, RGBA,
 * A or L and when the texture memory is X- or Y-tiled.  It uploads
 * the texture data by mapping the texture memory without a GTT fence, thus
 * acquiring a tiled view of the memory, and then copying sucessive
 * spans within each tile.
 *
 * This is a performance win over the conventional texture upload path because
 * it avoids the performance penalty of writing through the write-combine
 * buffer. In the conventional texture upload path,
 * texstore.c:store_texsubimage(), the texture memory is mapped through a GTT
 * fence, thus acquiring a linear view of the memory, then each row in the
 * image is memcpy'd. In this fast path, we replace each row's copy with
 * a sequence of copies over each linear span in tile.
 *
 * One use case is Google Chrome's paint rectangles.  Chrome (as
 * of version 21) renders each page as a tiling of 256x256 GL_BGRA textures.
 * Each page's content is initially uploaded with glTexImage2D and damaged
 * regions are updated with glTexSubImage2D. On some workloads, the
 * performance gain of this fastpath on Sandybridge is over 5x.
 */
static bool
brw_texsubimage_tiled_memcpy(struct gl_context * ctx,
                             GLuint dims,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             GLsizei width, GLsizei height, GLsizei depth,
                             GLenum format, GLenum type,
                             const GLvoid *pixels,
                             const struct gl_pixelstore_attrib *packing)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_texture_image *image = brw_texture_image(texImage);
   int src_pitch;

   /* The miptree's buffer. */
   struct brw_bo *bo;

   uint32_t cpp;
   isl_memcpy_type copy_type;

   /* This fastpath is restricted to specific texture types:
    * a 2D BGRA, RGBA, L8 or A8 texture. It could be generalized to support
    * more types.
    *
    * FINISHME: The restrictions below on packing alignment and packing row
    * length are likely unneeded now because we calculate the source stride
    * with _mesa_image_row_stride. However, before removing the restrictions
    * we need tests.
    */
   if (!devinfo->has_llc ||
       !(type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       !(texImage->TexObject->Target == GL_TEXTURE_2D ||
         texImage->TexObject->Target == GL_TEXTURE_RECTANGLE) ||
       pixels == NULL ||
       packing->BufferObj ||
       packing->Alignment > 4 ||
       packing->SkipPixels > 0 ||
       packing->SkipRows > 0 ||
       (packing->RowLength != 0 && packing->RowLength != width) ||
       packing->SwapBytes ||
       packing->LsbFirst ||
       packing->Invert)
      return false;

   /* Only a simple blit, no scale, bias or other mapping. */
   if (ctx->_ImageTransferState)
      return false;

   copy_type = brw_miptree_get_memcpy_type(texImage->TexFormat, format, type,
                                             &cpp);
   if (copy_type == ISL_MEMCPY_INVALID)
      return false;

   /* If this is a nontrivial texture view, let another path handle it instead. */
   if (texImage->TexObject->Attrib.MinLayer)
      return false;

   if (!image->mt ||
       (image->mt->surf.tiling != ISL_TILING_X &&
        image->mt->surf.tiling != ISL_TILING_Y0)) {
      /* The algorithm is written only for X- or Y-tiled memory. */
      return false;
   }

   /* linear_to_tiled() assumes that if the object is swizzled, it is using
    * I915_BIT6_SWIZZLE_9_10 for X and I915_BIT6_SWIZZLE_9 for Y.  This is only
    * true on gfx5 and above.
    *
    * The killer on top is that some gfx4 have an L-shaped swizzle mode, where
    * parts of the memory aren't swizzled at all. Userspace just can't handle
    * that.
    */
   if (devinfo->ver < 5 && brw->has_swizzling)
      return false;

   int level = texImage->Level + texImage->TexObject->Attrib.MinLevel;

   /* Since we are going to write raw data to the miptree, we need to resolve
    * any pending fast color clears before we start.
    */
   assert(image->mt->surf.logical_level0_px.depth == 1);
   assert(image->mt->surf.logical_level0_px.array_len == 1);

   brw_miptree_access_raw(brw, image->mt, level, 0, true);

   bo = image->mt->bo;

   if (brw_batch_references(&brw->batch, bo)) {
      perf_debug("Flushing before mapping a referenced bo.\n");
      brw_batch_flush(brw);
   }

   void *map = brw_bo_map(brw, bo, MAP_WRITE | MAP_RAW);
   if (map == NULL) {
      DBG("%s: failed to map bo\n", __func__);
      return false;
   }

   src_pitch = _mesa_image_row_stride(packing, width, format, type);

   /* We postponed printing this message until having committed to executing
    * the function.
    */
   DBG("%s: level=%d offset=(%d,%d) (w,h)=(%d,%d) format=0x%x type=0x%x "
       "mesa_format=0x%x tiling=%d "
       "packing=(alignment=%d row_length=%d skip_pixels=%d skip_rows=%d) ",
       __func__, texImage->Level, xoffset, yoffset, width, height,
       format, type, texImage->TexFormat, image->mt->surf.tiling,
       packing->Alignment, packing->RowLength, packing->SkipPixels,
       packing->SkipRows);

   /* Adjust x and y offset based on miplevel */
   unsigned level_x, level_y;
   brw_miptree_get_image_offset(image->mt, level, 0, &level_x, &level_y);
   xoffset += level_x;
   yoffset += level_y;

   isl_memcpy_linear_to_tiled(
      xoffset * cpp, (xoffset + width) * cpp,
      yoffset, yoffset + height,
      map,
      pixels,
      image->mt->surf.row_pitch_B, src_pitch,
      brw->has_swizzling,
      image->mt->surf.tiling,
      copy_type
   );

   brw_bo_unmap(bo);
   return true;
}


static void
brw_upload_tex(struct gl_context * ctx,
               GLuint dims,
               struct gl_texture_image *texImage,
               GLint xoffset, GLint yoffset, GLint zoffset,
               GLsizei width, GLsizei height, GLsizei depth,
               GLenum format, GLenum type,
               const GLvoid * pixels,
               const struct gl_pixelstore_attrib *packing)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_mipmap_tree *mt = brw_texture_image(texImage)->mt;
   bool ok;

   /* Check that there is actually data to store. */
   if (pixels == NULL && !packing->BufferObj)
      return;

   bool tex_busy = mt &&
      (brw_batch_references(&brw->batch, mt->bo) || brw_bo_busy(mt->bo));

   if (packing->BufferObj || tex_busy ||
       mt->aux_usage == ISL_AUX_USAGE_CCS_E) {
      ok = brw_texsubimage_blorp(brw, dims, texImage,
                                   xoffset, yoffset, zoffset,
                                   width, height, depth, format, type,
                                   pixels, packing);
      if (ok)
         return;
   }

   ok = brw_texsubimage_tiled_memcpy(ctx, dims, texImage,
                                       xoffset, yoffset, zoffset,
                                       width, height, depth,
                                       format, type, pixels, packing);
   if (ok)
     return;

   _mesa_store_texsubimage(ctx, dims, texImage,
                           xoffset, yoffset, zoffset,
                           width, height, depth,
                           format, type, pixels, packing);
}


static void
brw_teximage(struct gl_context * ctx,
             GLuint dims,
             struct gl_texture_image *texImage,
             GLenum format, GLenum type, const void *pixels,
             const struct gl_pixelstore_attrib *unpack)
{
   DBG("%s mesa_format %s target %s format %s type %s level %d %dx%dx%d\n",
       __func__, _mesa_get_format_name(texImage->TexFormat),
       _mesa_enum_to_string(texImage->TexObject->Target),
       _mesa_enum_to_string(format), _mesa_enum_to_string(type),
       texImage->Level, texImage->Width, texImage->Height, texImage->Depth);

   /* Allocate storage for texture data. */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
      return;
   }

   assert(brw_texture_image(texImage)->mt);

   brw_upload_tex(ctx, dims, texImage, 0, 0, 0,
                    texImage->Width, texImage->Height, texImage->Depth,
                    format, type, pixels, unpack);
}


static void
brw_texsubimage(struct gl_context * ctx,
                GLuint dims,
                struct gl_texture_image *texImage,
                GLint xoffset, GLint yoffset, GLint zoffset,
                GLsizei width, GLsizei height, GLsizei depth,
                GLenum format, GLenum type,
                const GLvoid * pixels,
                const struct gl_pixelstore_attrib *packing)
{
   DBG("%s mesa_format %s target %s format %s type %s level %d %dx%dx%d\n",
       __func__, _mesa_get_format_name(texImage->TexFormat),
       _mesa_enum_to_string(texImage->TexObject->Target),
       _mesa_enum_to_string(format), _mesa_enum_to_string(type),
       texImage->Level, texImage->Width, texImage->Height, texImage->Depth);

   brw_upload_tex(ctx, dims, texImage, xoffset, yoffset, zoffset,
                    width, height, depth, format, type, pixels, packing);
}


static void
brw_set_texture_image_mt(struct brw_context *brw,
                         struct gl_texture_image *image,
                         GLenum internal_format,
                         mesa_format format,
                         struct brw_mipmap_tree *mt)

{
   struct gl_texture_object *texobj = image->TexObject;
   struct brw_texture_object *intel_texobj = brw_texture_object(texobj);
   struct brw_texture_image *intel_image = brw_texture_image(image);

   _mesa_init_teximage_fields(&brw->ctx, image,
                              mt->surf.logical_level0_px.width,
                              mt->surf.logical_level0_px.height, 1,
                              0, internal_format, format);

   brw->ctx.Driver.FreeTextureImageBuffer(&brw->ctx, image);

   intel_texobj->needs_validate = true;
   intel_image->base.RowStride = mt->surf.row_pitch_B / mt->cpp;
   assert(mt->surf.row_pitch_B % mt->cpp == 0);

   brw_miptree_reference(&intel_image->mt, mt);

   /* Immediately validate the image to the object. */
   brw_miptree_reference(&intel_texobj->mt, mt);
}


void
brw_set_texbuffer2(__DRIcontext *pDRICtx, GLint target,
                   GLint texture_format,
                   __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = dPriv->driverPrivate;
   struct brw_context *brw = pDRICtx->driverPrivate;
   struct gl_context *ctx = &brw->ctx;
   struct brw_renderbuffer *rb;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   mesa_format texFormat = MESA_FORMAT_NONE;
   GLenum internal_format = 0;

   _mesa_glthread_finish(ctx);

   texObj = _mesa_get_current_tex_object(ctx, target);

   if (!texObj)
      return;

   if (dPriv->lastStamp != dPriv->dri2.stamp ||
       !pDRICtx->driScreenPriv->dri2.useInvalidate)
      brw_update_renderbuffers(pDRICtx, dPriv);

   rb = brw_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   /* If the miptree isn't set, then intel_update_renderbuffers was unable
    * to get the BO for the drawable from the window system.
    */
   if (!rb || !rb->mt)
      return;

   /* Neither the EGL and GLX texture_from_pixmap specs say anything about
    * sRGB.  They are both from a time where sRGB was considered an extra
    * encoding step you did as part of rendering/blending and not a format.
    * Even though we have concept of sRGB visuals, X has classically assumed
    * that your data is just bits and sRGB rendering is entirely a client-side
    * rendering construct.  The assumption is that the result of BindTexImage
    * is a texture with a linear format even if it was rendered with sRGB
    * encoding enabled.
    */
   texFormat = _mesa_get_srgb_format_linear(brw_rb_format(rb));

   if (rb->mt->cpp == 4) {
      /* The extra texture_format parameter indicates whether the alpha
       * channel should be respected or ignored.  If we set internal_format to
       * GL_RGB, the texture handling code is smart enough to swap the format
       * or apply a swizzle if the underlying format is RGBA so we don't need
       * to stomp it to RGBX or anything like that.
       */
      if (texture_format == __DRI_TEXTURE_FORMAT_RGB)
         internal_format = GL_RGB;
      else
         internal_format = GL_RGBA;
   } else if (rb->mt->cpp == 2) {
      internal_format = GL_RGB;
   }

   brw_miptree_finish_external(brw, rb->mt);

   _mesa_lock_texture(&brw->ctx, texObj);
   texImage = _mesa_get_tex_image(ctx, texObj, target, 0);
   brw_set_texture_image_mt(brw, texImage, internal_format,
                              texFormat, rb->mt);
   _mesa_unlock_texture(&brw->ctx, texObj);
}

void
brw_release_texbuffer(__DRIcontext *pDRICtx, GLint target,
                      __DRIdrawable *dPriv)
{
   struct brw_context *brw = pDRICtx->driverPrivate;
   struct gl_context *ctx = &brw->ctx;
   struct gl_texture_object *tex_obj;
   struct brw_texture_object *intel_tex;

   tex_obj = _mesa_get_current_tex_object(ctx, target);
   if (!tex_obj)
      return;

   _mesa_lock_texture(&brw->ctx, tex_obj);

   intel_tex = brw_texture_object(tex_obj);
   if (!intel_tex->mt) {
      _mesa_unlock_texture(&brw->ctx, tex_obj);
      return;
   }

   /* The brw_miptree_prepare_external below as well as the finish_external
    * above in brw_set_texbuffer2 *should* do nothing.  The BindTexImage call
    * from both GLX and EGL has TexImage2D and not TexSubImage2D semantics so
    * the texture is not immutable.  This means that the user cannot create a
    * texture view of the image with a different format.  Since the only three
    * formats available when using BindTexImage are all UNORM, we can never
    * end up with an sRGB format being used for texturing and so we shouldn't
    * get any format-related resolves when texturing from it.
    *
    * While very unlikely, it is possible that the client could use the bound
    * texture with GL_ARB_image_load_store.  In that case, we'll do a resolve
    * but that's not actually a problem as it just means that we lose
    * compression on this texture until the next time it's used as a render
    * target.
    *
    * The only other way we could end up with an unexpected aux usage would be
    * if we rendered to the image from the same context as we have it bound as
    * a texture between BindTexImage and ReleaseTexImage.  However, the spec
    * clearly calls this case out and says you shouldn't do that.  It doesn't
    * explicitly prevent binding the texture to a framebuffer but it says the
    * results of trying to render to it while bound are undefined.
    *
    * Just to keep everything safe and sane, we do a prepare_external but it
    * should be a no-op in almost all cases.  On the off chance that someone
    * ever triggers this, we should at least warn them.
    */
   if (intel_tex->mt->aux_buf &&
       brw_miptree_get_aux_state(intel_tex->mt, 0, 0) !=
       isl_drm_modifier_get_default_aux_state(intel_tex->mt->drm_modifier)) {
      _mesa_warning(ctx, "Aux state changed between BindTexImage and "
                         "ReleaseTexImage.  Most likely someone tried to draw "
                         "to the pixmap bound in BindTexImage or used it with "
                         "image_load_store.");
   }

   brw_miptree_prepare_external(brw, intel_tex->mt);

   _mesa_unlock_texture(&brw->ctx, tex_obj);
}

static GLboolean
brw_bind_renderbuffer_tex_image(struct gl_context *ctx,
                                struct gl_renderbuffer *rb,
                                struct gl_texture_image *image)
{
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   struct brw_texture_image *intel_image = brw_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct brw_texture_object *intel_texobj = brw_texture_object(texobj);

   /* We can only handle RB allocated with AllocRenderbufferStorage, or
    * window-system renderbuffers.
    */
   assert(!rb->TexImage);

   if (!irb->mt)
      return false;

   _mesa_lock_texture(ctx, texobj);
   _mesa_init_teximage_fields(ctx, image, rb->Width, rb->Height, 1, 0,
                              rb->InternalFormat, rb->Format);
   image->NumSamples = rb->NumSamples;

   brw_miptree_reference(&intel_image->mt, irb->mt);

   /* Immediately validate the image to the object. */
   brw_miptree_reference(&intel_texobj->mt, intel_image->mt);

   intel_texobj->needs_validate = true;
   _mesa_unlock_texture(ctx, texobj);

   return true;
}

void
brw_set_texbuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
   /* The old interface didn't have the format argument, so copy our
    * implementation's behavior at the time.
    */
   brw_set_texbuffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

static void
brw_image_target_texture(struct gl_context *ctx, GLenum target,
                         struct gl_texture_object *texObj,
                         struct gl_texture_image *texImage,
                         GLeglImageOES image_handle,
                         bool storage)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_mipmap_tree *mt;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   __DRIimage *image;

   image = dri_screen->dri2.image->lookupEGLImage(dri_screen, image_handle,
                                                  dri_screen->loaderPrivate);
   if (image == NULL)
      return;

   /* Disallow depth/stencil textures: we don't have a way to pass the
    * separate stencil miptree of a GL_DEPTH_STENCIL texture through.
    */
   if (image->has_depthstencil) {
      _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
      return;
   }

   mt = brw_miptree_create_for_dri_image(brw, image, target, image->format,
                                           false);
   if (mt == NULL)
      return;

   struct brw_texture_object *intel_texobj = brw_texture_object(texObj);
   intel_texobj->planar_format = image->planar_format;
   intel_texobj->yuv_color_space = image->yuv_color_space;

   GLenum internal_format =
      image->internal_format != 0 ?
      image->internal_format : _mesa_get_format_base_format(mt->format);

   /* Fix the internal format when _mesa_get_format_base_format(mt->format)
    * isn't a valid one for that particular format.
    */
   if (brw->mesa_format_supports_render[image->format]) {
      if (image->format == MESA_FORMAT_R10G10B10A2_UNORM ||
          image->format == MESA_FORMAT_R10G10B10X2_UNORM ||
          image->format == MESA_FORMAT_B10G10R10A2_UNORM ||
          image->format == MESA_FORMAT_B10G10R10X2_UNORM)
         internal_format = GL_RGB10_A2;
   }

   /* Guess sized internal format for dma-bufs, as specified by
    * EXT_EGL_image_storage.
    */
   if (storage && target == GL_TEXTURE_2D && image->imported_dmabuf) {
      internal_format = driGLFormatToSizedInternalGLFormat(image->format);
      if (internal_format == GL_NONE) {
         _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
         return;
      }
   }

   brw_set_texture_image_mt(brw, texImage, internal_format, mt->format, mt);
   brw_miptree_release(&mt);
}

static void
brw_image_target_texture_2d(struct gl_context *ctx, GLenum target,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage,
                            GLeglImageOES image_handle)
{
   brw_image_target_texture(ctx, target, texObj, texImage, image_handle,
                              false);
}

static void
brw_image_target_tex_storage(struct gl_context *ctx, GLenum target,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage,
                             GLeglImageOES image_handle)
{
   struct brw_texture_object *intel_texobj = brw_texture_object(texObj);
   brw_image_target_texture(ctx, target, texObj, texImage, image_handle,
                              true);

   /* The miptree is in a validated state, so no need to check later. */
   intel_texobj->needs_validate = false;
   intel_texobj->validated_first_level = 0;
   intel_texobj->validated_last_level = 0;
   intel_texobj->_Format = texImage->TexFormat;
}

static bool
brw_gettexsubimage_blorp(struct brw_context *brw,
                         struct gl_texture_image *tex_image,
                         unsigned x, unsigned y, unsigned z,
                         unsigned width, unsigned height, unsigned depth,
                         GLenum format, GLenum type, const void *pixels,
                         const struct gl_pixelstore_attrib *packing)
{
   struct brw_texture_image *intel_image = brw_texture_image(tex_image);
   const unsigned mt_level = tex_image->Level + tex_image->TexObject->Attrib.MinLevel;
   const unsigned mt_z = tex_image->TexObject->Attrib.MinLayer + tex_image->Face + z;

   /* The blorp path can't understand crazy format hackery */
   if (_mesa_base_tex_format(&brw->ctx, tex_image->InternalFormat) !=
       _mesa_get_format_base_format(tex_image->TexFormat))
      return false;

   return brw_blorp_download_miptree(brw, intel_image->mt,
                                     tex_image->TexFormat, SWIZZLE_XYZW,
                                     mt_level, x, y, mt_z,
                                     width, height, depth,
                                     tex_image->TexObject->Target,
                                     format, type, false, pixels, packing);
}

/**
 * \brief A fast path for glGetTexImage.
 *
 * \see brw_readpixels_tiled_memcpy()
 */
static bool
brw_gettexsubimage_tiled_memcpy(struct gl_context *ctx,
                                struct gl_texture_image *texImage,
                                GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height,
                                GLenum format, GLenum type,
                                GLvoid *pixels,
                                const struct gl_pixelstore_attrib *packing)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_texture_image *image = brw_texture_image(texImage);
   int dst_pitch;

   /* The miptree's buffer. */
   struct brw_bo *bo;

   uint32_t cpp;
   isl_memcpy_type copy_type;

   /* This fastpath is restricted to specific texture types:
    * a 2D BGRA, RGBA, L8 or A8 texture. It could be generalized to support
    * more types.
    *
    * FINISHME: The restrictions below on packing alignment and packing row
    * length are likely unneeded now because we calculate the destination stride
    * with _mesa_image_row_stride. However, before removing the restrictions
    * we need tests.
    */
   if (!devinfo->has_llc ||
       !(type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       !(texImage->TexObject->Target == GL_TEXTURE_2D ||
         texImage->TexObject->Target == GL_TEXTURE_RECTANGLE) ||
       pixels == NULL ||
       packing->BufferObj ||
       packing->Alignment > 4 ||
       packing->SkipPixels > 0 ||
       packing->SkipRows > 0 ||
       (packing->RowLength != 0 && packing->RowLength != width) ||
       packing->SwapBytes ||
       packing->LsbFirst ||
       packing->Invert)
      return false;

   /* We can't handle copying from RGBX or BGRX because the tiled_memcpy
    * function doesn't set the last channel to 1. Note this checks BaseFormat
    * rather than TexFormat in case the RGBX format is being simulated with an
    * RGBA format.
    */
   if (texImage->_BaseFormat == GL_RGB)
      return false;

   copy_type = brw_miptree_get_memcpy_type(texImage->TexFormat, format, type,
                                             &cpp);
   if (copy_type == ISL_MEMCPY_INVALID)
      return false;

   /* If this is a nontrivial texture view, let another path handle it instead. */
   if (texImage->TexObject->Attrib.MinLayer)
      return false;

   if (!image->mt ||
       (image->mt->surf.tiling != ISL_TILING_X &&
        image->mt->surf.tiling != ISL_TILING_Y0)) {
      /* The algorithm is written only for X- or Y-tiled memory. */
      return false;
   }

   /* tiled_to_linear() assumes that if the object is swizzled, it is using
    * I915_BIT6_SWIZZLE_9_10 for X and I915_BIT6_SWIZZLE_9 for Y.  This is only
    * true on gfx5 and above.
    *
    * The killer on top is that some gfx4 have an L-shaped swizzle mode, where
    * parts of the memory aren't swizzled at all. Userspace just can't handle
    * that.
    */
   if (devinfo->ver < 5 && brw->has_swizzling)
      return false;

   int level = texImage->Level + texImage->TexObject->Attrib.MinLevel;

   /* Since we are going to write raw data to the miptree, we need to resolve
    * any pending fast color clears before we start.
    */
   assert(image->mt->surf.logical_level0_px.depth == 1);
   assert(image->mt->surf.logical_level0_px.array_len == 1);

   brw_miptree_access_raw(brw, image->mt, level, 0, true);

   bo = image->mt->bo;

   if (brw_batch_references(&brw->batch, bo)) {
      perf_debug("Flushing before mapping a referenced bo.\n");
      brw_batch_flush(brw);
   }

   void *map = brw_bo_map(brw, bo, MAP_READ | MAP_RAW);
   if (map == NULL) {
      DBG("%s: failed to map bo\n", __func__);
      return false;
   }

   dst_pitch = _mesa_image_row_stride(packing, width, format, type);

   DBG("%s: level=%d x,y=(%d,%d) (w,h)=(%d,%d) format=0x%x type=0x%x "
       "mesa_format=0x%x tiling=%d "
       "packing=(alignment=%d row_length=%d skip_pixels=%d skip_rows=%d)\n",
       __func__, texImage->Level, xoffset, yoffset, width, height,
       format, type, texImage->TexFormat, image->mt->surf.tiling,
       packing->Alignment, packing->RowLength, packing->SkipPixels,
       packing->SkipRows);

   /* Adjust x and y offset based on miplevel */
   unsigned level_x, level_y;
   brw_miptree_get_image_offset(image->mt, level, 0, &level_x, &level_y);
   xoffset += level_x;
   yoffset += level_y;

   isl_memcpy_tiled_to_linear(
      xoffset * cpp, (xoffset + width) * cpp,
      yoffset, yoffset + height,
      pixels,
      map,
      dst_pitch, image->mt->surf.row_pitch_B,
      brw->has_swizzling,
      image->mt->surf.tiling,
      copy_type
   );

   brw_bo_unmap(bo);
   return true;
}

static void
brw_get_tex_sub_image(struct gl_context *ctx,
                      GLint xoffset, GLint yoffset, GLint zoffset,
                      GLsizei width, GLsizei height, GLint depth,
                      GLenum format, GLenum type, GLvoid *pixels,
                      struct gl_texture_image *texImage)
{
   struct brw_context *brw = brw_context(ctx);
   bool ok;

   DBG("%s\n", __func__);

   if (ctx->Pack.BufferObj) {
      if (brw_gettexsubimage_blorp(brw, texImage,
                                     xoffset, yoffset, zoffset,
                                     width, height, depth, format, type,
                                     pixels, &ctx->Pack))
         return;

      perf_debug("%s: fallback to CPU mapping in PBO case\n", __func__);
   }

   ok = brw_gettexsubimage_tiled_memcpy(ctx, texImage, xoffset, yoffset,
                                          width, height,
                                          format, type, pixels, &ctx->Pack);

   if(ok)
      return;

   _mesa_meta_GetTexSubImage(ctx, xoffset, yoffset, zoffset,
                             width, height, depth,
                             format, type, pixels, texImage);

   DBG("%s - DONE\n", __func__);
}

static void
flush_astc_denorms(struct gl_context *ctx, GLuint dims,
                   struct gl_texture_image *texImage,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth)
{
   struct compressed_pixelstore store;
   _mesa_compute_compressed_pixelstore(dims, texImage->TexFormat,
                                       width, height, depth,
                                       &ctx->Unpack, &store);

   for (int slice = 0; slice < store.CopySlices; slice++) {

      /* Map dest texture buffer */
      GLubyte *dstMap;
      GLint dstRowStride;
      ctx->Driver.MapTextureImage(ctx, texImage, slice + zoffset,
                                  xoffset, yoffset, width, height,
                                  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                                  &dstMap, &dstRowStride);
      if (!dstMap)
         continue;

      for (int i = 0; i < store.CopyRowsPerSlice; i++) {

         /* An ASTC block is stored in little endian mode. The byte that
          * contains bits 0..7 is stored at the lower address in memory.
          */
         struct astc_void_extent {
            uint16_t header : 12;
            uint16_t dontcare[3];
            uint16_t R;
            uint16_t G;
            uint16_t B;
            uint16_t A;
         } *blocks = (struct astc_void_extent*) dstMap;

         /* Iterate over every copied block in the row */
         for (int j = 0; j < store.CopyBytesPerRow / 16; j++) {

            /* Check if the header matches that of an LDR void-extent block */
            if (blocks[j].header == 0xDFC) {

               /* Flush UNORM16 values that would be denormalized */
               if (blocks[j].A < 4) blocks[j].A = 0;
               if (blocks[j].B < 4) blocks[j].B = 0;
               if (blocks[j].G < 4) blocks[j].G = 0;
               if (blocks[j].R < 4) blocks[j].R = 0;
            }
         }

         dstMap += dstRowStride;
      }

      ctx->Driver.UnmapTextureImage(ctx, texImage, slice + zoffset);
   }
}


static void
brw_compressedtexsubimage(struct gl_context *ctx, GLuint dims,
                          struct gl_texture_image *texImage,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format,
                          GLsizei imageSize, const GLvoid *data)
{
   /* Upload the compressed data blocks */
   _mesa_store_compressed_texsubimage(ctx, dims, texImage,
                                      xoffset, yoffset, zoffset,
                                      width, height, depth,
                                      format, imageSize, data);

   /* Fix up copied ASTC blocks if necessary */
   GLenum gl_format = _mesa_compressed_format_to_glenum(ctx,
                                                        texImage->TexFormat);
   bool is_linear_astc = _mesa_is_astc_format(gl_format) &&
                        !_mesa_is_srgb_format(gl_format);
   struct brw_context *brw = (struct brw_context*) ctx;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   if (devinfo->ver == 9 &&
       !intel_device_info_is_9lp(devinfo) &&
       is_linear_astc)
      flush_astc_denorms(ctx, dims, texImage,
                         xoffset, yoffset, zoffset,
                         width, height, depth);
}

void
brw_init_texture_image_functions(struct dd_function_table *functions)
{
   functions->TexImage = brw_teximage;
   functions->TexSubImage = brw_texsubimage;
   functions->CompressedTexSubImage = brw_compressedtexsubimage;
   functions->EGLImageTargetTexture2D = brw_image_target_texture_2d;
   functions->EGLImageTargetTexStorage = brw_image_target_tex_storage;
   functions->BindRenderbufferTexImage = brw_bind_renderbuffer_tex_image;
   functions->GetTexSubImage = brw_get_tex_sub_image;
}
