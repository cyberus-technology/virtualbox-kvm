#include "swrast/swrast.h"
#include "main/renderbuffer.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/mipmap.h"
#include "drivers/common/meta.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_buffer_objects.h"
#include "brw_mipmap_tree.h"
#include "brw_tex.h"
#include "brw_fbo.h"
#include "brw_state.h"
#include "util/u_memory.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static struct gl_texture_image *
brw_new_texture_image(struct gl_context *ctx)
{
   DBG("%s\n", __func__);
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(brw_texture_image);
}

static void
brw_delete_texture_image(struct gl_context *ctx, struct gl_texture_image *img)
{
   /* nothing special (yet) for brw_texture_image */
   _mesa_delete_texture_image(ctx, img);
}


static struct gl_texture_object *
brw_new_texture_object(struct gl_context *ctx, GLuint name, GLenum target)
{
   struct brw_texture_object *obj = CALLOC_STRUCT(brw_texture_object);

   (void) ctx;

   DBG("%s\n", __func__);

   if (obj == NULL)
      return NULL;

   _mesa_initialize_texture_object(ctx, &obj->base, name, target);

   obj->needs_validate = true;

   return &obj->base;
}

static void
brw_delete_texture_object(struct gl_context *ctx,
                          struct gl_texture_object *texObj)
{
   struct brw_texture_object *brw_obj = brw_texture_object(texObj);

   brw_miptree_release(&brw_obj->mt);
   _mesa_delete_texture_object(ctx, texObj);
}

static GLboolean
brw_alloc_texture_image_buffer(struct gl_context *ctx,
                               struct gl_texture_image *image)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_texture_image *intel_image = brw_texture_image(image);
   struct gl_texture_object *texobj = image->TexObject;
   struct brw_texture_object *intel_texobj = brw_texture_object(texobj);

   assert(image->Border == 0);

   /* Quantize sample count */
   if (image->NumSamples) {
      image->NumSamples = brw_quantize_num_samples(brw->screen, image->NumSamples);
      if (!image->NumSamples)
         return false;
   }

   /* Because the driver uses AllocTextureImageBuffer() internally, it may end
    * up mismatched with FreeTextureImageBuffer(), but that is safe to call
    * multiple times.
    */
   ctx->Driver.FreeTextureImageBuffer(ctx, image);

   if (!_swrast_init_texture_image(image))
      return false;

   if (intel_texobj->mt &&
       brw_miptree_match_image(intel_texobj->mt, image)) {
      brw_miptree_reference(&intel_image->mt, intel_texobj->mt);
      DBG("%s: alloc obj %p level %d %dx%dx%d using object's miptree %p\n",
          __func__, texobj, image->Level,
          image->Width, image->Height, image->Depth, intel_texobj->mt);
   } else {
      intel_image->mt = brw_miptree_create_for_teximage(brw, intel_texobj,
                                                          intel_image,
                                                          MIPTREE_CREATE_DEFAULT);
      if (!intel_image->mt)
         return false;

      /* Even if the object currently has a mipmap tree associated
       * with it, this one is a more likely candidate to represent the
       * whole object since our level didn't fit what was there
       * before, and any lower levels would fit into our miptree.
       */
      brw_miptree_reference(&intel_texobj->mt, intel_image->mt);

      DBG("%s: alloc obj %p level %d %dx%dx%d using new miptree %p\n",
          __func__, texobj, image->Level,
          image->Width, image->Height, image->Depth, intel_image->mt);
   }

   intel_texobj->needs_validate = true;

   return true;
}

/**
 * ctx->Driver.AllocTextureStorage() handler.
 *
 * Compare this to _mesa_AllocTextureStorage_sw, which would call into
 * brw_alloc_texture_image_buffer() above.
 */
