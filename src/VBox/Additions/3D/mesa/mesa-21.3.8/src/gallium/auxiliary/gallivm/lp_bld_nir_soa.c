/**************************************************************************
 *
 * Copyright 2019 Red Hat.
 * All Rights Reserved.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **************************************************************************/

#include "lp_bld_nir.h"
#include "lp_bld_init.h"
#include "lp_bld_flow.h"
#include "lp_bld_logic.h"
#include "lp_bld_gather.h"
#include "lp_bld_const.h"
#include "lp_bld_struct.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_coro.h"
#include "lp_bld_printf.h"
#include "util/u_math.h"

static int bit_size_to_shift_size(int bit_size)
{
   switch (bit_size) {
   case 64:
      return 3;
   default:
   case 32:
      return 2;
   case 16:
      return 1;
   case 8:
      return 0;
   }
}

/*
 * combine the execution mask if there is one with the current mask.
 */
static LLVMValueRef
mask_vec(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context * bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_exec_mask *exec_mask = &bld->exec_mask;
   LLVMValueRef bld_mask = bld->mask ? lp_build_mask_value(bld->mask) : NULL;
   if (!exec_mask->has_mask) {
      return bld_mask;
   }
   if (!bld_mask)
      return exec_mask->exec_mask;
   return LLVMBuildAnd(builder, lp_build_mask_value(bld->mask),
                       exec_mask->exec_mask, "");
}

static LLVMValueRef
emit_fetch_64bit(
   struct lp_build_nir_context * bld_base,
   LLVMValueRef input,
   LLVMValueRef input2)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef res;
   int i;
   LLVMValueRef shuffles[2 * (LP_MAX_VECTOR_WIDTH/32)];
   int len = bld_base->base.type.length * 2;
   assert(len <= (2 * (LP_MAX_VECTOR_WIDTH/32)));

   for (i = 0; i < bld_base->base.type.length * 2; i+=2) {
#if UTIL_ARCH_LITTLE_ENDIAN
      shuffles[i] = lp_build_const_int32(gallivm, i / 2);
      shuffles[i + 1] = lp_build_const_int32(gallivm, i / 2 + bld_base->base.type.length);
#else
      shuffles[i] = lp_build_const_int32(gallivm, i / 2 + bld_base->base.type.length);
      shuffles[i + 1] = lp_build_const_int32(gallivm, i / 2);
#endif
   }
   res = LLVMBuildShuffleVector(builder, input, input2, LLVMConstVector(shuffles, len), "");

   return LLVMBuildBitCast(builder, res, bld_base->dbl_bld.vec_type, "");
}

static void
emit_store_64bit_split(struct lp_build_nir_context *bld_base,
                       LLVMValueRef value,
                       LLVMValueRef split_values[2])
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned i;
   LLVMValueRef shuffles[LP_MAX_VECTOR_WIDTH/32];
   LLVMValueRef shuffles2[LP_MAX_VECTOR_WIDTH/32];
   int len = bld_base->base.type.length * 2;

   value = LLVMBuildBitCast(gallivm->builder, value, LLVMVectorType(LLVMFloatTypeInContext(gallivm->context), len), "");
   for (i = 0; i < bld_base->base.type.length; i++) {
#if UTIL_ARCH_LITTLE_ENDIAN
      shuffles[i] = lp_build_const_int32(gallivm, i * 2);
      shuffles2[i] = lp_build_const_int32(gallivm, (i * 2) + 1);
#else
      shuffles[i] = lp_build_const_int32(gallivm, i * 2 + 1);
      shuffles2[i] = lp_build_const_int32(gallivm, i * 2);
#endif
   }

   split_values[0] = LLVMBuildShuffleVector(builder, value,
                                 LLVMGetUndef(LLVMTypeOf(value)),
                                 LLVMConstVector(shuffles,
                                                 bld_base->base.type.length),
                                 "");
   split_values[1] = LLVMBuildShuffleVector(builder, value,
                                  LLVMGetUndef(LLVMTypeOf(value)),
                                  LLVMConstVector(shuffles2,
                                                  bld_base->base.type.length),
                                  "");
}

static void
emit_store_64bit_chan(struct lp_build_nir_context *bld_base,
                      LLVMValueRef chan_ptr,
                      LLVMValueRef chan_ptr2,
                      LLVMValueRef value)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct lp_build_context *float_bld = &bld_base->base;
   LLVMValueRef split_vals[2];

   emit_store_64bit_split(bld_base, value, split_vals);

   lp_exec_mask_store(&bld->exec_mask, float_bld, split_vals[0], chan_ptr);
   lp_exec_mask_store(&bld->exec_mask, float_bld, split_vals[1], chan_ptr2);
}

static LLVMValueRef
get_soa_array_offsets(struct lp_build_context *uint_bld,
                      LLVMValueRef indirect_index,
                      int num_components,
                      unsigned chan_index,
                      bool need_perelement_offset)
{
   struct gallivm_state *gallivm = uint_bld->gallivm;
   LLVMValueRef chan_vec =
      lp_build_const_int_vec(uint_bld->gallivm, uint_bld->type, chan_index);
   LLVMValueRef length_vec =
      lp_build_const_int_vec(gallivm, uint_bld->type, uint_bld->type.length);
   LLVMValueRef index_vec;

   /* index_vec = (indirect_index * 4 + chan_index) * length + offsets */
   index_vec = lp_build_mul(uint_bld, indirect_index, lp_build_const_int_vec(uint_bld->gallivm, uint_bld->type, num_components));
   index_vec = lp_build_add(uint_bld, index_vec, chan_vec);
   index_vec = lp_build_mul(uint_bld, index_vec, length_vec);

   if (need_perelement_offset) {
      LLVMValueRef pixel_offsets;
      unsigned i;
     /* build pixel offset vector: {0, 1, 2, 3, ...} */
      pixel_offsets = uint_bld->undef;
      for (i = 0; i < uint_bld->type.length; i++) {
         LLVMValueRef ii = lp_build_const_int32(gallivm, i);
         pixel_offsets = LLVMBuildInsertElement(gallivm->builder, pixel_offsets,
                                                ii, ii, "");
      }
      index_vec = lp_build_add(uint_bld, index_vec, pixel_offsets);
   }
   return index_vec;
}

static LLVMValueRef
build_gather(struct lp_build_nir_context *bld_base,
             struct lp_build_context *bld,
             LLVMValueRef base_ptr,
             LLVMValueRef indexes,
             LLVMValueRef overflow_mask,
             LLVMValueRef indexes2)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef res;
   unsigned i;

   if (indexes2)
      res = LLVMGetUndef(LLVMVectorType(LLVMFloatTypeInContext(gallivm->context), bld_base->base.type.length * 2));
   else
      res = bld->undef;
   /*
    * overflow_mask is a vector telling us which channels
    * in the vector overflowed. We use the overflow behavior for
    * constant buffers which is defined as:
    * Out of bounds access to constant buffer returns 0 in all
    * components. Out of bounds behavior is always with respect
    * to the size of the buffer bound at that slot.
    */

   if (overflow_mask) {
      /*
       * We avoid per-element control flow here (also due to llvm going crazy,
       * though I suspect it's better anyway since overflow is likely rare).
       * Note that since we still fetch from buffers even if num_elements was
       * zero (in this case we'll fetch from index zero) the jit func callers
       * MUST provide valid fake constant buffers of size 4x32 (the values do
       * not matter), otherwise we'd still need (not per element though)
       * control flow.
       */
      indexes = lp_build_select(uint_bld, overflow_mask, uint_bld->zero, indexes);
      if (indexes2)
         indexes2 = lp_build_select(uint_bld, overflow_mask, uint_bld->zero, indexes2);
   }

   /*
    * Loop over elements of index_vec, load scalar value, insert it into 'res'.
    */
   for (i = 0; i < bld->type.length * (indexes2 ? 2 : 1); i++) {
      LLVMValueRef si, di;
      LLVMValueRef index;
      LLVMValueRef scalar_ptr, scalar;

      di = lp_build_const_int32(gallivm, i);
      if (indexes2)
         si = lp_build_const_int32(gallivm, i >> 1);
      else
         si = di;

      if (indexes2 && (i & 1)) {
         index = LLVMBuildExtractElement(builder,
                                         indexes2, si, "");
      } else {
         index = LLVMBuildExtractElement(builder,
                                         indexes, si, "");
      }
      scalar_ptr = LLVMBuildGEP(builder, base_ptr,
                                &index, 1, "gather_ptr");
      scalar = LLVMBuildLoad(builder, scalar_ptr, "");

      res = LLVMBuildInsertElement(builder, res, scalar, di, "");
   }

   if (overflow_mask) {
      if (indexes2) {
         res = LLVMBuildBitCast(builder, res, bld_base->dbl_bld.vec_type, "");
         overflow_mask = LLVMBuildSExt(builder, overflow_mask,
                                       bld_base->dbl_bld.int_vec_type, "");
         res = lp_build_select(&bld_base->dbl_bld, overflow_mask,
                               bld_base->dbl_bld.zero, res);
      } else
         res = lp_build_select(bld, overflow_mask, bld->zero, res);
   }

   return res;
}

/**
 * Scatter/store vector.
 */
static void
emit_mask_scatter(struct lp_build_nir_soa_context *bld,
                  LLVMValueRef base_ptr,
                  LLVMValueRef indexes,
                  LLVMValueRef values,
                  struct lp_exec_mask *mask)
{
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned i;
   LLVMValueRef pred = mask->has_mask ? mask->exec_mask : NULL;

   /*
    * Loop over elements of index_vec, store scalar value.
    */
   for (i = 0; i < bld->bld_base.base.type.length; i++) {
      LLVMValueRef ii = lp_build_const_int32(gallivm, i);
      LLVMValueRef index = LLVMBuildExtractElement(builder, indexes, ii, "");
      LLVMValueRef scalar_ptr = LLVMBuildGEP(builder, base_ptr, &index, 1, "scatter_ptr");
      LLVMValueRef val = LLVMBuildExtractElement(builder, values, ii, "scatter_val");
      LLVMValueRef scalar_pred = pred ?
         LLVMBuildExtractElement(builder, pred, ii, "scatter_pred") : NULL;

      if (0)
         lp_build_printf(gallivm, "scatter %d: val %f at %d %p\n",
                         ii, val, index, scalar_ptr);

      if (scalar_pred) {
         LLVMValueRef real_val, dst_val;
         dst_val = LLVMBuildLoad(builder, scalar_ptr, "");
         scalar_pred = LLVMBuildTrunc(builder, scalar_pred, LLVMInt1TypeInContext(gallivm->context), "");
         real_val = LLVMBuildSelect(builder, scalar_pred, val, dst_val, "");
         LLVMBuildStore(builder, real_val, scalar_ptr);
      }
      else {
         LLVMBuildStore(builder, val, scalar_ptr);
      }
   }
}

