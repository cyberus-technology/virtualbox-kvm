/*
 * Copyright © 2008-2012 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/**
 * @file brw_bufmgr.h
 *
 * Public definitions of Intel-specific bufmgr functions.
 */

#ifndef BRW_BUFMGR_H
#define BRW_BUFMGR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "c11/threads.h"
#include "util/u_atomic.h"
#include "util/list.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct intel_device_info;
struct brw_context;

/**
 * Memory zones.  When allocating a buffer, you can request that it is
 * placed into a specific region of the virtual address space (PPGTT).
 *
 * Most buffers can go anywhere (BRW_MEMZONE_OTHER).  Some buffers are
 * accessed via an offset from a base address.  STATE_BASE_ADDRESS has
 * a maximum 4GB size for each region, so we need to restrict those
 * buffers to be within 4GB of the base.  Each memory zone corresponds
 * to a particular base address.
 *
 * Currently, i965 partitions the address space into two regions:
 *
 * - Low 4GB
 * - Full 48-bit address space
 *
 * Eventually, we hope to carve out 4GB of VMA for each base address.
 */
enum brw_memory_zone {
   BRW_MEMZONE_LOW_4G,
   BRW_MEMZONE_OTHER,

   /* Shaders - Instruction State Base Address */
   BRW_MEMZONE_SHADER  = BRW_MEMZONE_LOW_4G,

   /* Scratch - General State Base Address */
   BRW_MEMZONE_SCRATCH = BRW_MEMZONE_LOW_4G,

   /* Surface State Base Address */
   BRW_MEMZONE_SURFACE = BRW_MEMZONE_LOW_4G,

   /* Dynamic State Base Address */
   BRW_MEMZONE_DYNAMIC = BRW_MEMZONE_LOW_4G,
};

#define BRW_MEMZONE_COUNT (BRW_MEMZONE_OTHER + 1)

struct brw_bo {
   /**
    * Size in bytes of the buffer object.
    *
    * The size may be larger than the size originally requested for the
    * allocation, such as being aligned to page size.
    */
   uint64_t size;

   /** Buffer manager context associated with this buffer object */
   struct brw_bufmgr *bufmgr;

   /** The GEM handle for this buffer object. */
   uint32_t gem_handle;

   /**
    * Offset of the buffer inside the Graphics Translation Table.
    *
    * This is effectively our GPU address for the buffer and we use it
    * as our base for all state pointers into the buffer. However, since the
    * kernel may be forced to move it around during the course of the
    * buffer's lifetime, we can only know where the buffer was on the last
    * execbuf. We presume, and are usually right, that the buffer will not
    * move and so we use that last offset for the next batch and by doing
    * so we can avoid having the kernel perform a relocation fixup pass as
    * our pointers inside the batch will be using the correct base offset.
    *
    * Since we do use it as a base address for the next batch of pointers,
    * the kernel treats our offset as a request, and if possible will
    * arrange the buffer to placed at that address (trying to balance
    * the cost of buffer migration versus the cost of performing
    * relocations). Furthermore, we can force the kernel to place the buffer,
    * or report a failure if we specified a conflicting offset, at our chosen
    * offset by specifying EXEC_OBJECT_PINNED.
    *
    * Note the GTT may be either per context, or shared globally across the
    * system. On a shared system, our buffers have to contend for address
    * space with both aperture mappings and framebuffers and so are more
    * likely to be moved. On a full ppGTT system, each batch exists in its
    * own GTT, and so each buffer may have their own offset within each
    * context.
    */
   uint64_t gtt_offset;

   /**
    * The validation list index for this buffer, or -1 when not in a batch.
    * Note that a single buffer may be in multiple batches (contexts), and
    * this is a global field, which refers to the last batch using the BO.
    * It should not be considered authoritative, but can be used to avoid a
    * linear walk of the validation list in the common case by guessing that
    * exec_bos[bo->index] == bo and confirming whether that's the case.
    */
   unsigned index;

   /**
    * Boolean of whether the GPU is definitely not accessing the buffer.
    *
    * This is only valid when reusable, since non-reusable
    * buffers are those that have been shared with other
    * processes, so we don't know their state.
    */
   bool idle;

