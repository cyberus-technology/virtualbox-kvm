/*
 * Copyright © 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef GENX_BOILERPLATE_H
#define GENX_BOILERPLATE_H

#include <assert.h>

#include "genxml/gen_macros.h"

#include "brw_context.h"
#include "brw_batch.h"

UNUSED static void *
emit_dwords(struct brw_context *brw, unsigned n)
{
   brw_batch_begin(brw, n);
   uint32_t *map = brw->batch.map_next;
   brw->batch.map_next += n;
   brw_batch_advance(brw);
   return map;
}

struct brw_address {
   struct brw_bo *bo;
   unsigned reloc_flags;
   uint32_t offset;
};

#define __gen_address_type struct brw_address
#define __gen_user_data struct brw_context

static uint64_t
__gen_combine_address(struct brw_context *brw, void *location,
                      struct brw_address address, uint32_t delta)
{
   struct brw_batch *batch = &brw->batch;
   uint32_t offset;

   if (address.bo == NULL) {
      return address.offset + delta;
   } else {
      if (GFX_VER < 6 && brw_ptr_in_state_buffer(batch, location)) {
         offset = (char *) location - (char *) brw->batch.state.map;
         return brw_state_reloc(batch, offset, address.bo,
                                address.offset + delta,
                                address.reloc_flags);
      }

      assert(!brw_ptr_in_state_buffer(batch, location));

      offset = (char *) location - (char *) brw->batch.batch.map;
      return brw_batch_reloc(batch, offset, address.bo,
                             address.offset + delta,
                             address.reloc_flags);
   }
}

UNUSED static struct brw_address
rw_bo(struct brw_bo *bo, uint32_t offset)
{
   return (struct brw_address) {
            .bo = bo,
            .offset = offset,
            .reloc_flags = RELOC_WRITE,
   };
}

UNUSED static struct brw_address
ro_bo(struct brw_bo *bo, uint32_t offset)
{
   return (struct brw_address) {
            .bo = bo,
            .offset = offset,
   };
}

UNUSED static struct brw_address
rw_32_bo(struct brw_bo *bo, uint32_t offset)
{
   return (struct brw_address) {
            .bo = bo,
            .offset = offset,
            .reloc_flags = RELOC_WRITE | RELOC_32BIT,
   };
}

UNUSED static struct brw_address
ro_32_bo(struct brw_bo *bo, uint32_t offset)
{
   return (struct brw_address) {
            .bo = bo,
            .offset = offset,
            .reloc_flags = RELOC_32BIT,
   };
}

UNUSED static struct brw_address
ggtt_bo(struct brw_bo *bo, uint32_t offset)
{
   return (struct brw_address) {
            .bo = bo,
            .offset = offset,
            .reloc_flags = RELOC_WRITE | RELOC_NEEDS_GGTT,
   };
}

#include "genxml/genX_pack.h"

#define _brw_cmd_length(cmd) cmd ## _length
#define _brw_cmd_length_bias(cmd) cmd ## _length_bias
#define _brw_cmd_header(cmd) cmd ## _header
#define _brw_cmd_pack(cmd) cmd ## _pack

#define brw_batch_emit(brw, cmd, name)                  \
   for (struct cmd name = { _brw_cmd_header(cmd) },     \
        *_dst = emit_dwords(brw, _brw_cmd_length(cmd)); \
        __builtin_expect(_dst != NULL, 1);              \
        _brw_cmd_pack(cmd)(brw, (void *)_dst, &name),   \
        _dst = NULL)

#define brw_batch_emitn(brw, cmd, n, ...) ({           \
      uint32_t *_dw = emit_dwords(brw, n);             \
      struct cmd template = {                          \
         _brw_cmd_header(cmd),                         \
         .DWordLength = n - _brw_cmd_length_bias(cmd), \
         __VA_ARGS__                                   \
      };                                               \
      _brw_cmd_pack(cmd)(brw, _dw, &template);         \
      _dw + 1; /* Array starts at dw[1] */             \
   })

#define brw_state_emit(brw, cmd, align, offset, name)              \
   for (struct cmd name = {},                                      \
        *_dst = brw_state_batch(brw, _brw_cmd_length(cmd) * 4,     \
                                align, offset);                    \
        __builtin_expect(_dst != NULL, 1);                         \
        _brw_cmd_pack(cmd)(brw, (void *)_dst, &name),              \
        _dst = NULL)

#endif