static void emit_load_var(struct lp_build_nir_context *bld_base,
                           nir_variable_mode deref_mode,
                           unsigned num_components,
                           unsigned bit_size,
                           nir_variable *var,
                           unsigned vertex_index,
                           LLVMValueRef indir_vertex_index,
                           unsigned const_index,
                           LLVMValueRef indir_index,
                           LLVMValueRef result[NIR_MAX_VEC_COMPONENTS])
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   int dmul = bit_size == 64 ? 2 : 1;
   unsigned location = var->data.driver_location;
   unsigned location_frac = var->data.location_frac;

   if (!var->data.compact && !indir_index)
      location += const_index;
   else if (var->data.compact) {
      location += const_index / 4;
      location_frac += const_index % 4;
      const_index = 0;
   }
   switch (deref_mode) {
   case nir_var_shader_in:
      for (unsigned i = 0; i < num_components; i++) {
         int idx = (i * dmul) + location_frac;
         int comp_loc = location;

         if (bit_size == 64 && idx >= 4) {
            comp_loc++;
            idx = idx % 4;
         }

         if (bld->gs_iface) {
            LLVMValueRef vertex_index_val = lp_build_const_int32(gallivm, vertex_index);
            LLVMValueRef attrib_index_val = lp_build_const_int32(gallivm, comp_loc);
            LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx);
            LLVMValueRef result2;

            result[i] = bld->gs_iface->fetch_input(bld->gs_iface, &bld_base->base,
                                                   false, vertex_index_val, 0, attrib_index_val, swizzle_index_val);
            if (bit_size == 64) {
               LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx + 1);
               result2 = bld->gs_iface->fetch_input(bld->gs_iface, &bld_base->base,
                                                    false, vertex_index_val, 0, attrib_index_val, swizzle_index_val);
               result[i] = emit_fetch_64bit(bld_base, result[i], result2);
            }
         } else if (bld->tes_iface) {
            LLVMValueRef vertex_index_val = lp_build_const_int32(gallivm, vertex_index);
            LLVMValueRef attrib_index_val;
            LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx);
            LLVMValueRef result2;

            if (indir_index) {
               if (var->data.compact) {
                  swizzle_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, idx));
                  attrib_index_val = lp_build_const_int32(gallivm, comp_loc);
               } else
                  attrib_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, comp_loc));
            } else
               attrib_index_val = lp_build_const_int32(gallivm, comp_loc);

            if (var->data.patch) {
               result[i] = bld->tes_iface->fetch_patch_input(bld->tes_iface, &bld_base->base,
                                                             indir_index ? true : false, attrib_index_val, swizzle_index_val);
               if (bit_size == 64) {
                  LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx + 1);
                  result2 = bld->tes_iface->fetch_patch_input(bld->tes_iface, &bld_base->base,
                                                              indir_index ? true : false, attrib_index_val, swizzle_index_val);
                  result[i] = emit_fetch_64bit(bld_base, result[i], result2);
               }
            }
            else {
               result[i] = bld->tes_iface->fetch_vertex_input(bld->tes_iface, &bld_base->base,
                                                              indir_vertex_index ? true : false,
                                                              indir_vertex_index ? indir_vertex_index : vertex_index_val,
                                                              (indir_index && !var->data.compact) ? true : false, attrib_index_val,
                                                              (indir_index && var->data.compact) ? true : false, swizzle_index_val);
               if (bit_size == 64) {
                  LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx + 1);
                  result2 = bld->tes_iface->fetch_vertex_input(bld->tes_iface, &bld_base->base,
                                                               indir_vertex_index ? true : false,
                                                               indir_vertex_index ? indir_vertex_index : vertex_index_val,
                                                               indir_index ? true : false, attrib_index_val, false, swizzle_index_val);
                  result[i] = emit_fetch_64bit(bld_base, result[i], result2);
               }
            }
         } else if (bld->tcs_iface) {
            LLVMValueRef vertex_index_val = lp_build_const_int32(gallivm, vertex_index);
            LLVMValueRef attrib_index_val;
            LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx);

            if (indir_index) {
               if (var->data.compact) {
                  swizzle_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, idx));
                  attrib_index_val = lp_build_const_int32(gallivm, comp_loc);
               } else
                  attrib_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, comp_loc));
            } else
               attrib_index_val = lp_build_const_int32(gallivm, comp_loc);
            result[i] = bld->tcs_iface->emit_fetch_input(bld->tcs_iface, &bld_base->base,
                                                         indir_vertex_index ? true : false, indir_vertex_index ? indir_vertex_index : vertex_index_val,
                                                         (indir_index && !var->data.compact) ? true : false, attrib_index_val,
                                                         (indir_index && var->data.compact) ? true : false, swizzle_index_val);
            if (bit_size == 64) {
               LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx + 1);
               LLVMValueRef result2 = bld->tcs_iface->emit_fetch_input(bld->tcs_iface, &bld_base->base,
                                                                       indir_vertex_index ? true : false, indir_vertex_index ? indir_vertex_index : vertex_index_val,
                                                                       indir_index ? true : false, attrib_index_val,
                                                                       false, swizzle_index_val);
               result[i] = emit_fetch_64bit(bld_base, result[i], result2);
            }
         } else {
            if (indir_index) {
               LLVMValueRef attrib_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, comp_loc));
               LLVMValueRef index_vec = get_soa_array_offsets(&bld_base->uint_bld,
                                                              attrib_index_val, 4, idx,
                                                              TRUE);
               LLVMValueRef index_vec2 = NULL;
               LLVMTypeRef fptr_type;
               LLVMValueRef inputs_array;
               fptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
               inputs_array = LLVMBuildBitCast(gallivm->builder, bld->inputs_array, fptr_type, "");

               if (bit_size == 64)
                  index_vec2 = get_soa_array_offsets(&bld_base->uint_bld,
                                                     indir_index, 4, idx + 1, TRUE);

               /* Gather values from the input register array */
               result[i] = build_gather(bld_base, &bld_base->base, inputs_array, index_vec, NULL, index_vec2);
            } else {
               if (bld->indirects & nir_var_shader_in) {
                  LLVMValueRef lindex = lp_build_const_int32(gallivm,
                                                             comp_loc * 4 + idx);
                  LLVMValueRef input_ptr = lp_build_pointer_get(gallivm->builder,
                                                             bld->inputs_array, lindex);
                  if (bit_size == 64) {
                     LLVMValueRef lindex2 = lp_build_const_int32(gallivm,
                                                                 comp_loc * 4 + (idx + 1));
                     LLVMValueRef input_ptr2 = lp_build_pointer_get(gallivm->builder,
                                                                    bld->inputs_array, lindex2);
                     result[i] = emit_fetch_64bit(bld_base, input_ptr, input_ptr2);
                  } else {
                     result[i] = input_ptr;
                  }
               } else {
                  if (bit_size == 64) {
                     LLVMValueRef tmp[2];
                     tmp[0] = bld->inputs[comp_loc][idx];
                     tmp[1] = bld->inputs[comp_loc][idx + 1];
                     result[i] = emit_fetch_64bit(bld_base, tmp[0], tmp[1]);
                  } else {
                     result[i] = bld->inputs[comp_loc][idx];
                  }
               }
            }
         }
      }
      break;
   case nir_var_shader_out:
      if (bld->fs_iface && bld->fs_iface->fb_fetch) {
         bld->fs_iface->fb_fetch(bld->fs_iface, &bld_base->base, var->data.location, result);
         return;
      }
      for (unsigned i = 0; i < num_components; i++) {
         int idx = (i * dmul) + location_frac;
         if (bld->tcs_iface) {
            LLVMValueRef vertex_index_val = lp_build_const_int32(gallivm, vertex_index);
            LLVMValueRef attrib_index_val;
            LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx);

            if (indir_index)
               attrib_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, var->data.driver_location));
            else
               attrib_index_val = lp_build_const_int32(gallivm, location);

            result[i] = bld->tcs_iface->emit_fetch_output(bld->tcs_iface, &bld_base->base,
                                                          indir_vertex_index ? true : false, indir_vertex_index ? indir_vertex_index : vertex_index_val,
                                                          (indir_index && !var->data.compact) ? true : false, attrib_index_val,
                                                          (indir_index && var->data.compact) ? true : false, swizzle_index_val, 0);
            if (bit_size == 64) {
               LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, idx + 1);
               LLVMValueRef result2 = bld->tcs_iface->emit_fetch_output(bld->tcs_iface, &bld_base->base,
                                                                        indir_vertex_index ? true : false, indir_vertex_index ? indir_vertex_index : vertex_index_val,
                                                                        indir_index ? true : false, attrib_index_val,
                                                                        false, swizzle_index_val, 0);
               result[i] = emit_fetch_64bit(bld_base, result[i], result2);
            }
         }
      }
      break;
   default:
      break;
   }
}

static void emit_store_chan(struct lp_build_nir_context *bld_base,
                            nir_variable_mode deref_mode,
                            unsigned bit_size,
                            unsigned location, unsigned comp,
                            unsigned chan,
                            LLVMValueRef dst)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *float_bld = &bld_base->base;

   if (bit_size == 64) {
      chan *= 2;
      chan += comp;
      if (chan >= 4) {
         chan -= 4;
         location++;
      }
      emit_store_64bit_chan(bld_base, bld->outputs[location][chan],
                            bld->outputs[location][chan + 1], dst);
   } else {
      dst = LLVMBuildBitCast(builder, dst, float_bld->vec_type, "");
      lp_exec_mask_store(&bld->exec_mask, float_bld, dst,
                         bld->outputs[location][chan + comp]);
   }
}

static void emit_store_tcs_chan(struct lp_build_nir_context *bld_base,
                                bool is_compact,
                                unsigned bit_size,
                                unsigned location,
                                unsigned const_index,
                                LLVMValueRef indir_vertex_index,
                                LLVMValueRef indir_index,
                                unsigned comp,
                                unsigned chan,
                                LLVMValueRef chan_val)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   unsigned swizzle = chan;
   if (bit_size == 64) {
      swizzle *= 2;
      swizzle += comp;
      if (swizzle >= 4) {
         swizzle -= 4;
         location++;
      }
   } else
      swizzle += comp;
   LLVMValueRef attrib_index_val;
   LLVMValueRef swizzle_index_val = lp_build_const_int32(gallivm, swizzle);

   if (indir_index) {
      if (is_compact) {
         swizzle_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, swizzle));
         attrib_index_val = lp_build_const_int32(gallivm, const_index + location);
      } else
         attrib_index_val = lp_build_add(&bld_base->uint_bld, indir_index, lp_build_const_int_vec(gallivm, bld_base->uint_bld.type, location));
   } else
      attrib_index_val = lp_build_const_int32(gallivm, const_index + location);
   if (bit_size == 64) {
      LLVMValueRef split_vals[2];
      LLVMValueRef swizzle_index_val2 = lp_build_const_int32(gallivm, swizzle + 1);
      emit_store_64bit_split(bld_base, chan_val, split_vals);
      bld->tcs_iface->emit_store_output(bld->tcs_iface, &bld_base->base, 0,
                                        indir_vertex_index ? true : false,
                                        indir_vertex_index,
                                        indir_index ? true : false,
                                        attrib_index_val,
                                        false, swizzle_index_val,
                                        split_vals[0], mask_vec(bld_base));
      bld->tcs_iface->emit_store_output(bld->tcs_iface, &bld_base->base, 0,
                                        indir_vertex_index ? true : false,
                                        indir_vertex_index,
                                        indir_index ? true : false,
                                        attrib_index_val,
                                        false, swizzle_index_val2,
                                        split_vals[1], mask_vec(bld_base));
   } else {
      chan_val = LLVMBuildBitCast(builder, chan_val, bld_base->base.vec_type, "");
      bld->tcs_iface->emit_store_output(bld->tcs_iface, &bld_base->base, 0,
                                        indir_vertex_index ? true : false,
                                        indir_vertex_index,
                                        indir_index && !is_compact ? true : false,
                                        attrib_index_val,
                                        indir_index && is_compact ? true : false,
                                        swizzle_index_val,
                                        chan_val, mask_vec(bld_base));
   }
}

static void emit_store_var(struct lp_build_nir_context *bld_base,
                           nir_variable_mode deref_mode,
                           unsigned num_components,
                           unsigned bit_size,
                           nir_variable *var,
                           unsigned writemask,
                           LLVMValueRef indir_vertex_index,
                           unsigned const_index,
                           LLVMValueRef indir_index,
                           LLVMValueRef dst)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   switch (deref_mode) {
   case nir_var_shader_out: {
      unsigned location = var->data.driver_location;
      unsigned comp = var->data.location_frac;
      if (bld_base->shader->info.stage == MESA_SHADER_FRAGMENT) {
         if (var->data.location == FRAG_RESULT_STENCIL)
            comp = 1;
         else if (var->data.location == FRAG_RESULT_DEPTH)
            comp = 2;
      }

      if (var->data.compact) {
         location += const_index / 4;
         comp += const_index % 4;
         const_index = 0;
      }

      for (unsigned chan = 0; chan < num_components; chan++) {
         if (writemask & (1u << chan)) {
            LLVMValueRef chan_val = (num_components == 1) ? dst : LLVMBuildExtractValue(builder, dst, chan, "");
            if (bld->tcs_iface) {
               emit_store_tcs_chan(bld_base, var->data.compact, bit_size, location, const_index, indir_vertex_index, indir_index, comp, chan, chan_val);
            } else
               emit_store_chan(bld_base, deref_mode, bit_size, location + const_index, comp, chan, chan_val);
         }
      }
      break;
   }
   default:
      break;
   }
}

static LLVMValueRef emit_load_reg(struct lp_build_nir_context *bld_base,
                                  struct lp_build_context *reg_bld,
                                  const nir_reg_src *reg,
                                  LLVMValueRef indir_src,
                                  LLVMValueRef reg_storage)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   int nc = reg->reg->num_components;
   LLVMValueRef vals[NIR_MAX_VEC_COMPONENTS] = { NULL };
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   if (reg->reg->num_array_elems) {
      LLVMValueRef indirect_val = lp_build_const_int_vec(gallivm, uint_bld->type, reg->base_offset);
      if (reg->indirect) {
         LLVMValueRef max_index = lp_build_const_int_vec(gallivm, uint_bld->type, reg->reg->num_array_elems - 1);
         indirect_val = LLVMBuildAdd(builder, indirect_val, indir_src, "");
         indirect_val = lp_build_min(uint_bld, indirect_val, max_index);
      }
      reg_storage = LLVMBuildBitCast(builder, reg_storage, LLVMPointerType(reg_bld->elem_type, 0), "");
      for (unsigned i = 0; i < nc; i++) {
         LLVMValueRef indirect_offset = get_soa_array_offsets(uint_bld, indirect_val, nc, i, TRUE);
         vals[i] = build_gather(bld_base, reg_bld, reg_storage, indirect_offset, NULL, NULL);
      }
   } else {
      for (unsigned i = 0; i < nc; i++) {
         LLVMValueRef this_storage = nc == 1 ? reg_storage : lp_build_array_get_ptr(gallivm, reg_storage,
                                                                                    lp_build_const_int32(gallivm, i));
         vals[i] = LLVMBuildLoad(builder, this_storage, "");
      }
   }
   return nc == 1 ? vals[0] : lp_nir_array_build_gather_values(builder, vals, nc);
}

