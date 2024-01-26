/*
 * Copyright © 2020 Intel Corporation
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

#include "brw_nir_rt.h"
#include "brw_nir_rt_builder.h"
#include "nir_phi_builder.h"

/** Insert the appropriate return instruction at the end of the shader */
void
brw_nir_lower_shader_returns(nir_shader *shader)
{
   nir_function_impl *impl = nir_shader_get_entrypoint(shader);

   /* Reserve scratch space at the start of the shader's per-thread scratch
    * space for the return BINDLESS_SHADER_RECORD address and data payload.
    * When a shader is called, the calling shader will write the return BSR
    * address in this region of the callee's scratch space.
    *
    * We could also put it at the end of the caller's scratch space.  However,
    * doing this way means that a shader never accesses its caller's scratch
    * space unless given an explicit pointer (such as for ray payloads).  It
    * also makes computing the address easier given that we want to apply an
    * alignment to the scratch offset to ensure we can make alignment
    * assumptions in the called shader.
    *
    * This isn't needed for ray-gen shaders because they end the thread and
    * never return to the calling trampoline shader.
    */
   assert(shader->scratch_size == 0);
   if (shader->info.stage != MESA_SHADER_RAYGEN)
      shader->scratch_size = BRW_BTD_STACK_CALLEE_DATA_SIZE;

   nir_builder b;
   nir_builder_init(&b, impl);

   set_foreach(impl->end_block->predecessors, block_entry) {
      struct nir_block *block = (void *)block_entry->key;
      b.cursor = nir_after_block_before_jump(block);

      switch (shader->info.stage) {
      case MESA_SHADER_RAYGEN:
         /* A raygen shader is always the root of the shader call tree.  When
          * it ends, we retire the bindless stack ID and no further shaders
          * will be executed.
          */
         brw_nir_btd_retire(&b);
         break;

      case MESA_SHADER_ANY_HIT:
         /* The default action of an any-hit shader is to accept the ray
          * intersection.
          */
         nir_accept_ray_intersection(&b);
         break;

      case MESA_SHADER_CALLABLE:
      case MESA_SHADER_MISS:
      case MESA_SHADER_CLOSEST_HIT:
         /* Callable, miss, and closest-hit shaders don't take any special
          * action at the end.  They simply return back to the previous shader
          * in the call stack.
          */
         brw_nir_btd_return(&b);
         break;

      case MESA_SHADER_INTERSECTION:
         /* This will be handled by brw_nir_lower_intersection_shader */
         break;

      default:
         unreachable("Invalid callable shader stage");
      }

      assert(impl->end_block->predecessors->entries == 1);
      break;
   }

   nir_metadata_preserve(impl, nir_metadata_block_index |
                               nir_metadata_dominance);
}

static void
store_resume_addr(nir_builder *b, nir_intrinsic_instr *call)
{
   uint32_t call_idx = nir_intrinsic_call_idx(call);
   uint32_t offset = nir_intrinsic_stack_size(call);

   /* First thing on the called shader's stack is the resume address
    * followed by a pointer to the payload.
    */
   nir_ssa_def *resume_record_addr =
      nir_iadd_imm(b, nir_load_btd_resume_sbt_addr_intel(b),
                   call_idx * BRW_BTD_RESUME_SBT_STRIDE);
   /* By the time we get here, any remaining shader/function memory
    * pointers have been lowered to SSA values.
    */
   assert(nir_get_shader_call_payload_src(call)->is_ssa);
   nir_ssa_def *payload_addr =
      nir_get_shader_call_payload_src(call)->ssa;
   brw_nir_rt_store_scratch(b, offset, BRW_BTD_STACK_ALIGN,
                            nir_vec2(b, resume_record_addr, payload_addr),
                            0xf /* write_mask */);

   nir_btd_stack_push_intel(b, offset);
}

