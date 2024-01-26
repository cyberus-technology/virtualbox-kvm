/*
 * Copyright © 2014 Intel Corporation
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

/** @file brw_fs_combine_constants.cpp
 *
 * This file contains the opt_combine_constants() pass that runs after the
 * regular optimization loop. It passes over the instruction list and
 * selectively promotes immediate values to registers by emitting a mov(1)
 * instruction.
 *
 * This is useful on Gen 7 particularly, because a few instructions can be
 * coissued (i.e., issued in the same cycle as another thread on the same EU
 * issues an instruction) under some circumstances, one of which is that they
 * cannot use immediate values.
 */

#include "brw_fs.h"
#include "brw_cfg.h"
#include "util/half_float.h"

using namespace brw;

static const bool debug = false;

/* Returns whether an instruction could co-issue if its immediate source were
 * replaced with a GRF source.
 */
static bool
could_coissue(const struct intel_device_info *devinfo, const fs_inst *inst)
{
   if (devinfo->ver != 7)
      return false;

   switch (inst->opcode) {
   case BRW_OPCODE_MOV:
   case BRW_OPCODE_CMP:
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_MUL:
      /* Only float instructions can coissue.  We don't have a great
       * understanding of whether or not something like float(int(a) + int(b))
       * would be considered float (based on the destination type) or integer
       * (based on the source types), so we take the conservative choice of
       * only promoting when both destination and source are float.
       */
      return inst->dst.type == BRW_REGISTER_TYPE_F &&
             inst->src[0].type == BRW_REGISTER_TYPE_F;
   default:
      return false;
   }
}

/**
 * Returns true for instructions that don't support immediate sources.
 */
static bool
must_promote_imm(const struct intel_device_info *devinfo, const fs_inst *inst)
{
   switch (inst->opcode) {
   case SHADER_OPCODE_POW:
      return devinfo->ver < 8;
   case BRW_OPCODE_MAD:
   case BRW_OPCODE_ADD3:
   case BRW_OPCODE_LRP:
      return true;
   default:
      return false;
   }
}

/** A box for putting fs_regs in a linked list. */
struct reg_link {
   DECLARE_RALLOC_CXX_OPERATORS(reg_link)

   reg_link(fs_reg *reg) : reg(reg) {}

   struct exec_node link;
   fs_reg *reg;
};

static struct exec_node *
link(void *mem_ctx, fs_reg *reg)
{
   reg_link *l = new(mem_ctx) reg_link(reg);
   return &l->link;
}

/**
 * Information about an immediate value.
 */
struct imm {
   /** The common ancestor of all blocks using this immediate value. */
   bblock_t *block;

   /**
    * The instruction generating the immediate value, if all uses are contained
    * within a single basic block. Otherwise, NULL.
    */
   fs_inst *inst;

   /**
    * A list of fs_regs that refer to this immediate.  If we promote it, we'll
    * have to patch these up to refer to the new GRF.
    */
   exec_list *uses;

   /** The immediate value */
   union {
      char bytes[8];
      double df;
      int64_t d64;
      float f;
      int32_t d;
      int16_t w;
   };
   uint8_t size;

   /** When promoting half-float we need to account for certain restrictions */
   bool is_half_float;

   /**
    * The GRF register and subregister number where we've decided to store the
    * constant value.
    */
   uint8_t subreg_offset;
   uint16_t nr;

   /** The number of coissuable instructions using this immediate. */
   uint16_t uses_by_coissue;

   /**
    * Whether this constant is used by an instruction that can't handle an
    * immediate source (and already has to be promoted to a GRF).
    */
   bool must_promote;

   uint16_t first_use_ip;
   uint16_t last_use_ip;
};

/** The working set of information about immediates. */
struct table {
   struct imm *imm;
   int size;
   int len;
};

static struct imm *
find_imm(struct table *table, void *data, uint8_t size)
{
   for (int i = 0; i < table->len; i++) {
      if (table->imm[i].size == size &&
          !memcmp(table->imm[i].bytes, data, size)) {
         return &table->imm[i];
      }
   }
   return NULL;
}

static struct imm *
new_imm(struct table *table, void *mem_ctx)
{
   if (table->len == table->size) {
      table->size *= 2;
      table->imm = reralloc(mem_ctx, table->imm, struct imm, table->size);
   }
   return &table->imm[table->len++];
}

/**
 * Comparator used for sorting an array of imm structures.
 *
 * We sort by basic block number, then last use IP, then first use IP (least
 * to greatest). This sorting causes immediates live in the same area to be
 * allocated to the same register in the hopes that all values will be dead
 * about the same time and the register can be reused.
 */