static void emit_store_reg(struct lp_build_nir_context *bld_base,
                           struct lp_build_context *reg_bld,
                           const nir_reg_dest *reg,
                           unsigned writemask,
                           LLVMValueRef indir_src,
                           LLVMValueRef reg_storage,
                           LLVMValueRef dst[NIR_MAX_VEC_COMPONENTS])
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   int nc = reg->reg->num_components;
   if (reg->reg->num_array_elems > 0) {
      LLVMValueRef indirect_val = lp_build_const_int_vec(gallivm, uint_bld->type, reg->base_offset);
      if (reg->indirect) {
         LLVMValueRef max_index = lp_build_const_int_vec(gallivm, uint_bld->type, reg->reg->num_array_elems - 1);
         indirect_val = LLVMBuildAdd(builder, indirect_val, indir_src, "");
         indirect_val = lp_build_min(uint_bld, indirect_val, max_index);
      }
      reg_storage = LLVMBuildBitCast(builder, reg_storage, LLVMPointerType(reg_bld->elem_type, 0), "");
      for (unsigned i = 0; i < nc; i++) {
         if (!(writemask & (1 << i)))
            continue;
         LLVMValueRef indirect_offset = get_soa_array_offsets(uint_bld, indirect_val, nc, i, TRUE);
         dst[i] = LLVMBuildBitCast(builder, dst[i], reg_bld->vec_type, "");
         emit_mask_scatter(bld, reg_storage, indirect_offset, dst[i], &bld->exec_mask);
      }
      return;
   }

   for (unsigned i = 0; i < nc; i++) {
      LLVMValueRef this_storage = nc == 1 ? reg_storage : lp_build_array_get_ptr(gallivm, reg_storage,
                                                         lp_build_const_int32(gallivm, i));
      dst[i] = LLVMBuildBitCast(builder, dst[i], reg_bld->vec_type, "");
      lp_exec_mask_store(&bld->exec_mask, reg_bld, dst[i], this_storage);
   }
}

static void emit_load_kernel_arg(struct lp_build_nir_context *bld_base,
                                 unsigned nc,
                                 unsigned bit_size,
                                 unsigned offset_bit_size,
                                 bool offset_is_uniform,
                                 LLVMValueRef offset,
                                 LLVMValueRef result[NIR_MAX_VEC_COMPONENTS])
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *bld_broad = get_int_bld(bld_base, true, bit_size);
   LLVMValueRef kernel_args_ptr = bld->kernel_args_ptr;
   unsigned size_shift = bit_size_to_shift_size(bit_size);
   struct lp_build_context *bld_offset = get_int_bld(bld_base, true, offset_bit_size);
   if (size_shift)
      offset = lp_build_shr(bld_offset, offset, lp_build_const_int_vec(gallivm, bld_offset->type, size_shift));

   LLVMTypeRef ptr_type = LLVMPointerType(bld_broad->elem_type, 0);
   kernel_args_ptr = LLVMBuildBitCast(builder, kernel_args_ptr, ptr_type, "");

   if (offset_is_uniform) {
      offset = LLVMBuildExtractElement(builder, offset, lp_build_const_int32(gallivm, 0), "");

      for (unsigned c = 0; c < nc; c++) {
         LLVMValueRef this_offset = LLVMBuildAdd(builder, offset, offset_bit_size == 64 ? lp_build_const_int64(gallivm, c) : lp_build_const_int32(gallivm, c), "");

         LLVMValueRef scalar = lp_build_pointer_get(builder, kernel_args_ptr, this_offset);
         result[c] = lp_build_broadcast_scalar(bld_broad, scalar);
      }
   }
}

static LLVMValueRef global_addr_to_ptr(struct gallivm_state *gallivm, LLVMValueRef addr_ptr, unsigned bit_size)
{
   LLVMBuilderRef builder = gallivm->builder;
   switch (bit_size) {
   case 8:
      addr_ptr = LLVMBuildIntToPtr(builder, addr_ptr, LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0), "");
      break;
   case 16:
      addr_ptr = LLVMBuildIntToPtr(builder, addr_ptr, LLVMPointerType(LLVMInt16TypeInContext(gallivm->context), 0), "");
      break;
   case 32:
   default:
      addr_ptr = LLVMBuildIntToPtr(builder, addr_ptr, LLVMPointerType(LLVMInt32TypeInContext(gallivm->context), 0), "");
      break;
   case 64:
      addr_ptr = LLVMBuildIntToPtr(builder, addr_ptr, LLVMPointerType(LLVMInt64TypeInContext(gallivm->context), 0), "");
      break;
   }
   return addr_ptr;
}

static void emit_load_global(struct lp_build_nir_context *bld_base,
                             unsigned nc,
                             unsigned bit_size,
                             unsigned addr_bit_size,
                             LLVMValueRef addr,
                             LLVMValueRef outval[NIR_MAX_VEC_COMPONENTS])
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   struct lp_build_context *res_bld;

   res_bld = get_int_bld(bld_base, true, bit_size);

   for (unsigned c = 0; c < nc; c++) {
      LLVMValueRef result = lp_build_alloca(gallivm, res_bld->vec_type, "");
      LLVMValueRef exec_mask = mask_vec(bld_base);
      struct lp_build_loop_state loop_state;
      lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));

      struct lp_build_if_state ifthen;
      LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
      cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
      lp_build_if(&ifthen, gallivm, cond);

      LLVMValueRef addr_ptr = LLVMBuildExtractElement(gallivm->builder, addr,
                                                      loop_state.counter, "");
      addr_ptr = global_addr_to_ptr(gallivm, addr_ptr, bit_size);

      LLVMValueRef value_ptr = lp_build_pointer_get(builder, addr_ptr, lp_build_const_int32(gallivm, c));

      LLVMValueRef temp_res;
      temp_res = LLVMBuildLoad(builder, result, "");
      temp_res = LLVMBuildInsertElement(builder, temp_res, value_ptr, loop_state.counter, "");
      LLVMBuildStore(builder, temp_res, result);
      lp_build_endif(&ifthen);
      lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                             NULL, LLVMIntUGE);
      outval[c] = LLVMBuildLoad(builder, result, "");
   }
}

static void emit_store_global(struct lp_build_nir_context *bld_base,
                              unsigned writemask,
                              unsigned nc, unsigned bit_size,
                              unsigned addr_bit_size,
                              LLVMValueRef addr,
                              LLVMValueRef dst)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;

   for (unsigned c = 0; c < nc; c++) {
      if (!(writemask & (1u << c)))
         continue;
      LLVMValueRef val = (nc == 1) ? dst : LLVMBuildExtractValue(builder, dst, c, "");

      LLVMValueRef exec_mask = mask_vec(bld_base);
      struct lp_build_loop_state loop_state;
      lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
      LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, val,
                                                       loop_state.counter, "");

      LLVMValueRef addr_ptr = LLVMBuildExtractElement(gallivm->builder, addr,
                                                      loop_state.counter, "");
      addr_ptr = global_addr_to_ptr(gallivm, addr_ptr, bit_size);
      switch (bit_size) {
      case 8:
         value_ptr = LLVMBuildBitCast(builder, value_ptr, LLVMInt8TypeInContext(gallivm->context), "");
         break;
      case 16:
         value_ptr = LLVMBuildBitCast(builder, value_ptr, LLVMInt16TypeInContext(gallivm->context), "");
         break;
      case 32:
         value_ptr = LLVMBuildBitCast(builder, value_ptr, LLVMInt32TypeInContext(gallivm->context), "");
         break;
      case 64:
         value_ptr = LLVMBuildBitCast(builder, value_ptr, LLVMInt64TypeInContext(gallivm->context), "");
         break;
      default:
         break;
      }
      struct lp_build_if_state ifthen;

      LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
      cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
      lp_build_if(&ifthen, gallivm, cond);
      lp_build_pointer_set(builder, addr_ptr, lp_build_const_int32(gallivm, c), value_ptr);
      lp_build_endif(&ifthen);
      lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                             NULL, LLVMIntUGE);
   }
}

static void emit_atomic_global(struct lp_build_nir_context *bld_base,
                               nir_intrinsic_op nir_op,
                               unsigned addr_bit_size,
                               unsigned val_bit_size,
                               LLVMValueRef addr,
                               LLVMValueRef val, LLVMValueRef val2,
                               LLVMValueRef *result)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   struct lp_build_context *atom_bld = get_int_bld(bld_base, true, val_bit_size);
   LLVMValueRef atom_res = lp_build_alloca(gallivm,
                                           LLVMTypeOf(val), "");
   LLVMValueRef exec_mask = mask_vec(bld_base);
   struct lp_build_loop_state loop_state;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));

   LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, val,
                                                    loop_state.counter, "");

   LLVMValueRef addr_ptr = LLVMBuildExtractElement(gallivm->builder, addr,
                                                   loop_state.counter, "");
   addr_ptr = global_addr_to_ptr(gallivm, addr_ptr, 32);
   struct lp_build_if_state ifthen;
   LLVMValueRef cond, temp_res;
   LLVMValueRef scalar;
   cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
   cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
   lp_build_if(&ifthen, gallivm, cond);

   addr_ptr = LLVMBuildBitCast(gallivm->builder, addr_ptr, LLVMPointerType(LLVMTypeOf(value_ptr), 0), "");
   if (nir_op == nir_intrinsic_global_atomic_comp_swap) {
      LLVMValueRef cas_src_ptr = LLVMBuildExtractElement(gallivm->builder, val2,
                                                         loop_state.counter, "");
      cas_src_ptr = LLVMBuildBitCast(gallivm->builder, cas_src_ptr, atom_bld->elem_type, "");
      scalar = LLVMBuildAtomicCmpXchg(builder, addr_ptr, value_ptr,
                                      cas_src_ptr,
                                      LLVMAtomicOrderingSequentiallyConsistent,
                                      LLVMAtomicOrderingSequentiallyConsistent,
                                      false);
      scalar = LLVMBuildExtractValue(gallivm->builder, scalar, 0, "");
   } else {
      LLVMAtomicRMWBinOp op;
      switch (nir_op) {
      case nir_intrinsic_global_atomic_add:
         op = LLVMAtomicRMWBinOpAdd;
         break;
      case nir_intrinsic_global_atomic_exchange:

         op = LLVMAtomicRMWBinOpXchg;
         break;
      case nir_intrinsic_global_atomic_and:
         op = LLVMAtomicRMWBinOpAnd;
         break;
      case nir_intrinsic_global_atomic_or:
         op = LLVMAtomicRMWBinOpOr;
         break;
      case nir_intrinsic_global_atomic_xor:
         op = LLVMAtomicRMWBinOpXor;
         break;
      case nir_intrinsic_global_atomic_umin:
         op = LLVMAtomicRMWBinOpUMin;
         break;
      case nir_intrinsic_global_atomic_umax:
         op = LLVMAtomicRMWBinOpUMax;
         break;
      case nir_intrinsic_global_atomic_imin:
         op = LLVMAtomicRMWBinOpMin;
         break;
      case nir_intrinsic_global_atomic_imax:
         op = LLVMAtomicRMWBinOpMax;
         break;
      default:
         unreachable("unknown atomic op");
      }

      scalar = LLVMBuildAtomicRMW(builder, op,
                                  addr_ptr, value_ptr,
                                  LLVMAtomicOrderingSequentiallyConsistent,
                                  false);
   }
   temp_res = LLVMBuildLoad(builder, atom_res, "");
   temp_res = LLVMBuildInsertElement(builder, temp_res, scalar, loop_state.counter, "");
   LLVMBuildStore(builder, temp_res, atom_res);
   lp_build_else(&ifthen);
   temp_res = LLVMBuildLoad(builder, atom_res, "");
   bool is_float = LLVMTypeOf(val) == bld_base->base.vec_type;
   LLVMValueRef zero_val;
   if (is_float) {
      if (val_bit_size == 64)
         zero_val = lp_build_const_double(gallivm, 0);
      else
         zero_val = lp_build_const_float(gallivm, 0);
   } else {
      if (val_bit_size == 64)
         zero_val = lp_build_const_int64(gallivm, 0);
      else
         zero_val = lp_build_const_int32(gallivm, 0);
   }

   temp_res = LLVMBuildInsertElement(builder, temp_res, zero_val, loop_state.counter, "");
   LLVMBuildStore(builder, temp_res, atom_res);
   lp_build_endif(&ifthen);
   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                          NULL, LLVMIntUGE);
   *result = LLVMBuildLoad(builder, atom_res, "");
}

static void emit_load_ubo(struct lp_build_nir_context *bld_base,
                          unsigned nc,
                          unsigned bit_size,
                          bool offset_is_uniform,
                          LLVMValueRef index,
                          LLVMValueRef offset,
                          LLVMValueRef result[NIR_MAX_VEC_COMPONENTS])
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   struct lp_build_context *bld_broad = get_int_bld(bld_base, true, bit_size);
   LLVMValueRef consts_ptr = lp_build_array_get(gallivm, bld->consts_ptr, index);
   unsigned size_shift = bit_size_to_shift_size(bit_size);
   if (size_shift)
      offset = lp_build_shr(uint_bld, offset, lp_build_const_int_vec(gallivm, uint_bld->type, size_shift));

   LLVMTypeRef ptr_type = LLVMPointerType(bld_broad->elem_type, 0);
   consts_ptr = LLVMBuildBitCast(builder, consts_ptr, ptr_type, "");

   if (offset_is_uniform) {
      offset = LLVMBuildExtractElement(builder, offset, lp_build_const_int32(gallivm, 0), "");

      for (unsigned c = 0; c < nc; c++) {
         LLVMValueRef this_offset = LLVMBuildAdd(builder, offset, lp_build_const_int32(gallivm, c), "");

         LLVMValueRef scalar = lp_build_pointer_get(builder, consts_ptr, this_offset);
         result[c] = lp_build_broadcast_scalar(bld_broad, scalar);
      }
   } else {
      LLVMValueRef overflow_mask;
      LLVMValueRef num_consts = lp_build_array_get(gallivm, bld->const_sizes_ptr, index);

      num_consts = lp_build_broadcast_scalar(uint_bld, num_consts);
      if (bit_size == 64)
         num_consts = lp_build_shr_imm(uint_bld, num_consts, 1);
      else if (bit_size == 16)
         num_consts = lp_build_shl_imm(uint_bld, num_consts, 1);
      else if (bit_size == 8)
         num_consts = lp_build_shl_imm(uint_bld, num_consts, 2);

      for (unsigned c = 0; c < nc; c++) {
         LLVMValueRef this_offset = lp_build_add(uint_bld, offset, lp_build_const_int_vec(gallivm, uint_bld->type, c));
         overflow_mask = lp_build_compare(gallivm, uint_bld->type, PIPE_FUNC_GEQUAL,
                                          this_offset, num_consts);
         result[c] = build_gather(bld_base, bld_broad, consts_ptr, this_offset, overflow_mask, NULL);
      }
   }
}