static bool
lower_shader_calls_instr(struct nir_builder *b, nir_instr *instr, void *data)
{
   if (instr->type != nir_instr_type_intrinsic)
      return false;

   /* Leave nir_intrinsic_rt_resume to be lowered by
    * brw_nir_lower_rt_intrinsics()
    */
   nir_intrinsic_instr *call = nir_instr_as_intrinsic(instr);

   switch (call->intrinsic) {
   case nir_intrinsic_rt_trace_ray: {
      b->cursor = nir_instr_remove(instr);

      store_resume_addr(b, call);

      nir_ssa_def *as_addr = call->src[0].ssa;
      nir_ssa_def *ray_flags = call->src[1].ssa;
      /* From the SPIR-V spec:
       *
       *    "Only the 8 least-significant bits of Cull Mask are used by this
       *    instruction - other bits are ignored.
       *
       *    Only the 4 least-significant bits of SBT Offset and SBT Stride are
       *    used by this instruction - other bits are ignored.
       *
       *    Only the 16 least-significant bits of Miss Index are used by this
       *    instruction - other bits are ignored."
       */
      nir_ssa_def *cull_mask = nir_iand_imm(b, call->src[2].ssa, 0xff);
      nir_ssa_def *sbt_offset = nir_iand_imm(b, call->src[3].ssa, 0xf);
      nir_ssa_def *sbt_stride = nir_iand_imm(b, call->src[4].ssa, 0xf);
      nir_ssa_def *miss_index = nir_iand_imm(b, call->src[5].ssa, 0xffff);
      nir_ssa_def *ray_orig = call->src[6].ssa;
      nir_ssa_def *ray_t_min = call->src[7].ssa;
      nir_ssa_def *ray_dir = call->src[8].ssa;
      nir_ssa_def *ray_t_max = call->src[9].ssa;

      /* The hardware packet takes the address to the root node in the
       * acceleration structure, not the acceleration structure itself. To
       * find that, we have to read the root node offset from the acceleration
       * structure which is the first QWord.
       */
      nir_ssa_def *root_node_ptr =
         nir_iadd(b, as_addr, nir_load_global(b, as_addr, 256, 1, 64));

      /* The hardware packet requires an address to the first element of the
       * hit SBT.
       *
       * In order to calculate this, we must multiply the "SBT Offset"
       * provided to OpTraceRay by the SBT stride provided for the hit SBT in
       * the call to vkCmdTraceRay() and add that to the base address of the
       * hit SBT. This stride is not to be confused with the "SBT Stride"
       * provided to OpTraceRay which is in units of this stride. It's a
       * rather terrible overload of the word "stride". The hardware docs
       * calls the SPIR-V stride value the "shader index multiplier" which is
       * a much more sane name.
       */
      nir_ssa_def *hit_sbt_stride_B =
         nir_load_ray_hit_sbt_stride_intel(b);
      nir_ssa_def *hit_sbt_offset_B =
         nir_umul_32x16(b, sbt_offset, nir_u2u32(b, hit_sbt_stride_B));
      nir_ssa_def *hit_sbt_addr =
         nir_iadd(b, nir_load_ray_hit_sbt_addr_intel(b),
                     nir_u2u64(b, hit_sbt_offset_B));

      /* The hardware packet takes an address to the miss BSR. */
      nir_ssa_def *miss_sbt_stride_B =
         nir_load_ray_miss_sbt_stride_intel(b);
      nir_ssa_def *miss_sbt_offset_B =
         nir_umul_32x16(b, miss_index, nir_u2u32(b, miss_sbt_stride_B));
      nir_ssa_def *miss_sbt_addr =
         nir_iadd(b, nir_load_ray_miss_sbt_addr_intel(b),
                     nir_u2u64(b, miss_sbt_offset_B));

      struct brw_nir_rt_mem_ray_defs ray_defs = {
         .root_node_ptr = root_node_ptr,
         .ray_flags = nir_u2u16(b, ray_flags),
         .ray_mask = cull_mask,
         .hit_group_sr_base_ptr = hit_sbt_addr,
         .hit_group_sr_stride = nir_u2u16(b, hit_sbt_stride_B),
         .miss_sr_ptr = miss_sbt_addr,
         .orig = ray_orig,
         .t_near = ray_t_min,
         .dir = ray_dir,
         .t_far = ray_t_max,
         .shader_index_multiplier = sbt_stride,
      };
      brw_nir_rt_store_mem_ray(b, &ray_defs, BRW_RT_BVH_LEVEL_WORLD);
      nir_trace_ray_initial_intel(b);
      return true;
   }

   case nir_intrinsic_rt_execute_callable: {
      b->cursor = nir_instr_remove(instr);

      store_resume_addr(b, call);

      nir_ssa_def *sbt_offset32 =
         nir_imul(b, call->src[0].ssa,
                     nir_u2u32(b, nir_load_callable_sbt_stride_intel(b)));
      nir_ssa_def *sbt_addr =
         nir_iadd(b, nir_load_callable_sbt_addr_intel(b),
                     nir_u2u64(b, sbt_offset32));
      brw_nir_btd_spawn(b, sbt_addr);
      return true;
   }

   default:
      return false;
   }
}

bool
brw_nir_lower_shader_calls(nir_shader *shader)
{
   return nir_shader_instructions_pass(shader,
                                       lower_shader_calls_instr,
                                       nir_metadata_block_index |
                                       nir_metadata_dominance,
                                       NULL);
}

/** Creates a trivial return shader
 *
 * This is a callable shader that doesn't really do anything.  It just loads
 * the resume address from the stack and does a return.
 */
nir_shader *
brw_nir_create_trivial_return_shader(const struct brw_compiler *compiler,
                                     void *mem_ctx)
{
   const nir_shader_compiler_options *nir_options =
      compiler->glsl_compiler_options[MESA_SHADER_CALLABLE].NirOptions;

   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_CALLABLE,
                                                  nir_options,
                                                  "RT Trivial Return");
   ralloc_steal(mem_ctx, b.shader);
   nir_shader *nir = b.shader;

   NIR_PASS_V(nir, brw_nir_lower_shader_returns);

   return nir;
}