   int refcount;
   const char *name;

   uint64_t kflags;

   /**
    * Kenel-assigned global name for this object
    *
    * List contains both flink named and prime fd'd objects
    */
   unsigned int global_name;

   /**
    * Current tiling mode
    */
   uint32_t tiling_mode;
   uint32_t swizzle_mode;
   uint32_t stride;

   time_t free_time;

   /** Mapped address for the buffer, saved across map/unmap cycles */
   void *map_cpu;
   /** GTT virtual address for the buffer, saved across map/unmap cycles */
   void *map_gtt;
   /** WC CPU address for the buffer, saved across map/unmap cycles */
   void *map_wc;

   /** BO cache list */
   struct list_head head;

   /**
    * List of GEM handle exports of this buffer (bo_export).
    *
    * Hold bufmgr->lock when using this list.
    */
   struct list_head exports;

   /**
    * Boolean of whether this buffer can be re-used
    */
   bool reusable;

   /**
    * Boolean of whether this buffer has been shared with an external client.
    */
   bool external;

   /**
    * Boolean of whether this buffer is cache coherent
    */
   bool cache_coherent;
};

#define BO_ALLOC_BUSY       (1<<0)
#define BO_ALLOC_ZEROED     (1<<1)

/**
 * Allocate a buffer object.
 *
 * Buffer objects are not necessarily initially mapped into CPU virtual
 * address space or graphics device aperture.  They must be mapped
 * using brw_bo_map() to be used by the CPU.
 */
struct brw_bo *brw_bo_alloc(struct brw_bufmgr *bufmgr, const char *name,
                            uint64_t size, enum brw_memory_zone memzone);

/**
 * Allocate a tiled buffer object.
 *
 * Alignment for tiled objects is set automatically; the 'flags'
 * argument provides a hint about how the object will be used initially.
 *
 * Valid tiling formats are:
 *  I915_TILING_NONE
 *  I915_TILING_X
 *  I915_TILING_Y
 */
struct brw_bo *brw_bo_alloc_tiled(struct brw_bufmgr *bufmgr,
                                  const char *name,
                                  uint64_t size,
                                  enum brw_memory_zone memzone,
                                  uint32_t tiling_mode,
                                  uint32_t pitch,
                                  unsigned flags);

/**
 * Allocate a tiled buffer object.
 *
 * Alignment for tiled objects is set automatically; the 'flags'
 * argument provides a hint about how the object will be used initially.
 *
 * Valid tiling formats are:
 *  I915_TILING_NONE
 *  I915_TILING_X
 *  I915_TILING_Y
 *
 * Note the tiling format may be rejected; callers should check the
 * 'tiling_mode' field on return, as well as the pitch value, which
 * may have been rounded up to accommodate for tiling restrictions.
 */
struct brw_bo *brw_bo_alloc_tiled_2d(struct brw_bufmgr *bufmgr,
                                     const char *name,
                                     int x, int y, int cpp,
                                     enum brw_memory_zone memzone,
                                     uint32_t tiling_mode,
                                     uint32_t *pitch,
                                     unsigned flags);

/** Takes a reference on a buffer object */
static inline void
brw_bo_reference(struct brw_bo *bo)
{
   p_atomic_inc(&bo->refcount);
}

/**
 * Releases a reference on a buffer object, freeing the data if
 * no references remain.
 */
void brw_bo_unreference(struct brw_bo *bo);

/* Must match MapBufferRange interface (for convenience) */
#define MAP_READ        GL_MAP_READ_BIT
#define MAP_WRITE       GL_MAP_WRITE_BIT
#define MAP_ASYNC       GL_MAP_UNSYNCHRONIZED_BIT
#define MAP_PERSISTENT  GL_MAP_PERSISTENT_BIT
#define MAP_COHERENT    GL_MAP_COHERENT_BIT
/* internal */
#define MAP_INTERNAL_MASK       (0xffu << 24)
#define MAP_RAW                 (0x01 << 24)

/**
 * Maps the buffer into userspace.
 *
 * This function will block waiting for any existing execution on the
 * buffer to complete, first.  The resulting mapping is returned.
 */
MUST_CHECK void *brw_bo_map(struct brw_context *brw, struct brw_bo *bo, unsigned flags);