static void emit_load_mem(struct lp_build_nir_context *bld_base,
                          unsigned nc,
                          unsigned bit_size,
                          LLVMValueRef index,
                          LLVMValueRef offset,
                          LLVMValueRef outval[NIR_MAX_VEC_COMPONENTS])
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef ssbo_limit = NULL;
   struct lp_build_context *load_bld;
   uint32_t shift_val = bit_size_to_shift_size(bit_size);

   load_bld = get_int_bld(bld_base, true, bit_size);

   offset = LLVMBuildAShr(gallivm->builder, offset, lp_build_const_int_vec(gallivm, uint_bld->type, shift_val), "");

   /* although the index is dynamically uniform that doesn't count if exec mask isn't set, so read the one-by-one */

   LLVMValueRef result[NIR_MAX_VEC_COMPONENTS];
   for (unsigned c = 0; c < nc; c++)
      result[c] = lp_build_alloca(gallivm, load_bld->vec_type, "");

   LLVMValueRef exec_mask = mask_vec(bld_base);
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
   struct lp_build_loop_state loop_state;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   LLVMValueRef loop_cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
   LLVMValueRef loop_offset = LLVMBuildExtractElement(gallivm->builder, offset, loop_state.counter, "");

   struct lp_build_if_state exec_ifthen;
   lp_build_if(&exec_ifthen, gallivm, loop_cond);

   LLVMValueRef mem_ptr;

   if (index) {
      LLVMValueRef ssbo_idx = LLVMBuildExtractElement(gallivm->builder, index, loop_state.counter, "");
      LLVMValueRef ssbo_size_ptr = lp_build_array_get(gallivm, bld->ssbo_sizes_ptr, ssbo_idx);
      LLVMValueRef ssbo_ptr = lp_build_array_get(gallivm, bld->ssbo_ptr, ssbo_idx);
      ssbo_limit = LLVMBuildAShr(gallivm->builder, ssbo_size_ptr, lp_build_const_int32(gallivm, shift_val), "");
      mem_ptr = ssbo_ptr;
   } else
      mem_ptr = bld->shared_ptr;

   for (unsigned c = 0; c < nc; c++) {
      LLVMValueRef loop_index = LLVMBuildAdd(builder, loop_offset, lp_build_const_int32(gallivm, c), "");
      LLVMValueRef do_fetch = lp_build_const_int32(gallivm, -1);
      if (ssbo_limit) {
         LLVMValueRef ssbo_oob_cmp = lp_build_compare(gallivm, lp_elem_type(uint_bld->type), PIPE_FUNC_LESS, loop_index, ssbo_limit);
         do_fetch = LLVMBuildAnd(builder, do_fetch, ssbo_oob_cmp, "");
      }

      struct lp_build_if_state ifthen;
      LLVMValueRef fetch_cond, temp_res;

      fetch_cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, do_fetch, lp_build_const_int32(gallivm, 0), "");

      lp_build_if(&ifthen, gallivm, fetch_cond);
      LLVMValueRef scalar;
      if (bit_size != 32) {
         LLVMValueRef mem_ptr2 = LLVMBuildBitCast(builder, mem_ptr, LLVMPointerType(load_bld->elem_type, 0), "");
         scalar = lp_build_pointer_get(builder, mem_ptr2, loop_index);
      } else
         scalar = lp_build_pointer_get(builder, mem_ptr, loop_index);

      temp_res = LLVMBuildLoad(builder, result[c], "");
      temp_res = LLVMBuildInsertElement(builder, temp_res, scalar, loop_state.counter, "");
      LLVMBuildStore(builder, temp_res, result[c]);
      lp_build_else(&ifthen);
      temp_res = LLVMBuildLoad(builder, result[c], "");
      LLVMValueRef zero;
      if (bit_size == 64)
         zero = LLVMConstInt(LLVMInt64TypeInContext(gallivm->context), 0, 0);
      else if (bit_size == 16)
         zero = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), 0, 0);
      else if (bit_size == 8)
         zero = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), 0, 0);
      else
         zero = lp_build_const_int32(gallivm, 0);
      temp_res = LLVMBuildInsertElement(builder, temp_res, zero, loop_state.counter, "");
      LLVMBuildStore(builder, temp_res, result[c]);
      lp_build_endif(&ifthen);
   }

   lp_build_endif(&exec_ifthen);
   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                          NULL, LLVMIntUGE);
   for (unsigned c = 0; c < nc; c++)
      outval[c] = LLVMBuildLoad(gallivm->builder, result[c], "");

}

static void emit_store_mem(struct lp_build_nir_context *bld_base,
                           unsigned writemask,
                           unsigned nc,
                           unsigned bit_size,
                           LLVMValueRef index,
                           LLVMValueRef offset,
                           LLVMValueRef dst)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   LLVMValueRef mem_ptr;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef ssbo_limit = NULL;
   struct lp_build_context *store_bld;
   uint32_t shift_val = bit_size_to_shift_size(bit_size);
   store_bld = get_int_bld(bld_base, true, bit_size);

   offset = lp_build_shr_imm(uint_bld, offset, shift_val);

   LLVMValueRef exec_mask = mask_vec(bld_base);
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
   struct lp_build_loop_state loop_state;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   LLVMValueRef loop_cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
   LLVMValueRef loop_offset = LLVMBuildExtractElement(gallivm->builder, offset, loop_state.counter, "");

   struct lp_build_if_state exec_ifthen;
   lp_build_if(&exec_ifthen, gallivm, loop_cond);

   if (index) {
      LLVMValueRef ssbo_idx = LLVMBuildExtractElement(gallivm->builder, index, loop_state.counter, "");
      LLVMValueRef ssbo_size_ptr = lp_build_array_get(gallivm, bld->ssbo_sizes_ptr, ssbo_idx);
      LLVMValueRef ssbo_ptr = lp_build_array_get(gallivm, bld->ssbo_ptr, ssbo_idx);
      ssbo_limit = LLVMBuildAShr(gallivm->builder, ssbo_size_ptr, lp_build_const_int32(gallivm, shift_val), "");
      mem_ptr = ssbo_ptr;
   } else
      mem_ptr = bld->shared_ptr;

   for (unsigned c = 0; c < nc; c++) {
      if (!(writemask & (1u << c)))
         continue;
      LLVMValueRef loop_index = LLVMBuildAdd(builder, loop_offset, lp_build_const_int32(gallivm, c), "");
      LLVMValueRef val = (nc == 1) ? dst : LLVMBuildExtractValue(builder, dst, c, "");
      LLVMValueRef do_store = lp_build_const_int32(gallivm, -1);

      if (ssbo_limit) {
         LLVMValueRef ssbo_oob_cmp = lp_build_compare(gallivm, lp_elem_type(uint_bld->type), PIPE_FUNC_LESS, loop_index, ssbo_limit);
         do_store = LLVMBuildAnd(builder, do_store, ssbo_oob_cmp, "");
      }

      LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, val,
                                                       loop_state.counter, "");
      value_ptr = LLVMBuildBitCast(gallivm->builder, value_ptr, store_bld->elem_type, "");
      struct lp_build_if_state ifthen;
      LLVMValueRef store_cond;

      store_cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, do_store, lp_build_const_int32(gallivm, 0), "");
      lp_build_if(&ifthen, gallivm, store_cond);
      if (bit_size != 32) {
         LLVMValueRef mem_ptr2 = LLVMBuildBitCast(builder, mem_ptr, LLVMPointerType(store_bld->elem_type, 0), "");
         lp_build_pointer_set(builder, mem_ptr2, loop_index, value_ptr);
      } else
         lp_build_pointer_set(builder, mem_ptr, loop_index, value_ptr);
      lp_build_endif(&ifthen);
   }

   lp_build_endif(&exec_ifthen);
   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                             NULL, LLVMIntUGE);

}

static void emit_atomic_mem(struct lp_build_nir_context *bld_base,
                            nir_intrinsic_op nir_op,
                            uint32_t bit_size,
                            LLVMValueRef index, LLVMValueRef offset,
                            LLVMValueRef val, LLVMValueRef val2,
                            LLVMValueRef *result)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef ssbo_limit = NULL;
   uint32_t shift_val = bit_size_to_shift_size(bit_size);
   struct lp_build_context *atomic_bld = get_int_bld(bld_base, true, bit_size);

   offset = lp_build_shr_imm(uint_bld, offset, shift_val);
   LLVMValueRef atom_res = lp_build_alloca(gallivm,
                                           atomic_bld->vec_type, "");

   LLVMValueRef exec_mask = mask_vec(bld_base);
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
   struct lp_build_loop_state loop_state;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   LLVMValueRef loop_cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
   LLVMValueRef loop_offset = LLVMBuildExtractElement(gallivm->builder, offset, loop_state.counter, "");

   struct lp_build_if_state exec_ifthen;
   lp_build_if(&exec_ifthen, gallivm, loop_cond);

   LLVMValueRef mem_ptr;
   if (index) {
      LLVMValueRef ssbo_idx = LLVMBuildExtractElement(gallivm->builder, index, loop_state.counter, "");
      LLVMValueRef ssbo_size_ptr = lp_build_array_get(gallivm, bld->ssbo_sizes_ptr, ssbo_idx);
      LLVMValueRef ssbo_ptr = lp_build_array_get(gallivm, bld->ssbo_ptr, ssbo_idx);
      ssbo_limit = LLVMBuildAShr(gallivm->builder, ssbo_size_ptr, lp_build_const_int32(gallivm, shift_val), "");
      mem_ptr = ssbo_ptr;
   } else
      mem_ptr = bld->shared_ptr;

   LLVMValueRef do_fetch = lp_build_const_int32(gallivm, -1);
   if (ssbo_limit) {
      LLVMValueRef ssbo_oob_cmp = lp_build_compare(gallivm, lp_elem_type(uint_bld->type), PIPE_FUNC_LESS, loop_offset, ssbo_limit);
      do_fetch = LLVMBuildAnd(builder, do_fetch, ssbo_oob_cmp, "");
   }

   LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, val,
                                                    loop_state.counter, "");
   value_ptr = LLVMBuildBitCast(gallivm->builder, value_ptr, atomic_bld->elem_type, "");

   LLVMValueRef scalar_ptr;
   if (bit_size != 32) {
      LLVMValueRef mem_ptr2 = LLVMBuildBitCast(builder, mem_ptr, LLVMPointerType(atomic_bld->elem_type, 0), "");
      scalar_ptr = LLVMBuildGEP(builder, mem_ptr2, &loop_offset, 1, "");
   } else
      scalar_ptr = LLVMBuildGEP(builder, mem_ptr, &loop_offset, 1, "");

   struct lp_build_if_state ifthen;
   LLVMValueRef inner_cond, temp_res;
   LLVMValueRef scalar;

   inner_cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, do_fetch, lp_build_const_int32(gallivm, 0), "");
   lp_build_if(&ifthen, gallivm, inner_cond);

   if (nir_op == nir_intrinsic_ssbo_atomic_comp_swap || nir_op == nir_intrinsic_shared_atomic_comp_swap) {
      LLVMValueRef cas_src_ptr = LLVMBuildExtractElement(gallivm->builder, val2,
                                                         loop_state.counter, "");
      cas_src_ptr = LLVMBuildBitCast(gallivm->builder, cas_src_ptr, atomic_bld->elem_type, "");
      scalar = LLVMBuildAtomicCmpXchg(builder, scalar_ptr, value_ptr,
                                      cas_src_ptr,
                                      LLVMAtomicOrderingSequentiallyConsistent,
                                      LLVMAtomicOrderingSequentiallyConsistent,
                                      false);
      scalar = LLVMBuildExtractValue(gallivm->builder, scalar, 0, "");
   } else {
      LLVMAtomicRMWBinOp op;

      switch (nir_op) {
      case nir_intrinsic_shared_atomic_add:
      case nir_intrinsic_ssbo_atomic_add:
         op = LLVMAtomicRMWBinOpAdd;
         break;
      case nir_intrinsic_shared_atomic_exchange:
      case nir_intrinsic_ssbo_atomic_exchange:
         op = LLVMAtomicRMWBinOpXchg;
         break;
      case nir_intrinsic_shared_atomic_and:
      case nir_intrinsic_ssbo_atomic_and:
         op = LLVMAtomicRMWBinOpAnd;
         break;
      case nir_intrinsic_shared_atomic_or:
      case nir_intrinsic_ssbo_atomic_or:
         op = LLVMAtomicRMWBinOpOr;
         break;
      case nir_intrinsic_shared_atomic_xor:
      case nir_intrinsic_ssbo_atomic_xor:
         op = LLVMAtomicRMWBinOpXor;
         break;
      case nir_intrinsic_shared_atomic_umin:
      case nir_intrinsic_ssbo_atomic_umin:
         op = LLVMAtomicRMWBinOpUMin;
         break;
      case nir_intrinsic_shared_atomic_umax:
      case nir_intrinsic_ssbo_atomic_umax:
         op = LLVMAtomicRMWBinOpUMax;
         break;
      case nir_intrinsic_ssbo_atomic_imin:
      case nir_intrinsic_shared_atomic_imin:
         op = LLVMAtomicRMWBinOpMin;
         break;
      case nir_intrinsic_ssbo_atomic_imax:
      case nir_intrinsic_shared_atomic_imax:
         op = LLVMAtomicRMWBinOpMax;
         break;
      default:
         unreachable("unknown atomic op");
      }
      scalar = LLVMBuildAtomicRMW(builder, op,
                                  scalar_ptr, value_ptr,
                                  LLVMAtomicOrderingSequentiallyConsistent,
                                  false);
   }
   temp_res = LLVMBuildLoad(builder, atom_res, "");
   temp_res = LLVMBuildInsertElement(builder, temp_res, scalar, loop_state.counter, "");
   LLVMBuildStore(builder, temp_res, atom_res);
   lp_build_else(&ifthen);
   temp_res = LLVMBuildLoad(builder, atom_res, "");
   LLVMValueRef zero = bit_size == 64 ? lp_build_const_int64(gallivm, 0) : lp_build_const_int32(gallivm, 0);
   temp_res = LLVMBuildInsertElement(builder, temp_res, zero, loop_state.counter, "");
   LLVMBuildStore(builder, temp_res, atom_res);
   lp_build_endif(&ifthen);

   lp_build_endif(&exec_ifthen);
   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                          NULL, LLVMIntUGE);
   *result = LLVMBuildLoad(builder, atom_res, "");
}

