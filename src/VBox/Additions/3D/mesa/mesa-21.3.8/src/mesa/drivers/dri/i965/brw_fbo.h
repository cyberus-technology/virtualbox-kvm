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

#ifndef BRW_FBO_H
#define BRW_FBO_H

#include <stdbool.h>
#include <assert.h>
#include "main/formats.h"
#include "main/macros.h"
#include "brw_context.h"
#include "brw_mipmap_tree.h"
#include "brw_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

struct brw_mipmap_tree;

/**
 * Intel renderbuffer, derived from gl_renderbuffer.
 */
struct brw_renderbuffer
{
   struct swrast_renderbuffer Base;
   /**
    * The real renderbuffer storage.
    *
    * This is multisampled if NumSamples is > 1.
    */
   struct brw_mipmap_tree *mt;

   /**
    * Downsampled contents for window-system MSAA renderbuffers.
    *
    * For window system MSAA color buffers, the singlesample_mt is shared with
    * other processes in DRI2 (and in DRI3, it's the image buffer managed by
    * glx_dri3.c), while mt is private to our process.  To do a swapbuffers,
    * we have to downsample out of mt into singlesample_mt.  For depth and
    * stencil buffers, the singlesample_mt is also private, and since we don't
    * expect to need to do resolves (except if someone does a glReadPixels()
    * or glCopyTexImage()), we just temporarily allocate singlesample_mt when
    * asked to map the renderbuffer.
    */
   struct brw_mipmap_tree *singlesample_mt;

   /* Gen < 6 doesn't have layer specifier for render targets or depth. Driver
    * needs to manually offset surfaces to correct level/layer. There are,
    * however, alignment restrictions to respect as well and in come cases
    * the only option is to use temporary single slice surface which driver
    * copies after rendering to the full miptree.
    *
    * See brw_renderbuffer_move_to_temp().
    */
   struct brw_mipmap_tree *align_wa_mt;

   /**
    * \name Miptree view
    * \{
    *
    * Multiple renderbuffers may simultaneously wrap a single texture and each
    * provide a different view into that texture. The fields below indicate
    * which miptree slice is wrapped by this renderbuffer.  The fields' values
    * are consistent with the 'level' and 'layer' parameters of
    * glFramebufferTextureLayer().
    *
    * For renderbuffers not created with glFramebufferTexture*(), mt_level and
    * mt_layer are 0.
    */
   unsigned int mt_level;
   unsigned int mt_layer;

   /* The number of attached logical layers. */
   unsigned int layer_count;
   /** \} */

   GLuint draw_x, draw_y; /**< Offset of drawing within the region */

   /**
    * Set to true at every draw call, to indicate if a window-system
    * renderbuffer needs to be downsampled before using singlesample_mt.
    */
   bool need_downsample;

   /**
    * Set to true when doing an brw_renderbuffer_map()/unmap() that requires
    * an upsample at the end.
    */
   bool need_map_upsample;

   /**
    * Set to true if singlesample_mt is temporary storage that persists only
    * for the duration of a mapping.
    */
   bool singlesample_mt_is_tmp;

   /**
    * Set to true when application specifically asked for a sRGB visual.
    */
   bool need_srgb;
};


/**
 * gl_renderbuffer is a base class which we subclass.  The Class field
 * is used for simple run-time type checking.
 */
#define INTEL_RB_CLASS 0x12345678


/**
 * Return a gl_renderbuffer ptr casted to brw_renderbuffer.
 * NULL will be returned if the rb isn't really an brw_renderbuffer.
 * This is determined by checking the ClassID.
 */
static inline struct brw_renderbuffer *
brw_renderbuffer(struct gl_renderbuffer *rb)
{
   struct brw_renderbuffer *irb = (struct brw_renderbuffer *) rb;
   if (irb && irb->Base.Base.ClassID == INTEL_RB_CLASS)
      return irb;
   else
      return NULL;
}

static inline struct brw_mipmap_tree *
brw_renderbuffer_get_mt(struct brw_renderbuffer *irb)
{
   if (!irb)
      return NULL;

   return (irb->align_wa_mt) ? irb->align_wa_mt : irb->mt;
}

/**
 * \brief Return the framebuffer attachment specified by attIndex.
 *
 * If the framebuffer lacks the specified attachment, then return null.
 *
 * If the attached renderbuffer is a wrapper, then return wrapped
 * renderbuffer.
 */
static inline struct brw_renderbuffer *
brw_get_renderbuffer(struct gl_framebuffer *fb, gl_buffer_index attIndex)
{
   struct gl_renderbuffer *rb;

   assert((unsigned)attIndex < ARRAY_SIZE(fb->Attachment));

   rb = fb->Attachment[attIndex].Renderbuffer;
   if (!rb)
      return NULL;

   return brw_renderbuffer(rb);
}


static inline mesa_format
brw_rb_format(const struct brw_renderbuffer *rb)
{
   return rb->Base.Base.Format;
}

extern struct brw_renderbuffer *
brw_create_winsys_renderbuffer(struct brw_screen *screen,
                               mesa_format format, unsigned num_samples);

struct brw_renderbuffer *
brw_create_private_renderbuffer(struct brw_screen *screen,
                                mesa_format format, unsigned num_samples);

struct gl_renderbuffer*
brw_create_wrapped_renderbuffer(struct gl_context *ctx,
                                int width, int height,
                                mesa_format format);

extern void
brw_fbo_init(struct brw_context *brw);

void
brw_renderbuffer_set_draw_offset(struct brw_renderbuffer *irb);

static inline uint32_t
brw_renderbuffer_get_tile_offsets(struct brw_renderbuffer *irb,
                                  uint32_t *tile_x,
                                  uint32_t *tile_y)
{
   if (irb->align_wa_mt) {
      *tile_x = 0;
      *tile_y = 0;
      return 0;
   }

   return brw_miptree_get_tile_offsets(irb->mt, irb->mt_level, irb->mt_layer,
                                       tile_x, tile_y);
}

bool
brw_renderbuffer_has_hiz(struct brw_renderbuffer *irb);


void brw_renderbuffer_move_to_temp(struct brw_context *brw,
                                     struct brw_renderbuffer *irb,
                                     bool invalidate);

void
brw_renderbuffer_downsample(struct brw_context *brw,
                            struct brw_renderbuffer *irb);

void
brw_renderbuffer_upsample(struct brw_context *brw,
                          struct brw_renderbuffer *irb);

void brw_cache_sets_clear(struct brw_context *brw);
void brw_cache_flush_for_read(struct brw_context *brw, struct brw_bo *bo);
void brw_cache_flush_for_render(struct brw_context *brw, struct brw_bo *bo,
                                enum isl_format format,
                                enum isl_aux_usage aux_usage);
void brw_cache_flush_for_depth(struct brw_context *brw, struct brw_bo *bo);
void brw_render_cache_add_bo(struct brw_context *brw, struct brw_bo *bo,
                             enum isl_format format,
                             enum isl_aux_usage aux_usage);
void brw_depth_cache_add_bo(struct brw_context *brw, struct brw_bo *bo);

unsigned
brw_quantize_num_samples(struct brw_screen *intel, unsigned num_samples);

#ifdef __cplusplus
}
#endif

#endif /* BRW_FBO_H */
