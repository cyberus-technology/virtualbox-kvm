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

#ifndef LP_BLD_NIR_H
#define LP_BLD_NIR_H

#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_limits.h"
#include "lp_bld_type.h"

#include "gallivm/lp_bld_tgsi.h"
#include "nir.h"

struct nir_shader;

void lp_build_nir_soa(struct gallivm_state *gallivm,
                      struct nir_shader *shader,
                      const struct lp_build_tgsi_params *params,
                      LLVMValueRef (*outputs)[4]);

struct lp_build_nir_context
{
   struct lp_build_context base;
   struct lp_build_context uint_bld;
   struct lp_build_context int_bld;
   struct lp_build_context uint8_bld;
   struct lp_build_context int8_bld;
   struct lp_build_context uint16_bld;
   struct lp_build_context int16_bld;
   struct lp_build_context half_bld;
   struct lp_build_context dbl_bld;
   struct lp_build_context uint64_bld;
   struct lp_build_context int64_bld;

   LLVMValueRef *ssa_defs;
   struct hash_table *regs;
   struct hash_table *vars;

   /** Value range analysis hash table used in code generation. */
   struct hash_table *range_ht;

   LLVMValueRef aniso_filter_table;

   nir_shader *shader;

   void (*load_ubo)(struct lp_build_nir_context *bld_base,
                    unsigned nc,
                    unsigned bit_size,
                    bool offset_is_uniform,
                    LLVMValueRef index, LLVMValueRef offset, LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);

   void (*load_kernel_arg)(struct lp_build_nir_context *bld_base,
                           unsigned nc,
                           unsigned bit_size,
                           unsigned offset_bit_size,
                           bool offset_is_uniform,
                           LLVMValueRef offset, LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);

   void (*load_global)(struct lp_build_nir_context *bld_base,
                       unsigned nc, unsigned bit_size,
                       unsigned offset_bit_size,
                       LLVMValueRef offset, LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);

   void (*store_global)(struct lp_build_nir_context *bld_base,
                        unsigned writemask,
                        unsigned nc, unsigned bit_size,
                        unsigned addr_bit_size,
                        LLVMValueRef addr, LLVMValueRef dst);

   void (*atomic_global)(struct lp_build_nir_context *bld_base,
                         nir_intrinsic_op op,
                         unsigned addr_bit_size,
                         unsigned val_bit_size,
                         LLVMValueRef addr,
                         LLVMValueRef val, LLVMValueRef val2,
                         LLVMValueRef *result);

   /* for SSBO and shared memory */
   void (*load_mem)(struct lp_build_nir_context *bld_base,
                    unsigned nc, unsigned bit_size,
                    LLVMValueRef index, LLVMValueRef offset, LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);
   void (*store_mem)(struct lp_build_nir_context *bld_base,
                     unsigned writemask, unsigned nc, unsigned bit_size,
                     LLVMValueRef index, LLVMValueRef offset, LLVMValueRef dst);

   void (*atomic_mem)(struct lp_build_nir_context *bld_base,
                      nir_intrinsic_op op,
                      unsigned bit_size,
                      LLVMValueRef index, LLVMValueRef offset,
                      LLVMValueRef val, LLVMValueRef val2,
                      LLVMValueRef *result);

   void (*barrier)(struct lp_build_nir_context *bld_base);

   void (*image_op)(struct lp_build_nir_context *bld_base,
                    struct lp_img_params *params);
   void (*image_size)(struct lp_build_nir_context *bld_base,
                      struct lp_sampler_size_query_params *params);
   LLVMValueRef (*get_ssbo_size)(struct lp_build_nir_context *bld_base,
                                 LLVMValueRef index);

   void (*load_var)(struct lp_build_nir_context *bld_base,
                    nir_variable_mode deref_mode,
                    unsigned num_components,
                    unsigned bit_size,
                    nir_variable *var,
                    unsigned vertex_index,
                    LLVMValueRef indir_vertex_index,
                    unsigned const_index,
                    LLVMValueRef indir_index,
                    LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);
   void (*store_var)(struct lp_build_nir_context *bld_base,
                     nir_variable_mode deref_mode,
                     unsigned num_components,
                     unsigned bit_size,
                     nir_variable *var,
                     unsigned writemask,
                     LLVMValueRef indir_vertex_index,
                     unsigned const_index,
                     LLVMValueRef indir_index,
                     LLVMValueRef dst);

