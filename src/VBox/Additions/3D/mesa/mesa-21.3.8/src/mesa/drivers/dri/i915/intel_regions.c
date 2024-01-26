/**************************************************************************
 * 
 * Copyright 2006 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Provide additional functionality on top of bufmgr buffers:
 *   - 2d semantics and blit operations
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 *   - some logic for moving the buffers to the best memory pools for
 *     given operations.
 *
 * Most of this is to make it easier to implement the fixed-layout
 * mipmap tree required by intel hardware in the face of GL's
 * programming interface where each image can be specifed in random
 * order and it isn't clear what layout the tree should have until the
 * last moment.
 */

#include <sys/ioctl.h>
#include <errno.h>

#include "main/hash.h"
#include "intel_context.h"
#include "intel_regions.h"
#include "intel_blit.h"
#include "intel_buffer_objects.h"
#include "intel_bufmgr.h"
#include "intel_batchbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_REGION

static struct intel_region *
intel_region_alloc_internal(struct intel_screen *screen,
			    GLuint cpp,
			    GLuint width, GLuint height, GLuint pitch,
			    uint32_t tiling, drm_intel_bo *buffer)
{
   struct intel_region *region;

   region = calloc(sizeof(*region), 1);
   if (region == NULL)
      return region;

   region->cpp = cpp;
   region->width = width;
   region->height = height;
   region->pitch = pitch;
   region->refcount = 1;
   region->bo = buffer;
   region->tiling = tiling;

   DBG("%s <-- %p\n", __func__, region);
   return region;
}

struct intel_region *
intel_region_alloc(struct intel_screen *screen,
		   uint32_t tiling,
                   GLuint cpp, GLuint width, GLuint height,
		   bool expect_accelerated_upload)
{
   drm_intel_bo *buffer;
   unsigned long flags = 0;
   unsigned long aligned_pitch;
   struct intel_region *region;

   if (expect_accelerated_upload)
      flags |= BO_ALLOC_FOR_RENDER;

   buffer = drm_intel_bo_alloc_tiled(screen->bufmgr, "region",
				     width, height, cpp,
				     &tiling, &aligned_pitch, flags);
   if (buffer == NULL)
      return NULL;

   region = intel_region_alloc_internal(screen, cpp, width, height,
                                        aligned_pitch, tiling, buffer);
   if (region == NULL) {
      drm_intel_bo_unreference(buffer);
      return NULL;
   }

   return region;
}

bool
intel_region_flink(struct intel_region *region, uint32_t *name)
{
   if (region->name == 0) {
      if (drm_intel_bo_flink(region->bo, &region->name))
	 return false;
   }

   *name = region->name;

   return true;
}

struct intel_region *
intel_region_alloc_for_handle(struct intel_screen *screen,
			      GLuint cpp,
			      GLuint width, GLuint height, GLuint pitch,
			      GLuint handle, const char *name)
{
   struct intel_region *region;
   drm_intel_bo *buffer;
   int ret;
   uint32_t bit_6_swizzle, tiling;

   buffer = drm_intel_bo_gem_create_from_name(screen->bufmgr, name, handle);
   if (buffer == NULL)
      return NULL;
   ret = drm_intel_bo_get_tiling(buffer, &tiling, &bit_6_swizzle);
   if (ret != 0) {
      fprintf(stderr, "Couldn't get tiling of buffer %d (%s): %s\n",
	      handle, name, strerror(-ret));
      drm_intel_bo_unreference(buffer);
      return NULL;
   }

   region = intel_region_alloc_internal(screen, cpp,
					width, height, pitch, tiling, buffer);
   if (region == NULL) {
      drm_intel_bo_unreference(buffer);
      return NULL;
   }

   region->name = handle;

   return region;
}

struct intel_region *
intel_region_alloc_for_fd(struct intel_screen *screen,
                          GLuint cpp,
                          GLuint width, GLuint height, GLuint pitch,
                          GLuint size,
                          int fd, const char *name)
{
   struct intel_region *region;
   drm_intel_bo *buffer;
   int ret;
   uint32_t bit_6_swizzle, tiling;

   buffer = drm_intel_bo_gem_create_from_prime(screen->bufmgr, fd, size);
   if (buffer == NULL)
      return NULL;
   ret = drm_intel_bo_get_tiling(buffer, &tiling, &bit_6_swizzle);
   if (ret != 0) {
      fprintf(stderr, "Couldn't get tiling of buffer (%s): %s\n",
	      name, strerror(-ret));
      drm_intel_bo_unreference(buffer);
      return NULL;
   }

   region = intel_region_alloc_internal(screen, cpp,
					width, height, pitch, tiling, buffer);
   if (region == NULL) {
      drm_intel_bo_unreference(buffer);
      return NULL;
   }

   return region;
}

void
intel_region_reference(struct intel_region **dst, struct intel_region *src)
{
   DBG("%s: %p(%d) -> %p(%d)\n", __func__,
	*dst, *dst ? (*dst)->refcount : 0, src, src ? src->refcount : 0);

   if (src != *dst) {
      if (*dst)
	 intel_region_release(dst);

      if (src)
         src->refcount++;
      *dst = src;
   }
}

void
intel_region_release(struct intel_region **region_handle)
{
   struct intel_region *region = *region_handle;

   if (region == NULL) {
      DBG("%s NULL\n", __func__);
      return;
   }

   DBG("%s %p %d\n", __func__, region, region->refcount - 1);

   assert(region->refcount > 0);
   region->refcount--;

   if (region->refcount == 0) {
      drm_intel_bo_unreference(region->bo);

      free(region);
   }
   *region_handle = NULL;
}

/**
 * This function computes masks that may be used to select the bits of the X
 * and Y coordinates that indicate the offset within a tile.  If the region is
 * untiled, the masks are set to 0.
 */
void
intel_region_get_tile_masks(struct intel_region *region,
                            uint32_t *mask_x, uint32_t *mask_y)
{
   int cpp = region->cpp;
   uint32_t tiling = region->tiling;

   switch (tiling) {
   default:
      assert(false);
   case I915_TILING_NONE:
      *mask_x = *mask_y = 0;
      break;
   case I915_TILING_X:
      *mask_x = 512 / cpp - 1;
      *mask_y = 7;
      break;
   case I915_TILING_Y:
      *mask_x = 128 / cpp - 1;
      *mask_y = 31;
      break;
   }
}

/**
 * Compute the offset (in bytes) from the start of the region to the given x
 * and y coordinate.  For tiled regions, caller must ensure that x and y are
 * multiples of the tile size.
 */
uint32_t
intel_region_get_aligned_offset(struct intel_region *region, uint32_t x,
                                uint32_t y)
{
   int cpp = region->cpp;
   uint32_t pitch = region->pitch;
   uint32_t tiling = region->tiling;

   switch (tiling) {
   default:
      assert(false);
   case I915_TILING_NONE:
      return y * pitch + x * cpp;
   case I915_TILING_X:
      assert((x % (512 / cpp)) == 0);
      assert((y % 8) == 0);
      return y * pitch + x / (512 / cpp) * 4096;
   case I915_TILING_Y:
      assert((x % (128 / cpp)) == 0);
      assert((y % 32) == 0);
      return y * pitch + x / (128 / cpp) * 4096;
   }
}
