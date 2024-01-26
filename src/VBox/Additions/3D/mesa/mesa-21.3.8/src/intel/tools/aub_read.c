/*
 * Copyright © 2016-2018 Intel Corporation
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "common/intel_gem.h"
#include "util/macros.h"

#include "aub_read.h"
#include "intel_context.h"
#include "intel_aub.h"

#define TYPE(dw)       (((dw) >> 29) & 7)
#define OPCODE(dw)     (((dw) >> 23) & 0x3f)
#define SUBOPCODE(dw)  (((dw) >> 16) & 0x7f)

#define MAKE_HEADER(type, opcode, subopcode) \
                   ((((unsigned) (type)) << 29) | ((opcode) << 23) | ((subopcode) << 16))

#define TYPE_AUB            0x7

/* Classic AUB opcodes */
#define OPCODE_AUB          0x01
#define SUBOPCODE_HEADER    0x05
#define SUBOPCODE_BLOCK     0x41
#define SUBOPCODE_BMP       0x1e

/* Newer version AUB opcode */
#define OPCODE_NEW_AUB      0x2e
#define SUBOPCODE_REG_POLL  0x02
#define SUBOPCODE_REG_WRITE 0x03
#define SUBOPCODE_MEM_POLL  0x05
#define SUBOPCODE_MEM_WRITE 0x06
#define SUBOPCODE_VERSION   0x0e

static PRINTFLIKE(3, 4) void
parse_error(struct aub_read *read, const uint32_t *p, const char *fmt, ...)
{
   if (!read->error)
      return;

   va_list ap;
   va_start(ap, fmt);

   char msg[80];
   vsnprintf(msg, sizeof(msg), fmt, ap);
   read->error(read->user_data, p, msg);

   va_end(ap);
}

static bool
handle_trace_header(struct aub_read *read, const uint32_t *p)
{
   /* The intel_aubdump tool from IGT is kind enough to put a PCI-ID= tag in
    * the AUB header comment.  If the user hasn't specified a hardware
    * generation, try to use the one from the AUB file.
    */
   const uint32_t *end = p + (p[0] & 0xffff) + 2;
   int aub_pci_id = 0;

   if (end > &p[12] && p[12] > 0) {
      if (sscanf((char *)&p[13], "PCI-ID=%i", &aub_pci_id) > 0) {
         if (!intel_get_device_info_from_pci_id(aub_pci_id, &read->devinfo)) {
            parse_error(read, p,
                        "can't find device information: pci_id=0x%x\n", aub_pci_id);
            return false;
         }
      }
   }

   char app_name[33];
   strncpy(app_name, (const char *)&p[2], 32);
   app_name[32] = 0;

   if (read->info)
      read->info(read->user_data, aub_pci_id, app_name);

   return true;
}

static bool
handle_memtrace_version(struct aub_read *read, const uint32_t *p)
{
   int header_length = p[0] & 0xffff;
   char app_name[64];
   int app_name_len = MIN2(4 * (header_length + 1 - 5), ARRAY_SIZE(app_name) - 1);
   int pci_id_len = 0;
   int aub_pci_id = 0;

   strncpy(app_name, (const char *)&p[5], app_name_len);
   app_name[app_name_len] = 0;

   if (sscanf(app_name, "PCI-ID=%i %n", &aub_pci_id, &pci_id_len) > 0) {
      if (!intel_get_device_info_from_pci_id(aub_pci_id, &read->devinfo)) {
         parse_error(read, p, "can't find device information: pci_id=0x%x\n", aub_pci_id);
         return false;
      }

      if (read->info)
         read->info(read->user_data, aub_pci_id, app_name + pci_id_len);
   }

   return true;
}

