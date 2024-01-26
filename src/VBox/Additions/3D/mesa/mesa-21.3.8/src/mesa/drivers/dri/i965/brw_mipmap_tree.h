/*
 * Copyright 2006 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** @file intel_mipmap_tree.h
 *
 * This file defines the structure that wraps a BO and describes how the
 * mipmap levels and slices of a texture are laid out.
 *
 * The hardware has a fixed layout of a texture depending on parameters such
 * as the target/type (2D, 3D, CUBE), width, height, pitch, and number of
 * mipmap levels.  The individual level/layer slices are each 2D rectangles of
 * pixels at some x/y offset from the start of the brw_bo.
 *
 * Original OpenGL allowed texture miplevels to be specified in arbitrary
 * order, and a texture may change size over time.  Thus, each
 * brw_texture_image has a reference to a miptree that contains the pixel
 * data sized appropriately for it, which will later be referenced by/copied
 * to the brw_texture_object at draw time (brw_finalize_mipmap_tree()) so
 * that there's a single miptree for the complete texture.
 */

#ifndef BRW_MIPMAP_TREE_H
#define BRW_MIPMAP_TREE_H

#include <assert.h>

#include "main/mtypes.h"
#include "isl/isl.h"
#include "blorp/blorp.h"
#include "brw_bufmgr.h"
#include "brw_context.h"
#include <GL/internal/dri_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

struct brw_context;
struct brw_renderbuffer;

struct brw_texture_image;

/**
 * This bit extends the set of GL_MAP_*_BIT enums.
 *
 * When calling brw_miptree_map() on an ETC-transcoded-to-RGB miptree or a
 * depthstencil-split-to-separate-stencil miptree, we'll normally make a
 * temporary and recreate the kind of data requested by Mesa core, since we're
 * satisfying some glGetTexImage() request or something.
 *
 * However, occasionally you want to actually map the miptree's current data
 * without transcoding back.  This flag to brw_miptree_map() gets you that.
 */
#define BRW_MAP_DIRECT_BIT 0x80000000

struct brw_miptree_map {
   /** Bitfield of GL_MAP_*_BIT and BRW_MAP_*_BIT. */
   GLbitfield mode;
   /** Region of interest for the map. */
   int x, y, w, h;
   /** Possibly malloced temporary buffer for the mapping. */
   void *buffer;
   /** Possible pointer to a temporary linear miptree for the mapping. */
   struct brw_mipmap_tree *linear_mt;
   /** Pointer to the start of (map_x, map_y) returned by the mapping. */
   void *ptr;
   /** Stride of the mapping. */
   int stride;

   void (*unmap)(struct brw_context *brw,
                 struct brw_mipmap_tree *mt,
                 struct brw_miptree_map *map,
                 unsigned int level,
                 unsigned int slice);
};

/**
 * Describes the location of each texture image within a miptree.
 */
struct brw_mipmap_level
{
   /** Offset to this miptree level, used in computing x_offset. */
   GLuint level_x;
   /** Offset to this miptree level, used in computing y_offset. */
   GLuint level_y;

   /**
    * \brief Is HiZ enabled for this level?
    *
    * If \c mt->level[l].has_hiz is set, then (1) \c mt->hiz_mt has been
    * allocated and (2) the HiZ memory for the slices in this level reside at
    * \c mt->hiz_mt->level[l].
    */
   bool has_hiz;

   /**
    * \brief List of 2D images in this mipmap level.
    *
    * This may be a list of cube faces, array slices in 2D array texture, or
    * layers in a 3D texture. The list's length is \c depth.
    */
   struct brw_mipmap_slice {
      /**
       * Mapping information. Persistent for the duration of
       * brw_miptree_map/unmap on this slice.
       */
      struct brw_miptree_map *map;
   } *slice;
};

/**
 * Miptree aux buffer. These buffers are associated with a miptree, but the
 * format is managed by the hardware.
 *
 * For Gfx7+, we always give the hardware the start of the buffer, and let it
 * handle all accesses to the buffer. Therefore we don't need the full miptree
 * layout structure for this buffer.
 */
struct brw_miptree_aux_buffer
{
   struct isl_surf surf;