   LLVMValueRef (*load_reg)(struct lp_build_nir_context *bld_base,
                            struct lp_build_context *reg_bld,
                            const nir_reg_src *reg,
                            LLVMValueRef indir_src,
                            LLVMValueRef reg_storage);
   void (*store_reg)(struct lp_build_nir_context *bld_base,
                     struct lp_build_context *reg_bld,
                     const nir_reg_dest *reg,
                     unsigned writemask,
                     LLVMValueRef indir_src,
                     LLVMValueRef reg_storage,
                     LLVMValueRef dst[NIR_MAX_VEC_COMPONENTS]);

   void (*load_scratch)(struct lp_build_nir_context *bld_base,
                        unsigned nc, unsigned bit_size,
                        LLVMValueRef offset,
                        LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);
   void (*store_scratch)(struct lp_build_nir_context *bld_base,
                         unsigned writemask, unsigned nc,
                         unsigned bit_size, LLVMValueRef offset,
                         LLVMValueRef val);

   void (*emit_var_decl)(struct lp_build_nir_context *bld_base,
                         nir_variable *var);

   void (*tex)(struct lp_build_nir_context *bld_base,
               struct lp_sampler_params *params);

   void (*tex_size)(struct lp_build_nir_context *bld_base,
                    struct lp_sampler_size_query_params *params);

   void (*sysval_intrin)(struct lp_build_nir_context *bld_base,
                         nir_intrinsic_instr *instr,
                         LLVMValueRef result[NIR_MAX_VEC_COMPONENTS]);
   void (*discard)(struct lp_build_nir_context *bld_base,
                   LLVMValueRef cond);

   void (*bgnloop)(struct lp_build_nir_context *bld_base);
   void (*endloop)(struct lp_build_nir_context *bld_base);
   void (*if_cond)(struct lp_build_nir_context *bld_base, LLVMValueRef cond);
   void (*else_stmt)(struct lp_build_nir_context *bld_base);
   void (*endif_stmt)(struct lp_build_nir_context *bld_base);
   void (*break_stmt)(struct lp_build_nir_context *bld_base);
   void (*continue_stmt)(struct lp_build_nir_context *bld_base);

   void (*emit_vertex)(struct lp_build_nir_context *bld_base, uint32_t stream_id);
   void (*end_primitive)(struct lp_build_nir_context *bld_base, uint32_t stream_id);

   void (*vote)(struct lp_build_nir_context *bld_base, LLVMValueRef src, nir_intrinsic_instr *instr, LLVMValueRef dst[4]);
   void (*elect)(struct lp_build_nir_context *bld_base, LLVMValueRef dst[4]);
   void (*reduce)(struct lp_build_nir_context *bld_base, LLVMValueRef src, nir_intrinsic_instr *instr, LLVMValueRef dst[4]);
   void (*ballot)(struct lp_build_nir_context *bld_base, LLVMValueRef src, nir_intrinsic_instr *instr, LLVMValueRef dst[4]);
   void (*read_invocation)(struct lp_build_nir_context *bld_base,
                           LLVMValueRef src, unsigned bit_size, LLVMValueRef invoc,
                           LLVMValueRef dst[4]);
   void (*helper_invocation)(struct lp_build_nir_context *bld_base, LLVMValueRef *dst);

   void (*interp_at)(struct lp_build_nir_context *bld_base,
                     unsigned num_components,
                     nir_variable *var,
                     bool centroid, bool sample,
                     unsigned const_index,
                     LLVMValueRef indir_index,
                     LLVMValueRef offsets[2], LLVMValueRef dst[4]);
//   LLVMValueRef main_function
};

struct lp_build_nir_soa_context
{
   struct lp_build_nir_context bld_base;

   /* Builder for scalar elements of shader's data type (float) */
   struct lp_build_context elem_bld;
   struct lp_build_context uint_elem_bld;