/**
 * Reduces the refcount on the userspace mapping of the buffer
 * object.
 */
static inline int brw_bo_unmap(UNUSED struct brw_bo *bo) { return 0; }

/** Write data into an object. */
int brw_bo_subdata(struct brw_bo *bo, uint64_t offset,
                   uint64_t size, const void *data);
/**
 * Waits for rendering to an object by the GPU to have completed.
 *
 * This is not required for any access to the BO by bo_map,
 * bo_subdata, etc.  It is merely a way for the driver to implement
 * glFinish.
 */
void brw_bo_wait_rendering(struct brw_bo *bo);

/**
 * Unref a buffer manager instance.
 */
void brw_bufmgr_unref(struct brw_bufmgr *bufmgr);

/**
 * Get the current tiling (and resulting swizzling) mode for the bo.
 *
 * \param buf Buffer to get tiling mode for
 * \param tiling_mode returned tiling mode
 * \param swizzle_mode returned swizzling mode
 */
int brw_bo_get_tiling(struct brw_bo *bo, uint32_t *tiling_mode,
                      uint32_t *swizzle_mode);

/**
 * Create a visible name for a buffer which can be used by other apps
 *
 * \param buf Buffer to create a name for
 * \param name Returned name
 */
int brw_bo_flink(struct brw_bo *bo, uint32_t *name);

/**
 * Returns 1 if mapping the buffer for write could cause the process
 * to block, due to the object being active in the GPU.
 */
int brw_bo_busy(struct brw_bo *bo);

/**
 * Specify the volatility of the buffer.
 * \param bo Buffer to create a name for
 * \param madv The purgeable status
 *
 * Use I915_MADV_DONTNEED to mark the buffer as purgeable, and it will be
 * reclaimed under memory pressure. If you subsequently require the buffer,
 * then you must pass I915_MADV_WILLNEED to mark the buffer as required.
 *
 * Returns 1 if the buffer was retained, or 0 if it was discarded whilst
 * marked as I915_MADV_DONTNEED.
 */
int brw_bo_madvise(struct brw_bo *bo, int madv);

struct brw_bufmgr *brw_bufmgr_get_for_fd(struct intel_device_info *devinfo,
                                         int fd, bool bo_reuse);

struct brw_bo *brw_bo_gem_create_from_name(struct brw_bufmgr *bufmgr,
                                           const char *name,
                                           unsigned int handle);

int brw_bo_wait(struct brw_bo *bo, int64_t timeout_ns);

uint32_t brw_create_hw_context(struct brw_bufmgr *bufmgr);

int brw_hw_context_set_priority(struct brw_bufmgr *bufmgr,
                                uint32_t ctx_id,
                                int priority);

void brw_destroy_hw_context(struct brw_bufmgr *bufmgr, uint32_t ctx_id);

int brw_bufmgr_get_fd(struct brw_bufmgr *bufmgr);

int brw_bo_gem_export_to_prime(struct brw_bo *bo, int *prime_fd);
struct brw_bo *brw_bo_gem_create_from_prime(struct brw_bufmgr *bufmgr,
                                            int prime_fd);
struct brw_bo *brw_bo_gem_create_from_prime_tiled(struct brw_bufmgr *bufmgr,
                                                  int prime_fd,
                                                  uint32_t tiling_mode,
                                                  uint32_t stride);

uint32_t brw_bo_export_gem_handle(struct brw_bo *bo);

/**
 * Exports a bo as a GEM handle into a given DRM file descriptor
 * \param bo Buffer to export
 * \param drm_fd File descriptor where the new handle is created
 * \param out_handle Pointer to store the new handle
 *
 * Returns 0 if the buffer was successfully exported, a non zero error code
 * otherwise.
 */
int brw_bo_export_gem_handle_for_device(struct brw_bo *bo, int drm_fd,
                                        uint32_t *out_handle);

int brw_reg_read(struct brw_bufmgr *bufmgr, uint32_t offset,
                 uint64_t *result);

bool brw_using_softpin(struct brw_bufmgr *bufmgr);

/** @{ */

#if defined(__cplusplus)
}
#endif
#endif /* BRW_BUFMGR_H */