static bool
handle_trace_block(struct aub_read *read, const uint32_t *p)
{
   int operation = p[1] & AUB_TRACE_OPERATION_MASK;
   int type = p[1] & AUB_TRACE_TYPE_MASK;
   int address_space = p[1] & AUB_TRACE_ADDRESS_SPACE_MASK;
   int header_length = p[0] & 0xffff;
   enum drm_i915_gem_engine_class engine = I915_ENGINE_CLASS_RENDER;
   const void *data = p + header_length + 2;
   uint64_t address = intel_48b_address((read->devinfo.ver >= 8 ? ((uint64_t) p[5] << 32) : 0) |
                                        ((uint64_t) p[3]));
   uint32_t size = p[4];

   switch (operation) {
   case AUB_TRACE_OP_DATA_WRITE:
      if (address_space == AUB_TRACE_MEMTYPE_GTT) {
         if (read->local_write)
            read->local_write(read->user_data, address, data, size);
      break;
   case AUB_TRACE_OP_COMMAND_WRITE:
      switch (type) {
      case AUB_TRACE_TYPE_RING_PRB0:
         engine = I915_ENGINE_CLASS_RENDER;
         break;
      case AUB_TRACE_TYPE_RING_PRB1:
         engine = I915_ENGINE_CLASS_VIDEO;
         break;
      case AUB_TRACE_TYPE_RING_PRB2:
         engine = I915_ENGINE_CLASS_COPY;
         break;
      default:
         parse_error(read, p, "command write to unknown ring %d\n", type);
         return false;
      }

      if (read->ring_write)
         read->ring_write(read->user_data, engine, data, size);
      break;
      }
   }

   return true;
}

static void
handle_memtrace_reg_write(struct aub_read *read, const uint32_t *p)
{
   uint32_t offset = p[1];
   uint32_t value = p[5];

   if (read->reg_write)
      read->reg_write(read->user_data, offset, value);

   enum drm_i915_gem_engine_class engine;
   uint64_t context_descriptor;

   switch (offset) {
   case EXECLIST_SUBMITPORT_RCSUNIT: /* render elsp */
      read->render_elsp[read->render_elsp_index++] = value;
      if (read->render_elsp_index < 4)
         return;

      read->render_elsp_index = 0;
      engine = I915_ENGINE_CLASS_RENDER;
      context_descriptor = (uint64_t)read->render_elsp[2] << 32 |
         read->render_elsp[3];
      break;
   case EXECLIST_SUBMITPORT_VCSUNIT0: /* video elsp */
      read->video_elsp[read->video_elsp_index++] = value;
      if (read->video_elsp_index < 4)
         return;

      read->video_elsp_index = 0;
      engine = I915_ENGINE_CLASS_VIDEO;
      context_descriptor = (uint64_t)read->video_elsp[2] << 32 |
         read->video_elsp[3];
      break;
   case EXECLIST_SUBMITPORT_BCSUNIT: /* blitter elsp */
      read->blitter_elsp[read->blitter_elsp_index++] = value;
      if (read->blitter_elsp_index < 4)
         return;

      read->blitter_elsp_index = 0;
      engine = I915_ENGINE_CLASS_COPY;
      context_descriptor = (uint64_t)read->blitter_elsp[2] << 32 |
         read->blitter_elsp[3];
      break;
   case EXECLIST_SQ_CONTENTS0_RCSUNIT: /* render elsq0 lo */
      read->render_elsp[3] = value;
      return;
   case (EXECLIST_SQ_CONTENTS0_RCSUNIT + 4): /* render elsq0 hi */
      read->render_elsp[2] = value;
      return;
   case EXECLIST_SQ_CONTENTS0_VCSUNIT0: /* video elsq0 lo */
      read->video_elsp[3] = value;
      return;
   case EXECLIST_SQ_CONTENTS0_VCSUNIT0 + 4: /* video elsq0 hi */
      read->video_elsp[2] = value;
      return;
   case EXECLIST_SQ_CONTENTS0_BCSUNIT: /* blitter elsq0 lo */
      read->blitter_elsp[3] = value;
      return;
   case (EXECLIST_SQ_CONTENTS0_BCSUNIT + 4): /* blitter elsq0 hi */
      read->blitter_elsp[2] = value;
      return;
   case EXECLIST_CONTROL_RCSUNIT: /* render elsc */
      engine = I915_ENGINE_CLASS_RENDER;
      context_descriptor = (uint64_t)read->render_elsp[2] << 32 |
         read->render_elsp[3];
      break;
   case EXECLIST_CONTROL_VCSUNIT0: /* video_elsc */
      engine = I915_ENGINE_CLASS_VIDEO;
      context_descriptor = (uint64_t)read->video_elsp[2] << 32 |
         read->video_elsp[3];
      break;
   case EXECLIST_CONTROL_BCSUNIT: /* blitter elsc */
      engine = I915_ENGINE_CLASS_COPY;
      context_descriptor = (uint64_t)read->blitter_elsp[2] << 32 |
         read->blitter_elsp[3];
      break;
   default:
      return;
   }

   if (read->execlist_write)
      read->execlist_write(read->user_data, engine, context_descriptor);
}

