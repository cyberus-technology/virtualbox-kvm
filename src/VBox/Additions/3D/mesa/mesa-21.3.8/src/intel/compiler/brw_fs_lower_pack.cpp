/*
 * Copyright © 2015 Connor Abbott
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

#include "brw_fs.h"
#include "brw_cfg.h"
#include "brw_fs_builder.h"

using namespace brw;

bool
fs_visitor::lower_pack()
{
   bool progress = false;

   foreach_block_and_inst_safe(block, fs_inst, inst, cfg) {
      if (inst->opcode != FS_OPCODE_PACK)
         continue;

      assert(inst->dst.file == VGRF);
      assert(inst->saturate == false);
      fs_reg dst = inst->dst;

      const fs_builder ibld(this, block, inst);
      for (unsigned i = 0; i < inst->sources; i++)
         ibld.MOV(subscript(dst, inst->src[i].type, i), inst->src[i]);

      inst->remove(block);
      progress = true;
   }

   if (progress)
      invalidate_analysis(DEPENDENCY_INSTRUCTIONS);

   return progress;
}