   /**
    * Buffer object containing the pixel data.
    *
    * @see RENDER_SURFACE_STATE.AuxiliarySurfaceBaseAddress
    * @see 3DSTATE_HIER_DEPTH_BUFFER.AuxiliarySurfaceBaseAddress
    */
   struct brw_bo *bo;

   /**
    * Offset into bo where the surface starts.
    *
    * @see brw_mipmap_aux_buffer::bo
    *
    * @see RENDER_SURFACE_STATE.AuxiliarySurfaceBaseAddress
    * @see 3DSTATE_DEPTH_BUFFER.SurfaceBaseAddress
    * @see 3DSTATE_HIER_DEPTH_BUFFER.SurfaceBaseAddress
    * @see 3DSTATE_STENCIL_BUFFER.SurfaceBaseAddress
    */
   uint32_t offset;

   /**
    * Buffer object containing the indirect clear color.
    *
    * @see create_ccs_buf_for_image
    * @see RENDER_SURFACE_STATE.ClearValueAddress
    */
   struct brw_bo *clear_color_bo;

   /**
    * Offset into bo where the clear color can be found.
    *
    * @see create_ccs_buf_for_image
    * @see RENDER_SURFACE_STATE.ClearValueAddress
    */
   uint32_t clear_color_offset;
};

struct brw_mipmap_tree
{
   struct isl_surf surf;

   /**
    * Buffer object containing the surface.
    *
    * @see brw_mipmap_tree::offset
    * @see RENDER_SURFACE_STATE.SurfaceBaseAddress
    * @see RENDER_SURFACE_STATE.AuxiliarySurfaceBaseAddress
    * @see 3DSTATE_DEPTH_BUFFER.SurfaceBaseAddress
    * @see 3DSTATE_HIER_DEPTH_BUFFER.SurfaceBaseAddress
    * @see 3DSTATE_STENCIL_BUFFER.SurfaceBaseAddress
    */
   struct brw_bo *bo;

   /**
    * @brief One of GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, etc.
    *
    * @see RENDER_SURFACE_STATE.SurfaceType
    * @see RENDER_SURFACE_STATE.SurfaceArray
    * @see 3DSTATE_DEPTH_BUFFER.SurfaceType
    */
   GLenum target;

   /**
    * Generally, this is just the same as the gl_texture_image->TexFormat or
    * gl_renderbuffer->Format.
    *
    * However, for textures and renderbuffers with packed depth/stencil formats
    * on hardware where we want or need to use separate stencil, there will be
    * two miptrees for storing the data.  If the depthstencil texture or rb is
    * MESA_FORMAT_Z32_FLOAT_S8X24_UINT, then mt->format will be
    * MESA_FORMAT_Z_FLOAT32, otherwise for MESA_FORMAT_Z24_UNORM_S8_UINT objects it will be
    * MESA_FORMAT_Z24_UNORM_X8_UINT.
    *
    * @see RENDER_SURFACE_STATE.SurfaceFormat
    * @see 3DSTATE_DEPTH_BUFFER.SurfaceFormat
    */
   mesa_format format;

   GLuint first_level;
   GLuint last_level;

   /** Bytes per pixel (or bytes per block if compressed) */
   GLuint cpp;

   bool compressed;

   /* Includes image offset tables: */
   struct brw_mipmap_level level[MAX_TEXTURE_LEVELS];

   /**
    * Offset into bo where the surface starts.
    *
    * @see brw_mipmap_tree::bo
    *
    * @see RENDER_SURFACE_STATE.AuxiliarySurfaceBaseAddress
    * @see 3DSTATE_DEPTH_BUFFER.SurfaceBaseAddress
    * @see 3DSTATE_HIER_DEPTH_BUFFER.SurfaceBaseAddress
    * @see 3DSTATE_STENCIL_BUFFER.SurfaceBaseAddress
    */
   uint32_t offset;

   /**
    * \brief The type of auxiliary compression used by this miptree.
    *
    * This describes the type of auxiliary compression that is intended to be
    * used by this miptree.  An aux usage of ISL_AUX_USAGE_NONE means that
    * auxiliary compression is permanently disabled.  An aux usage other than
    * ISL_AUX_USAGE_NONE does not imply that the auxiliary buffer has actually
    * been allocated nor does it imply that auxiliary compression will always
    * be enabled for this surface.  For instance, with CCS_D, we may allocate
    * the CCS on-the-fly and it may not be used for texturing if the miptree
    * is fully resolved.
    */
   enum isl_aux_usage aux_usage;