static int
compare(const void *_a, const void *_b)
{
   const struct imm *a = (const struct imm *)_a,
                    *b = (const struct imm *)_b;

   int block_diff = a->block->num - b->block->num;
   if (block_diff)
      return block_diff;

   int end_diff = a->last_use_ip - b->last_use_ip;
   if (end_diff)
      return end_diff;

   return a->first_use_ip - b->first_use_ip;
}

static bool
get_constant_value(const struct intel_device_info *devinfo,
                   const fs_inst *inst, uint32_t src_idx,
                   void *out, brw_reg_type *out_type)
{
   const bool can_do_source_mods = inst->can_do_source_mods(devinfo);
   const fs_reg *src = &inst->src[src_idx];

   *out_type = src->type;

   switch (*out_type) {
   case BRW_REGISTER_TYPE_DF: {
      double val = !can_do_source_mods ? src->df : fabs(src->df);
      memcpy(out, &val, 8);
      break;
   }
   case BRW_REGISTER_TYPE_F: {
      float val = !can_do_source_mods ? src->f : fabsf(src->f);
      memcpy(out, &val, 4);
      break;
   }
   case BRW_REGISTER_TYPE_HF: {
      uint16_t val = src->d & 0xffffu;
      if (can_do_source_mods)
         val = _mesa_float_to_half(fabsf(_mesa_half_to_float(val)));
      memcpy(out, &val, 2);
      break;
   }
   case BRW_REGISTER_TYPE_Q: {
      int64_t val = !can_do_source_mods ? src->d64 : llabs(src->d64);
      memcpy(out, &val, 8);
      break;
   }
   case BRW_REGISTER_TYPE_UQ:
      memcpy(out, &src->u64, 8);
      break;
   case BRW_REGISTER_TYPE_D: {
      int32_t val = !can_do_source_mods ? src->d : abs(src->d);
      memcpy(out, &val, 4);
      break;
   }
   case BRW_REGISTER_TYPE_UD:
      memcpy(out, &src->ud, 4);
      break;
   case BRW_REGISTER_TYPE_W: {
      int16_t val = src->d & 0xffffu;
      if (can_do_source_mods)
         val = abs(val);
      memcpy(out, &val, 2);
      break;
   }
   case BRW_REGISTER_TYPE_UW:
      memcpy(out, &src->ud, 2);
      break;
   default:
      return false;
   };

   return true;
}

static struct brw_reg
build_imm_reg_for_copy(struct imm *imm)
{
   switch (imm->size) {
   case 8:
      return brw_imm_d(imm->d64);
   case 4:
      return brw_imm_d(imm->d);
   case 2:
      return brw_imm_w(imm->w);
   default:
      unreachable("not implemented");
   }
}

static inline uint32_t
get_alignment_for_imm(const struct imm *imm)
{
   if (imm->is_half_float)
      return 4; /* At least MAD seems to require this */
   else
      return imm->size;
}

static bool
needs_negate(const fs_reg *reg, const struct imm *imm)
{
   switch (reg->type) {
   case BRW_REGISTER_TYPE_DF:
      return signbit(reg->df) != signbit(imm->df);
   case BRW_REGISTER_TYPE_F:
      return signbit(reg->f) != signbit(imm->f);
   case BRW_REGISTER_TYPE_Q:
      return (reg->d64 < 0) != (imm->d64 < 0);
   case BRW_REGISTER_TYPE_D:
      return (reg->d < 0) != (imm->d < 0);
   case BRW_REGISTER_TYPE_HF:
      return (reg->d & 0x8000u) != (imm->w & 0x8000u);
   case BRW_REGISTER_TYPE_W:
      return ((int16_t)reg->d < 0) != (imm->w < 0);
   case BRW_REGISTER_TYPE_UQ:
   case BRW_REGISTER_TYPE_UD:
   case BRW_REGISTER_TYPE_UW:
      return false;
   default:
      unreachable("not implemented");
   };
}

static bool
representable_as_hf(float f, uint16_t *hf)
{
   union fi u;
   uint16_t h = _mesa_float_to_half(f);
   u.f = _mesa_half_to_float(h);

   if (u.f == f) {
      *hf = h;
      return true;
   }

   return false;
}

static bool
representable_as_w(int d, int16_t *w)
{
   int res = ((d & 0xffff8000) + 0x8000) & 0xffff7fff;
   if (!res) {
      *w = d;
      return true;
   }

   return false;
}

static bool
representable_as_uw(unsigned ud, uint16_t *uw)
{
   if (!(ud & 0xffff0000)) {
      *uw = ud;
      return true;
   }

   return false;
}