static GLboolean
brw_alloc_texture_storage(struct gl_context *ctx,
                          struct gl_texture_object *texobj,
                          GLsizei levels, GLsizei width,
                          GLsizei height, GLsizei depth)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_texture_object *intel_texobj = brw_texture_object(texobj);
   struct gl_texture_image *first_image = texobj->Image[0][0];
   int num_samples = brw_quantize_num_samples(brw->screen,
                                                first_image->NumSamples);
   const int numFaces = _mesa_num_tex_faces(texobj->Target);
   int face;
   int level;

   /* If the object's current miptree doesn't match what we need, make a new
    * one.
    */
   if (!intel_texobj->mt ||
       !brw_miptree_match_image(intel_texobj->mt, first_image) ||
       intel_texobj->mt->last_level != levels - 1) {
      brw_miptree_release(&intel_texobj->mt);

      brw_get_image_dims(first_image, &width, &height, &depth);
      intel_texobj->mt = brw_miptree_create(brw, texobj->Target,
                                              first_image->TexFormat,
                                              0, levels - 1,
                                              width, height, depth,
                                              MAX2(num_samples, 1),
                                              MIPTREE_CREATE_DEFAULT);

      if (intel_texobj->mt == NULL) {
         return false;
      }
   }

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < levels; level++) {
         struct gl_texture_image *image = texobj->Image[face][level];
         struct brw_texture_image *intel_image = brw_texture_image(image);

         image->NumSamples = num_samples;

         _swrast_free_texture_image_buffer(ctx, image);
         if (!_swrast_init_texture_image(image))
            return false;

         brw_miptree_reference(&intel_image->mt, intel_texobj->mt);
      }
   }

   /* The miptree is in a validated state, so no need to check later. */
   intel_texobj->needs_validate = false;
   intel_texobj->validated_first_level = 0;
   intel_texobj->validated_last_level = levels - 1;
   intel_texobj->_Format = first_image->TexFormat;

   return true;
}


static void
brw_free_texture_image_buffer(struct gl_context * ctx,
                              struct gl_texture_image *texImage)
{
   struct brw_texture_image *brw_image = brw_texture_image(texImage);

   DBG("%s\n", __func__);

   brw_miptree_release(&brw_image->mt);

   _swrast_free_texture_image_buffer(ctx, texImage);
}

/**
 * Map texture memory/buffer into user space.
 * Note: the region of interest parameters are ignored here.
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 * \param mapOut  returns start of mapping of region of interest
 * \param rowStrideOut  returns row stride in bytes
 */
static void
brw_map_texture_image(struct gl_context *ctx,
                      struct gl_texture_image *tex_image,
                      GLuint slice,
                      GLuint x, GLuint y, GLuint w, GLuint h,
                      GLbitfield mode,
                      GLubyte **map,
                      GLint *out_stride)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_texture_image *intel_image = brw_texture_image(tex_image);
   struct brw_mipmap_tree *mt = intel_image->mt;
   ptrdiff_t stride;

   /* Our texture data is always stored in a miptree. */
   assert(mt);

   /* Check that our caller wasn't confused about how to map a 1D texture. */
   assert(tex_image->TexObject->Target != GL_TEXTURE_1D_ARRAY || h == 1);

   /* brw_miptree_map operates on a unified "slice" number that references the
    * cube face, since it's all just slices to the miptree code.
    */
   if (tex_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      slice = tex_image->Face;

   brw_miptree_map(brw, mt,
                     tex_image->Level + tex_image->TexObject->Attrib.MinLevel,
                     slice + tex_image->TexObject->Attrib.MinLayer,
                     x, y, w, h, mode,
                     (void **)map, &stride);

   *out_stride = stride;
}

static void
brw_unmap_texture_image(struct gl_context *ctx,
                        struct gl_texture_image *tex_image, GLuint slice)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_texture_image *intel_image = brw_texture_image(tex_image);
   struct brw_mipmap_tree *mt = intel_image->mt;

   if (tex_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      slice = tex_image->Face;

   brw_miptree_unmap(brw, mt,
         tex_image->Level + tex_image->TexObject->Attrib.MinLevel,
         slice + tex_image->TexObject->Attrib.MinLayer);
}

static GLboolean
brw_texture_view(struct gl_context *ctx,
                 struct gl_texture_object *texObj,
                 struct gl_texture_object *origTexObj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_texture_object *intel_tex = brw_texture_object(texObj);
   struct brw_texture_object *intel_orig_tex = brw_texture_object(origTexObj);

   assert(intel_orig_tex->mt);
   brw_miptree_reference(&intel_tex->mt, intel_orig_tex->mt);

   /* Since we can only make views of immutable-format textures,
    * we can assume that everything is in origTexObj's miptree.
    *
    * Mesa core has already made us a copy of all the teximage objects,
    * except it hasn't copied our mt pointers, etc.
    */
   const int numFaces = _mesa_num_tex_faces(texObj->Target);
   const int numLevels = texObj->Attrib.NumLevels;

   int face;
   int level;

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < numLevels; level++) {
         struct gl_texture_image *image = texObj->Image[face][level];
         struct brw_texture_image *intel_image = brw_texture_image(image);

         brw_miptree_reference(&intel_image->mt, intel_orig_tex->mt);
      }
   }

   /* The miptree is in a validated state, so no need to check later. */
   intel_tex->needs_validate = false;
   intel_tex->validated_first_level = 0;
   intel_tex->validated_last_level = numLevels - 1;

   /* Set the validated texture format, with the same adjustments that
    * would have been applied to determine the underlying texture's
    * mt->format.
    */
   intel_tex->_Format = brw_depth_format_for_depthstencil_format(
         brw_lower_compressed_format(brw, texObj->Image[0][0]->TexFormat));

   return GL_TRUE;
}