static void emit_barrier(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state * gallivm = bld_base->base.gallivm;

   LLVMBasicBlockRef resume = lp_build_insert_new_block(gallivm, "resume");

   lp_build_coro_suspend_switch(gallivm, bld->coro, resume, false);
   LLVMPositionBuilderAtEnd(gallivm->builder, resume);
}

static LLVMValueRef emit_get_ssbo_size(struct lp_build_nir_context *bld_base,
                                       LLVMValueRef index)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *bld_broad = &bld_base->uint_bld;
   LLVMValueRef size_ptr = lp_build_array_get(bld_base->base.gallivm, bld->ssbo_sizes_ptr,
                                              LLVMBuildExtractElement(builder, index, lp_build_const_int32(gallivm, 0), ""));
   return lp_build_broadcast_scalar(bld_broad, size_ptr);
}

static void emit_image_op(struct lp_build_nir_context *bld_base,
                          struct lp_img_params *params)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;

   params->type = bld_base->base.type;
   params->context_ptr = bld->context_ptr;
   params->thread_data_ptr = bld->thread_data_ptr;
   params->exec_mask = mask_vec(bld_base);

   if (params->image_index_offset)
      params->image_index_offset = LLVMBuildExtractElement(gallivm->builder, params->image_index_offset,
                                                           lp_build_const_int32(gallivm, 0), "");

   bld->image->emit_op(bld->image,
                       bld->bld_base.base.gallivm,
                       params);

}

static void emit_image_size(struct lp_build_nir_context *bld_base,
                            struct lp_sampler_size_query_params *params)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;

   params->int_type = bld_base->int_bld.type;
   params->context_ptr = bld->context_ptr;

   if (params->texture_unit_offset)
      params->texture_unit_offset = LLVMBuildExtractElement(gallivm->builder, params->texture_unit_offset,
                                                            lp_build_const_int32(gallivm, 0), "");
   bld->image->emit_size_query(bld->image,
                               bld->bld_base.base.gallivm,
                               params);

}

static void init_var_slots(struct lp_build_nir_context *bld_base,
                           nir_variable *var, unsigned sc)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   unsigned slots = glsl_count_attribute_slots(var->type, false) * 4;

   if (!bld->outputs)
     return;
   for (unsigned comp = sc; comp < slots + sc; comp++) {
      unsigned this_loc = var->data.driver_location + (comp / 4);
      unsigned this_chan = comp % 4;

      if (!bld->outputs[this_loc][this_chan])
         bld->outputs[this_loc][this_chan] = lp_build_alloca(bld_base->base.gallivm,
                                                             bld_base->base.vec_type, "output");
   }
}

static void emit_var_decl(struct lp_build_nir_context *bld_base,
                          nir_variable *var)
{
   unsigned sc = var->data.location_frac;
   switch (var->data.mode) {
   case nir_var_shader_out: {
      if (bld_base->shader->info.stage == MESA_SHADER_FRAGMENT) {
         if (var->data.location == FRAG_RESULT_STENCIL)
            sc = 1;
         else if (var->data.location == FRAG_RESULT_DEPTH)
            sc = 2;
      }
      init_var_slots(bld_base, var, sc);
      break;
   }
   default:
      break;
   }
}

static void emit_tex(struct lp_build_nir_context *bld_base,
                     struct lp_sampler_params *params)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;

   params->type = bld_base->base.type;
   params->context_ptr = bld->context_ptr;
   params->thread_data_ptr = bld->thread_data_ptr;

   if (params->texture_index_offset && bld_base->shader->info.stage != MESA_SHADER_FRAGMENT) {
      /* this is horrible but this can be dynamic */
      LLVMValueRef coords[5];
      LLVMValueRef *orig_texel_ptr;
      struct lp_build_context *uint_bld = &bld_base->uint_bld;
      LLVMValueRef result[4] = { LLVMGetUndef(bld_base->base.vec_type),
                                 LLVMGetUndef(bld_base->base.vec_type),
                                 LLVMGetUndef(bld_base->base.vec_type),
                                 LLVMGetUndef(bld_base->base.vec_type) };
      LLVMValueRef texel[4], orig_offset, orig_lod;
      unsigned i;
      orig_texel_ptr = params->texel;
      orig_lod = params->lod;
      for (i = 0; i < 5; i++) {
         coords[i] = params->coords[i];
      }
      orig_offset = params->texture_index_offset;

      for (unsigned v = 0; v < uint_bld->type.length; v++) {
         LLVMValueRef idx = lp_build_const_int32(gallivm, v);
         LLVMValueRef new_coords[5];
         for (i = 0; i < 5; i++) {
            new_coords[i] = LLVMBuildExtractElement(gallivm->builder,
                                                    coords[i], idx, "");
         }
         params->coords = new_coords;
         params->texture_index_offset = LLVMBuildExtractElement(gallivm->builder,
                                                                orig_offset,
                                                                idx, "");
         params->type = lp_elem_type(bld_base->base.type);

         if (orig_lod)
            params->lod = LLVMBuildExtractElement(gallivm->builder, orig_lod, idx, "");
         params->texel = texel;
         bld->sampler->emit_tex_sample(bld->sampler,
                                       gallivm,
                                       params);

         for (i = 0; i < 4; i++) {
            result[i] = LLVMBuildInsertElement(gallivm->builder, result[i], texel[i], idx, "");
         }
      }
      for (i = 0; i < 4; i++) {
         orig_texel_ptr[i] = result[i];
      }
      return;
   }

   if (params->texture_index_offset) {
      struct lp_build_loop_state loop_state;
      LLVMValueRef exec_mask = mask_vec(bld_base);
      LLVMValueRef outer_cond = LLVMBuildICmp(builder, LLVMIntNE, exec_mask, bld_base->uint_bld.zero, "");
      LLVMValueRef res_store = lp_build_alloca(gallivm, bld_base->uint_bld.elem_type, "");
      lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
      LLVMValueRef if_cond = LLVMBuildExtractElement(gallivm->builder, outer_cond, loop_state.counter, "");

      struct lp_build_if_state ifthen;
      lp_build_if(&ifthen, gallivm, if_cond);
      LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, params->texture_index_offset,
                                                       loop_state.counter, "");
      LLVMBuildStore(builder, value_ptr, res_store);
      lp_build_endif(&ifthen);
      lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length),
                             NULL, LLVMIntUGE);
      LLVMValueRef idx_val = LLVMBuildLoad(builder, res_store, "");
      params->texture_index_offset = idx_val;
   }

   params->type = bld_base->base.type;
   bld->sampler->emit_tex_sample(bld->sampler,
                                 bld->bld_base.base.gallivm,
                                 params);
}

static void emit_tex_size(struct lp_build_nir_context *bld_base,
                          struct lp_sampler_size_query_params *params)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;

   params->int_type = bld_base->int_bld.type;
   params->context_ptr = bld->context_ptr;

   if (params->texture_unit_offset)
      params->texture_unit_offset = LLVMBuildExtractElement(bld_base->base.gallivm->builder,
                                                             params->texture_unit_offset,
                                                             lp_build_const_int32(bld_base->base.gallivm, 0), "");
   bld->sampler->emit_size_query(bld->sampler,
                                 bld->bld_base.base.gallivm,
                                 params);
}

static void emit_sysval_intrin(struct lp_build_nir_context *bld_base,
                               nir_intrinsic_instr *instr,
                               LLVMValueRef result[NIR_MAX_VEC_COMPONENTS])
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_context *bld_broad = get_int_bld(bld_base, true, instr->dest.ssa.bit_size);
   switch (instr->intrinsic) {
   case nir_intrinsic_load_instance_id:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.instance_id);
      break;
   case nir_intrinsic_load_base_instance:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.base_instance);
      break;
   case nir_intrinsic_load_base_vertex:
      result[0] = bld->system_values.basevertex;
      break;
   case nir_intrinsic_load_first_vertex:
      result[0] = bld->system_values.firstvertex;
      break;
   case nir_intrinsic_load_vertex_id:
      result[0] = bld->system_values.vertex_id;
      break;
   case nir_intrinsic_load_primitive_id:
      result[0] = bld->system_values.prim_id;
      break;
   case nir_intrinsic_load_workgroup_id: {
      LLVMValueRef tmp[3];
      for (unsigned i = 0; i < 3; i++) {
         tmp[i] = LLVMBuildExtractElement(gallivm->builder, bld->system_values.block_id, lp_build_const_int32(gallivm, i), "");
         if (instr->dest.ssa.bit_size == 64)
            tmp[i] = LLVMBuildZExt(gallivm->builder, tmp[i], bld_base->uint64_bld.elem_type, "");
         result[i] = lp_build_broadcast_scalar(bld_broad, tmp[i]);
      }
      break;
   }
   case nir_intrinsic_load_local_invocation_id:
      for (unsigned i = 0; i < 3; i++)
         result[i] = LLVMBuildExtractValue(gallivm->builder, bld->system_values.thread_id, i, "");
      break;
   case nir_intrinsic_load_local_invocation_index: {
      LLVMValueRef tmp, tmp2;
      tmp = lp_build_broadcast_scalar(&bld_base->uint_bld, LLVMBuildExtractElement(gallivm->builder, bld->system_values.block_size, lp_build_const_int32(gallivm, 1), ""));
      tmp2 = lp_build_broadcast_scalar(&bld_base->uint_bld, LLVMBuildExtractElement(gallivm->builder, bld->system_values.block_size, lp_build_const_int32(gallivm, 0), ""));
      tmp = lp_build_mul(&bld_base->uint_bld, tmp, tmp2);
      tmp = lp_build_mul(&bld_base->uint_bld, tmp, LLVMBuildExtractValue(gallivm->builder, bld->system_values.thread_id, 2, ""));

      tmp2 = lp_build_broadcast_scalar(&bld_base->uint_bld, LLVMBuildExtractElement(gallivm->builder, bld->system_values.block_size, lp_build_const_int32(gallivm, 0), ""));
      tmp2 = lp_build_mul(&bld_base->uint_bld, tmp2, LLVMBuildExtractValue(gallivm->builder, bld->system_values.thread_id, 1, ""));
      tmp = lp_build_add(&bld_base->uint_bld, tmp, tmp2);
      tmp = lp_build_add(&bld_base->uint_bld, tmp, LLVMBuildExtractValue(gallivm->builder, bld->system_values.thread_id, 0, ""));
      result[0] = tmp;
      break;
   }
   case nir_intrinsic_load_num_workgroups: {
      LLVMValueRef tmp[3];
      for (unsigned i = 0; i < 3; i++) {
         tmp[i] = LLVMBuildExtractElement(gallivm->builder, bld->system_values.grid_size, lp_build_const_int32(gallivm, i), "");
         if (instr->dest.ssa.bit_size == 64)
            tmp[i] = LLVMBuildZExt(gallivm->builder, tmp[i], bld_base->uint64_bld.elem_type, "");
         result[i] = lp_build_broadcast_scalar(bld_broad, tmp[i]);
      }
      break;
   }
   case nir_intrinsic_load_invocation_id:
      if (bld_base->shader->info.stage == MESA_SHADER_TESS_CTRL)
         result[0] = bld->system_values.invocation_id;
      else
         result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.invocation_id);
      break;
   case nir_intrinsic_load_front_face:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.front_facing);
      break;
   case nir_intrinsic_load_draw_id:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.draw_id);
      break;
   default:
      break;
   case nir_intrinsic_load_workgroup_size:
     for (unsigned i = 0; i < 3; i++)
       result[i] = lp_build_broadcast_scalar(&bld_base->uint_bld, LLVMBuildExtractElement(gallivm->builder, bld->system_values.block_size, lp_build_const_int32(gallivm, i), ""));
     break;
   case nir_intrinsic_load_work_dim:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.work_dim);
      break;
   case nir_intrinsic_load_tess_coord:
      for (unsigned i = 0; i < 3; i++) {
	 result[i] = LLVMBuildExtractValue(gallivm->builder, bld->system_values.tess_coord, i, "");
      }
      break;
   case nir_intrinsic_load_tess_level_outer:
      for (unsigned i = 0; i < 4; i++)
         result[i] = lp_build_broadcast_scalar(&bld_base->base, LLVMBuildExtractValue(gallivm->builder, bld->system_values.tess_outer, i, ""));
      break;
   case nir_intrinsic_load_tess_level_inner:
      for (unsigned i = 0; i < 2; i++)
         result[i] = lp_build_broadcast_scalar(&bld_base->base, LLVMBuildExtractValue(gallivm->builder, bld->system_values.tess_inner, i, ""));
      break;
   case nir_intrinsic_load_patch_vertices_in:
      result[0] = bld->system_values.vertices_in;
      break;
   case nir_intrinsic_load_sample_id:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.sample_id);
      break;
   case nir_intrinsic_load_sample_pos:
      for (unsigned i = 0; i < 2; i++) {
         LLVMValueRef idx = LLVMBuildMul(gallivm->builder, bld->system_values.sample_id, lp_build_const_int32(gallivm, 2), "");
         idx = LLVMBuildAdd(gallivm->builder, idx, lp_build_const_int32(gallivm, i), "");
         LLVMValueRef val = lp_build_array_get(gallivm, bld->system_values.sample_pos, idx);
         result[i] = lp_build_broadcast_scalar(&bld_base->base, val);
      }
      break;
   case nir_intrinsic_load_sample_mask_in:
      result[0] = bld->system_values.sample_mask_in;
      break;
   case nir_intrinsic_load_view_index:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.view_index);
      break;
   case nir_intrinsic_load_subgroup_invocation: {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
      for(unsigned i = 0; i < bld->bld_base.base.type.length; ++i)
         elems[i] = lp_build_const_int32(gallivm, i);
      result[0] = LLVMConstVector(elems, bld->bld_base.base.type.length);
      break;
   }
   case nir_intrinsic_load_subgroup_id:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.subgroup_id);
      break;
   case nir_intrinsic_load_num_subgroups:
      result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.num_subgroups);
      break;
   }
}