static bool
supports_src_as_imm(const struct intel_device_info *devinfo, enum opcode op)
{
   switch (op) {
   case BRW_OPCODE_ADD3:
      return devinfo->verx10 >= 125;
   case BRW_OPCODE_MAD:
      return devinfo->ver == 12 && devinfo->verx10 < 125;
   default:
      return false;
   }
}

static bool
can_promote_src_as_imm(const struct intel_device_info *devinfo, fs_inst *inst,
                       unsigned src_idx)
{
   bool can_promote = false;

   /* Experiment shows that we can only support src0 as immediate */
   if (src_idx != 0)
      return false;

   if (!supports_src_as_imm(devinfo, inst->opcode))
      return false;

   /* TODO - Fix the codepath below to use a bfloat16 immediate on XeHP,
    *        since HF/F mixed mode has been removed from the hardware.
    */
   switch (inst->src[src_idx].type) {
   case BRW_REGISTER_TYPE_F: {
      uint16_t hf;
      if (representable_as_hf(inst->src[src_idx].f, &hf)) {
         inst->src[src_idx] = retype(brw_imm_uw(hf), BRW_REGISTER_TYPE_HF);
         can_promote = true;
      }
      break;
   }
   case BRW_REGISTER_TYPE_W: {
      int16_t w;
      if (representable_as_w(inst->src[src_idx].d, &w)) {
         inst->src[src_idx] = brw_imm_w(w);
         can_promote = true;
      }
      break;
   }
   case BRW_REGISTER_TYPE_UW: {
      uint16_t uw;
      if (representable_as_uw(inst->src[src_idx].ud, &uw)) {
         inst->src[src_idx] = brw_imm_uw(uw);
         can_promote = true;
      }
      break;
   }
   default:
      break;
   }

   return can_promote;
}

