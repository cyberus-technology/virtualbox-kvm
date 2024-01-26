/*
 * Copyright 2003 VMware, Inc.
 * Copyright © 2007 Intel Corporation
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

/**
 * @file intel_upload.c
 *
 * Batched upload via BOs.
 */

#include "main/macros.h"
#include "brw_bufmgr.h"
#include "brw_context.h"
#include "brw_buffer_objects.h"

void
brw_upload_finish(struct brw_uploader *upload)
{
   assert((upload->bo == NULL) == (upload->map == NULL));
   if (!upload->bo)
      return;

   brw_bo_unmap(upload->bo);
   brw_bo_unreference(upload->bo);
   upload->bo = NULL;
   upload->map = NULL;
   upload->next_offset = 0;
}

/**
 * Interface for getting memory for uploading streamed data to the GPU
 *
 * In most cases, streamed data (for GPU state structures, for example) is
 * uploaded through brw_state_batch(), since that interface allows relocations
 * from the streamed space returned to other BOs.  However, that interface has
 * the restriction that the amount of space allocated has to be "small".
 *
 * This interface, on the other hand, is able to handle arbitrary sized
 * allocation requests, though it will batch small allocations into the same
 * BO for efficiency and reduced memory footprint.
 *
 * \note The returned pointer is valid only until brw_upload_finish().
 *
 * \param out_bo Pointer to a BO, which must point to a valid BO or NULL on
 * entry, and will have a reference to the new BO containing the state on
 * return.
 *
 * \param out_offset Offset within the buffer object that the data will land.
 */
void *
brw_upload_space(struct brw_uploader *upload,
                 uint32_t size,
                 uint32_t alignment,
                 struct brw_bo **out_bo,
                 uint32_t *out_offset)
{
   uint32_t offset;

   offset = ALIGN_NPOT(upload->next_offset, alignment);
   if (upload->bo && offset + size > upload->bo->size) {
      brw_upload_finish(upload);
      offset = 0;
   }

   assert((upload->bo == NULL) == (upload->map == NULL));
   if (!upload->bo) {
      upload->bo = brw_bo_alloc(upload->bufmgr, "streamed data",
                                MAX2(upload->default_size, size),
                                BRW_MEMZONE_OTHER);
      upload->map = brw_bo_map(NULL, upload->bo,
                               MAP_READ | MAP_WRITE |
                               MAP_PERSISTENT | MAP_ASYNC);
   }

   upload->next_offset = offset + size;

   *out_offset = offset;
   if (*out_bo != upload->bo) {
      brw_bo_unreference(*out_bo);
      *out_bo = upload->bo;
      brw_bo_reference(upload->bo);
   }

   return upload->map + offset;
}

/**
 * Handy interface to upload some data to temporary GPU memory quickly.
 *
 * References to this memory should not be retained across batch flushes.
 */
void
brw_upload_data(struct brw_uploader *upload,
                const void *data,
                uint32_t size,
                uint32_t alignment,
                struct brw_bo **out_bo,
                uint32_t *out_offset)
{
   void *dst = brw_upload_space(upload, size, alignment, out_bo, out_offset);
   memcpy(dst, data, size);
}

void
brw_upload_init(struct brw_uploader *upload,
                struct brw_bufmgr *bufmgr,
                unsigned default_size)
{
   upload->bufmgr = bufmgr;
   upload->bo = NULL;
   upload->map = NULL;
   upload->next_offset = 0;
   upload->default_size = default_size;
}