static void emit_helper_invocation(struct lp_build_nir_context *bld_base,
                                   LLVMValueRef *dst)
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   *dst = lp_build_cmp(uint_bld, PIPE_FUNC_NOTEQUAL, mask_vec(bld_base), lp_build_const_int_vec(gallivm, uint_bld->type, -1));
}

static void bgnloop(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   lp_exec_bgnloop(&bld->exec_mask, true);
}

static void endloop(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   lp_exec_endloop(bld_base->base.gallivm, &bld->exec_mask);
}

static void if_cond(struct lp_build_nir_context *bld_base, LLVMValueRef cond)
{
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   lp_exec_mask_cond_push(&bld->exec_mask, LLVMBuildBitCast(builder, cond, bld_base->base.int_vec_type, ""));
}

static void else_stmt(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   lp_exec_mask_cond_invert(&bld->exec_mask);
}

static void endif_stmt(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   lp_exec_mask_cond_pop(&bld->exec_mask);
}

static void break_stmt(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;

   lp_exec_break(&bld->exec_mask, NULL, false);
}

static void continue_stmt(struct lp_build_nir_context *bld_base)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   lp_exec_continue(&bld->exec_mask);
}

static void discard(struct lp_build_nir_context *bld_base, LLVMValueRef cond)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   LLVMValueRef mask;

   if (!cond) {
      if (bld->exec_mask.has_mask) {
         mask = LLVMBuildNot(builder, bld->exec_mask.exec_mask, "kilp");
      } else {
         mask = LLVMConstNull(bld->bld_base.base.int_vec_type);
      }
   } else {
      mask = LLVMBuildNot(builder, cond, "");
      if (bld->exec_mask.has_mask) {
         LLVMValueRef invmask;
         invmask = LLVMBuildNot(builder, bld->exec_mask.exec_mask, "kilp");
         mask = LLVMBuildOr(builder, mask, invmask, "");
      }
   }
   lp_build_mask_update(bld->mask, mask);
}

static void
increment_vec_ptr_by_mask(struct lp_build_nir_context * bld_base,
                          LLVMValueRef ptr,
                          LLVMValueRef mask)
{
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   LLVMValueRef current_vec = LLVMBuildLoad(builder, ptr, "");

   current_vec = LLVMBuildSub(builder, current_vec, mask, "");

   LLVMBuildStore(builder, current_vec, ptr);
}

static void
clear_uint_vec_ptr_from_mask(struct lp_build_nir_context * bld_base,
                             LLVMValueRef ptr,
                             LLVMValueRef mask)
{
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   LLVMValueRef current_vec = LLVMBuildLoad(builder, ptr, "");

   current_vec = lp_build_select(&bld_base->uint_bld,
                                 mask,
                                 bld_base->uint_bld.zero,
                                 current_vec);

   LLVMBuildStore(builder, current_vec, ptr);
}

static LLVMValueRef
clamp_mask_to_max_output_vertices(struct lp_build_nir_soa_context * bld,
                                  LLVMValueRef current_mask_vec,
                                  LLVMValueRef total_emitted_vertices_vec)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *int_bld = &bld->bld_base.int_bld;
   LLVMValueRef max_mask = lp_build_cmp(int_bld, PIPE_FUNC_LESS,
                                            total_emitted_vertices_vec,
                                            bld->max_output_vertices_vec);

   return LLVMBuildAnd(builder, current_mask_vec, max_mask, "");
}

static void emit_vertex(struct lp_build_nir_context *bld_base, uint32_t stream_id)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;

   if (stream_id >= bld->gs_vertex_streams)
      return;
   assert(bld->gs_iface->emit_vertex);
   LLVMValueRef total_emitted_vertices_vec =
      LLVMBuildLoad(builder, bld->total_emitted_vertices_vec_ptr[stream_id], "");
   LLVMValueRef mask = mask_vec(bld_base);
   mask = clamp_mask_to_max_output_vertices(bld, mask,
                                            total_emitted_vertices_vec);
   bld->gs_iface->emit_vertex(bld->gs_iface, &bld->bld_base.base,
                              bld->outputs,
                              total_emitted_vertices_vec,
                              mask,
                              lp_build_const_int_vec(bld->bld_base.base.gallivm, bld->bld_base.base.type, stream_id));

   increment_vec_ptr_by_mask(bld_base, bld->emitted_vertices_vec_ptr[stream_id],
                             mask);
   increment_vec_ptr_by_mask(bld_base, bld->total_emitted_vertices_vec_ptr[stream_id],
                             mask);
}

static void
end_primitive_masked(struct lp_build_nir_context * bld_base,
                     LLVMValueRef mask, uint32_t stream_id)
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;

   if (stream_id >= bld->gs_vertex_streams)
      return;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef emitted_vertices_vec =
      LLVMBuildLoad(builder, bld->emitted_vertices_vec_ptr[stream_id], "");
   LLVMValueRef emitted_prims_vec =
      LLVMBuildLoad(builder, bld->emitted_prims_vec_ptr[stream_id], "");
   LLVMValueRef total_emitted_vertices_vec =
      LLVMBuildLoad(builder, bld->total_emitted_vertices_vec_ptr[stream_id], "");

   LLVMValueRef emitted_mask = lp_build_cmp(uint_bld,
                                            PIPE_FUNC_NOTEQUAL,
                                            emitted_vertices_vec,
                                            uint_bld->zero);
   mask = LLVMBuildAnd(builder, mask, emitted_mask, "");
   bld->gs_iface->end_primitive(bld->gs_iface, &bld->bld_base.base,
				total_emitted_vertices_vec,
				emitted_vertices_vec, emitted_prims_vec, mask, stream_id);
   increment_vec_ptr_by_mask(bld_base, bld->emitted_prims_vec_ptr[stream_id],
                             mask);
   clear_uint_vec_ptr_from_mask(bld_base, bld->emitted_vertices_vec_ptr[stream_id],
                                mask);
}

static void end_primitive(struct lp_build_nir_context *bld_base, uint32_t stream_id)
{
   ASSERTED struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;

   assert(bld->gs_iface->end_primitive);

   LLVMValueRef mask = mask_vec(bld_base);
   end_primitive_masked(bld_base, mask, stream_id);
}

static void
emit_prologue(struct lp_build_nir_soa_context *bld)
{
   struct gallivm_state * gallivm = bld->bld_base.base.gallivm;
   if (bld->indirects & nir_var_shader_in && !bld->gs_iface && !bld->tcs_iface && !bld->tes_iface) {
      uint32_t num_inputs = util_bitcount64(bld->bld_base.shader->info.inputs_read);
      unsigned index, chan;
      LLVMTypeRef vec_type = bld->bld_base.base.vec_type;
      LLVMValueRef array_size = lp_build_const_int32(gallivm, num_inputs * 4);
      bld->inputs_array = lp_build_array_alloca(gallivm,
                                               vec_type, array_size,
                                               "input_array");

      for (index = 0; index < num_inputs; ++index) {
         for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            LLVMValueRef lindex =
               lp_build_const_int32(gallivm, index * 4 + chan);
            LLVMValueRef input_ptr =
               LLVMBuildGEP(gallivm->builder, bld->inputs_array,
                            &lindex, 1, "");
            LLVMValueRef value = bld->inputs[index][chan];
            if (value)
               LLVMBuildStore(gallivm->builder, value, input_ptr);
         }
      }
   }
}

static void emit_vote(struct lp_build_nir_context *bld_base, LLVMValueRef src,
                      nir_intrinsic_instr *instr, LLVMValueRef result[4])
{
   struct gallivm_state * gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   uint32_t bit_size = nir_src_bit_size(instr->src[0]);
   LLVMValueRef exec_mask = mask_vec(bld_base);
   struct lp_build_loop_state loop_state;
   LLVMValueRef outer_cond = LLVMBuildICmp(builder, LLVMIntNE, exec_mask, bld_base->uint_bld.zero, "");

   LLVMValueRef res_store = lp_build_alloca(gallivm, bld_base->uint_bld.elem_type, "");
   LLVMValueRef eq_store = lp_build_alloca(gallivm, get_int_bld(bld_base, true, bit_size)->elem_type, "");
   LLVMValueRef init_val = NULL;
   if (instr->intrinsic == nir_intrinsic_vote_ieq ||
       instr->intrinsic == nir_intrinsic_vote_feq) {
      /* for equal we unfortunately have to loop and find the first valid one. */
      lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
      LLVMValueRef if_cond = LLVMBuildExtractElement(gallivm->builder, outer_cond, loop_state.counter, "");

      struct lp_build_if_state ifthen;
      lp_build_if(&ifthen, gallivm, if_cond);
      LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, src,
                                                       loop_state.counter, "");
      LLVMBuildStore(builder, value_ptr, eq_store);
      LLVMBuildStore(builder, lp_build_const_int32(gallivm, -1), res_store);
      lp_build_endif(&ifthen);
      lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length),
                             NULL, LLVMIntUGE);
      init_val = LLVMBuildLoad(builder, eq_store, "");
   } else {
      LLVMBuildStore(builder, lp_build_const_int32(gallivm, instr->intrinsic == nir_intrinsic_vote_any ? 0 : -1), res_store);
   }

   LLVMValueRef res;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, src,
                                                       loop_state.counter, "");
   struct lp_build_if_state ifthen;
   LLVMValueRef if_cond;
   if_cond = LLVMBuildExtractElement(gallivm->builder, outer_cond, loop_state.counter, "");

   lp_build_if(&ifthen, gallivm, if_cond);
   res = LLVMBuildLoad(builder, res_store, "");

   if (instr->intrinsic == nir_intrinsic_vote_feq) {
      struct lp_build_context *flt_bld = get_flt_bld(bld_base, bit_size);
      LLVMValueRef tmp = LLVMBuildFCmp(builder, LLVMRealUEQ,
                                       LLVMBuildBitCast(builder, init_val, flt_bld->elem_type, ""),
                                       LLVMBuildBitCast(builder, value_ptr, flt_bld->elem_type, ""), "");
      tmp = LLVMBuildSExt(builder, tmp, bld_base->uint_bld.elem_type, "");
      res = LLVMBuildAnd(builder, res, tmp, "");
   } else if (instr->intrinsic == nir_intrinsic_vote_ieq) {
      LLVMValueRef tmp = LLVMBuildICmp(builder, LLVMIntEQ, init_val, value_ptr, "");
      tmp = LLVMBuildSExt(builder, tmp, bld_base->uint_bld.elem_type, "");
      res = LLVMBuildAnd(builder, res, tmp, "");
   } else if (instr->intrinsic == nir_intrinsic_vote_any)
      res = LLVMBuildOr(builder, res, value_ptr, "");
   else
      res = LLVMBuildAnd(builder, res, value_ptr, "");
   LLVMBuildStore(builder, res, res_store);
   lp_build_endif(&ifthen);
   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length),
                          NULL, LLVMIntUGE);
   result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, LLVMBuildLoad(builder, res_store, ""));
}

