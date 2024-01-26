/*
 * Copyright 2005 VMware, Inc.
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

#ifndef BRW_BUFFEROBJ_H
#define BRW_BUFFEROBJ_H

#include "main/mtypes.h"

struct brw_context;
struct gl_buffer_object;


/**
 * Intel vertex/pixel buffer object, derived from Mesa's gl_buffer_object.
 */
struct brw_buffer_object
{
   struct gl_buffer_object Base;
   struct brw_bo *buffer;     /* the low-level buffer manager's buffer handle */

   struct brw_bo *range_map_bo[MAP_COUNT];

   /**
    * Alignment offset from the range_map_bo temporary mapping to the returned
    * obj->Pointer (caused by GL_ARB_map_buffer_alignment).
    */
   unsigned map_extra[MAP_COUNT];

   /** @{
    * Tracking for what range of the BO may currently be in use by the GPU.
    *
    * Users often want to either glBufferSubData() or glMapBufferRange() a
    * buffer object where some subset of it is busy on the GPU, without either
    * stalling or doing an extra blit (since our blits are extra expensive,
    * given that we have to reupload most of the 3D state when switching
    * rings).  We wish they'd just use glMapBufferRange() with the
    * UNSYNC|INVALIDATE_RANGE flag or the INVALIDATE_BUFFER flag, but lots
    * don't.
    *
    * To work around apps, we track what range of the BO we might have used on
    * the GPU as vertex data, tranform feedback output, buffer textures, etc.,
    * and just do glBufferSubData() with an unsynchronized map when they're
    * outside of that range.
    *
    * If gpu_active_start > gpu_active_end, then the GPU is not currently
    * accessing the BO (and we can map it without synchronization).
    */
   uint32_t gpu_active_start;
   uint32_t gpu_active_end;

   /** @{
    * Tracking for what range of the BO may contain valid data.
    *
    * Users may create a large buffer object and only fill part of it
    * with valid data.  This is a conservative estimate of what part
    * of the buffer contains valid data that we have to preserve.
    */
   uint32_t valid_data_start;
   uint32_t valid_data_end;
   /** @} */

   /**
    * If we've avoided stalls/blits using the active tracking, flag the buffer
    * for (occasional) stalling in the future to avoid getting stuck in a
    * cycle of blitting on buffer wraparound.
    */
   bool prefer_stall_to_blit;
   /** @} */
};


/* Get the bm buffer associated with a GL bufferobject:
 */
struct brw_bo *brw_bufferobj_buffer(struct brw_context *brw,
                                    struct brw_buffer_object *obj,
                                    uint32_t offset,
                                    uint32_t size,
                                    bool write);

void brw_upload_data(struct brw_uploader *upload,
                     const void *data,
                     uint32_t size,
                     uint32_t alignment,
                     struct brw_bo **out_bo,
                     uint32_t *out_offset);

void *brw_upload_space(struct brw_uploader *upload,
                       uint32_t size,
                       uint32_t alignment,
                       struct brw_bo **out_bo,
                       uint32_t *out_offset);

void brw_upload_finish(struct brw_uploader *upload);
void brw_upload_init(struct brw_uploader *upload,
                     struct brw_bufmgr *bufmgr,
                     unsigned default_size);

/* Hook the bufferobject implementation into mesa:
 */
void brw_init_buffer_object_functions(struct dd_function_table *functions);

static inline struct brw_buffer_object *
brw_buffer_object(struct gl_buffer_object *obj)
{
   return (struct brw_buffer_object *) obj;
}

struct brw_memory_object {
   struct gl_memory_object Base;
   struct brw_bo *bo;
};

static inline struct brw_memory_object *
brw_memory_object(struct gl_memory_object *obj)
{
   return (struct brw_memory_object *)obj;
}

#endif