   /**
    * \brief Whether or not this miptree supports fast clears.
    */
   bool supports_fast_clear;

   /**
    * \brief Maps miptree slices to their current aux state
    *
    * This two-dimensional array is indexed as [level][layer] and stores an
    * aux state for each slice.
    */
   enum isl_aux_state **aux_state;

   /**
    * \brief Stencil miptree for depthstencil textures.
    *
    * This miptree is used for depthstencil textures and renderbuffers that
    * require separate stencil.  It always has the true copy of the stencil
    * bits, regardless of mt->format.
    *
    * \see 3DSTATE_STENCIL_BUFFER
    * \see brw_miptree_map_depthstencil()
    * \see brw_miptree_unmap_depthstencil()
    */
   struct brw_mipmap_tree *stencil_mt;

   /**
    * \brief Shadow miptree for sampling when the main isn't supported by HW.
    *
    * To workaround various sampler bugs and limitations, we blit the main
    * texture into a new texture that can be sampled.
    *
    * This miptree may be used for:
    * - Stencil texturing (pre-BDW) as required by GL_ARB_stencil_texturing.
    * - To store the decompressed ETC/EAC data in case we emulate the ETC
    *   compression on Gen 7 or earlier GPUs.
    */
   struct brw_mipmap_tree *shadow_mt;
   bool shadow_needs_update;

   /**
    * \brief CCS, MCS, or HiZ auxiliary buffer.
    *
    * NULL if no auxiliary buffer is in use for this surface.
    *
    * For single-sampled color miptrees:
    *    This buffer contains the Color Control Surface, which stores the
    *    necessary information to implement lossless color compression (CCS_E)
    *    and "fast color clear" (CCS_D) behaviour.
    *
    * For multi-sampled color miptrees:
    *    This buffer contains the Multisample Control Surface, which stores the
    *    necessary information to implement compressed MSAA
    *    (INTEL_MSAA_FORMAT_CMS).
    *
    * For depth miptrees:
    *    This buffer contains the Hierarchical Depth Buffer, which stores the
    *    necessary information to implement lossless depth compression and fast
    *    depth clear behavior.
    *
    *    To determine if HiZ is enabled, do not check this pointer. Instead,
    *    use brw_miptree_level_has_hiz().
    */
   struct brw_miptree_aux_buffer *aux_buf;

   /**
    * Planes 1 and 2 in case this is a planar surface.
    */
   struct brw_mipmap_tree *plane[2];

   /**
    * Fast clear color for this surface.  For depth surfaces, the clear value
    * is stored as a float32 in the red component.
    */
   union isl_color_value fast_clear_color;

   /**
    * For external surfaces, this is DRM format modifier that was used to
    * create or import the surface.  For internal surfaces, this will always
    * be DRM_FORMAT_MOD_INVALID.
    */
   uint64_t drm_modifier;

   /* These are also refcounted:
    */
   GLuint refcount;
};

bool
brw_miptree_alloc_aux(struct brw_context *brw,
                        struct brw_mipmap_tree *mt);

enum brw_miptree_create_flags {
   /** No miptree create flags */
   MIPTREE_CREATE_DEFAULT  = 0,

   /** Miptree creation should try to allocate a currently busy BO
    *
    * This may be advantageous if we know the next thing to touch the BO will
    * be the GPU because the BO will likely already be in the GTT and maybe
    * even in some caches.  If there is a chance that the next thing to touch
    * the miptree BO will be the CPU, this flag should not be set.
    */
   MIPTREE_CREATE_BUSY     = 1 << 0,

   /** Create the miptree with auxiliary compression disabled
    *
    * This does not prevent the caller of brw_miptree_create from coming
    * along later and turning auxiliary compression back on but it does mean
    * that the miptree will be created with mt->aux_usage == NONE.
    */
   MIPTREE_CREATE_NO_AUX   = 1 << 1,
};

struct brw_mipmap_tree *brw_miptree_create(struct brw_context *brw,
                                           GLenum target,
                                           mesa_format format,
                                           GLuint first_level,
                                           GLuint last_level,
                                           GLuint width0,
                                           GLuint height0,
                                           GLuint depth0,
                                           GLuint num_samples,
                                           enum brw_miptree_create_flags flags);