static void emit_ballot(struct lp_build_nir_context *bld_base, LLVMValueRef src, nir_intrinsic_instr *instr, LLVMValueRef result[4])
{
   struct gallivm_state * gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef exec_mask = mask_vec(bld_base);
   struct lp_build_loop_state loop_state;
   src = LLVMBuildAnd(builder, src, exec_mask, "");
   LLVMValueRef res_store = lp_build_alloca(gallivm, bld_base->int_bld.elem_type, "");
   LLVMValueRef res;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, src,
                                                    loop_state.counter, "");
   res = LLVMBuildLoad(builder, res_store, "");
   res = LLVMBuildOr(builder,
                     res,
                     LLVMBuildAnd(builder, value_ptr, LLVMBuildShl(builder, lp_build_const_int32(gallivm, 1), loop_state.counter, ""), ""), "");
   LLVMBuildStore(builder, res, res_store);

   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length),
                          NULL, LLVMIntUGE);
   result[0] = lp_build_broadcast_scalar(&bld_base->uint_bld, LLVMBuildLoad(builder, res_store, ""));
}

static void emit_elect(struct lp_build_nir_context *bld_base, LLVMValueRef result[4])
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef exec_mask = mask_vec(bld_base);
   struct lp_build_loop_state loop_state;

   LLVMValueRef idx_store = lp_build_alloca(gallivm, bld_base->int_bld.elem_type, "");
   LLVMValueRef found_store = lp_build_alloca(gallivm, bld_base->int_bld.elem_type, "");
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, exec_mask,
                                                    loop_state.counter, "");
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder,
                                     LLVMIntEQ,
                                     value_ptr,
                                     lp_build_const_int32(gallivm, -1), "");
   LLVMValueRef cond2 = LLVMBuildICmp(gallivm->builder,
                                      LLVMIntEQ,
                                      LLVMBuildLoad(builder, found_store, ""),
                                      lp_build_const_int32(gallivm, 0), "");

   cond = LLVMBuildAnd(builder, cond, cond2, "");
   struct lp_build_if_state ifthen;
   lp_build_if(&ifthen, gallivm, cond);
   LLVMBuildStore(builder, lp_build_const_int32(gallivm, 1), found_store);
   LLVMBuildStore(builder, loop_state.counter, idx_store);
   lp_build_endif(&ifthen);
   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length),
                          NULL, LLVMIntUGE);

   result[0] = LLVMBuildInsertElement(builder, bld_base->uint_bld.zero,
                                      lp_build_const_int32(gallivm, -1),
                                      LLVMBuildLoad(builder, idx_store, ""),
                                      "");
}

static void emit_reduce(struct lp_build_nir_context *bld_base, LLVMValueRef src,
                        nir_intrinsic_instr *instr, LLVMValueRef result[4])
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   uint32_t bit_size = nir_src_bit_size(instr->src[0]);
   /* can't use llvm reduction intrinsics because of exec_mask */
   LLVMValueRef exec_mask = mask_vec(bld_base);
   struct lp_build_loop_state loop_state;
   nir_op reduction_op = nir_intrinsic_reduction_op(instr);

   LLVMValueRef res_store = NULL;
   LLVMValueRef scan_store;
   struct lp_build_context *int_bld = get_int_bld(bld_base, true, bit_size);

   if (instr->intrinsic != nir_intrinsic_reduce)
      res_store = lp_build_alloca(gallivm, int_bld->vec_type, "");

   scan_store = lp_build_alloca(gallivm, int_bld->elem_type, "");

   struct lp_build_context elem_bld;
   bool is_flt = reduction_op == nir_op_fadd ||
      reduction_op == nir_op_fmul ||
      reduction_op == nir_op_fmin ||
      reduction_op == nir_op_fmax;
   bool is_unsigned = reduction_op == nir_op_umin ||
      reduction_op == nir_op_umax;

   struct lp_build_context *vec_bld = is_flt ? get_flt_bld(bld_base, bit_size) :
      get_int_bld(bld_base, is_unsigned, bit_size);

   lp_build_context_init(&elem_bld, gallivm, lp_elem_type(vec_bld->type));

   LLVMValueRef store_val = NULL;
   /*
    * Put the identity value for the operation into the storage
    */
   switch (reduction_op) {
   case nir_op_fmin: {
      LLVMValueRef flt_max = bit_size == 64 ? LLVMConstReal(LLVMDoubleTypeInContext(gallivm->context), INFINITY) :
         (bit_size == 16 ? LLVMConstReal(LLVMHalfTypeInContext(gallivm->context), INFINITY) : lp_build_const_float(gallivm, INFINITY));
      store_val = LLVMBuildBitCast(builder, flt_max, int_bld->elem_type, "");
      break;
   }
   case nir_op_fmax: {
      LLVMValueRef flt_min = bit_size == 64 ? LLVMConstReal(LLVMDoubleTypeInContext(gallivm->context), -INFINITY) :
         (bit_size == 16 ? LLVMConstReal(LLVMHalfTypeInContext(gallivm->context), -INFINITY) : lp_build_const_float(gallivm, -INFINITY));
      store_val = LLVMBuildBitCast(builder, flt_min, int_bld->elem_type, "");
      break;
   }
   case nir_op_fmul: {
      LLVMValueRef flt_one = bit_size == 64 ? LLVMConstReal(LLVMDoubleTypeInContext(gallivm->context), 1.0) :
         (bit_size == 16 ? LLVMConstReal(LLVMHalfTypeInContext(gallivm->context), 1.0) : lp_build_const_float(gallivm, 1.0));
      store_val = LLVMBuildBitCast(builder, flt_one, int_bld->elem_type, "");
      break;
   }
   case nir_op_umin:
      switch (bit_size) {
      case 8:
         store_val = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), UINT8_MAX, 0);
         break;
      case 16:
         store_val = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), UINT16_MAX, 0);
         break;
      case 32:
      default:
         store_val  = lp_build_const_int32(gallivm, UINT_MAX);
         break;
      case 64:
         store_val  = lp_build_const_int64(gallivm, UINT64_MAX);
         break;
      }
      break;
   case nir_op_imin:
      switch (bit_size) {
      case 8:
         store_val = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), INT8_MAX, 0);
         break;
      case 16:
         store_val = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), INT16_MAX, 0);
         break;
      case 32:
      default:
         store_val  = lp_build_const_int32(gallivm, INT_MAX);
         break;
      case 64:
         store_val  = lp_build_const_int64(gallivm, INT64_MAX);
         break;
      }
      break;
   case nir_op_imax:
      switch (bit_size) {
      case 8:
         store_val = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), INT8_MIN, 0);
         break;
      case 16:
         store_val = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), INT16_MIN, 0);
         break;
      case 32:
      default:
         store_val  = lp_build_const_int32(gallivm, INT_MIN);
         break;
      case 64:
         store_val  = lp_build_const_int64(gallivm, INT64_MIN);
         break;
      }
      break;
   case nir_op_imul:
      switch (bit_size) {
      case 8:
         store_val = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), 1, 0);
         break;
      case 16:
         store_val = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), 1, 0);
         break;
      case 32:
      default:
         store_val  = lp_build_const_int32(gallivm, 1);
         break;
      case 64:
         store_val  = lp_build_const_int64(gallivm, 1);
         break;
      }
      break;
   case nir_op_iand:
      switch (bit_size) {
      case 8:
         store_val = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), 0xff, 0);
         break;
      case 16:
         store_val = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), 0xffff, 0);
         break;
      case 32:
      default:
         store_val  = lp_build_const_int32(gallivm, 0xffffffff);
         break;
      case 64:
         store_val  = lp_build_const_int64(gallivm, 0xffffffffffffffffLL);
         break;
      }
      break;
   default:
      break;
   }
   if (store_val)
      LLVMBuildStore(builder, store_val, scan_store);

   LLVMValueRef outer_cond = LLVMBuildICmp(builder, LLVMIntNE, exec_mask, bld_base->uint_bld.zero, "");

   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));

   struct lp_build_if_state ifthen;
   LLVMValueRef if_cond = LLVMBuildExtractElement(gallivm->builder, outer_cond, loop_state.counter, "");
   lp_build_if(&ifthen, gallivm, if_cond);
   LLVMValueRef value = LLVMBuildExtractElement(gallivm->builder, src, loop_state.counter, "");

   LLVMValueRef res = NULL;
   LLVMValueRef scan_val = LLVMBuildLoad(gallivm->builder, scan_store, "");
   if (instr->intrinsic != nir_intrinsic_reduce)
      res = LLVMBuildLoad(gallivm->builder, res_store, "");

   if (instr->intrinsic == nir_intrinsic_exclusive_scan)
      res = LLVMBuildInsertElement(builder, res, scan_val, loop_state.counter, "");

   if (is_flt) {
      scan_val = LLVMBuildBitCast(builder, scan_val, elem_bld.elem_type, "");
      value = LLVMBuildBitCast(builder, value, elem_bld.elem_type, "");
   }
   switch (reduction_op) {
   case nir_op_fadd:
   case nir_op_iadd:
      scan_val = lp_build_add(&elem_bld, value, scan_val);
      break;
   case nir_op_fmul:
   case nir_op_imul:
      scan_val = lp_build_mul(&elem_bld, value, scan_val);
      break;
   case nir_op_imin:
   case nir_op_umin:
   case nir_op_fmin:
      scan_val = lp_build_min(&elem_bld, value, scan_val);
      break;
   case nir_op_imax:
   case nir_op_umax:
   case nir_op_fmax:
      scan_val = lp_build_max(&elem_bld, value, scan_val);
      break;
   case nir_op_iand:
      scan_val = lp_build_and(&elem_bld, value, scan_val);
      break;
   case nir_op_ior:
      scan_val = lp_build_or(&elem_bld, value, scan_val);
      break;
   case nir_op_ixor:
      scan_val = lp_build_xor(&elem_bld, value, scan_val);
      break;
   default:
      assert(0);
      break;
   }
   if (is_flt)
      scan_val = LLVMBuildBitCast(builder, scan_val, int_bld->elem_type, "");
   LLVMBuildStore(builder, scan_val, scan_store);

   if (instr->intrinsic == nir_intrinsic_inclusive_scan) {
      res = LLVMBuildInsertElement(builder, res, scan_val, loop_state.counter, "");
   }

   if (instr->intrinsic != nir_intrinsic_reduce)
      LLVMBuildStore(builder, res, res_store);
   lp_build_endif(&ifthen);

   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length),
                          NULL, LLVMIntUGE);
   if (instr->intrinsic == nir_intrinsic_reduce)
      result[0] = lp_build_broadcast_scalar(int_bld, LLVMBuildLoad(builder, scan_store, ""));
   else
      result[0] = LLVMBuildLoad(builder, res_store, "");
}

static void emit_read_invocation(struct lp_build_nir_context *bld_base,
                                 LLVMValueRef src,
                                 unsigned bit_size,
                                 LLVMValueRef invoc,
                                 LLVMValueRef result[4])
{
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef idx;
   struct lp_build_context *uint_bld = get_int_bld(bld_base, true, bit_size);

   /* have to find the first active invocation */
   LLVMValueRef exec_mask = mask_vec(bld_base);
   struct lp_build_loop_state loop_state;
   LLVMValueRef res_store = lp_build_alloca(gallivm, bld_base->int_bld.elem_type, "");
   LLVMValueRef outer_cond = LLVMBuildICmp(builder, LLVMIntNE, exec_mask, bld_base->uint_bld.zero, "");
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, bld_base->uint_bld.type.length));

   LLVMValueRef if_cond = LLVMBuildExtractElement(gallivm->builder, outer_cond, loop_state.counter, "");
   struct lp_build_if_state ifthen;

   lp_build_if(&ifthen, gallivm, if_cond);
   LLVMValueRef store_val = loop_state.counter;
   if (invoc)
      store_val = LLVMBuildExtractElement(gallivm->builder, invoc, loop_state.counter, "");
   LLVMBuildStore(builder, store_val, res_store);
   lp_build_endif(&ifthen);

   lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, -1),
                          lp_build_const_int32(gallivm, -1), LLVMIntEQ);
   idx = LLVMBuildLoad(builder, res_store, "");

   LLVMValueRef value = LLVMBuildExtractElement(gallivm->builder,
                                                src, idx, "");
   result[0] = lp_build_broadcast_scalar(uint_bld, value);
}

static void
emit_interp_at(struct lp_build_nir_context *bld_base,
               unsigned num_components,
               nir_variable *var,
               bool centroid,
               bool sample,
               unsigned const_index,
               LLVMValueRef indir_index,
               LLVMValueRef offsets[2],
               LLVMValueRef dst[4])
{
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;

   for (unsigned i = 0; i < num_components; i++) {
      dst[i] = bld->fs_iface->interp_fn(bld->fs_iface, &bld_base->base,
                                        const_index + var->data.driver_location, i + var->data.location_frac,
                                        centroid, sample, indir_index, offsets);
   }
}

static LLVMValueRef get_scratch_thread_offsets(struct gallivm_state *gallivm,
                                               struct lp_type type,
                                               unsigned scratch_size)
{
   LLVMTypeRef elem_type = lp_build_int_elem_type(gallivm, type);
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   if (type.length == 1)
      return LLVMConstInt(elem_type, 0, 0);

   for (i = 0; i < type.length; ++i)
      elems[i] = LLVMConstInt(elem_type, scratch_size * i, 0);

   return LLVMConstVector(elems, type.length);
}