static void
brw_texture_barrier(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->ver >= 6) {
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                  PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                  PIPE_CONTROL_CS_STALL);

      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE);
   } else {
      brw_emit_mi_flush(brw);
   }
}

/* Return the usual surface usage flags for the given format. */
static isl_surf_usage_flags_t
isl_surf_usage(mesa_format format)
{
   switch(_mesa_get_format_base_format(format)) {
   case GL_DEPTH_COMPONENT:
      return ISL_SURF_USAGE_DEPTH_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
   case GL_DEPTH_STENCIL:
      return ISL_SURF_USAGE_DEPTH_BIT | ISL_SURF_USAGE_STENCIL_BIT |
             ISL_SURF_USAGE_TEXTURE_BIT;
   case GL_STENCIL_INDEX:
      return ISL_SURF_USAGE_STENCIL_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
   default:
      return ISL_SURF_USAGE_RENDER_TARGET_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
   }
}

static GLboolean
intel_texture_for_memory_object(struct gl_context *ctx,
                                          struct gl_texture_object *tex_obj,
                                          struct gl_memory_object *mem_obj,
                                          GLsizei levels, GLsizei width,
                                          GLsizei height, GLsizei depth,
                                          GLuint64 offset)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_memory_object *intel_memobj = brw_memory_object(mem_obj);
   struct brw_texture_object *intel_texobj = brw_texture_object(tex_obj);
   struct gl_texture_image *image = tex_obj->Image[0][0];
   struct isl_surf surf;

   /* Only color formats are supported. */
   if (!_mesa_is_format_color_format(image->TexFormat))
      return GL_FALSE;

   isl_tiling_flags_t tiling_flags = ISL_TILING_ANY_MASK;
   if (tex_obj->TextureTiling == GL_LINEAR_TILING_EXT)
      tiling_flags = ISL_TILING_LINEAR_BIT;

   UNUSED const bool isl_surf_created_successfully =
      isl_surf_init(&brw->screen->isl_dev, &surf,
                    .dim = get_isl_surf_dim(tex_obj->Target),
                    .format = brw_isl_format_for_mesa_format(image->TexFormat),
                    .width = width,
                    .height = height,
                    .depth = depth,
                    .levels = levels,
                    .array_len = tex_obj->Target == GL_TEXTURE_3D ? 1 : depth,
                    .samples = MAX2(image->NumSamples, 1),
                    .usage = isl_surf_usage(image->TexFormat),
                    .tiling_flags = tiling_flags);

   assert(isl_surf_created_successfully);

   intel_texobj->mt = brw_miptree_create_for_bo(brw,
                                                intel_memobj->bo,
                                                image->TexFormat,
                                                offset,
                                                width,
                                                height,
                                                depth,
                                                surf.row_pitch_B,
                                                surf.tiling,
                                                MIPTREE_CREATE_NO_AUX);
   assert(intel_texobj->mt);
   brw_alloc_texture_image_buffer(ctx, image);

   intel_texobj->needs_validate = false;
   intel_texobj->validated_first_level = 0;
   intel_texobj->validated_last_level = levels - 1;
   intel_texobj->_Format = image->TexFormat;

   return GL_TRUE;
}

void
brw_init_texture_functions(struct dd_function_table *functions)
{
   functions->NewTextureObject = brw_new_texture_object;
   functions->NewTextureImage = brw_new_texture_image;
   functions->DeleteTextureImage = brw_delete_texture_image;
   functions->DeleteTexture = brw_delete_texture_object;
   functions->AllocTextureImageBuffer = brw_alloc_texture_image_buffer;
   functions->FreeTextureImageBuffer = brw_free_texture_image_buffer;
   functions->AllocTextureStorage = brw_alloc_texture_storage;
   functions->MapTextureImage = brw_map_texture_image;
   functions->UnmapTextureImage = brw_unmap_texture_image;
   functions->TextureView = brw_texture_view;
   functions->TextureBarrier = brw_texture_barrier;
   functions->SetTextureStorageForMemoryObject = intel_texture_for_memory_object;
}