   LLVMValueRef consts_ptr;
   LLVMValueRef const_sizes_ptr;
   LLVMValueRef consts[LP_MAX_TGSI_CONST_BUFFERS];
   LLVMValueRef consts_sizes[LP_MAX_TGSI_CONST_BUFFERS];
   const LLVMValueRef (*inputs)[TGSI_NUM_CHANNELS];
   LLVMValueRef (*outputs)[TGSI_NUM_CHANNELS];
   LLVMValueRef context_ptr;
   LLVMValueRef thread_data_ptr;

   LLVMValueRef ssbo_ptr;
   LLVMValueRef ssbo_sizes_ptr;
   LLVMValueRef ssbos[LP_MAX_TGSI_SHADER_BUFFERS];
   LLVMValueRef ssbo_sizes[LP_MAX_TGSI_SHADER_BUFFERS];

   LLVMValueRef shared_ptr;
   LLVMValueRef scratch_ptr;
   unsigned scratch_size;

   const struct lp_build_coro_suspend_info *coro;

   const struct lp_build_sampler_soa *sampler;
   const struct lp_build_image_soa *image;

   const struct lp_build_gs_iface *gs_iface;
   const struct lp_build_tcs_iface *tcs_iface;
   const struct lp_build_tes_iface *tes_iface;
   const struct lp_build_fs_iface *fs_iface;
   LLVMValueRef emitted_prims_vec_ptr[PIPE_MAX_VERTEX_STREAMS];
   LLVMValueRef total_emitted_vertices_vec_ptr[PIPE_MAX_VERTEX_STREAMS];
   LLVMValueRef emitted_vertices_vec_ptr[PIPE_MAX_VERTEX_STREAMS];
   LLVMValueRef max_output_vertices_vec;
   struct lp_bld_tgsi_system_values system_values;

   nir_variable_mode indirects;
   struct lp_build_mask_context *mask;
   struct lp_exec_mask exec_mask;

   /* We allocate/use this array of inputs if (indirects & nir_var_shader_in) is
    * set. The inputs[] array above is unused then.
    */
   LLVMValueRef inputs_array;

   LLVMValueRef kernel_args_ptr;
   unsigned gs_vertex_streams;
};

bool
lp_build_nir_llvm(struct lp_build_nir_context *bld_base,
                  struct nir_shader *nir);

void lp_build_opt_nir(struct nir_shader *nir);

static inline LLVMValueRef
lp_nir_array_build_gather_values(LLVMBuilderRef builder,
                                 LLVMValueRef * values,
                                 unsigned value_count)
{
   LLVMTypeRef arr_type = LLVMArrayType(LLVMTypeOf(values[0]), value_count);
   LLVMValueRef arr = LLVMGetUndef(arr_type);
   unsigned i;

   for (i = 0; i < value_count; i++) {
      arr = LLVMBuildInsertValue(builder, arr, values[i], i, "");
   }
   return arr;
}

static inline struct lp_build_context *get_flt_bld(struct lp_build_nir_context *bld_base,
                                                   unsigned op_bit_size)
{
   switch (op_bit_size) {
   case 64:
      return &bld_base->dbl_bld;
   case 16:
      return &bld_base->half_bld;
   default:
   case 32:
      return &bld_base->base;
   }
}

static inline struct lp_build_context *get_int_bld(struct lp_build_nir_context *bld_base,
                                                   bool is_unsigned,
                                                   unsigned op_bit_size)
{
   if (is_unsigned) {
      switch (op_bit_size) {
      case 64:
         return &bld_base->uint64_bld;
      case 32:
      default:
         return &bld_base->uint_bld;
      case 16:
         return &bld_base->uint16_bld;
      case 8:
         return &bld_base->uint8_bld;
      }
   } else {
      switch (op_bit_size) {
      case 64:
         return &bld_base->int64_bld;
      default:
      case 32:
         return &bld_base->int_bld;
      case 16:
         return &bld_base->int16_bld;
      case 8:
         return &bld_base->int8_bld;
      }
   }
}

#endif