static void
emit_load_scratch(struct lp_build_nir_context *bld_base,
                  unsigned nc, unsigned bit_size,
                  LLVMValueRef offset,
                  LLVMValueRef outval[NIR_MAX_VEC_COMPONENTS])
{
   struct gallivm_state * gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   struct lp_build_context *load_bld;
   LLVMValueRef thread_offsets = get_scratch_thread_offsets(gallivm, uint_bld->type, bld->scratch_size);;
   uint32_t shift_val = bit_size_to_shift_size(bit_size);

   load_bld = get_int_bld(bld_base, true, bit_size);

   offset = lp_build_add(uint_bld, offset, thread_offsets);
   offset = lp_build_shr_imm(uint_bld, offset, shift_val);
   for (unsigned c = 0; c < nc; c++) {
      LLVMValueRef loop_index = lp_build_add(uint_bld, offset, lp_build_const_int_vec(gallivm, uint_bld->type, c));
      LLVMValueRef exec_mask = mask_vec(bld_base);

      LLVMValueRef result = lp_build_alloca(gallivm, load_bld->vec_type, "");
      struct lp_build_loop_state loop_state;
      lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));

      struct lp_build_if_state ifthen;
      LLVMValueRef cond, temp_res;

      loop_index = LLVMBuildExtractElement(gallivm->builder, loop_index,
                                           loop_state.counter, "");
      cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
      cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");

      lp_build_if(&ifthen, gallivm, cond);
      LLVMValueRef scalar;
      LLVMValueRef ptr2 = LLVMBuildBitCast(builder, bld->scratch_ptr, LLVMPointerType(load_bld->elem_type, 0), "");
      scalar = lp_build_pointer_get(builder, ptr2, loop_index);

      temp_res = LLVMBuildLoad(builder, result, "");
      temp_res = LLVMBuildInsertElement(builder, temp_res, scalar, loop_state.counter, "");
      LLVMBuildStore(builder, temp_res, result);
      lp_build_else(&ifthen);
      temp_res = LLVMBuildLoad(builder, result, "");
      LLVMValueRef zero;
      if (bit_size == 64)
         zero = LLVMConstInt(LLVMInt64TypeInContext(gallivm->context), 0, 0);
      else if (bit_size == 16)
         zero = LLVMConstInt(LLVMInt16TypeInContext(gallivm->context), 0, 0);
      else if (bit_size == 8)
         zero = LLVMConstInt(LLVMInt8TypeInContext(gallivm->context), 0, 0);
      else
         zero = lp_build_const_int32(gallivm, 0);
      temp_res = LLVMBuildInsertElement(builder, temp_res, zero, loop_state.counter, "");
      LLVMBuildStore(builder, temp_res, result);
      lp_build_endif(&ifthen);
      lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                                NULL, LLVMIntUGE);
      outval[c] = LLVMBuildLoad(gallivm->builder, result, "");
   }
}

static void
emit_store_scratch(struct lp_build_nir_context *bld_base,
                   unsigned writemask, unsigned nc,
                   unsigned bit_size, LLVMValueRef offset,
                   LLVMValueRef dst)
{
   struct gallivm_state * gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_nir_soa_context *bld = (struct lp_build_nir_soa_context *)bld_base;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   struct lp_build_context *store_bld;
   LLVMValueRef thread_offsets = get_scratch_thread_offsets(gallivm, uint_bld->type, bld->scratch_size);;
   uint32_t shift_val = bit_size_to_shift_size(bit_size);
   store_bld = get_int_bld(bld_base, true, bit_size);

   LLVMValueRef exec_mask = mask_vec(bld_base);
   offset = lp_build_add(uint_bld, offset, thread_offsets);
   offset = lp_build_shr_imm(uint_bld, offset, shift_val);

   for (unsigned c = 0; c < nc; c++) {
      if (!(writemask & (1u << c)))
         continue;
      LLVMValueRef val = (nc == 1) ? dst : LLVMBuildExtractValue(builder, dst, c, "");
      LLVMValueRef loop_index = lp_build_add(uint_bld, offset, lp_build_const_int_vec(gallivm, uint_bld->type, c));

      struct lp_build_loop_state loop_state;
      lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));

      LLVMValueRef value_ptr = LLVMBuildExtractElement(gallivm->builder, val,
                                                       loop_state.counter, "");
      value_ptr = LLVMBuildBitCast(gallivm->builder, value_ptr, store_bld->elem_type, "");

      struct lp_build_if_state ifthen;
      LLVMValueRef cond;

      loop_index = LLVMBuildExtractElement(gallivm->builder, loop_index,
                                                        loop_state.counter, "");

      cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, exec_mask, uint_bld->zero, "");
      cond = LLVMBuildExtractElement(gallivm->builder, cond, loop_state.counter, "");
      lp_build_if(&ifthen, gallivm, cond);

      LLVMValueRef ptr2 = LLVMBuildBitCast(builder, bld->scratch_ptr, LLVMPointerType(store_bld->elem_type, 0), "");
      lp_build_pointer_set(builder, ptr2, loop_index, value_ptr);

      lp_build_endif(&ifthen);
      lp_build_loop_end_cond(&loop_state, lp_build_const_int32(gallivm, uint_bld->type.length),
                             NULL, LLVMIntUGE);
   }
}

void lp_build_nir_soa(struct gallivm_state *gallivm,
                      struct nir_shader *shader,
                      const struct lp_build_tgsi_params *params,
                      LLVMValueRef (*outputs)[4])
{
   struct lp_build_nir_soa_context bld;
   struct lp_type type = params->type;
   struct lp_type res_type;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
   res_type.length = type.length;
   res_type.sign = 1;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.bld_base.base, gallivm, type);
   lp_build_context_init(&bld.bld_base.uint_bld, gallivm, lp_uint_type(type));
   lp_build_context_init(&bld.bld_base.int_bld, gallivm, lp_int_type(type));
   lp_build_context_init(&bld.elem_bld, gallivm, lp_elem_type(type));
   lp_build_context_init(&bld.uint_elem_bld, gallivm, lp_elem_type(lp_uint_type(type)));
   {
      struct lp_type dbl_type;
      dbl_type = type;
      dbl_type.width *= 2;
      lp_build_context_init(&bld.bld_base.dbl_bld, gallivm, dbl_type);
   }
   {
      struct lp_type half_type;
      half_type = type;
      half_type.width /= 2;
      lp_build_context_init(&bld.bld_base.half_bld, gallivm, half_type);
   }
   {
      struct lp_type uint64_type;
      uint64_type = lp_uint_type(type);
      uint64_type.width *= 2;
      lp_build_context_init(&bld.bld_base.uint64_bld, gallivm, uint64_type);
   }
   {
      struct lp_type int64_type;
      int64_type = lp_int_type(type);
      int64_type.width *= 2;
      lp_build_context_init(&bld.bld_base.int64_bld, gallivm, int64_type);
   }
   {
      struct lp_type uint16_type;
      uint16_type = lp_uint_type(type);
      uint16_type.width /= 2;
      lp_build_context_init(&bld.bld_base.uint16_bld, gallivm, uint16_type);
   }
   {
      struct lp_type int16_type;
      int16_type = lp_int_type(type);
      int16_type.width /= 2;
      lp_build_context_init(&bld.bld_base.int16_bld, gallivm, int16_type);
   }
   {
      struct lp_type uint8_type;
      uint8_type = lp_uint_type(type);
      uint8_type.width /= 4;
      lp_build_context_init(&bld.bld_base.uint8_bld, gallivm, uint8_type);
   }
   {
      struct lp_type int8_type;
      int8_type = lp_int_type(type);
      int8_type.width /= 4;
      lp_build_context_init(&bld.bld_base.int8_bld, gallivm, int8_type);
   }
   bld.bld_base.load_var = emit_load_var;
   bld.bld_base.store_var = emit_store_var;
   bld.bld_base.load_reg = emit_load_reg;
   bld.bld_base.store_reg = emit_store_reg;
   bld.bld_base.emit_var_decl = emit_var_decl;
   bld.bld_base.load_ubo = emit_load_ubo;
   bld.bld_base.load_kernel_arg = emit_load_kernel_arg;
   bld.bld_base.load_global = emit_load_global;
   bld.bld_base.store_global = emit_store_global;
   bld.bld_base.atomic_global = emit_atomic_global;
   bld.bld_base.tex = emit_tex;
   bld.bld_base.tex_size = emit_tex_size;
   bld.bld_base.bgnloop = bgnloop;
   bld.bld_base.endloop = endloop;
   bld.bld_base.if_cond = if_cond;
   bld.bld_base.else_stmt = else_stmt;
   bld.bld_base.endif_stmt = endif_stmt;
   bld.bld_base.break_stmt = break_stmt;
   bld.bld_base.continue_stmt = continue_stmt;
   bld.bld_base.sysval_intrin = emit_sysval_intrin;
   bld.bld_base.discard = discard;
   bld.bld_base.emit_vertex = emit_vertex;
   bld.bld_base.end_primitive = end_primitive;
   bld.bld_base.load_mem = emit_load_mem;
   bld.bld_base.store_mem = emit_store_mem;
   bld.bld_base.get_ssbo_size = emit_get_ssbo_size;
   bld.bld_base.atomic_mem = emit_atomic_mem;
   bld.bld_base.barrier = emit_barrier;
   bld.bld_base.image_op = emit_image_op;
   bld.bld_base.image_size = emit_image_size;
   bld.bld_base.vote = emit_vote;
   bld.bld_base.elect = emit_elect;
   bld.bld_base.reduce = emit_reduce;
   bld.bld_base.ballot = emit_ballot;
   bld.bld_base.read_invocation = emit_read_invocation;
   bld.bld_base.helper_invocation = emit_helper_invocation;
   bld.bld_base.interp_at = emit_interp_at;
   bld.bld_base.load_scratch = emit_load_scratch;
   bld.bld_base.store_scratch = emit_store_scratch;

   bld.mask = params->mask;
   bld.inputs = params->inputs;
   bld.outputs = outputs;
   bld.consts_ptr = params->consts_ptr;
   bld.const_sizes_ptr = params->const_sizes_ptr;
   bld.ssbo_ptr = params->ssbo_ptr;
   bld.ssbo_sizes_ptr = params->ssbo_sizes_ptr;
   bld.sampler = params->sampler;
//   bld.bld_base.info = params->info;

   bld.context_ptr = params->context_ptr;
   bld.thread_data_ptr = params->thread_data_ptr;
   bld.bld_base.aniso_filter_table = params->aniso_filter_table;
   bld.image = params->image;
   bld.shared_ptr = params->shared_ptr;
   bld.coro = params->coro;
   bld.kernel_args_ptr = params->kernel_args;
   bld.indirects = 0;
   if (params->info->indirect_files & (1 << TGSI_FILE_INPUT))
      bld.indirects |= nir_var_shader_in;

   bld.gs_iface = params->gs_iface;
   bld.tcs_iface = params->tcs_iface;
   bld.tes_iface = params->tes_iface;
   bld.fs_iface = params->fs_iface;
   if (bld.gs_iface) {
      struct lp_build_context *uint_bld = &bld.bld_base.uint_bld;

      bld.gs_vertex_streams = params->gs_vertex_streams;
      bld.max_output_vertices_vec = lp_build_const_int_vec(gallivm, bld.bld_base.int_bld.type,
                                                           shader->info.gs.vertices_out);
      for (int i = 0; i < params->gs_vertex_streams; i++) {
         bld.emitted_prims_vec_ptr[i] =
            lp_build_alloca(gallivm, uint_bld->vec_type, "emitted_prims_ptr");
         bld.emitted_vertices_vec_ptr[i] =
            lp_build_alloca(gallivm, uint_bld->vec_type, "emitted_vertices_ptr");
         bld.total_emitted_vertices_vec_ptr[i] =
            lp_build_alloca(gallivm, uint_bld->vec_type, "total_emitted_vertices_ptr");
      }
   }
   lp_exec_mask_init(&bld.exec_mask, &bld.bld_base.int_bld);

   bld.system_values = *params->system_values;

   bld.bld_base.shader = shader;

   if (shader->scratch_size) {
      bld.scratch_ptr = lp_build_array_alloca(gallivm,
                                              LLVMInt8TypeInContext(gallivm->context),
                                              lp_build_const_int32(gallivm, shader->scratch_size * type.length),
                                              "scratch");
   }
   bld.scratch_size = shader->scratch_size;
   emit_prologue(&bld);
   lp_build_nir_llvm(&bld.bld_base, shader);

   if (bld.gs_iface) {
      LLVMBuilderRef builder = bld.bld_base.base.gallivm->builder;
      LLVMValueRef total_emitted_vertices_vec;
      LLVMValueRef emitted_prims_vec;

      for (int i = 0; i < params->gs_vertex_streams; i++) {
         end_primitive_masked(&bld.bld_base, lp_build_mask_value(bld.mask), i);

         total_emitted_vertices_vec =
            LLVMBuildLoad(builder, bld.total_emitted_vertices_vec_ptr[i], "");

         emitted_prims_vec =
            LLVMBuildLoad(builder, bld.emitted_prims_vec_ptr[i], "");
         bld.gs_iface->gs_epilogue(bld.gs_iface,
                                   total_emitted_vertices_vec,
                                   emitted_prims_vec, i);
      }
   }
   lp_exec_mask_fini(&bld.exec_mask);
}