struct brw_mipmap_tree *
brw_miptree_create_for_bo(struct brw_context *brw,
                          struct brw_bo *bo,
                          mesa_format format,
                          uint32_t offset,
                          uint32_t width,
                          uint32_t height,
                          uint32_t depth,
                          int pitch,
                          enum isl_tiling tiling,
                          enum brw_miptree_create_flags flags);

struct brw_mipmap_tree *
brw_miptree_create_for_dri_image(struct brw_context *brw,
                                 __DRIimage *image,
                                 GLenum target,
                                 mesa_format format,
                                 bool allow_internal_aux);

bool
brw_update_winsys_renderbuffer_miptree(struct brw_context *intel,
                                         struct brw_renderbuffer *irb,
                                         struct brw_mipmap_tree *singlesample_mt,
                                         uint32_t width, uint32_t height,
                                         uint32_t pitch);

/**
 * Create a miptree appropriate as the storage for a non-texture renderbuffer.
 * The miptree has the following properties:
 *     - The target is GL_TEXTURE_2D.
 *     - There are no levels other than the base level 0.
 *     - Depth is 1.
 */
struct brw_mipmap_tree*
brw_miptree_create_for_renderbuffer(struct brw_context *brw,
                                    mesa_format format,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t num_samples);

mesa_format
brw_depth_format_for_depthstencil_format(mesa_format format);

mesa_format
brw_lower_compressed_format(struct brw_context *brw, mesa_format format);

unsigned
brw_get_num_logical_layers(const struct brw_mipmap_tree *mt, unsigned level);

/** \brief Assert that the level and layer are valid for the miptree. */
void
brw_miptree_check_level_layer(const struct brw_mipmap_tree *mt,
                                uint32_t level,
                                uint32_t layer);

void brw_miptree_reference(struct brw_mipmap_tree **dst,
                           struct brw_mipmap_tree *src);

void brw_miptree_release(struct brw_mipmap_tree **mt);

/* Check if an image fits an existing mipmap tree layout
 */
bool brw_miptree_match_image(struct brw_mipmap_tree *mt,
                                    struct gl_texture_image *image);

void
brw_miptree_get_image_offset(const struct brw_mipmap_tree *mt,
                             GLuint level, GLuint slice,
                             GLuint *x, GLuint *y);

enum isl_surf_dim
get_isl_surf_dim(GLenum target);

enum isl_dim_layout
get_isl_dim_layout(const struct intel_device_info *devinfo,
                   enum isl_tiling tiling, GLenum target);

void
brw_get_image_dims(struct gl_texture_image *image,
                     int *width, int *height, int *depth);

uint32_t
brw_miptree_get_tile_offsets(const struct brw_mipmap_tree *mt,
                               GLuint level, GLuint slice,
                               uint32_t *tile_x,
                               uint32_t *tile_y);
uint32_t
brw_miptree_get_aligned_offset(const struct brw_mipmap_tree *mt,
                                 uint32_t x, uint32_t y);

void
brw_miptree_copy_slice(struct brw_context *brw,
                         struct brw_mipmap_tree *src_mt,
                         unsigned src_level, unsigned src_layer,
                         struct brw_mipmap_tree *dst_mt,
                         unsigned dst_level, unsigned dst_layer);

void
brw_miptree_copy_teximage(struct brw_context *brw,
                            struct brw_texture_image *brw_image,
                            struct brw_mipmap_tree *dst_mt);

/**
 * \name Miptree HiZ functions
 * \{
 *
 * It is safe to call the "slice_set_need_resolve" and "slice_resolve"
 * functions on a miptree without HiZ. In that case, each function is a no-op.
 */

bool
brw_miptree_level_has_hiz(const struct brw_mipmap_tree *mt, uint32_t level);

/**\}*/

bool
brw_miptree_has_color_unresolved(const struct brw_mipmap_tree *mt,
                                   unsigned start_level, unsigned num_levels,
                                   unsigned start_layer, unsigned num_layers);


#define INTEL_REMAINING_LAYERS UINT32_MAX
#define INTEL_REMAINING_LEVELS UINT32_MAX