bool
fs_visitor::opt_combine_constants()
{
   void *const_ctx = ralloc_context(NULL);

   struct table table;
   table.size = 8;
   table.len = 0;
   table.imm = ralloc_array(const_ctx, struct imm, table.size);

   const brw::idom_tree &idom = idom_analysis.require();
   unsigned ip = -1;

   /* Make a pass through all instructions and count the number of times each
    * constant is used by coissueable instructions or instructions that cannot
    * take immediate arguments.
    */
   foreach_block_and_inst(block, fs_inst, inst, cfg) {
      ip++;

      if (!could_coissue(devinfo, inst) && !must_promote_imm(devinfo, inst))
         continue;

      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file != IMM)
            continue;

         if (can_promote_src_as_imm(devinfo, inst, i))
            continue;

         char data[8];
         brw_reg_type type;
         if (!get_constant_value(devinfo, inst, i, data, &type))
            continue;

         uint8_t size = type_sz(type);

         struct imm *imm = find_imm(&table, data, size);

         if (imm) {
            bblock_t *intersection = idom.intersect(block, imm->block);
            if (intersection != imm->block)
               imm->inst = NULL;
            imm->block = intersection;
            imm->uses->push_tail(link(const_ctx, &inst->src[i]));
            imm->uses_by_coissue += could_coissue(devinfo, inst);
            imm->must_promote = imm->must_promote || must_promote_imm(devinfo, inst);
            imm->last_use_ip = ip;
            if (type == BRW_REGISTER_TYPE_HF)
               imm->is_half_float = true;
         } else {
            imm = new_imm(&table, const_ctx);
            imm->block = block;
            imm->inst = inst;
            imm->uses = new(const_ctx) exec_list();
            imm->uses->push_tail(link(const_ctx, &inst->src[i]));
            memcpy(imm->bytes, data, size);
            imm->size = size;
            imm->is_half_float = type == BRW_REGISTER_TYPE_HF;
            imm->uses_by_coissue = could_coissue(devinfo, inst);
            imm->must_promote = must_promote_imm(devinfo, inst);
            imm->first_use_ip = ip;
            imm->last_use_ip = ip;
         }
      }
   }

   /* Remove constants from the table that don't have enough uses to make them
    * profitable to store in a register.
    */
   for (int i = 0; i < table.len;) {
      struct imm *imm = &table.imm[i];

      if (!imm->must_promote && imm->uses_by_coissue < 4) {
         table.imm[i] = table.imm[table.len - 1];
         table.len--;
         continue;
      }
      i++;
   }
   if (table.len == 0) {
      ralloc_free(const_ctx);
      return false;
   }
   if (cfg->num_blocks != 1)
      qsort(table.imm, table.len, sizeof(struct imm), compare);

   /* Insert MOVs to load the constant values into GRFs. */
   fs_reg reg(VGRF, alloc.allocate(1));
   reg.stride = 0;
   for (int i = 0; i < table.len; i++) {
      struct imm *imm = &table.imm[i];
      /* Insert it either before the instruction that generated the immediate
       * or after the last non-control flow instruction of the common ancestor.
       */
      exec_node *n = (imm->inst ? imm->inst :
                      imm->block->last_non_control_flow_inst()->next);

      /* From the BDW and CHV PRM, 3D Media GPGPU, Special Restrictions:
       *
       *   "In Align16 mode, the channel selects and channel enables apply to a
       *    pair of half-floats, because these parameters are defined for DWord
       *    elements ONLY. This is applicable when both source and destination
       *    are half-floats."
       *
       * This means that Align16 instructions that use promoted HF immediates
       * and use a <0,1,0>:HF region would read 2 HF slots instead of
       * replicating the single one we want. To avoid this, we always populate
       * both HF slots within a DWord with the constant.
       */
      const uint32_t width = devinfo->ver == 8 && imm->is_half_float ? 2 : 1;
      const fs_builder ibld = bld.at(imm->block, n).exec_all().group(width, 0);

      /* Put the immediate in an offset aligned to its size. Some instructions
       * seem to have additional alignment requirements, so account for that
       * too.
       */
      reg.offset = ALIGN(reg.offset, get_alignment_for_imm(imm));

      /* Ensure we have enough space in the register to copy the immediate */
      struct brw_reg imm_reg = build_imm_reg_for_copy(imm);
      if (reg.offset + type_sz(imm_reg.type) * width > REG_SIZE) {
         reg.nr = alloc.allocate(1);
         reg.offset = 0;
      }

      ibld.MOV(retype(reg, imm_reg.type), imm_reg);
      imm->nr = reg.nr;
      imm->subreg_offset = reg.offset;

      reg.offset += imm->size * width;
   }
   shader_stats.promoted_constants = table.len;

   /* Rewrite the immediate sources to refer to the new GRFs. */
   for (int i = 0; i < table.len; i++) {
      foreach_list_typed(reg_link, link, link, table.imm[i].uses) {
         fs_reg *reg = link->reg;
#ifdef DEBUG
         switch (reg->type) {
         case BRW_REGISTER_TYPE_DF:
            assert((isnan(reg->df) && isnan(table.imm[i].df)) ||
                   (fabs(reg->df) == fabs(table.imm[i].df)));
            break;
         case BRW_REGISTER_TYPE_F:
            assert((isnan(reg->f) && isnan(table.imm[i].f)) ||
                   (fabsf(reg->f) == fabsf(table.imm[i].f)));
            break;
         case BRW_REGISTER_TYPE_HF:
            assert((isnan(_mesa_half_to_float(reg->d & 0xffffu)) &&
                    isnan(_mesa_half_to_float(table.imm[i].w))) ||
                   (fabsf(_mesa_half_to_float(reg->d & 0xffffu)) ==
                    fabsf(_mesa_half_to_float(table.imm[i].w))));
            break;
         case BRW_REGISTER_TYPE_Q:
            assert(abs(reg->d64) == abs(table.imm[i].d64));
            break;
         case BRW_REGISTER_TYPE_UQ:
            assert(reg->d64 == table.imm[i].d64);
            break;
         case BRW_REGISTER_TYPE_D:
            assert(abs(reg->d) == abs(table.imm[i].d));
            break;
         case BRW_REGISTER_TYPE_UD:
            assert(reg->d == table.imm[i].d);
            break;
         case BRW_REGISTER_TYPE_W:
            assert(abs((int16_t) (reg->d & 0xffff)) == table.imm[i].w);
            break;
         case BRW_REGISTER_TYPE_UW:
            assert((reg->ud & 0xffffu) == (uint16_t) table.imm[i].w);
            break;
         default:
            break;
         }
#endif

         reg->file = VGRF;
         reg->offset = table.imm[i].subreg_offset;
         reg->stride = 0;
         reg->negate = needs_negate(reg, &table.imm[i]);
         reg->nr = table.imm[i].nr;
      }
   }

   if (debug) {
      for (int i = 0; i < table.len; i++) {
         struct imm *imm = &table.imm[i];

         printf("0x%016" PRIx64 " - block %3d, reg %3d sub %2d, "
                "Uses: (%2d, %2d), IP: %4d to %4d, length %4d\n",
                (uint64_t)(imm->d & BITFIELD64_MASK(imm->size * 8)),
                imm->block->num,
                imm->nr,
                imm->subreg_offset,
                imm->must_promote,
                imm->uses_by_coissue,
                imm->first_use_ip,
                imm->last_use_ip,
                imm->last_use_ip - imm->first_use_ip);
      }
   }

   ralloc_free(const_ctx);
   invalidate_analysis(DEPENDENCY_INSTRUCTIONS | DEPENDENCY_VARIABLES);

   return true;
}
