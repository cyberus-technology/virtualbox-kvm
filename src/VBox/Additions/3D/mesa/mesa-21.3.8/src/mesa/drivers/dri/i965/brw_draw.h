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

#ifndef BRW_DRAW_H
#define BRW_DRAW_H

#include "main/mtypes.h"
#include "brw_bufmgr.h"

struct brw_context;

uint32_t *
brw_emit_vertex_buffer_state(struct brw_context *brw,
                             unsigned buffer_nr,
                             struct brw_bo *bo,
                             unsigned start_offset,
                             unsigned end_offset,
                             unsigned stride,
                             unsigned step_rate,
                             uint32_t *__map);

#define EMIT_VERTEX_BUFFER_STATE(...) __map = \
   brw_emit_vertex_buffer_state(__VA_ARGS__, __map)

void brw_draw_prims(struct gl_context *ctx,
                    const struct _mesa_prim *prims,
                    unsigned nr_prims,
                    const struct _mesa_index_buffer *ib,
                    bool index_bounds_valid,
                    bool primitive_restart,
                    unsigned restart_index,
                    unsigned min_index,
                    unsigned max_index,
                    unsigned num_instances,
                    unsigned base_instance);

void brw_init_draw_functions(struct dd_function_table *functions);
void brw_draw_init( struct brw_context *brw );
void brw_draw_destroy( struct brw_context *brw );

void brw_prepare_shader_draw_parameters(struct brw_context *);

/* brw_primitive_restart.c */
GLboolean
brw_handle_primitive_restart(struct gl_context *ctx,
                             const struct _mesa_prim *prims,
                             GLuint nr_prims,
                             const struct _mesa_index_buffer *ib,
                             GLuint num_instances, GLuint base_instance,
                             bool primitive_restart,
                             unsigned restart_index);

void
brw_draw_indirect_prims(struct gl_context *ctx,
                        GLuint mode,
                        struct gl_buffer_object *indirect_data,
                        GLsizeiptr indirect_offset,
                        unsigned draw_count,
                        unsigned stride,
                        struct gl_buffer_object *indirect_params,
                        GLsizeiptr indirect_params_offset,
                        const struct _mesa_index_buffer *ib,
                        bool primitive_restart,
                        unsigned restart_index);
#endif