/** Prepare a miptree for access
 *
 * This function should be called prior to any access to miptree in order to
 * perform any needed resolves.
 *
 * \param[in]  start_level    The first mip level to be accessed
 *
 * \param[in]  num_levels     The number of miplevels to be accessed or
 *                            INTEL_REMAINING_LEVELS to indicate every level
 *                            above start_level will be accessed
 *
 * \param[in]  start_layer    The first array slice or 3D layer to be accessed
 *
 * \param[in]  num_layers     The number of array slices or 3D layers be
 *                            accessed or INTEL_REMAINING_LAYERS to indicate
 *                            every layer above start_layer will be accessed
 *
 * \param[in]  aux_supported  Whether or not the access will support the
 *                            miptree's auxiliary compression format;  this
 *                            must be false for uncompressed miptrees
 *
 * \param[in]  fast_clear_supported Whether or not the access will support
 *                                  fast clears in the miptree's auxiliary
 *                                  compression format
 */
void
brw_miptree_prepare_access(struct brw_context *brw,
                             struct brw_mipmap_tree *mt,
                             uint32_t start_level, uint32_t num_levels,
                             uint32_t start_layer, uint32_t num_layers,
                             enum isl_aux_usage aux_usage,
                             bool fast_clear_supported);

/** Complete a write operation
 *
 * This function should be called after any operation writes to a miptree.
 * This will update the miptree's compression state so that future resolves
 * happen correctly.  Technically, this function can be called before the
 * write occurs but the caller must ensure that they don't interlace
 * brw_miptree_prepare_access and brw_miptree_finish_write calls to
 * overlapping layer/level ranges.
 *
 * \param[in]  level             The mip level that was written
 *
 * \param[in]  start_layer       The first array slice or 3D layer written
 *
 * \param[in]  num_layers        The number of array slices or 3D layers
 *                               written or INTEL_REMAINING_LAYERS to indicate
 *                               every layer above start_layer was written
 *
 * \param[in]  written_with_aux  Whether or not the write was done with
 *                               auxiliary compression enabled
 */
void
brw_miptree_finish_write(struct brw_context *brw,
                           struct brw_mipmap_tree *mt, uint32_t level,
                           uint32_t start_layer, uint32_t num_layers,
                           enum isl_aux_usage aux_usage);

/** Get the auxiliary compression state of a miptree slice */
enum isl_aux_state
brw_miptree_get_aux_state(const struct brw_mipmap_tree *mt,
                            uint32_t level, uint32_t layer);

/** Set the auxiliary compression state of a miptree slice range
 *
 * This function directly sets the auxiliary compression state of a slice
 * range of a miptree.  It only modifies data structures and does not do any
 * resolves.  This should only be called by code which directly performs
 * compression operations such as fast clears and resolves.  Most code should
 * use brw_miptree_prepare_access or brw_miptree_finish_write.
 */
void
brw_miptree_set_aux_state(struct brw_context *brw,
                            struct brw_mipmap_tree *mt, uint32_t level,
                            uint32_t start_layer, uint32_t num_layers,
                            enum isl_aux_state aux_state);

/**
 * Prepare a miptree for raw access
 *
 * This helper prepares the miptree for access that knows nothing about any
 * sort of compression whatsoever.  This is useful when mapping the surface or
 * using it with the blitter.
 */
static inline void
brw_miptree_access_raw(struct brw_context *brw,
                         struct brw_mipmap_tree *mt,
                         uint32_t level, uint32_t layer,
                         bool write)
{
   brw_miptree_prepare_access(brw, mt, level, 1, layer, 1,
                                ISL_AUX_USAGE_NONE, false);
   if (write)
      brw_miptree_finish_write(brw, mt, level, layer, 1, ISL_AUX_USAGE_NONE);
}

enum isl_aux_usage
brw_miptree_texture_aux_usage(struct brw_context *brw,
                                struct brw_mipmap_tree *mt,
                                enum isl_format view_format,
                                enum gfx9_astc5x5_wa_tex_type astc5x5_wa_bits);
void
brw_miptree_prepare_texture(struct brw_context *brw,
                              struct brw_mipmap_tree *mt,
                              enum isl_format view_format,
                              uint32_t start_level, uint32_t num_levels,
                              uint32_t start_layer, uint32_t num_layers,
                              enum gfx9_astc5x5_wa_tex_type astc5x5_wa_bits);