static void
handle_memtrace_mem_write(struct aub_read *read, const uint32_t *p)
{
   const void *data = p + 5;
   uint64_t addr = intel_48b_address(*(uint64_t*)&p[1]);
   uint32_t size = p[4];
   uint32_t address_space = p[3] >> 28;

   switch (address_space) {
   case 0: /* GGTT */
      if (read->ggtt_write)
         read->ggtt_write(read->user_data, addr, data, size);
      break;
   case 1: /* Local */
      if (read->local_write)
         read->local_write(read->user_data, addr, data, size);
      break;
   case 2: /* Physical */
      if (read->phys_write)
         read->phys_write(read->user_data, addr, data, size);
      break;
   case 4: /* GGTT Entry */
      if (read->ggtt_entry_write)
         read->ggtt_entry_write(read->user_data, addr, data, size);
      break;
   }
}

int
aub_read_command(struct aub_read *read, const void *data, uint32_t data_len)
{
   const uint32_t *p = data, *next;
   ASSERTED const uint32_t *end = data + data_len;
   uint32_t h, header_length, bias;

   assert(data_len >= 4);

   h = *p;
   header_length = h & 0xffff;

   switch (OPCODE(h)) {
   case OPCODE_AUB:
      bias = 2;
      break;
   case OPCODE_NEW_AUB:
      bias = 1;
      break;
   default:
      parse_error(read, data, "unknown opcode %d\n", OPCODE(h));
      return -1;
   }

   next = p + header_length + bias;
   if ((h & 0xffff0000) == MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_BLOCK)) {
      assert(end - p >= 4);
      next += p[4] / 4;
   }

   if (next > end) {
      parse_error(read, data,
            "input ends unexpectedly (command length: %zu, remaining bytes: %zu)\n",
            (uintptr_t)next - (uintptr_t)data,
            (uintptr_t)end  - (uintptr_t)data);
      return -1;
   }

   switch (h & 0xffff0000) {
   case MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_HEADER):
      if (!handle_trace_header(read, p))
         return -1;
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_BLOCK):
      if (!handle_trace_block(read, p))
         return -1;
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_AUB, SUBOPCODE_BMP):
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_VERSION):
      if (!handle_memtrace_version(read, p))
         return -1;
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_REG_WRITE):
      handle_memtrace_reg_write(read, p);
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_WRITE):
      handle_memtrace_mem_write(read, p);
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_MEM_POLL):
      /* fprintf(outfile, "memory poll block (dwords %d):\n", h & 0xffff); */
      break;
   case MAKE_HEADER(TYPE_AUB, OPCODE_NEW_AUB, SUBOPCODE_REG_POLL):
      break;
   default:
      parse_error(read, p,
                  "unknown block type=0x%x, opcode=0x%x, subopcode=0x%x (%08x)\n",
                  TYPE(h), OPCODE(h), SUBOPCODE(h), h);
      return -1;
   }

   return (next - p) * sizeof(*p);
}
