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

#ifndef INTEL_CONTEXT_H
#define INTEL_CONTEXT_H

#include <stdint.h>

#define RING_SIZE         (1 * 4096)
#define PPHWSP_SIZE         (1 * 4096)

#define GFX11_LR_CONTEXT_RENDER_SIZE    (14 * 4096)
#define GFX10_LR_CONTEXT_RENDER_SIZE    (19 * 4096)
#define GFX9_LR_CONTEXT_RENDER_SIZE     (22 * 4096)
#define GFX8_LR_CONTEXT_RENDER_SIZE     (20 * 4096)
#define GFX8_LR_CONTEXT_OTHER_SIZE      (2 * 4096)

#define CONTEXT_RENDER_SIZE GFX9_LR_CONTEXT_RENDER_SIZE /* largest size */
#define CONTEXT_OTHER_SIZE GFX8_LR_CONTEXT_OTHER_SIZE

#define MI_LOAD_REGISTER_IMM_n(n) ((0x22 << 23) | (2 * (n) - 1))
#define MI_LRI_FORCE_POSTED       (1<<12)

#define MI_BATCH_BUFFER_END (0xA << 23)

#define HWS_PGA_RCSUNIT      0x02080
#define HWS_PGA_VCSUNIT0   0x12080
#define HWS_PGA_BCSUNIT      0x22080

#define GFX_MODE_RCSUNIT   0x0229c
#define GFX_MODE_VCSUNIT0   0x1229c
#define GFX_MODE_BCSUNIT   0x2229c

#define EXECLIST_SUBMITPORT_RCSUNIT   0x02230
#define EXECLIST_SUBMITPORT_VCSUNIT0   0x12230
#define EXECLIST_SUBMITPORT_BCSUNIT   0x22230

#define EXECLIST_STATUS_RCSUNIT      0x02234
#define EXECLIST_STATUS_VCSUNIT0   0x12234
#define EXECLIST_STATUS_BCSUNIT      0x22234

#define EXECLIST_SQ_CONTENTS0_RCSUNIT   0x02510
#define EXECLIST_SQ_CONTENTS0_VCSUNIT0   0x12510
#define EXECLIST_SQ_CONTENTS0_BCSUNIT   0x22510

#define EXECLIST_CONTROL_RCSUNIT   0x02550
#define EXECLIST_CONTROL_VCSUNIT0   0x12550
#define EXECLIST_CONTROL_BCSUNIT   0x22550

#define MEMORY_MAP_SIZE (64 /* MiB */ * 1024 * 1024)

#define PTE_SIZE 4
#define GFX8_PTE_SIZE 8

#define NUM_PT_ENTRIES (ALIGN(MEMORY_MAP_SIZE, 4096) / 4096)
#define PT_SIZE ALIGN(NUM_PT_ENTRIES * GFX8_PTE_SIZE, 4096)

#define CONTEXT_FLAGS (0x339)   /* Normal Priority | L3-LLC Coherency |
                                 * PPGTT Enabled |
                                 * Legacy Context with 64 bit VA support |
                                 * Valid
                                 */

#define MI_LOAD_REGISTER_IMM_vals(data, flags, ...) do {                \
      uint32_t __regs[] = { __VA_ARGS__ };                              \
      assert((ARRAY_SIZE(__regs) % 2) == 0);                            \
      *(data)++ = MI_LOAD_REGISTER_IMM_n(ARRAY_SIZE(__regs) / 2) | (flags); \
      for (unsigned __e = 0; __e < ARRAY_SIZE(__regs); __e++)           \
         *(data)++ = __regs[__e];                                       \
   } while (0)


struct intel_context_parameters {
   uint64_t pml4_addr;
   uint64_t ring_addr;
   uint32_t ring_size;
};

typedef void (*intel_context_init_t)(const struct intel_context_parameters *, uint32_t *, uint32_t *);

#include "gfx8_context.h"
#include "gfx10_context.h"

#endif /* INTEL_CONTEXT_H */