void
brw_miptree_prepare_image(struct brw_context *brw,
                            struct brw_mipmap_tree *mt);

enum isl_aux_usage
brw_miptree_render_aux_usage(struct brw_context *brw,
                               struct brw_mipmap_tree *mt,
                               enum isl_format render_format,
                               bool blend_enabled,
                               bool draw_aux_disabled);
void
brw_miptree_prepare_render(struct brw_context *brw,
                             struct brw_mipmap_tree *mt, uint32_t level,
                             uint32_t start_layer, uint32_t layer_count,
                             enum isl_aux_usage aux_usage);
void
brw_miptree_finish_render(struct brw_context *brw,
                            struct brw_mipmap_tree *mt, uint32_t level,
                            uint32_t start_layer, uint32_t layer_count,
                            enum isl_aux_usage aux_usage);
void
brw_miptree_prepare_depth(struct brw_context *brw,
                            struct brw_mipmap_tree *mt, uint32_t level,
                            uint32_t start_layer, uint32_t layer_count);
void
brw_miptree_finish_depth(struct brw_context *brw,
                           struct brw_mipmap_tree *mt, uint32_t level,
                           uint32_t start_layer, uint32_t layer_count,
                           bool depth_written);
void
brw_miptree_prepare_external(struct brw_context *brw,
                               struct brw_mipmap_tree *mt);
void
brw_miptree_finish_external(struct brw_context *brw,
                              struct brw_mipmap_tree *mt);

void
brw_miptree_make_shareable(struct brw_context *brw,
                             struct brw_mipmap_tree *mt);

void
brw_miptree_updownsample(struct brw_context *brw,
                           struct brw_mipmap_tree *src,
                           struct brw_mipmap_tree *dst);

void
brw_update_r8stencil(struct brw_context *brw,
                       struct brw_mipmap_tree *mt);

void
brw_miptree_map(struct brw_context *brw,
                struct brw_mipmap_tree *mt,
                unsigned int level,
                unsigned int slice,
                unsigned int x,
                unsigned int y,
                unsigned int w,
                unsigned int h,
                GLbitfield mode,
                void **out_ptr,
                ptrdiff_t *out_stride);

void
brw_miptree_unmap(struct brw_context *brw,
                  struct brw_mipmap_tree *mt,
                  unsigned int level,
                  unsigned int slice);

bool
brw_miptree_sample_with_hiz(struct brw_context *brw,
                              struct brw_mipmap_tree *mt);

bool
brw_miptree_set_clear_color(struct brw_context *brw,
                              struct brw_mipmap_tree *mt,
                              union isl_color_value clear_color);

/* Get a clear color suitable for filling out an ISL surface state. */
union isl_color_value
brw_miptree_get_clear_color(const struct brw_mipmap_tree *mt,
                              struct brw_bo **clear_color_bo,
                              uint64_t *clear_color_offset);


static inline int
brw_miptree_blt_pitch(struct brw_mipmap_tree *mt)
{
   int pitch = mt->surf.row_pitch_B;
   if (mt->surf.tiling != ISL_TILING_LINEAR)
      pitch /= 4;
   return pitch;
}

isl_memcpy_type
brw_miptree_get_memcpy_type(mesa_format tiledFormat, GLenum format, GLenum type,
                              uint32_t *cpp);

static inline bool
brw_miptree_needs_fake_etc(struct brw_context *brw,
                             struct brw_mipmap_tree *mt)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   bool is_etc = _mesa_is_format_etc2(mt->format) ||
                 (mt->format == MESA_FORMAT_ETC1_RGB8);

   return devinfo->ver < 8 && !devinfo->is_baytrail && is_etc;
}

static inline bool
brw_miptree_has_etc_shadow(struct brw_context *brw,
                             struct brw_mipmap_tree *mt)
{
   return brw_miptree_needs_fake_etc(brw, mt) && mt->shadow_mt;
}

void
brw_miptree_update_etc_shadow_levels(struct brw_context *brw,
                                       struct brw_mipmap_tree *mt);

#ifdef __cplusplus
}
#endif

#endif
