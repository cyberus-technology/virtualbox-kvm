/*
 * Copyright 2017 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Karol Herbst <kherbst@redhat.com>
 */

#include "compiler/nir/nir.h"

#include "util/u_debug.h"

#include "codegen/nv50_ir.h"
#include "codegen/nv50_ir_from_common.h"
#include "codegen/nv50_ir_lowering_helper.h"
#include "codegen/nv50_ir_util.h"
#include "tgsi/tgsi_from_mesa.h"

#if __cplusplus >= 201103L
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif
#include <cstring>
#include <list>
#include <vector>

namespace {

#if __cplusplus >= 201103L
using std::hash;
using std::unordered_map;
#else
using std::tr1::hash;
using std::tr1::unordered_map;
#endif

using namespace nv50_ir;

int
type_size(const struct glsl_type *type, bool bindless)
{
   return glsl_count_attribute_slots(type, false);
}

static void
function_temp_type_info(const struct glsl_type *type, unsigned *size, unsigned *align)
{
   assert(glsl_type_is_vector_or_scalar(type));

   unsigned comp_size = glsl_type_is_boolean(type) ? 4 : glsl_get_bit_size(type) / 8;
   unsigned length = glsl_get_vector_elements(type);

   *size = comp_size * length;
   *align = 0x10;
}

class Converter : public ConverterCommon
{
public:
   Converter(Program *, nir_shader *, nv50_ir_prog_info *, nv50_ir_prog_info_out *);

   bool run();
private:
   typedef std::vector<LValue*> LValues;
   typedef unordered_map<unsigned, LValues> NirDefMap;
   typedef unordered_map<unsigned, nir_load_const_instr*> ImmediateMap;
   typedef unordered_map<unsigned, BasicBlock*> NirBlockMap;

   CacheMode convert(enum gl_access_qualifier);
   TexTarget convert(glsl_sampler_dim, bool isArray, bool isShadow);
   LValues& convert(nir_alu_dest *);
   BasicBlock* convert(nir_block *);
   LValues& convert(nir_dest *);
   SVSemantic convert(nir_intrinsic_op);
   Value* convert(nir_load_const_instr*, uint8_t);
   LValues& convert(nir_register *);
   LValues& convert(nir_ssa_def *);

   Value* getSrc(nir_alu_src *, uint8_t component = 0);
   Value* getSrc(nir_register *, uint8_t);
   Value* getSrc(nir_src *, uint8_t, bool indirect = false);
   Value* getSrc(nir_ssa_def *, uint8_t);

   // returned value is the constant part of the given source (either the
   // nir_src or the selected source component of an intrinsic). Even though
   // this is mostly an optimization to be able to skip indirects in a few
   // cases, sometimes we require immediate values or set some fileds on
   // instructions (e.g. tex) in order for codegen to consume those.
   // If the found value has not a constant part, the Value gets returned
   // through the Value parameter.
   uint32_t getIndirect(nir_src *, uint8_t, Value *&);
   // isScalar indicates that the addressing is scalar, vec4 addressing is
   // assumed otherwise
   uint32_t getIndirect(nir_intrinsic_instr *, uint8_t s, uint8_t c, Value *&,
                        bool isScalar = false);

   uint32_t getSlotAddress(nir_intrinsic_instr *, uint8_t idx, uint8_t slot);

   void setInterpolate(nv50_ir_varying *,
                       uint8_t,
                       bool centroid,
                       unsigned semantics);

   Instruction *loadFrom(DataFile, uint8_t, DataType, Value *def, uint32_t base,
                         uint8_t c, Value *indirect0 = NULL,
                         Value *indirect1 = NULL, bool patch = false);
   void storeTo(nir_intrinsic_instr *, DataFile, operation, DataType,
                Value *src, uint8_t idx, uint8_t c, Value *indirect0 = NULL,
                Value *indirect1 = NULL);

   bool isFloatType(nir_alu_type);
   bool isSignedType(nir_alu_type);
   bool isResultFloat(nir_op);
   bool isResultSigned(nir_op);

   DataType getDType(nir_alu_instr *);
   DataType getDType(nir_intrinsic_instr *);
   DataType getDType(nir_op, uint8_t);

   DataFile getFile(nir_intrinsic_op);

   std::vector<DataType> getSTypes(nir_alu_instr *);
   DataType getSType(nir_src &, bool isFloat, bool isSigned);

   operation getOperation(nir_intrinsic_op);
   operation getOperation(nir_op);
   operation getOperation(nir_texop);
   operation preOperationNeeded(nir_op);

   int getSubOp(nir_intrinsic_op);
   int getSubOp(nir_op);

   CondCode getCondCode(nir_op);

   bool assignSlots();
   bool parseNIR();

   bool visit(nir_alu_instr *);
   bool visit(nir_block *);
   bool visit(nir_cf_node *);
   bool visit(nir_function *);
   bool visit(nir_if *);
   bool visit(nir_instr *);
   bool visit(nir_intrinsic_instr *);
   bool visit(nir_jump_instr *);
   bool visit(nir_load_const_instr*);
   bool visit(nir_loop *);
   bool visit(nir_ssa_undef_instr *);
   bool visit(nir_tex_instr *);

   // tex stuff
   unsigned int getNIRArgCount(TexInstruction::Target&);

   nir_shader *nir;

   NirDefMap ssaDefs;
   NirDefMap regDefs;
   ImmediateMap immediates;
   NirBlockMap blocks;
   unsigned int curLoopDepth;
   unsigned int curIfDepth;

   BasicBlock *exit;
   Value *zero;
   Instruction *immInsertPos;

   int clipVertexOutput;

   union {
      struct {
         Value *position;
      } fp;
   };
};

Converter::Converter(Program *prog, nir_shader *nir, nv50_ir_prog_info *info,
                     nv50_ir_prog_info_out *info_out)
   : ConverterCommon(prog, info, info_out),
     nir(nir),
     curLoopDepth(0),
     curIfDepth(0),
     exit(NULL),
     immInsertPos(NULL),
     clipVertexOutput(-1)
{
   zero = mkImm((uint32_t)0);
}

BasicBlock *
Converter::convert(nir_block *block)
{
   NirBlockMap::iterator it = blocks.find(block->index);
   if (it != blocks.end())
      return it->second;

   BasicBlock *bb = new BasicBlock(func);
   blocks[block->index] = bb;
   return bb;
}

bool
Converter::isFloatType(nir_alu_type type)
{
   return nir_alu_type_get_base_type(type) == nir_type_float;
}

bool
Converter::isSignedType(nir_alu_type type)
{
   return nir_alu_type_get_base_type(type) == nir_type_int;
}

bool
Converter::isResultFloat(nir_op op)
{
   const nir_op_info &info = nir_op_infos[op];
   if (info.output_type != nir_type_invalid)
      return isFloatType(info.output_type);

   ERROR("isResultFloat not implemented for %s\n", nir_op_infos[op].name);
   assert(false);
   return true;
}

bool
Converter::isResultSigned(nir_op op)
{
   switch (op) {
   // there is no umul and we get wrong results if we treat all muls as signed
   case nir_op_imul:
   case nir_op_inot:
      return false;
   default:
      const nir_op_info &info = nir_op_infos[op];
      if (info.output_type != nir_type_invalid)
         return isSignedType(info.output_type);
      ERROR("isResultSigned not implemented for %s\n", nir_op_infos[op].name);
      assert(false);
      return true;
   }
}

DataType
Converter::getDType(nir_alu_instr *insn)
{
   if (insn->dest.dest.is_ssa)
      return getDType(insn->op, insn->dest.dest.ssa.bit_size);
   else
      return getDType(insn->op, insn->dest.dest.reg.reg->bit_size);
}

DataType
Converter::getDType(nir_intrinsic_instr *insn)
{
   bool isFloat, isSigned;
   switch (insn->intrinsic) {
   case nir_intrinsic_bindless_image_atomic_fadd:
   case nir_intrinsic_global_atomic_fadd:
   case nir_intrinsic_image_atomic_fadd:
   case nir_intrinsic_shared_atomic_fadd:
   case nir_intrinsic_ssbo_atomic_fadd:
      isFloat = true;
      isSigned = false;
      break;
   case nir_intrinsic_shared_atomic_imax:
   case nir_intrinsic_shared_atomic_imin:
   case nir_intrinsic_ssbo_atomic_imax:
   case nir_intrinsic_ssbo_atomic_imin:
      isFloat = false;
      isSigned = true;
      break;
   default:
      isFloat = false;
      isSigned = false;
      break;
   }

   if (insn->dest.is_ssa)
      return typeOfSize(insn->dest.ssa.bit_size / 8, isFloat, isSigned);
   else
      return typeOfSize(insn->dest.reg.reg->bit_size / 8, isFloat, isSigned);
}

DataType
Converter::getDType(nir_op op, uint8_t bitSize)
{
   DataType ty = typeOfSize(bitSize / 8, isResultFloat(op), isResultSigned(op));
   if (ty == TYPE_NONE) {
      ERROR("couldn't get Type for op %s with bitSize %u\n", nir_op_infos[op].name, bitSize);
      assert(false);
   }
   return ty;
}

std::vector<DataType>
Converter::getSTypes(nir_alu_instr *insn)
{
   const nir_op_info &info = nir_op_infos[insn->op];
   std::vector<DataType> res(info.num_inputs);

   for (uint8_t i = 0; i < info.num_inputs; ++i) {
      if (info.input_types[i] != nir_type_invalid) {
         res[i] = getSType(insn->src[i].src, isFloatType(info.input_types[i]), isSignedType(info.input_types[i]));
      } else {
         ERROR("getSType not implemented for %s idx %u\n", info.name, i);
         assert(false);
         res[i] = TYPE_NONE;
         break;
      }
   }

   return res;
}

DataType
Converter::getSType(nir_src &src, bool isFloat, bool isSigned)
{
   uint8_t bitSize;
   if (src.is_ssa)
      bitSize = src.ssa->bit_size;
   else
      bitSize = src.reg.reg->bit_size;

   DataType ty = typeOfSize(bitSize / 8, isFloat, isSigned);
   if (ty == TYPE_NONE) {
      const char *str;
      if (isFloat)
         str = "float";
      else if (isSigned)
         str = "int";
      else
         str = "uint";
      ERROR("couldn't get Type for %s with bitSize %u\n", str, bitSize);
      assert(false);
   }
   return ty;
}

DataFile
Converter::getFile(nir_intrinsic_op op)
{
   switch (op) {
   case nir_intrinsic_load_global:
   case nir_intrinsic_store_global:
   case nir_intrinsic_load_global_constant:
      return FILE_MEMORY_GLOBAL;
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_store_scratch:
      return FILE_MEMORY_LOCAL;
   case nir_intrinsic_load_shared:
   case nir_intrinsic_store_shared:
      return FILE_MEMORY_SHARED;
   case nir_intrinsic_load_kernel_input:
      return FILE_SHADER_INPUT;
   default:
      ERROR("couldn't get DateFile for op %s\n", nir_intrinsic_infos[op].name);
      assert(false);
   }
   return FILE_NULL;
}

operation
Converter::getOperation(nir_op op)
{
   switch (op) {
   // basic ops with float and int variants
   case nir_op_fabs:
   case nir_op_iabs:
      return OP_ABS;
   case nir_op_fadd:
   case nir_op_iadd:
      return OP_ADD;
   case nir_op_iand:
      return OP_AND;
   case nir_op_ifind_msb:
   case nir_op_ufind_msb:
      return OP_BFIND;
   case nir_op_fceil:
      return OP_CEIL;
   case nir_op_fcos:
      return OP_COS;
   case nir_op_f2f32:
   case nir_op_f2f64:
   case nir_op_f2i32:
   case nir_op_f2i64:
   case nir_op_f2u32:
   case nir_op_f2u64:
   case nir_op_i2f32:
   case nir_op_i2f64:
   case nir_op_i2i32:
   case nir_op_i2i64:
   case nir_op_u2f32:
   case nir_op_u2f64:
   case nir_op_u2u32:
   case nir_op_u2u64:
      return OP_CVT;
   case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
      return OP_DFDX;
   case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
      return OP_DFDY;
   case nir_op_fdiv:
   case nir_op_idiv:
   case nir_op_udiv:
      return OP_DIV;
   case nir_op_fexp2:
      return OP_EX2;
   case nir_op_ffloor:
      return OP_FLOOR;
   case nir_op_ffma:
      return OP_FMA;
   case nir_op_flog2:
      return OP_LG2;
   case nir_op_fmax:
   case nir_op_imax:
   case nir_op_umax:
      return OP_MAX;
   case nir_op_pack_64_2x32_split:
      return OP_MERGE;
   case nir_op_fmin:
   case nir_op_imin:
   case nir_op_umin:
      return OP_MIN;
   case nir_op_fmod:
   case nir_op_imod:
   case nir_op_umod:
   case nir_op_frem:
   case nir_op_irem:
      return OP_MOD;
   case nir_op_fmul:
   case nir_op_imul:
   case nir_op_imul_high:
   case nir_op_umul_high:
      return OP_MUL;
   case nir_op_fneg:
   case nir_op_ineg:
      return OP_NEG;
   case nir_op_inot:
      return OP_NOT;
   case nir_op_ior:
      return OP_OR;
   case nir_op_fpow:
      return OP_POW;
   case nir_op_frcp:
      return OP_RCP;
   case nir_op_frsq:
      return OP_RSQ;
   case nir_op_fsat:
      return OP_SAT;
   case nir_op_feq32:
   case nir_op_ieq32:
   case nir_op_fge32:
   case nir_op_ige32:
   case nir_op_uge32:
   case nir_op_flt32:
   case nir_op_ilt32:
   case nir_op_ult32:
   case nir_op_fneu32:
   case nir_op_ine32:
      return OP_SET;
   case nir_op_ishl:
      return OP_SHL;
   case nir_op_ishr:
   case nir_op_ushr:
      return OP_SHR;
   case nir_op_fsin:
      return OP_SIN;
   case nir_op_fsqrt:
      return OP_SQRT;
   case nir_op_ftrunc:
      return OP_TRUNC;
   case nir_op_ixor:
      return OP_XOR;
   default:
      ERROR("couldn't get operation for op %s\n", nir_op_infos[op].name);
      assert(false);
      return OP_NOP;
   }
}

operation
Converter::getOperation(nir_texop op)
{
   switch (op) {
   case nir_texop_tex:
      return OP_TEX;
   case nir_texop_lod:
      return OP_TXLQ;
   case nir_texop_txb:
      return OP_TXB;
   case nir_texop_txd:
      return OP_TXD;
   case nir_texop_txf:
   case nir_texop_txf_ms:
      return OP_TXF;
   case nir_texop_tg4:
      return OP_TXG;
   case nir_texop_txl:
      return OP_TXL;
   case nir_texop_query_levels:
   case nir_texop_texture_samples:
   case nir_texop_txs:
      return OP_TXQ;
   default:
      ERROR("couldn't get operation for nir_texop %u\n", op);
      assert(false);
      return OP_NOP;
   }
}

operation
Converter::getOperation(nir_intrinsic_op op)
{
   switch (op) {
   case nir_intrinsic_emit_vertex:
      return OP_EMIT;
   case nir_intrinsic_end_primitive:
      return OP_RESTART;
   case nir_intrinsic_bindless_image_atomic_add:
   case nir_intrinsic_image_atomic_add:
   case nir_intrinsic_bindless_image_atomic_and:
   case nir_intrinsic_image_atomic_and:
   case nir_intrinsic_bindless_image_atomic_comp_swap:
   case nir_intrinsic_image_atomic_comp_swap:
   case nir_intrinsic_bindless_image_atomic_exchange:
   case nir_intrinsic_image_atomic_exchange:
   case nir_intrinsic_bindless_image_atomic_imax:
   case nir_intrinsic_image_atomic_imax:
   case nir_intrinsic_bindless_image_atomic_umax:
   case nir_intrinsic_image_atomic_umax:
   case nir_intrinsic_bindless_image_atomic_imin:
   case nir_intrinsic_image_atomic_imin:
   case nir_intrinsic_bindless_image_atomic_umin:
   case nir_intrinsic_image_atomic_umin:
   case nir_intrinsic_bindless_image_atomic_or:
   case nir_intrinsic_image_atomic_or:
   case nir_intrinsic_bindless_image_atomic_xor:
   case nir_intrinsic_image_atomic_xor:
   case nir_intrinsic_bindless_image_atomic_inc_wrap:
   case nir_intrinsic_image_atomic_inc_wrap:
   case nir_intrinsic_bindless_image_atomic_dec_wrap:
   case nir_intrinsic_image_atomic_dec_wrap:
      return OP_SUREDP;
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_image_load:
      return OP_SULDP;
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_image_samples:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_image_size:
      return OP_SUQ;
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_image_store:
      return OP_SUSTP;
   default:
      ERROR("couldn't get operation for nir_intrinsic_op %u\n", op);
      assert(false);
      return OP_NOP;
   }
}

operation
Converter::preOperationNeeded(nir_op op)
{
   switch (op) {
   case nir_op_fcos:
   case nir_op_fsin:
      return OP_PRESIN;
   default:
      return OP_NOP;
   }
}

int
Converter::getSubOp(nir_op op)
{
   switch (op) {
   case nir_op_imul_high:
   case nir_op_umul_high:
      return NV50_IR_SUBOP_MUL_HIGH;
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr:
      return NV50_IR_SUBOP_SHIFT_WRAP;
   default:
      return 0;
   }
}

int
Converter::getSubOp(nir_intrinsic_op op)
{
   switch (op) {
   case nir_intrinsic_bindless_image_atomic_add:
   case nir_intrinsic_global_atomic_add:
   case nir_intrinsic_image_atomic_add:
   case nir_intrinsic_shared_atomic_add:
   case nir_intrinsic_ssbo_atomic_add:
      return  NV50_IR_SUBOP_ATOM_ADD;
   case nir_intrinsic_bindless_image_atomic_fadd:
   case nir_intrinsic_global_atomic_fadd:
   case nir_intrinsic_image_atomic_fadd:
   case nir_intrinsic_shared_atomic_fadd:
   case nir_intrinsic_ssbo_atomic_fadd:
      return  NV50_IR_SUBOP_ATOM_ADD;
   case nir_intrinsic_bindless_image_atomic_and:
   case nir_intrinsic_global_atomic_and:
   case nir_intrinsic_image_atomic_and:
   case nir_intrinsic_shared_atomic_and:
   case nir_intrinsic_ssbo_atomic_and:
      return  NV50_IR_SUBOP_ATOM_AND;
   case nir_intrinsic_bindless_image_atomic_comp_swap:
   case nir_intrinsic_global_atomic_comp_swap:
   case nir_intrinsic_image_atomic_comp_swap:
   case nir_intrinsic_shared_atomic_comp_swap:
   case nir_intrinsic_ssbo_atomic_comp_swap:
      return  NV50_IR_SUBOP_ATOM_CAS;
   case nir_intrinsic_bindless_image_atomic_exchange:
   case nir_intrinsic_global_atomic_exchange:
   case nir_intrinsic_image_atomic_exchange:
   case nir_intrinsic_shared_atomic_exchange:
   case nir_intrinsic_ssbo_atomic_exchange:
      return  NV50_IR_SUBOP_ATOM_EXCH;
   case nir_intrinsic_bindless_image_atomic_or:
   case nir_intrinsic_global_atomic_or:
   case nir_intrinsic_image_atomic_or:
   case nir_intrinsic_shared_atomic_or:
   case nir_intrinsic_ssbo_atomic_or:
      return  NV50_IR_SUBOP_ATOM_OR;
   case nir_intrinsic_bindless_image_atomic_imax:
   case nir_intrinsic_bindless_image_atomic_umax:
   case nir_intrinsic_global_atomic_imax:
   case nir_intrinsic_global_atomic_umax:
   case nir_intrinsic_image_atomic_imax:
   case nir_intrinsic_image_atomic_umax:
   case nir_intrinsic_shared_atomic_imax:
   case nir_intrinsic_shared_atomic_umax:
   case nir_intrinsic_ssbo_atomic_imax:
   case nir_intrinsic_ssbo_atomic_umax:
      return  NV50_IR_SUBOP_ATOM_MAX;
   case nir_intrinsic_bindless_image_atomic_imin:
   case nir_intrinsic_bindless_image_atomic_umin:
   case nir_intrinsic_global_atomic_imin:
   case nir_intrinsic_global_atomic_umin:
   case nir_intrinsic_image_atomic_imin:
   case nir_intrinsic_image_atomic_umin:
   case nir_intrinsic_shared_atomic_imin:
   case nir_intrinsic_shared_atomic_umin:
   case nir_intrinsic_ssbo_atomic_imin:
   case nir_intrinsic_ssbo_atomic_umin:
      return  NV50_IR_SUBOP_ATOM_MIN;
   case nir_intrinsic_bindless_image_atomic_xor:
   case nir_intrinsic_global_atomic_xor:
   case nir_intrinsic_image_atomic_xor:
   case nir_intrinsic_shared_atomic_xor:
   case nir_intrinsic_ssbo_atomic_xor:
      return  NV50_IR_SUBOP_ATOM_XOR;
   case nir_intrinsic_bindless_image_atomic_inc_wrap:
   case nir_intrinsic_image_atomic_inc_wrap:
      return NV50_IR_SUBOP_ATOM_INC;
   case nir_intrinsic_bindless_image_atomic_dec_wrap:
   case nir_intrinsic_image_atomic_dec_wrap:
      return NV50_IR_SUBOP_ATOM_DEC;

   case nir_intrinsic_group_memory_barrier:
   case nir_intrinsic_memory_barrier:
   case nir_intrinsic_memory_barrier_buffer:
   case nir_intrinsic_memory_barrier_image:
      return NV50_IR_SUBOP_MEMBAR(M, GL);
   case nir_intrinsic_memory_barrier_shared:
      return NV50_IR_SUBOP_MEMBAR(M, CTA);

   case nir_intrinsic_vote_all:
      return NV50_IR_SUBOP_VOTE_ALL;
   case nir_intrinsic_vote_any:
      return NV50_IR_SUBOP_VOTE_ANY;
   case nir_intrinsic_vote_ieq:
      return NV50_IR_SUBOP_VOTE_UNI;
   default:
      return 0;
   }
}

CondCode
Converter::getCondCode(nir_op op)
{
   switch (op) {
   case nir_op_feq32:
   case nir_op_ieq32:
      return CC_EQ;
   case nir_op_fge32:
   case nir_op_ige32:
   case nir_op_uge32:
      return CC_GE;
   case nir_op_flt32:
   case nir_op_ilt32:
   case nir_op_ult32:
      return CC_LT;
   case nir_op_fneu32:
      return CC_NEU;
   case nir_op_ine32:
      return CC_NE;
   default:
      ERROR("couldn't get CondCode for op %s\n", nir_op_infos[op].name);
      assert(false);
      return CC_FL;
   }
}

Converter::LValues&
Converter::convert(nir_alu_dest *dest)
{
   return convert(&dest->dest);
}

Converter::LValues&
Converter::convert(nir_dest *dest)
{
   if (dest->is_ssa)
      return convert(&dest->ssa);
   if (dest->reg.indirect) {
      ERROR("no support for indirects.");
      assert(false);
   }
   return convert(dest->reg.reg);
}

Converter::LValues&
Converter::convert(nir_register *reg)
{
   assert(!reg->num_array_elems);

   NirDefMap::iterator it = regDefs.find(reg->index);
   if (it != regDefs.end())
      return it->second;

   LValues newDef(reg->num_components);
   for (uint8_t i = 0; i < reg->num_components; i++)
      newDef[i] = getScratch(std::max(4, reg->bit_size / 8));
   return regDefs[reg->index] = newDef;
}

Converter::LValues&
Converter::convert(nir_ssa_def *def)
{
   NirDefMap::iterator it = ssaDefs.find(def->index);
   if (it != ssaDefs.end())
      return it->second;

   LValues newDef(def->num_components);
   for (uint8_t i = 0; i < def->num_components; i++)
      newDef[i] = getSSA(std::max(4, def->bit_size / 8));
   return ssaDefs[def->index] = newDef;
}

Value*
Converter::getSrc(nir_alu_src *src, uint8_t component)
{
   if (src->abs || src->negate) {
      ERROR("modifiers currently not supported on nir_alu_src\n");
      assert(false);
   }
   return getSrc(&src->src, src->swizzle[component]);
}

Value*
Converter::getSrc(nir_register *reg, uint8_t idx)
{
   NirDefMap::iterator it = regDefs.find(reg->index);
   if (it == regDefs.end())
      return convert(reg)[idx];
   return it->second[idx];
}

Value*
Converter::getSrc(nir_src *src, uint8_t idx, bool indirect)
{
   if (src->is_ssa)
      return getSrc(src->ssa, idx);

   if (src->reg.indirect) {
      if (indirect)
         return getSrc(src->reg.indirect, idx);
      ERROR("no support for indirects.");
      assert(false);
      return NULL;
   }

   return getSrc(src->reg.reg, idx);
}

Value*
Converter::getSrc(nir_ssa_def *src, uint8_t idx)
{
   ImmediateMap::iterator iit = immediates.find(src->index);
   if (iit != immediates.end())
      return convert((*iit).second, idx);

   NirDefMap::iterator it = ssaDefs.find(src->index);
   if (it == ssaDefs.end()) {
      ERROR("SSA value %u not found\n", src->index);
      assert(false);
      return NULL;
   }
   return it->second[idx];
}

uint32_t
Converter::getIndirect(nir_src *src, uint8_t idx, Value *&indirect)
{
   nir_const_value *offset = nir_src_as_const_value(*src);

   if (offset) {
      indirect = NULL;
      return offset[0].u32;
   }

   indirect = getSrc(src, idx, true);
   return 0;
}

uint32_t
Converter::getIndirect(nir_intrinsic_instr *insn, uint8_t s, uint8_t c, Value *&indirect, bool isScalar)
{
   int32_t idx = nir_intrinsic_base(insn) + getIndirect(&insn->src[s], c, indirect);
   if (indirect && !isScalar)
      indirect = mkOp2v(OP_SHL, TYPE_U32, getSSA(4, FILE_ADDRESS), indirect, loadImm(NULL, 4));
   return idx;
}

static void
vert_attrib_to_tgsi_semantic(gl_vert_attrib slot, unsigned *name, unsigned *index)
{
   assert(name && index);

   if (slot >= VERT_ATTRIB_MAX) {
      ERROR("invalid varying slot %u\n", slot);
      assert(false);
      return;
   }

   if (slot >= VERT_ATTRIB_GENERIC0 &&
       slot < VERT_ATTRIB_GENERIC0 + VERT_ATTRIB_GENERIC_MAX) {
      *name = TGSI_SEMANTIC_GENERIC;
      *index = slot - VERT_ATTRIB_GENERIC0;
      return;
   }

   if (slot >= VERT_ATTRIB_TEX0 &&
       slot < VERT_ATTRIB_TEX0 + VERT_ATTRIB_TEX_MAX) {
      *name = TGSI_SEMANTIC_TEXCOORD;
      *index = slot - VERT_ATTRIB_TEX0;
      return;
   }

   switch (slot) {
   case VERT_ATTRIB_COLOR0:
      *name = TGSI_SEMANTIC_COLOR;
      *index = 0;
      break;
   case VERT_ATTRIB_COLOR1:
      *name = TGSI_SEMANTIC_COLOR;
      *index = 1;
      break;
   case VERT_ATTRIB_EDGEFLAG:
      *name = TGSI_SEMANTIC_EDGEFLAG;
      *index = 0;
      break;
   case VERT_ATTRIB_FOG:
      *name = TGSI_SEMANTIC_FOG;
      *index = 0;
      break;
   case VERT_ATTRIB_NORMAL:
      *name = TGSI_SEMANTIC_NORMAL;
      *index = 0;
      break;
   case VERT_ATTRIB_POS:
      *name = TGSI_SEMANTIC_POSITION;
      *index = 0;
      break;
   case VERT_ATTRIB_POINT_SIZE:
      *name = TGSI_SEMANTIC_PSIZE;
      *index = 0;
      break;
   default:
      ERROR("unknown vert attrib slot %u\n", slot);
      assert(false);
      break;
   }
}

void
Converter::setInterpolate(nv50_ir_varying *var,
                          uint8_t mode,
                          bool centroid,
                          unsigned semantic)
{
   switch (mode) {
   case INTERP_MODE_FLAT:
      var->flat = 1;
      break;
   case INTERP_MODE_NONE:
      if (semantic == TGSI_SEMANTIC_COLOR)
         var->sc = 1;
      else if (semantic == TGSI_SEMANTIC_POSITION)
         var->linear = 1;
      break;
   case INTERP_MODE_NOPERSPECTIVE:
      var->linear = 1;
      break;
   case INTERP_MODE_SMOOTH:
      break;
   }
   var->centroid = centroid;
}

static uint16_t
calcSlots(const glsl_type *type, Program::Type stage, const shader_info &info,
          bool input, const nir_variable *var)
{
   if (!type->is_array())
      return type->count_attribute_slots(false);

   uint16_t slots;
   switch (stage) {
   case Program::TYPE_GEOMETRY:
      slots = type->count_attribute_slots(false);
      if (input)
         slots /= info.gs.vertices_in;
      break;
   case Program::TYPE_TESSELLATION_CONTROL:
   case Program::TYPE_TESSELLATION_EVAL:
      // remove first dimension
      if (var->data.patch || (!input && stage == Program::TYPE_TESSELLATION_EVAL))
         slots = type->count_attribute_slots(false);
      else
         slots = type->fields.array->count_attribute_slots(false);
      break;
   default:
      slots = type->count_attribute_slots(false);
      break;
   }

   return slots;
}

static uint8_t
getMaskForType(const glsl_type *type, uint8_t slot) {
   uint16_t comp = type->without_array()->components();
   comp = comp ? comp : 4;

   if (glsl_base_type_is_64bit(type->without_array()->base_type)) {
      comp *= 2;
      if (comp > 4) {
         if (slot % 2)
            comp -= 4;
         else
            comp = 4;
      }
   }

   return (1 << comp) - 1;
}

bool Converter::assignSlots() {
   unsigned name;
   unsigned index;

   info->io.viewportId = -1;
   info_out->numInputs = 0;
   info_out->numOutputs = 0;
   info_out->numSysVals = 0;

   uint8_t i;
   BITSET_FOREACH_SET(i, nir->info.system_values_read, SYSTEM_VALUE_MAX) {
      info_out->sv[info_out->numSysVals].sn = tgsi_get_sysval_semantic(i);
      info_out->sv[info_out->numSysVals].si = 0;
      info_out->sv[info_out->numSysVals].input = 0; // TODO inferSysValDirection(sn);

      switch (i) {
      case SYSTEM_VALUE_INSTANCE_ID:
         info_out->io.instanceId = info_out->numSysVals;
         break;
      case SYSTEM_VALUE_TESS_LEVEL_INNER:
      case SYSTEM_VALUE_TESS_LEVEL_OUTER:
         info_out->sv[info_out->numSysVals].patch = 1;
         break;
      case SYSTEM_VALUE_VERTEX_ID:
         info_out->io.vertexId = info_out->numSysVals;
         break;
      default:
         break;
      }

      info_out->numSysVals += 1;
   }

   if (prog->getType() == Program::TYPE_COMPUTE)
      return true;

   nir_foreach_shader_in_variable(var, nir) {
      const glsl_type *type = var->type;
      int slot = var->data.location;
      uint16_t slots = calcSlots(type, prog->getType(), nir->info, true, var);
      uint32_t vary = var->data.driver_location;

      assert(vary + slots <= PIPE_MAX_SHADER_INPUTS);

      switch(prog->getType()) {
      case Program::TYPE_FRAGMENT:
         tgsi_get_gl_varying_semantic((gl_varying_slot)slot, true,
                                      &name, &index);
         for (uint16_t i = 0; i < slots; ++i) {
            setInterpolate(&info_out->in[vary + i], var->data.interpolation,
                           var->data.centroid | var->data.sample, name);
         }
         break;
      case Program::TYPE_GEOMETRY:
         tgsi_get_gl_varying_semantic((gl_varying_slot)slot, true,
                                      &name, &index);
         break;
      case Program::TYPE_TESSELLATION_CONTROL:
      case Program::TYPE_TESSELLATION_EVAL:
         tgsi_get_gl_varying_semantic((gl_varying_slot)slot, true,
                                      &name, &index);
         if (var->data.patch && name == TGSI_SEMANTIC_PATCH)
            info_out->numPatchConstants = MAX2(info_out->numPatchConstants, index + slots);
         break;
      case Program::TYPE_VERTEX:
         if (slot >= VERT_ATTRIB_GENERIC0)
            slot = VERT_ATTRIB_GENERIC0 + vary;
         vert_attrib_to_tgsi_semantic((gl_vert_attrib)slot, &name, &index);
         switch (name) {
         case TGSI_SEMANTIC_EDGEFLAG:
            info_out->io.edgeFlagIn = vary;
            break;
         default:
            break;
         }
         break;
      default:
         ERROR("unknown shader type %u in assignSlots\n", prog->getType());
         return false;
      }

      for (uint16_t i = 0u; i < slots; ++i, ++vary) {
         nv50_ir_varying *v = &info_out->in[vary];

         v->patch = var->data.patch;
         v->sn = name;
         v->si = index + i;
         v->mask |= getMaskForType(type, i) << var->data.location_frac;
      }
      info_out->numInputs = std::max<uint8_t>(info_out->numInputs, vary);
   }

   nir_foreach_shader_out_variable(var, nir) {
      const glsl_type *type = var->type;
      int slot = var->data.location;
      uint16_t slots = calcSlots(type, prog->getType(), nir->info, false, var);
      uint32_t vary = var->data.driver_location;

      assert(vary < PIPE_MAX_SHADER_OUTPUTS);

      switch(prog->getType()) {
      case Program::TYPE_FRAGMENT:
         tgsi_get_gl_frag_result_semantic((gl_frag_result)slot, &name, &index);
         switch (name) {
         case TGSI_SEMANTIC_COLOR:
            if (!var->data.fb_fetch_output)
               info_out->prop.fp.numColourResults++;
            if (var->data.location == FRAG_RESULT_COLOR &&
                nir->info.outputs_written & BITFIELD64_BIT(var->data.location))
            info_out->prop.fp.separateFragData = true;
            // sometimes we get FRAG_RESULT_DATAX with data.index 0
            // sometimes we get FRAG_RESULT_DATA0 with data.index X
            index = index == 0 ? var->data.index : index;
            break;
         case TGSI_SEMANTIC_POSITION:
            info_out->io.fragDepth = vary;
            info_out->prop.fp.writesDepth = true;
            break;
         case TGSI_SEMANTIC_SAMPLEMASK:
            info_out->io.sampleMask = vary;
            break;
         default:
            break;
         }
         break;
      case Program::TYPE_GEOMETRY:
      case Program::TYPE_TESSELLATION_CONTROL:
      case Program::TYPE_TESSELLATION_EVAL:
      case Program::TYPE_VERTEX:
         tgsi_get_gl_varying_semantic((gl_varying_slot)slot, true,
                                      &name, &index);

         if (var->data.patch && name != TGSI_SEMANTIC_TESSINNER &&
             name != TGSI_SEMANTIC_TESSOUTER)
            info_out->numPatchConstants = MAX2(info_out->numPatchConstants, index + slots);

         switch (name) {
         case TGSI_SEMANTIC_CLIPDIST:
            info_out->io.genUserClip = -1;
            break;
         case TGSI_SEMANTIC_CLIPVERTEX:
            clipVertexOutput = vary;
            break;
         case TGSI_SEMANTIC_EDGEFLAG:
            info_out->io.edgeFlagOut = vary;
            break;
         case TGSI_SEMANTIC_POSITION:
            if (clipVertexOutput < 0)
               clipVertexOutput = vary;
            break;
         default:
            break;
         }
         break;
      default:
         ERROR("unknown shader type %u in assignSlots\n", prog->getType());
         return false;
      }

      for (uint16_t i = 0u; i < slots; ++i, ++vary) {
         nv50_ir_varying *v = &info_out->out[vary];
         v->patch = var->data.patch;
         v->sn = name;
         v->si = index + i;
         v->mask |= getMaskForType(type, i) << var->data.location_frac;

         if (nir->info.outputs_read & 1ull << slot)
            v->oread = 1;
      }
      info_out->numOutputs = std::max<uint8_t>(info_out->numOutputs, vary);
   }

   if (info_out->io.genUserClip > 0) {
      info_out->io.clipDistances = info_out->io.genUserClip;

      const unsigned int nOut = (info_out->io.genUserClip + 3) / 4;

      for (unsigned int n = 0; n < nOut; ++n) {
         unsigned int i = info_out->numOutputs++;
         info_out->out[i].id = i;
         info_out->out[i].sn = TGSI_SEMANTIC_CLIPDIST;
         info_out->out[i].si = n;
         info_out->out[i].mask = ((1 << info_out->io.clipDistances) - 1) >> (n * 4);
      }
   }

   return info->assignSlots(info_out) == 0;
}

uint32_t
Converter::getSlotAddress(nir_intrinsic_instr *insn, uint8_t idx, uint8_t slot)
{
   DataType ty;
   int offset = nir_intrinsic_component(insn);
   bool input;

   if (nir_intrinsic_infos[insn->intrinsic].has_dest)
      ty = getDType(insn);
   else
      ty = getSType(insn->src[0], false, false);

   switch (insn->intrinsic) {
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_per_vertex_input:
      input = true;
      break;
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      input = false;
      break;
   default:
      ERROR("unknown intrinsic in getSlotAddress %s",
            nir_intrinsic_infos[insn->intrinsic].name);
      input = false;
      assert(false);
      break;
   }

   if (typeSizeof(ty) == 8) {
      slot *= 2;
      slot += offset;
      if (slot >= 4) {
         idx += 1;
         slot -= 4;
      }
   } else {
      slot += offset;
   }

   assert(slot < 4);
   assert(!input || idx < PIPE_MAX_SHADER_INPUTS);
   assert(input || idx < PIPE_MAX_SHADER_OUTPUTS);

   const nv50_ir_varying *vary = input ? info_out->in : info_out->out;
   return vary[idx].slot[slot] * 4;
}

Instruction *
Converter::loadFrom(DataFile file, uint8_t i, DataType ty, Value *def,
                    uint32_t base, uint8_t c, Value *indirect0,
                    Value *indirect1, bool patch)
{
   unsigned int tySize = typeSizeof(ty);

   if (tySize == 8 &&
       (file == FILE_MEMORY_CONST || file == FILE_MEMORY_BUFFER || indirect0)) {
      Value *lo = getSSA();
      Value *hi = getSSA();

      Instruction *loi =
         mkLoad(TYPE_U32, lo,
                mkSymbol(file, i, TYPE_U32, base + c * tySize),
                indirect0);
      loi->setIndirect(0, 1, indirect1);
      loi->perPatch = patch;

      Instruction *hii =
         mkLoad(TYPE_U32, hi,
                mkSymbol(file, i, TYPE_U32, base + c * tySize + 4),
                indirect0);
      hii->setIndirect(0, 1, indirect1);
      hii->perPatch = patch;

      return mkOp2(OP_MERGE, ty, def, lo, hi);
   } else {
      Instruction *ld =
         mkLoad(ty, def, mkSymbol(file, i, ty, base + c * tySize), indirect0);
      ld->setIndirect(0, 1, indirect1);
      ld->perPatch = patch;
      return ld;
   }
}

void
Converter::storeTo(nir_intrinsic_instr *insn, DataFile file, operation op,
                   DataType ty, Value *src, uint8_t idx, uint8_t c,
                   Value *indirect0, Value *indirect1)
{
   uint8_t size = typeSizeof(ty);
   uint32_t address = getSlotAddress(insn, idx, c);

   if (size == 8 && indirect0) {
      Value *split[2];
      mkSplit(split, 4, src);

      if (op == OP_EXPORT) {
         split[0] = mkMov(getSSA(), split[0], ty)->getDef(0);
         split[1] = mkMov(getSSA(), split[1], ty)->getDef(0);
      }

      mkStore(op, TYPE_U32, mkSymbol(file, 0, TYPE_U32, address), indirect0,
              split[0])->perPatch = info_out->out[idx].patch;
      mkStore(op, TYPE_U32, mkSymbol(file, 0, TYPE_U32, address + 4), indirect0,
              split[1])->perPatch = info_out->out[idx].patch;
   } else {
      if (op == OP_EXPORT)
         src = mkMov(getSSA(size), src, ty)->getDef(0);
      mkStore(op, ty, mkSymbol(file, 0, ty, address), indirect0,
              src)->perPatch = info_out->out[idx].patch;
   }
}

bool
Converter::parseNIR()
{
   info_out->bin.tlsSpace = nir->scratch_size;
   info_out->io.clipDistances = nir->info.clip_distance_array_size;
   info_out->io.cullDistances = nir->info.cull_distance_array_size;
   info_out->io.layer_viewport_relative = nir->info.layer_viewport_relative;

   switch(prog->getType()) {
   case Program::TYPE_COMPUTE:
      info->prop.cp.numThreads[0] = nir->info.workgroup_size[0];
      info->prop.cp.numThreads[1] = nir->info.workgroup_size[1];
      info->prop.cp.numThreads[2] = nir->info.workgroup_size[2];
      info_out->bin.smemSize = std::max(info_out->bin.smemSize, nir->info.shared_size);
      break;
   case Program::TYPE_FRAGMENT:
      info_out->prop.fp.earlyFragTests = nir->info.fs.early_fragment_tests;
      prog->persampleInvocation =
         BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID) ||
         BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_POS);
      info_out->prop.fp.postDepthCoverage = nir->info.fs.post_depth_coverage;
      info_out->prop.fp.readsSampleLocations =
         BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_POS);
      info_out->prop.fp.usesDiscard = nir->info.fs.uses_discard || nir->info.fs.uses_demote;
      info_out->prop.fp.usesSampleMaskIn =
         !BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_MASK_IN);
      break;
   case Program::TYPE_GEOMETRY:
      info_out->prop.gp.instanceCount = nir->info.gs.invocations;
      info_out->prop.gp.maxVertices = nir->info.gs.vertices_out;
      info_out->prop.gp.outputPrim = nir->info.gs.output_primitive;
      break;
   case Program::TYPE_TESSELLATION_CONTROL:
   case Program::TYPE_TESSELLATION_EVAL:
      if (nir->info.tess.primitive_mode == GL_ISOLINES)
         info_out->prop.tp.domain = GL_LINES;
      else
         info_out->prop.tp.domain = nir->info.tess.primitive_mode;
      info_out->prop.tp.outputPatchSize = nir->info.tess.tcs_vertices_out;
      info_out->prop.tp.outputPrim =
         nir->info.tess.point_mode ? PIPE_PRIM_POINTS : PIPE_PRIM_TRIANGLES;
      info_out->prop.tp.partitioning = (nir->info.tess.spacing + 1) % 3;
      info_out->prop.tp.winding = !nir->info.tess.ccw;
      break;
   case Program::TYPE_VERTEX:
      info_out->prop.vp.usesDrawParameters =
         BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BASE_VERTEX) ||
         BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BASE_INSTANCE) ||
         BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_DRAW_ID);
      break;
   default:
      break;
   }

   return true;
}

bool
Converter::visit(nir_function *function)
{
   assert(function->impl);

   // usually the blocks will set everything up, but main is special
   BasicBlock *entry = new BasicBlock(prog->main);
   exit = new BasicBlock(prog->main);
   blocks[nir_start_block(function->impl)->index] = entry;
   prog->main->setEntry(entry);
   prog->main->setExit(exit);

   setPosition(entry, true);

   if (info_out->io.genUserClip > 0) {
      for (int c = 0; c < 4; ++c)
         clipVtx[c] = getScratch();
   }

   switch (prog->getType()) {
   case Program::TYPE_TESSELLATION_CONTROL:
      outBase = mkOp2v(
         OP_SUB, TYPE_U32, getSSA(),
         mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_LANEID, 0)),
         mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_INVOCATION_ID, 0)));
      break;
   case Program::TYPE_FRAGMENT: {
      Symbol *sv = mkSysVal(SV_POSITION, 3);
      fragCoord[3] = mkOp1v(OP_RDSV, TYPE_F32, getSSA(), sv);
      fp.position = mkOp1v(OP_RCP, TYPE_F32, fragCoord[3], fragCoord[3]);
      break;
   }
   default:
      break;
   }

   nir_index_ssa_defs(function->impl);
   foreach_list_typed(nir_cf_node, node, node, &function->impl->body) {
      if (!visit(node))
         return false;
   }

   bb->cfg.attach(&exit->cfg, Graph::Edge::TREE);
   setPosition(exit, true);

   if ((prog->getType() == Program::TYPE_VERTEX ||
        prog->getType() == Program::TYPE_TESSELLATION_EVAL)
       && info_out->io.genUserClip > 0)
      handleUserClipPlanes();

   // TODO: for non main function this needs to be a OP_RETURN
   mkOp(OP_EXIT, TYPE_NONE, NULL)->terminator = 1;
   return true;
}

bool
Converter::visit(nir_cf_node *node)
{
   switch (node->type) {
   case nir_cf_node_block:
      return visit(nir_cf_node_as_block(node));
   case nir_cf_node_if:
      return visit(nir_cf_node_as_if(node));
   case nir_cf_node_loop:
      return visit(nir_cf_node_as_loop(node));
   default:
      ERROR("unknown nir_cf_node type %u\n", node->type);
      return false;
   }
}

bool
Converter::visit(nir_block *block)
{
   if (!block->predecessors->entries && block->instr_list.is_empty())
      return true;

   BasicBlock *bb = convert(block);

   setPosition(bb, true);
   nir_foreach_instr(insn, block) {
      if (!visit(insn))
         return false;
   }
   return true;
}

bool
Converter::visit(nir_if *nif)
{
   curIfDepth++;

   DataType sType = getSType(nif->condition, false, false);
   Value *src = getSrc(&nif->condition, 0);

   nir_block *lastThen = nir_if_last_then_block(nif);
   nir_block *lastElse = nir_if_last_else_block(nif);

   BasicBlock *headBB = bb;
   BasicBlock *ifBB = convert(nir_if_first_then_block(nif));
   BasicBlock *elseBB = convert(nir_if_first_else_block(nif));

   bb->cfg.attach(&ifBB->cfg, Graph::Edge::TREE);
   bb->cfg.attach(&elseBB->cfg, Graph::Edge::TREE);

   bool insertJoins = lastThen->successors[0] == lastElse->successors[0];
   mkFlow(OP_BRA, elseBB, CC_EQ, src)->setType(sType);

   foreach_list_typed(nir_cf_node, node, node, &nif->then_list) {
      if (!visit(node))
         return false;
   }

   setPosition(convert(lastThen), true);
   if (!bb->isTerminated()) {
      BasicBlock *tailBB = convert(lastThen->successors[0]);
      mkFlow(OP_BRA, tailBB, CC_ALWAYS, NULL);
      bb->cfg.attach(&tailBB->cfg, Graph::Edge::FORWARD);
   } else {
      insertJoins = insertJoins && bb->getExit()->op == OP_BRA;
   }

   foreach_list_typed(nir_cf_node, node, node, &nif->else_list) {
      if (!visit(node))
         return false;
   }

   setPosition(convert(lastElse), true);
   if (!bb->isTerminated()) {
      BasicBlock *tailBB = convert(lastElse->successors[0]);
      mkFlow(OP_BRA, tailBB, CC_ALWAYS, NULL);
      bb->cfg.attach(&tailBB->cfg, Graph::Edge::FORWARD);
   } else {
      insertJoins = insertJoins && bb->getExit()->op == OP_BRA;
   }

   /* only insert joins for the most outer if */
   if (--curIfDepth)
      insertJoins = false;

   /* we made sure that all threads would converge at the same block */
   if (insertJoins) {
      BasicBlock *conv = convert(lastThen->successors[0]);
      setPosition(headBB->getExit(), false);
      headBB->joinAt = mkFlow(OP_JOINAT, conv, CC_ALWAYS, NULL);
      setPosition(conv, false);
      mkFlow(OP_JOIN, NULL, CC_ALWAYS, NULL)->fixed = 1;
   }

   return true;
}

// TODO: add convergency
bool
Converter::visit(nir_loop *loop)
{
   curLoopDepth += 1;
   func->loopNestingBound = std::max(func->loopNestingBound, curLoopDepth);

   BasicBlock *loopBB = convert(nir_loop_first_block(loop));
   BasicBlock *tailBB = convert(nir_cf_node_as_block(nir_cf_node_next(&loop->cf_node)));

   bb->cfg.attach(&loopBB->cfg, Graph::Edge::TREE);

   mkFlow(OP_PREBREAK, tailBB, CC_ALWAYS, NULL);
   setPosition(loopBB, false);
   mkFlow(OP_PRECONT, loopBB, CC_ALWAYS, NULL);

   foreach_list_typed(nir_cf_node, node, node, &loop->body) {
      if (!visit(node))
         return false;
   }

   if (!bb->isTerminated()) {
      mkFlow(OP_CONT, loopBB, CC_ALWAYS, NULL);
      bb->cfg.attach(&loopBB->cfg, Graph::Edge::BACK);
   }

   if (tailBB->cfg.incidentCount() == 0)
      loopBB->cfg.attach(&tailBB->cfg, Graph::Edge::TREE);

   curLoopDepth -= 1;

   return true;
}

bool
Converter::visit(nir_instr *insn)
{
   // we need an insertion point for on the fly generated immediate loads
   immInsertPos = bb->getExit();
   switch (insn->type) {
   case nir_instr_type_alu:
      return visit(nir_instr_as_alu(insn));
   case nir_instr_type_intrinsic:
      return visit(nir_instr_as_intrinsic(insn));
   case nir_instr_type_jump:
      return visit(nir_instr_as_jump(insn));
   case nir_instr_type_load_const:
      return visit(nir_instr_as_load_const(insn));
   case nir_instr_type_ssa_undef:
      return visit(nir_instr_as_ssa_undef(insn));
   case nir_instr_type_tex:
      return visit(nir_instr_as_tex(insn));
   default:
      ERROR("unknown nir_instr type %u\n", insn->type);
      return false;
   }
   return true;
}

SVSemantic
Converter::convert(nir_intrinsic_op intr)
{
   switch (intr) {
   case nir_intrinsic_load_base_vertex:
      return SV_BASEVERTEX;
   case nir_intrinsic_load_base_instance:
      return SV_BASEINSTANCE;
   case nir_intrinsic_load_draw_id:
      return SV_DRAWID;
   case nir_intrinsic_load_front_face:
      return SV_FACE;
   case nir_intrinsic_is_helper_invocation:
   case nir_intrinsic_load_helper_invocation:
      return SV_THREAD_KILL;
   case nir_intrinsic_load_instance_id:
      return SV_INSTANCE_ID;
   case nir_intrinsic_load_invocation_id:
      return SV_INVOCATION_ID;
   case nir_intrinsic_load_workgroup_size:
      return SV_NTID;
   case nir_intrinsic_load_local_invocation_id:
      return SV_TID;
   case nir_intrinsic_load_num_workgroups:
      return SV_NCTAID;
   case nir_intrinsic_load_patch_vertices_in:
      return SV_VERTEX_COUNT;
   case nir_intrinsic_load_primitive_id:
      return SV_PRIMITIVE_ID;
   case nir_intrinsic_load_sample_id:
      return SV_SAMPLE_INDEX;
   case nir_intrinsic_load_sample_mask_in:
      return SV_SAMPLE_MASK;
   case nir_intrinsic_load_sample_pos:
      return SV_SAMPLE_POS;
   case nir_intrinsic_load_subgroup_eq_mask:
      return SV_LANEMASK_EQ;
   case nir_intrinsic_load_subgroup_ge_mask:
      return SV_LANEMASK_GE;
   case nir_intrinsic_load_subgroup_gt_mask:
      return SV_LANEMASK_GT;
   case nir_intrinsic_load_subgroup_le_mask:
      return SV_LANEMASK_LE;
   case nir_intrinsic_load_subgroup_lt_mask:
      return SV_LANEMASK_LT;
   case nir_intrinsic_load_subgroup_invocation:
      return SV_LANEID;
   case nir_intrinsic_load_tess_coord:
      return SV_TESS_COORD;
   case nir_intrinsic_load_tess_level_inner:
      return SV_TESS_INNER;
   case nir_intrinsic_load_tess_level_outer:
      return SV_TESS_OUTER;
   case nir_intrinsic_load_vertex_id:
      return SV_VERTEX_ID;
   case nir_intrinsic_load_workgroup_id:
      return SV_CTAID;
   case nir_intrinsic_load_work_dim:
      return SV_WORK_DIM;
   default:
      ERROR("unknown SVSemantic for nir_intrinsic_op %s\n",
            nir_intrinsic_infos[intr].name);
      assert(false);
      return SV_LAST;
   }
}

bool
Converter::visit(nir_intrinsic_instr *insn)
{
   nir_intrinsic_op op = insn->intrinsic;
   const nir_intrinsic_info &opInfo = nir_intrinsic_infos[op];
   unsigned dest_components = nir_intrinsic_dest_components(insn);

   switch (op) {
   case nir_intrinsic_load_uniform: {
      LValues &newDefs = convert(&insn->dest);
      const DataType dType = getDType(insn);
      Value *indirect;
      uint32_t coffset = getIndirect(insn, 0, 0, indirect);
      for (uint8_t i = 0; i < dest_components; ++i) {
         loadFrom(FILE_MEMORY_CONST, 0, dType, newDefs[i], 16 * coffset, i, indirect);
      }
      break;
   }
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output: {
      Value *indirect;
      DataType dType = getSType(insn->src[0], false, false);
      uint32_t idx = getIndirect(insn, op == nir_intrinsic_store_output ? 1 : 2, 0, indirect);

      for (uint8_t i = 0u; i < nir_intrinsic_src_components(insn, 0); ++i) {
         if (!((1u << i) & nir_intrinsic_write_mask(insn)))
            continue;

         uint8_t offset = 0;
         Value *src = getSrc(&insn->src[0], i);
         switch (prog->getType()) {
         case Program::TYPE_FRAGMENT: {
            if (info_out->out[idx].sn == TGSI_SEMANTIC_POSITION) {
               // TGSI uses a different interface than NIR, TGSI stores that
               // value in the z component, NIR in X
               offset += 2;
               src = mkOp1v(OP_SAT, TYPE_F32, getScratch(), src);
            }
            break;
         }
         case Program::TYPE_GEOMETRY:
         case Program::TYPE_TESSELLATION_EVAL:
         case Program::TYPE_VERTEX: {
            if (info_out->io.genUserClip > 0 && idx == (uint32_t)clipVertexOutput) {
               mkMov(clipVtx[i], src);
               src = clipVtx[i];
            }
            break;
         }
         default:
            break;
         }

         storeTo(insn, FILE_SHADER_OUTPUT, OP_EXPORT, dType, src, idx, i + offset, indirect);
      }
      break;
   }
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_output: {
      LValues &newDefs = convert(&insn->dest);

      // FBFetch
      if (prog->getType() == Program::TYPE_FRAGMENT &&
          op == nir_intrinsic_load_output) {
         std::vector<Value*> defs, srcs;
         uint8_t mask = 0;

         srcs.push_back(getSSA());
         srcs.push_back(getSSA());
         Value *x = mkOp1v(OP_RDSV, TYPE_F32, getSSA(), mkSysVal(SV_POSITION, 0));
         Value *y = mkOp1v(OP_RDSV, TYPE_F32, getSSA(), mkSysVal(SV_POSITION, 1));
         mkCvt(OP_CVT, TYPE_U32, srcs[0], TYPE_F32, x)->rnd = ROUND_Z;
         mkCvt(OP_CVT, TYPE_U32, srcs[1], TYPE_F32, y)->rnd = ROUND_Z;

         srcs.push_back(mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_LAYER, 0)));
         srcs.push_back(mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_SAMPLE_INDEX, 0)));

         for (uint8_t i = 0u; i < dest_components; ++i) {
            defs.push_back(newDefs[i]);
            mask |= 1 << i;
         }

         TexInstruction *texi = mkTex(OP_TXF, TEX_TARGET_2D_MS_ARRAY, 0, 0, defs, srcs);
         texi->tex.levelZero = true;
         texi->tex.mask = mask;
         texi->tex.useOffsets = 0;
         texi->tex.r = 0xffff;
         texi->tex.s = 0xffff;

         info_out->prop.fp.readsFramebuffer = true;
         break;
      }

      const DataType dType = getDType(insn);
      Value *indirect;
      bool input = op != nir_intrinsic_load_output;
      operation nvirOp;
      uint32_t mode = 0;

      uint32_t idx = getIndirect(insn, op == nir_intrinsic_load_interpolated_input ? 1 : 0, 0, indirect);
      nv50_ir_varying& vary = input ? info_out->in[idx] : info_out->out[idx];

      // see load_barycentric_* handling
      if (prog->getType() == Program::TYPE_FRAGMENT) {
         if (op == nir_intrinsic_load_interpolated_input) {
            ImmediateValue immMode;
            if (getSrc(&insn->src[0], 1)->getUniqueInsn()->src(0).getImmediate(immMode))
               mode = immMode.reg.data.u32;
         }
         if (mode == NV50_IR_INTERP_DEFAULT)
            mode |= translateInterpMode(&vary, nvirOp);
         else {
            if (vary.linear) {
               nvirOp = OP_LINTERP;
               mode |= NV50_IR_INTERP_LINEAR;
            } else {
               nvirOp = OP_PINTERP;
               mode |= NV50_IR_INTERP_PERSPECTIVE;
            }
         }
      }

      for (uint8_t i = 0u; i < dest_components; ++i) {
         uint32_t address = getSlotAddress(insn, idx, i);
         Symbol *sym = mkSymbol(input ? FILE_SHADER_INPUT : FILE_SHADER_OUTPUT, 0, dType, address);
         if (prog->getType() == Program::TYPE_FRAGMENT) {
            int s = 1;
            if (typeSizeof(dType) == 8) {
               Value *lo = getSSA();
               Value *hi = getSSA();
               Instruction *interp;

               interp = mkOp1(nvirOp, TYPE_U32, lo, sym);
               if (nvirOp == OP_PINTERP)
                  interp->setSrc(s++, fp.position);
               if (mode & NV50_IR_INTERP_OFFSET)
                  interp->setSrc(s++, getSrc(&insn->src[0], 0));
               interp->setInterpolate(mode);
               interp->setIndirect(0, 0, indirect);

               Symbol *sym1 = mkSymbol(input ? FILE_SHADER_INPUT : FILE_SHADER_OUTPUT, 0, dType, address + 4);
               interp = mkOp1(nvirOp, TYPE_U32, hi, sym1);
               if (nvirOp == OP_PINTERP)
                  interp->setSrc(s++, fp.position);
               if (mode & NV50_IR_INTERP_OFFSET)
                  interp->setSrc(s++, getSrc(&insn->src[0], 0));
               interp->setInterpolate(mode);
               interp->setIndirect(0, 0, indirect);

               mkOp2(OP_MERGE, dType, newDefs[i], lo, hi);
            } else {
               Instruction *interp = mkOp1(nvirOp, dType, newDefs[i], sym);
               if (nvirOp == OP_PINTERP)
                  interp->setSrc(s++, fp.position);
               if (mode & NV50_IR_INTERP_OFFSET)
                  interp->setSrc(s++, getSrc(&insn->src[0], 0));
               interp->setInterpolate(mode);
               interp->setIndirect(0, 0, indirect);
            }
         } else {
            mkLoad(dType, newDefs[i], sym, indirect)->perPatch = vary.patch;
         }
      }
      break;
   }
   case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_sample: {
      LValues &newDefs = convert(&insn->dest);
      uint32_t mode;

      if (op == nir_intrinsic_load_barycentric_centroid ||
          op == nir_intrinsic_load_barycentric_sample) {
         mode = NV50_IR_INTERP_CENTROID;
      } else if (op == nir_intrinsic_load_barycentric_at_offset) {
         Value *offs[2];
         for (uint8_t c = 0; c < 2; c++) {
            offs[c] = getScratch();
            mkOp2(OP_MIN, TYPE_F32, offs[c], getSrc(&insn->src[0], c), loadImm(NULL, 0.4375f));
            mkOp2(OP_MAX, TYPE_F32, offs[c], offs[c], loadImm(NULL, -0.5f));
            mkOp2(OP_MUL, TYPE_F32, offs[c], offs[c], loadImm(NULL, 4096.0f));
            mkCvt(OP_CVT, TYPE_S32, offs[c], TYPE_F32, offs[c]);
         }
         mkOp3v(OP_INSBF, TYPE_U32, newDefs[0], offs[1], mkImm(0x1010), offs[0]);

         mode = NV50_IR_INTERP_OFFSET;
      } else if (op == nir_intrinsic_load_barycentric_pixel) {
         mode = NV50_IR_INTERP_DEFAULT;
      } else if (op == nir_intrinsic_load_barycentric_at_sample) {
         info_out->prop.fp.readsSampleLocations = true;
         Value *sample = getSSA();
         mkOp3(OP_SELP, TYPE_U32, sample, mkImm(0), getSrc(&insn->src[0], 0), mkImm(0))
            ->subOp = 2;
         mkOp1(OP_PIXLD, TYPE_U32, newDefs[0], sample)->subOp = NV50_IR_SUBOP_PIXLD_OFFSET;
         mode = NV50_IR_INTERP_OFFSET;
      } else {
         unreachable("all intrinsics already handled above");
      }

      loadImm(newDefs[1], mode);
      break;
   }
   case nir_intrinsic_demote:
   case nir_intrinsic_discard:
      mkOp(OP_DISCARD, TYPE_NONE, NULL);
      break;
   case nir_intrinsic_demote_if:
   case nir_intrinsic_discard_if: {
      Value *pred = getSSA(1, FILE_PREDICATE);
      if (insn->num_components > 1) {
         ERROR("nir_intrinsic_discard_if only with 1 component supported!\n");
         assert(false);
         return false;
      }
      mkCmp(OP_SET, CC_NE, TYPE_U8, pred, TYPE_U32, getSrc(&insn->src[0], 0), zero);
      mkOp(OP_DISCARD, TYPE_NONE, NULL)->setPredicate(CC_P, pred);
      break;
   }
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_front_face:
   case nir_intrinsic_is_helper_invocation:
   case nir_intrinsic_load_helper_invocation:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_invocation_id:
   case nir_intrinsic_load_workgroup_size:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_patch_vertices_in:
   case nir_intrinsic_load_primitive_id:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_mask_in:
   case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_subgroup_eq_mask:
   case nir_intrinsic_load_subgroup_ge_mask:
   case nir_intrinsic_load_subgroup_gt_mask:
   case nir_intrinsic_load_subgroup_le_mask:
   case nir_intrinsic_load_subgroup_lt_mask:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_load_tess_coord:
   case nir_intrinsic_load_tess_level_inner:
   case nir_intrinsic_load_tess_level_outer:
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_work_dim: {
      const DataType dType = getDType(insn);
      SVSemantic sv = convert(op);
      LValues &newDefs = convert(&insn->dest);

      for (uint8_t i = 0u; i < nir_intrinsic_dest_components(insn); ++i) {
         Value *def;
         if (typeSizeof(dType) == 8)
            def = getSSA();
         else
            def = newDefs[i];

         if (sv == SV_TID && info->prop.cp.numThreads[i] == 1) {
            loadImm(def, 0u);
         } else {
            Symbol *sym = mkSysVal(sv, i);
            Instruction *rdsv = mkOp1(OP_RDSV, TYPE_U32, def, sym);
            if (sv == SV_TESS_OUTER || sv == SV_TESS_INNER)
               rdsv->perPatch = 1;
         }

         if (typeSizeof(dType) == 8)
            mkOp2(OP_MERGE, dType, newDefs[i], def, loadImm(getSSA(), 0u));
      }
      break;
   }
   // constants
   case nir_intrinsic_load_subgroup_size: {
      LValues &newDefs = convert(&insn->dest);
      loadImm(newDefs[0], 32u);
      break;
   }
   case nir_intrinsic_vote_all:
   case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_ieq: {
      LValues &newDefs = convert(&insn->dest);
      Value *pred = getScratch(1, FILE_PREDICATE);
      mkCmp(OP_SET, CC_NE, TYPE_U32, pred, TYPE_U32, getSrc(&insn->src[0], 0), zero);
      mkOp1(OP_VOTE, TYPE_U32, pred, pred)->subOp = getSubOp(op);
      mkCvt(OP_CVT, TYPE_U32, newDefs[0], TYPE_U8, pred);
      break;
   }
   case nir_intrinsic_ballot: {
      LValues &newDefs = convert(&insn->dest);
      Value *pred = getSSA(1, FILE_PREDICATE);
      mkCmp(OP_SET, CC_NE, TYPE_U32, pred, TYPE_U32, getSrc(&insn->src[0], 0), zero);
      mkOp1(OP_VOTE, TYPE_U32, newDefs[0], pred)->subOp = NV50_IR_SUBOP_VOTE_ANY;
      break;
   }
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_read_invocation: {
      LValues &newDefs = convert(&insn->dest);
      const DataType dType = getDType(insn);
      Value *tmp = getScratch();

      if (op == nir_intrinsic_read_first_invocation) {
         mkOp1(OP_VOTE, TYPE_U32, tmp, mkImm(1))->subOp = NV50_IR_SUBOP_VOTE_ANY;
         mkOp1(OP_BREV, TYPE_U32, tmp, tmp);
         mkOp1(OP_BFIND, TYPE_U32, tmp, tmp)->subOp = NV50_IR_SUBOP_BFIND_SAMT;
      } else
         tmp = getSrc(&insn->src[1], 0);

      for (uint8_t i = 0; i < dest_components; ++i) {
         mkOp3(OP_SHFL, dType, newDefs[i], getSrc(&insn->src[0], i), tmp, mkImm(0x1f))
            ->subOp = NV50_IR_SUBOP_SHFL_IDX;
      }
      break;
   }
   case nir_intrinsic_load_per_vertex_input: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectVertex;
      Value *indirectOffset;
      uint32_t baseVertex = getIndirect(&insn->src[0], 0, indirectVertex);
      uint32_t idx = getIndirect(insn, 1, 0, indirectOffset);

      Value *vtxBase = mkOp2v(OP_PFETCH, TYPE_U32, getSSA(4, FILE_ADDRESS),
                              mkImm(baseVertex), indirectVertex);
      for (uint8_t i = 0u; i < dest_components; ++i) {
         uint32_t address = getSlotAddress(insn, idx, i);
         loadFrom(FILE_SHADER_INPUT, 0, dType, newDefs[i], address, 0,
                  indirectOffset, vtxBase, info_out->in[idx].patch);
      }
      break;
   }
   case nir_intrinsic_load_per_vertex_output: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectVertex;
      Value *indirectOffset;
      uint32_t baseVertex = getIndirect(&insn->src[0], 0, indirectVertex);
      uint32_t idx = getIndirect(insn, 1, 0, indirectOffset);
      Value *vtxBase = NULL;

      if (indirectVertex)
         vtxBase = indirectVertex;
      else
         vtxBase = loadImm(NULL, baseVertex);

      vtxBase = mkOp2v(OP_ADD, TYPE_U32, getSSA(4, FILE_ADDRESS), outBase, vtxBase);

      for (uint8_t i = 0u; i < dest_components; ++i) {
         uint32_t address = getSlotAddress(insn, idx, i);
         loadFrom(FILE_SHADER_OUTPUT, 0, dType, newDefs[i], address, 0,
                  indirectOffset, vtxBase, info_out->in[idx].patch);
      }
      break;
   }
   case nir_intrinsic_emit_vertex: {
      if (info_out->io.genUserClip > 0)
         handleUserClipPlanes();
      uint32_t idx = nir_intrinsic_stream_id(insn);
      mkOp1(getOperation(op), TYPE_U32, NULL, mkImm(idx))->fixed = 1;
      break;
   }
   case nir_intrinsic_end_primitive: {
      uint32_t idx = nir_intrinsic_stream_id(insn);
      if (idx)
         break;
      mkOp1(getOperation(op), TYPE_U32, NULL, mkImm(idx))->fixed = 1;
      break;
   }
   case nir_intrinsic_load_ubo: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectIndex;
      Value *indirectOffset;
      uint32_t index = getIndirect(&insn->src[0], 0, indirectIndex) + 1;
      uint32_t offset = getIndirect(&insn->src[1], 0, indirectOffset);

      for (uint8_t i = 0u; i < dest_components; ++i) {
         loadFrom(FILE_MEMORY_CONST, index, dType, newDefs[i], offset, i,
                  indirectOffset, indirectIndex);
      }
      break;
   }
   case nir_intrinsic_get_ssbo_size: {
      LValues &newDefs = convert(&insn->dest);
      const DataType dType = getDType(insn);
      Value *indirectBuffer;
      uint32_t buffer = getIndirect(&insn->src[0], 0, indirectBuffer);

      Symbol *sym = mkSymbol(FILE_MEMORY_BUFFER, buffer, dType, 0);
      mkOp1(OP_BUFQ, dType, newDefs[0], sym)->setIndirect(0, 0, indirectBuffer);
      break;
   }
   case nir_intrinsic_store_ssbo: {
      DataType sType = getSType(insn->src[0], false, false);
      Value *indirectBuffer;
      Value *indirectOffset;
      uint32_t buffer = getIndirect(&insn->src[1], 0, indirectBuffer);
      uint32_t offset = getIndirect(&insn->src[2], 0, indirectOffset);

      for (uint8_t i = 0u; i < nir_intrinsic_src_components(insn, 0); ++i) {
         if (!((1u << i) & nir_intrinsic_write_mask(insn)))
            continue;
         Symbol *sym = mkSymbol(FILE_MEMORY_BUFFER, buffer, sType,
                                offset + i * typeSizeof(sType));
         mkStore(OP_STORE, sType, sym, indirectOffset, getSrc(&insn->src[0], i))
            ->setIndirect(0, 1, indirectBuffer);
      }
      info_out->io.globalAccess |= 0x2;
      break;
   }
   case nir_intrinsic_load_ssbo: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectBuffer;
      Value *indirectOffset;
      uint32_t buffer = getIndirect(&insn->src[0], 0, indirectBuffer);
      uint32_t offset = getIndirect(&insn->src[1], 0, indirectOffset);

      for (uint8_t i = 0u; i < dest_components; ++i)
         loadFrom(FILE_MEMORY_BUFFER, buffer, dType, newDefs[i], offset, i,
                  indirectOffset, indirectBuffer);

      info_out->io.globalAccess |= 0x1;
      break;
   }
   case nir_intrinsic_shared_atomic_add:
   case nir_intrinsic_shared_atomic_fadd:
   case nir_intrinsic_shared_atomic_and:
   case nir_intrinsic_shared_atomic_comp_swap:
   case nir_intrinsic_shared_atomic_exchange:
   case nir_intrinsic_shared_atomic_or:
   case nir_intrinsic_shared_atomic_imax:
   case nir_intrinsic_shared_atomic_imin:
   case nir_intrinsic_shared_atomic_umax:
   case nir_intrinsic_shared_atomic_umin:
   case nir_intrinsic_shared_atomic_xor: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectOffset;
      uint32_t offset = getIndirect(&insn->src[0], 0, indirectOffset);
      Symbol *sym = mkSymbol(FILE_MEMORY_SHARED, 0, dType, offset);
      Instruction *atom = mkOp2(OP_ATOM, dType, newDefs[0], sym, getSrc(&insn->src[1], 0));
      if (op == nir_intrinsic_shared_atomic_comp_swap)
         atom->setSrc(2, getSrc(&insn->src[2], 0));
      atom->setIndirect(0, 0, indirectOffset);
      atom->subOp = getSubOp(op);
      break;
   }
   case nir_intrinsic_ssbo_atomic_add:
   case nir_intrinsic_ssbo_atomic_fadd:
   case nir_intrinsic_ssbo_atomic_and:
   case nir_intrinsic_ssbo_atomic_comp_swap:
   case nir_intrinsic_ssbo_atomic_exchange:
   case nir_intrinsic_ssbo_atomic_or:
   case nir_intrinsic_ssbo_atomic_imax:
   case nir_intrinsic_ssbo_atomic_imin:
   case nir_intrinsic_ssbo_atomic_umax:
   case nir_intrinsic_ssbo_atomic_umin:
   case nir_intrinsic_ssbo_atomic_xor: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectBuffer;
      Value *indirectOffset;
      uint32_t buffer = getIndirect(&insn->src[0], 0, indirectBuffer);
      uint32_t offset = getIndirect(&insn->src[1], 0, indirectOffset);

      Symbol *sym = mkSymbol(FILE_MEMORY_BUFFER, buffer, dType, offset);
      Instruction *atom = mkOp2(OP_ATOM, dType, newDefs[0], sym,
                                getSrc(&insn->src[2], 0));
      if (op == nir_intrinsic_ssbo_atomic_comp_swap)
         atom->setSrc(2, getSrc(&insn->src[3], 0));
      atom->setIndirect(0, 0, indirectOffset);
      atom->setIndirect(0, 1, indirectBuffer);
      atom->subOp = getSubOp(op);

      info_out->io.globalAccess |= 0x2;
      break;
   }
   case nir_intrinsic_global_atomic_add:
   case nir_intrinsic_global_atomic_fadd:
   case nir_intrinsic_global_atomic_and:
   case nir_intrinsic_global_atomic_comp_swap:
   case nir_intrinsic_global_atomic_exchange:
   case nir_intrinsic_global_atomic_or:
   case nir_intrinsic_global_atomic_imax:
   case nir_intrinsic_global_atomic_imin:
   case nir_intrinsic_global_atomic_umax:
   case nir_intrinsic_global_atomic_umin:
   case nir_intrinsic_global_atomic_xor: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *address;
      uint32_t offset = getIndirect(&insn->src[0], 0, address);

      Symbol *sym = mkSymbol(FILE_MEMORY_GLOBAL, 0, dType, offset);
      Instruction *atom =
         mkOp2(OP_ATOM, dType, newDefs[0], sym, getSrc(&insn->src[1], 0));
      if (op == nir_intrinsic_global_atomic_comp_swap)
         atom->setSrc(2, getSrc(&insn->src[2], 0));
      atom->setIndirect(0, 0, address);
      atom->subOp = getSubOp(op);

      info_out->io.globalAccess |= 0x2;
      break;
   }
   case nir_intrinsic_bindless_image_atomic_add:
   case nir_intrinsic_bindless_image_atomic_fadd:
   case nir_intrinsic_bindless_image_atomic_and:
   case nir_intrinsic_bindless_image_atomic_comp_swap:
   case nir_intrinsic_bindless_image_atomic_exchange:
   case nir_intrinsic_bindless_image_atomic_imax:
   case nir_intrinsic_bindless_image_atomic_umax:
   case nir_intrinsic_bindless_image_atomic_imin:
   case nir_intrinsic_bindless_image_atomic_umin:
   case nir_intrinsic_bindless_image_atomic_or:
   case nir_intrinsic_bindless_image_atomic_xor:
   case nir_intrinsic_bindless_image_atomic_inc_wrap:
   case nir_intrinsic_bindless_image_atomic_dec_wrap:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_image_atomic_add:
   case nir_intrinsic_image_atomic_fadd:
   case nir_intrinsic_image_atomic_and:
   case nir_intrinsic_image_atomic_comp_swap:
   case nir_intrinsic_image_atomic_exchange:
   case nir_intrinsic_image_atomic_imax:
   case nir_intrinsic_image_atomic_umax:
   case nir_intrinsic_image_atomic_imin:
   case nir_intrinsic_image_atomic_umin:
   case nir_intrinsic_image_atomic_or:
   case nir_intrinsic_image_atomic_xor:
   case nir_intrinsic_image_atomic_inc_wrap:
   case nir_intrinsic_image_atomic_dec_wrap:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_samples:
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_store: {
      std::vector<Value*> srcs, defs;
      Value *indirect;
      DataType ty;

      uint32_t mask = 0;
      TexInstruction::Target target =
         convert(nir_intrinsic_image_dim(insn), !!nir_intrinsic_image_array(insn), false);
      unsigned int argCount = getNIRArgCount(target);
      uint16_t location = 0;

      if (opInfo.has_dest) {
         LValues &newDefs = convert(&insn->dest);
         for (uint8_t i = 0u; i < newDefs.size(); ++i) {
            defs.push_back(newDefs[i]);
            mask |= 1 << i;
         }
      }

      int lod_src = -1;
      bool bindless = false;
      switch (op) {
      case nir_intrinsic_bindless_image_atomic_add:
      case nir_intrinsic_bindless_image_atomic_fadd:
      case nir_intrinsic_bindless_image_atomic_and:
      case nir_intrinsic_bindless_image_atomic_comp_swap:
      case nir_intrinsic_bindless_image_atomic_exchange:
      case nir_intrinsic_bindless_image_atomic_imax:
      case nir_intrinsic_bindless_image_atomic_umax:
      case nir_intrinsic_bindless_image_atomic_imin:
      case nir_intrinsic_bindless_image_atomic_umin:
      case nir_intrinsic_bindless_image_atomic_or:
      case nir_intrinsic_bindless_image_atomic_xor:
      case nir_intrinsic_bindless_image_atomic_inc_wrap:
      case nir_intrinsic_bindless_image_atomic_dec_wrap:
         ty = getDType(insn);
         bindless = true;
         info_out->io.globalAccess |= 0x2;
         mask = 0x1;
         break;
      case nir_intrinsic_image_atomic_add:
      case nir_intrinsic_image_atomic_fadd:
      case nir_intrinsic_image_atomic_and:
      case nir_intrinsic_image_atomic_comp_swap:
      case nir_intrinsic_image_atomic_exchange:
      case nir_intrinsic_image_atomic_imax:
      case nir_intrinsic_image_atomic_umax:
      case nir_intrinsic_image_atomic_imin:
      case nir_intrinsic_image_atomic_umin:
      case nir_intrinsic_image_atomic_or:
      case nir_intrinsic_image_atomic_xor:
      case nir_intrinsic_image_atomic_inc_wrap:
      case nir_intrinsic_image_atomic_dec_wrap:
         ty = getDType(insn);
         bindless = false;
         info_out->io.globalAccess |= 0x2;
         mask = 0x1;
         break;
      case nir_intrinsic_bindless_image_load:
      case nir_intrinsic_image_load:
         ty = TYPE_U32;
         bindless = op == nir_intrinsic_bindless_image_load;
         info_out->io.globalAccess |= 0x1;
         lod_src = 4;
         break;
      case nir_intrinsic_bindless_image_store:
      case nir_intrinsic_image_store:
         ty = TYPE_U32;
         bindless = op == nir_intrinsic_bindless_image_store;
         info_out->io.globalAccess |= 0x2;
         lod_src = 5;
         mask = 0xf;
         break;
      case nir_intrinsic_bindless_image_samples:
         mask = 0x8;
         FALLTHROUGH;
      case nir_intrinsic_image_samples:
         ty = TYPE_U32;
         bindless = op == nir_intrinsic_bindless_image_samples;
         mask = 0x8;
         break;
      case nir_intrinsic_bindless_image_size:
      case nir_intrinsic_image_size:
         assert(nir_src_as_uint(insn->src[1]) == 0);
         ty = TYPE_U32;
         bindless = op == nir_intrinsic_bindless_image_size;
         break;
      default:
         unreachable("unhandled image opcode");
         break;
      }

      if (bindless)
         indirect = getSrc(&insn->src[0], 0);
      else
         location = getIndirect(&insn->src[0], 0, indirect);

      // coords
      if (opInfo.num_srcs >= 2)
         for (unsigned int i = 0u; i < argCount; ++i)
            srcs.push_back(getSrc(&insn->src[1], i));

      // the sampler is just another src added after coords
      if (opInfo.num_srcs >= 3 && target.isMS())
         srcs.push_back(getSrc(&insn->src[2], 0));

      if (opInfo.num_srcs >= 4 && lod_src != 4) {
         unsigned components = opInfo.src_components[3] ? opInfo.src_components[3] : insn->num_components;
         for (uint8_t i = 0u; i < components; ++i)
            srcs.push_back(getSrc(&insn->src[3], i));
      }

      if (opInfo.num_srcs >= 5 && lod_src != 5)
         // 1 for aotmic swap
         for (uint8_t i = 0u; i < opInfo.src_components[4]; ++i)
            srcs.push_back(getSrc(&insn->src[4], i));

      TexInstruction *texi = mkTex(getOperation(op), target.getEnum(), location, 0, defs, srcs);
      texi->tex.bindless = bindless;
      texi->tex.format = nv50_ir::TexInstruction::translateImgFormat(nir_intrinsic_format(insn));
      texi->tex.mask = mask;
      texi->cache = convert(nir_intrinsic_access(insn));
      texi->setType(ty);
      texi->subOp = getSubOp(op);

      if (indirect)
         texi->setIndirectR(indirect);

      break;
   }
   case nir_intrinsic_store_scratch:
   case nir_intrinsic_store_shared: {
      DataType sType = getSType(insn->src[0], false, false);
      Value *indirectOffset;
      uint32_t offset = getIndirect(&insn->src[1], 0, indirectOffset);

      for (uint8_t i = 0u; i < nir_intrinsic_src_components(insn, 0); ++i) {
         if (!((1u << i) & nir_intrinsic_write_mask(insn)))
            continue;
         Symbol *sym = mkSymbol(getFile(op), 0, sType, offset + i * typeSizeof(sType));
         mkStore(OP_STORE, sType, sym, indirectOffset, getSrc(&insn->src[0], i));
      }
      break;
   }
   case nir_intrinsic_load_kernel_input:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_shared: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectOffset;
      uint32_t offset = getIndirect(&insn->src[0], 0, indirectOffset);

      for (uint8_t i = 0u; i < dest_components; ++i)
         loadFrom(getFile(op), 0, dType, newDefs[i], offset, i, indirectOffset);

      break;
   }
   case nir_intrinsic_control_barrier: {
      // TODO: add flag to shader_info
      info_out->numBarriers = 1;
      Instruction *bar = mkOp2(OP_BAR, TYPE_U32, NULL, mkImm(0), mkImm(0));
      bar->fixed = 1;
      bar->subOp = NV50_IR_SUBOP_BAR_SYNC;
      break;
   }
   case nir_intrinsic_group_memory_barrier:
   case nir_intrinsic_memory_barrier:
   case nir_intrinsic_memory_barrier_buffer:
   case nir_intrinsic_memory_barrier_image:
   case nir_intrinsic_memory_barrier_shared: {
      Instruction *bar = mkOp(OP_MEMBAR, TYPE_NONE, NULL);
      bar->fixed = 1;
      bar->subOp = getSubOp(op);
      break;
   }
   case nir_intrinsic_memory_barrier_tcs_patch:
      break;
   case nir_intrinsic_shader_clock: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);

      loadImm(newDefs[0], 0u);
      mkOp1(OP_RDSV, dType, newDefs[1], mkSysVal(SV_CLOCK, 0))->fixed = 1;
      break;
   }
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant: {
      const DataType dType = getDType(insn);
      LValues &newDefs = convert(&insn->dest);
      Value *indirectOffset;
      uint32_t offset = getIndirect(&insn->src[0], 0, indirectOffset);

      for (auto i = 0u; i < dest_components; ++i)
         loadFrom(FILE_MEMORY_GLOBAL, 0, dType, newDefs[i], offset, i, indirectOffset);

      info_out->io.globalAccess |= 0x1;
      break;
   }
   case nir_intrinsic_store_global: {
      DataType sType = getSType(insn->src[0], false, false);

      for (auto i = 0u; i < nir_intrinsic_src_components(insn, 0); ++i) {
         if (!((1u << i) & nir_intrinsic_write_mask(insn)))
            continue;
         if (typeSizeof(sType) == 8) {
            Value *split[2];
            mkSplit(split, 4, getSrc(&insn->src[0], i));

            Symbol *sym = mkSymbol(FILE_MEMORY_GLOBAL, 0, TYPE_U32, i * typeSizeof(sType));
            mkStore(OP_STORE, TYPE_U32, sym, getSrc(&insn->src[1], 0), split[0]);

            sym = mkSymbol(FILE_MEMORY_GLOBAL, 0, TYPE_U32, i * typeSizeof(sType) + 4);
            mkStore(OP_STORE, TYPE_U32, sym, getSrc(&insn->src[1], 0), split[1]);
         } else {
            Symbol *sym = mkSymbol(FILE_MEMORY_GLOBAL, 0, sType, i * typeSizeof(sType));
            mkStore(OP_STORE, sType, sym, getSrc(&insn->src[1], 0), getSrc(&insn->src[0], i));
         }
      }

      info_out->io.globalAccess |= 0x2;
      break;
   }
   default:
      ERROR("unknown nir_intrinsic_op %s\n", nir_intrinsic_infos[op].name);
      return false;
   }

   return true;
}

bool
Converter::visit(nir_jump_instr *insn)
{
   switch (insn->type) {
   case nir_jump_return:
      // TODO: this only works in the main function
      mkFlow(OP_BRA, exit, CC_ALWAYS, NULL);
      bb->cfg.attach(&exit->cfg, Graph::Edge::CROSS);
      break;
   case nir_jump_break:
   case nir_jump_continue: {
      bool isBreak = insn->type == nir_jump_break;
      nir_block *block = insn->instr.block;
      BasicBlock *target = convert(block->successors[0]);
      mkFlow(isBreak ? OP_BREAK : OP_CONT, target, CC_ALWAYS, NULL);
      bb->cfg.attach(&target->cfg, isBreak ? Graph::Edge::CROSS : Graph::Edge::BACK);
      break;
   }
   default:
      ERROR("unknown nir_jump_type %u\n", insn->type);
      return false;
   }

   return true;
}

Value*
Converter::convert(nir_load_const_instr *insn, uint8_t idx)
{
   Value *val;

   if (immInsertPos)
      setPosition(immInsertPos, true);
   else
      setPosition(bb, false);

   switch (insn->def.bit_size) {
   case 64:
      val = loadImm(getSSA(8), insn->value[idx].u64);
      break;
   case 32:
      val = loadImm(getSSA(4), insn->value[idx].u32);
      break;
   case 16:
      val = loadImm(getSSA(2), insn->value[idx].u16);
      break;
   case 8:
      val = loadImm(getSSA(1), insn->value[idx].u8);
      break;
   default:
      unreachable("unhandled bit size!\n");
   }
   setPosition(bb, true);
   return val;
}

bool
Converter::visit(nir_load_const_instr *insn)
{
   assert(insn->def.bit_size <= 64);
   immediates[insn->def.index] = insn;
   return true;
}

#define DEFAULT_CHECKS \
      if (insn->dest.dest.ssa.num_components > 1) { \
         ERROR("nir_alu_instr only supported with 1 component!\n"); \
         return false; \
      } \
      if (insn->dest.write_mask != 1) { \
         ERROR("nir_alu_instr only with write_mask of 1 supported!\n"); \
         return false; \
      }
bool
Converter::visit(nir_alu_instr *insn)
{
   const nir_op op = insn->op;
   const nir_op_info &info = nir_op_infos[op];
   DataType dType = getDType(insn);
   const std::vector<DataType> sTypes = getSTypes(insn);

   Instruction *oldPos = this->bb->getExit();

   switch (op) {
   case nir_op_fabs:
   case nir_op_iabs:
   case nir_op_fadd:
   case nir_op_iadd:
   case nir_op_iand:
   case nir_op_fceil:
   case nir_op_fcos:
   case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
   case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
   case nir_op_fdiv:
   case nir_op_idiv:
   case nir_op_udiv:
   case nir_op_fexp2:
   case nir_op_ffloor:
   case nir_op_ffma:
   case nir_op_flog2:
   case nir_op_fmax:
   case nir_op_imax:
   case nir_op_umax:
   case nir_op_fmin:
   case nir_op_imin:
   case nir_op_umin:
   case nir_op_fmod:
   case nir_op_imod:
   case nir_op_umod:
   case nir_op_fmul:
   case nir_op_imul:
   case nir_op_imul_high:
   case nir_op_umul_high:
   case nir_op_fneg:
   case nir_op_ineg:
   case nir_op_inot:
   case nir_op_ior:
   case nir_op_pack_64_2x32_split:
   case nir_op_fpow:
   case nir_op_frcp:
   case nir_op_frem:
   case nir_op_irem:
   case nir_op_frsq:
   case nir_op_fsat:
   case nir_op_ishr:
   case nir_op_ushr:
   case nir_op_fsin:
   case nir_op_fsqrt:
   case nir_op_ftrunc:
   case nir_op_ishl:
   case nir_op_ixor: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      operation preOp = preOperationNeeded(op);
      if (preOp != OP_NOP) {
         assert(info.num_inputs < 2);
         Value *tmp = getSSA(typeSizeof(dType));
         Instruction *i0 = mkOp(preOp, dType, tmp);
         Instruction *i1 = mkOp(getOperation(op), dType, newDefs[0]);
         if (info.num_inputs) {
            i0->setSrc(0, getSrc(&insn->src[0]));
            i1->setSrc(0, tmp);
         }
         i1->subOp = getSubOp(op);
      } else {
         Instruction *i = mkOp(getOperation(op), dType, newDefs[0]);
         for (unsigned s = 0u; s < info.num_inputs; ++s) {
            i->setSrc(s, getSrc(&insn->src[s]));
         }
         i->subOp = getSubOp(op);
      }
      break;
   }
   case nir_op_ifind_msb:
   case nir_op_ufind_msb: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      dType = sTypes[0];
      mkOp1(getOperation(op), dType, newDefs[0], getSrc(&insn->src[0]));
      break;
   }
   case nir_op_fround_even: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkCvt(OP_CVT, dType, newDefs[0], dType, getSrc(&insn->src[0]))->rnd = ROUND_NI;
      break;
   }
   // convert instructions
   case nir_op_f2f32:
   case nir_op_f2i32:
   case nir_op_f2u32:
   case nir_op_i2f32:
   case nir_op_i2i32:
   case nir_op_u2f32:
   case nir_op_u2u32:
   case nir_op_f2f64:
   case nir_op_f2i64:
   case nir_op_f2u64:
   case nir_op_i2f64:
   case nir_op_i2i64:
   case nir_op_u2f64:
   case nir_op_u2u64: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Instruction *i = mkOp1(getOperation(op), dType, newDefs[0], getSrc(&insn->src[0]));
      if (op == nir_op_f2i32 || op == nir_op_f2i64 || op == nir_op_f2u32 || op == nir_op_f2u64)
         i->rnd = ROUND_Z;
      i->sType = sTypes[0];
      break;
   }
   // compare instructions
   case nir_op_feq32:
   case nir_op_ieq32:
   case nir_op_fge32:
   case nir_op_ige32:
   case nir_op_uge32:
   case nir_op_flt32:
   case nir_op_ilt32:
   case nir_op_ult32:
   case nir_op_fneu32:
   case nir_op_ine32: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Instruction *i = mkCmp(getOperation(op),
                             getCondCode(op),
                             dType,
                             newDefs[0],
                             dType,
                             getSrc(&insn->src[0]),
                             getSrc(&insn->src[1]));
      if (info.num_inputs == 3)
         i->setSrc(2, getSrc(&insn->src[2]));
      i->sType = sTypes[0];
      break;
   }
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec8:
   case nir_op_vec16: {
      LValues &newDefs = convert(&insn->dest);
      for (LValues::size_type c = 0u; c < newDefs.size(); ++c) {
         mkMov(newDefs[c], getSrc(&insn->src[c]), dType);
      }
      break;
   }
   // (un)pack
   case nir_op_pack_64_2x32: {
      LValues &newDefs = convert(&insn->dest);
      Instruction *merge = mkOp(OP_MERGE, dType, newDefs[0]);
      merge->setSrc(0, getSrc(&insn->src[0], 0));
      merge->setSrc(1, getSrc(&insn->src[0], 1));
      break;
   }
   case nir_op_pack_half_2x16_split: {
      LValues &newDefs = convert(&insn->dest);
      Value *tmpH = getSSA();
      Value *tmpL = getSSA();

      mkCvt(OP_CVT, TYPE_F16, tmpL, TYPE_F32, getSrc(&insn->src[0]));
      mkCvt(OP_CVT, TYPE_F16, tmpH, TYPE_F32, getSrc(&insn->src[1]));
      mkOp3(OP_INSBF, TYPE_U32, newDefs[0], tmpH, mkImm(0x1010), tmpL);
      break;
   }
   case nir_op_unpack_half_2x16_split_x:
   case nir_op_unpack_half_2x16_split_y: {
      LValues &newDefs = convert(&insn->dest);
      Instruction *cvt = mkCvt(OP_CVT, TYPE_F32, newDefs[0], TYPE_F16, getSrc(&insn->src[0]));
      if (op == nir_op_unpack_half_2x16_split_y)
         cvt->subOp = 1;
      break;
   }
   case nir_op_unpack_64_2x32: {
      LValues &newDefs = convert(&insn->dest);
      mkOp1(OP_SPLIT, dType, newDefs[0], getSrc(&insn->src[0]))->setDef(1, newDefs[1]);
      break;
   }
   case nir_op_unpack_64_2x32_split_x: {
      LValues &newDefs = convert(&insn->dest);
      mkOp1(OP_SPLIT, dType, newDefs[0], getSrc(&insn->src[0]))->setDef(1, getSSA());
      break;
   }
   case nir_op_unpack_64_2x32_split_y: {
      LValues &newDefs = convert(&insn->dest);
      mkOp1(OP_SPLIT, dType, getSSA(), getSrc(&insn->src[0]))->setDef(1, newDefs[0]);
      break;
   }
   // special instructions
   case nir_op_fsign:
   case nir_op_isign: {
      DEFAULT_CHECKS;
      DataType iType;
      if (::isFloatType(dType))
         iType = TYPE_F32;
      else
         iType = TYPE_S32;

      LValues &newDefs = convert(&insn->dest);
      LValue *val0 = getScratch();
      LValue *val1 = getScratch();
      mkCmp(OP_SET, CC_GT, iType, val0, dType, getSrc(&insn->src[0]), zero);
      mkCmp(OP_SET, CC_LT, iType, val1, dType, getSrc(&insn->src[0]), zero);

      if (dType == TYPE_F64) {
         mkOp2(OP_SUB, iType, val0, val0, val1);
         mkCvt(OP_CVT, TYPE_F64, newDefs[0], iType, val0);
      } else if (dType == TYPE_S64 || dType == TYPE_U64) {
         mkOp2(OP_SUB, iType, val0, val1, val0);
         mkOp2(OP_SHR, iType, val1, val0, loadImm(NULL, 31));
         mkOp2(OP_MERGE, dType, newDefs[0], val0, val1);
      } else if (::isFloatType(dType))
         mkOp2(OP_SUB, iType, newDefs[0], val0, val1);
      else
         mkOp2(OP_SUB, iType, newDefs[0], val1, val0);
      break;
   }
   case nir_op_fcsel:
   case nir_op_b32csel: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkCmp(OP_SLCT, CC_NE, dType, newDefs[0], sTypes[0], getSrc(&insn->src[1]), getSrc(&insn->src[2]), getSrc(&insn->src[0]));
      break;
   }
   case nir_op_ibitfield_extract:
   case nir_op_ubitfield_extract: {
      DEFAULT_CHECKS;
      Value *tmp = getSSA();
      LValues &newDefs = convert(&insn->dest);
      mkOp3(OP_INSBF, dType, tmp, getSrc(&insn->src[2]), loadImm(NULL, 0x808), getSrc(&insn->src[1]));
      mkOp2(OP_EXTBF, dType, newDefs[0], getSrc(&insn->src[0]), tmp);
      break;
   }
   case nir_op_bfm: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp2(OP_BMSK, dType, newDefs[0], getSrc(&insn->src[1]), getSrc(&insn->src[0]))->subOp = NV50_IR_SUBOP_BMSK_W;
      break;
   }
   case nir_op_bitfield_insert: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      LValue *temp = getSSA();
      mkOp3(OP_INSBF, TYPE_U32, temp, getSrc(&insn->src[3]), mkImm(0x808), getSrc(&insn->src[2]));
      mkOp3(OP_INSBF, dType, newDefs[0], getSrc(&insn->src[1]), temp, getSrc(&insn->src[0]));
      break;
   }
   case nir_op_bit_count: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp2(OP_POPCNT, dType, newDefs[0], getSrc(&insn->src[0]), getSrc(&insn->src[0]));
      break;
   }
   case nir_op_bitfield_reverse: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp1(OP_BREV, TYPE_U32, newDefs[0], getSrc(&insn->src[0]));
      break;
   }
   case nir_op_find_lsb: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *tmp = getSSA();
      mkOp1(OP_BREV, TYPE_U32, tmp, getSrc(&insn->src[0]));
      mkOp1(OP_BFIND, TYPE_U32, newDefs[0], tmp)->subOp = NV50_IR_SUBOP_BFIND_SAMT;
      break;
   }
   case nir_op_extract_u8: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *prmt = getSSA();
      mkOp2(OP_OR, TYPE_U32, prmt, getSrc(&insn->src[1]), loadImm(NULL, 0x4440));
      mkOp3(OP_PERMT, TYPE_U32, newDefs[0], getSrc(&insn->src[0]), prmt, loadImm(NULL, 0));
      break;
   }
   case nir_op_extract_i8: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *prmt = getSSA();
      mkOp3(OP_MAD, TYPE_U32, prmt, getSrc(&insn->src[1]), loadImm(NULL, 0x1111), loadImm(NULL, 0x8880));
      mkOp3(OP_PERMT, TYPE_U32, newDefs[0], getSrc(&insn->src[0]), prmt, loadImm(NULL, 0));
      break;
   }
   case nir_op_extract_u16: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *prmt = getSSA();
      mkOp3(OP_MAD, TYPE_U32, prmt, getSrc(&insn->src[1]), loadImm(NULL, 0x22), loadImm(NULL, 0x4410));
      mkOp3(OP_PERMT, TYPE_U32, newDefs[0], getSrc(&insn->src[0]), prmt, loadImm(NULL, 0));
      break;
   }
   case nir_op_extract_i16: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *prmt = getSSA();
      mkOp3(OP_MAD, TYPE_U32, prmt, getSrc(&insn->src[1]), loadImm(NULL, 0x2222), loadImm(NULL, 0x9910));
      mkOp3(OP_PERMT, TYPE_U32, newDefs[0], getSrc(&insn->src[0]), prmt, loadImm(NULL, 0));
      break;
   }
   case nir_op_urol: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp3(OP_SHF, TYPE_U32, newDefs[0], getSrc(&insn->src[0]),
            getSrc(&insn->src[1]), getSrc(&insn->src[0]))
         ->subOp = NV50_IR_SUBOP_SHF_L |
                   NV50_IR_SUBOP_SHF_W |
                   NV50_IR_SUBOP_SHF_HI;
      break;
   }
   case nir_op_uror: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp3(OP_SHF, TYPE_U32, newDefs[0], getSrc(&insn->src[0]),
            getSrc(&insn->src[1]), getSrc(&insn->src[0]))
         ->subOp = NV50_IR_SUBOP_SHF_R |
                   NV50_IR_SUBOP_SHF_W |
                   NV50_IR_SUBOP_SHF_LO;
      break;
   }
   // boolean conversions
   case nir_op_b2f32: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp2(OP_AND, TYPE_U32, newDefs[0], getSrc(&insn->src[0]), loadImm(NULL, 1.0f));
      break;
   }
   case nir_op_b2f64: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *tmp = getSSA(4);
      mkOp2(OP_AND, TYPE_U32, tmp, getSrc(&insn->src[0]), loadImm(NULL, 0x3ff00000));
      mkOp2(OP_MERGE, TYPE_U64, newDefs[0], loadImm(NULL, 0), tmp);
      break;
   }
   case nir_op_f2b32:
   case nir_op_i2b32: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      Value *src1;
      if (typeSizeof(sTypes[0]) == 8) {
         src1 = loadImm(getSSA(8), 0.0);
      } else {
         src1 = zero;
      }
      CondCode cc = op == nir_op_f2b32 ? CC_NEU : CC_NE;
      mkCmp(OP_SET, cc, TYPE_U32, newDefs[0], sTypes[0], getSrc(&insn->src[0]), src1);
      break;
   }
   case nir_op_b2i32: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      mkOp2(OP_AND, TYPE_U32, newDefs[0], getSrc(&insn->src[0]), loadImm(NULL, 1));
      break;
   }
   case nir_op_b2i64: {
      DEFAULT_CHECKS;
      LValues &newDefs = convert(&insn->dest);
      LValue *def = getScratch();
      mkOp2(OP_AND, TYPE_U32, def, getSrc(&insn->src[0]), loadImm(NULL, 1));
      mkOp2(OP_MERGE, TYPE_S64, newDefs[0], def, loadImm(NULL, 0));
      break;
   }
   default:
      ERROR("unknown nir_op %s\n", info.name);
      assert(false);
      return false;
   }

   if (!oldPos) {
      oldPos = this->bb->getEntry();
      oldPos->precise = insn->exact;
   }

   if (unlikely(!oldPos))
      return true;

   while (oldPos->next) {
      oldPos = oldPos->next;
      oldPos->precise = insn->exact;
   }
   oldPos->saturate = insn->dest.saturate;

   return true;
}
#undef DEFAULT_CHECKS

bool
Converter::visit(nir_ssa_undef_instr *insn)
{
   LValues &newDefs = convert(&insn->def);
   for (uint8_t i = 0u; i < insn->def.num_components; ++i) {
      mkOp(OP_NOP, TYPE_NONE, newDefs[i]);
   }
   return true;
}

#define CASE_SAMPLER(ty) \
   case GLSL_SAMPLER_DIM_ ## ty : \
      if (isArray && !isShadow) \
         return TEX_TARGET_ ## ty ## _ARRAY; \
      else if (!isArray && isShadow) \
         return TEX_TARGET_## ty ## _SHADOW; \
      else if (isArray && isShadow) \
         return TEX_TARGET_## ty ## _ARRAY_SHADOW; \
      else \
         return TEX_TARGET_ ## ty

TexTarget
Converter::convert(glsl_sampler_dim dim, bool isArray, bool isShadow)
{
   switch (dim) {
   CASE_SAMPLER(1D);
   CASE_SAMPLER(2D);
   CASE_SAMPLER(CUBE);
   case GLSL_SAMPLER_DIM_3D:
      return TEX_TARGET_3D;
   case GLSL_SAMPLER_DIM_MS:
      if (isArray)
         return TEX_TARGET_2D_MS_ARRAY;
      return TEX_TARGET_2D_MS;
   case GLSL_SAMPLER_DIM_RECT:
      if (isShadow)
         return TEX_TARGET_RECT_SHADOW;
      return TEX_TARGET_RECT;
   case GLSL_SAMPLER_DIM_BUF:
      return TEX_TARGET_BUFFER;
   case GLSL_SAMPLER_DIM_EXTERNAL:
      return TEX_TARGET_2D;
   default:
      ERROR("unknown glsl_sampler_dim %u\n", dim);
      assert(false);
      return TEX_TARGET_COUNT;
   }
}
#undef CASE_SAMPLER

unsigned int
Converter::getNIRArgCount(TexInstruction::Target& target)
{
   unsigned int result = target.getArgCount();
   if (target.isCube() && target.isArray())
      result--;
   if (target.isMS())
      result--;
   return result;
}

CacheMode
Converter::convert(enum gl_access_qualifier access)
{
   if (access & ACCESS_VOLATILE)
      return CACHE_CV;
   if (access & ACCESS_COHERENT)
      return CACHE_CG;
   return CACHE_CA;
}

bool
Converter::visit(nir_tex_instr *insn)
{
   switch (insn->op) {
   case nir_texop_lod:
   case nir_texop_query_levels:
   case nir_texop_tex:
   case nir_texop_texture_samples:
   case nir_texop_tg4:
   case nir_texop_txb:
   case nir_texop_txd:
   case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txl:
   case nir_texop_txs: {
      LValues &newDefs = convert(&insn->dest);
      std::vector<Value*> srcs;
      std::vector<Value*> defs;
      std::vector<nir_src*> offsets;
      uint8_t mask = 0;
      bool lz = false;
      TexInstruction::Target target = convert(insn->sampler_dim, insn->is_array, insn->is_shadow);
      operation op = getOperation(insn->op);

      int r, s;
      int biasIdx = nir_tex_instr_src_index(insn, nir_tex_src_bias);
      int compIdx = nir_tex_instr_src_index(insn, nir_tex_src_comparator);
      int coordsIdx = nir_tex_instr_src_index(insn, nir_tex_src_coord);
      int ddxIdx = nir_tex_instr_src_index(insn, nir_tex_src_ddx);
      int ddyIdx = nir_tex_instr_src_index(insn, nir_tex_src_ddy);
      int msIdx = nir_tex_instr_src_index(insn, nir_tex_src_ms_index);
      int lodIdx = nir_tex_instr_src_index(insn, nir_tex_src_lod);
      int offsetIdx = nir_tex_instr_src_index(insn, nir_tex_src_offset);
      int sampOffIdx = nir_tex_instr_src_index(insn, nir_tex_src_sampler_offset);
      int texOffIdx = nir_tex_instr_src_index(insn, nir_tex_src_texture_offset);
      int sampHandleIdx = nir_tex_instr_src_index(insn, nir_tex_src_sampler_handle);
      int texHandleIdx = nir_tex_instr_src_index(insn, nir_tex_src_texture_handle);

      bool bindless = sampHandleIdx != -1 || texHandleIdx != -1;
      assert((sampHandleIdx != -1) == (texHandleIdx != -1));

      srcs.resize(insn->coord_components);
      for (uint8_t i = 0u; i < insn->coord_components; ++i)
         srcs[i] = getSrc(&insn->src[coordsIdx].src, i);

      // sometimes we get less args than target.getArgCount, but codegen expects the latter
      if (insn->coord_components) {
         uint32_t argCount = target.getArgCount();

         if (target.isMS())
            argCount -= 1;

         for (uint32_t i = 0u; i < (argCount - insn->coord_components); ++i)
            srcs.push_back(getSSA());
      }

      if (insn->op == nir_texop_texture_samples)
         srcs.push_back(zero);
      else if (!insn->num_srcs)
         srcs.push_back(loadImm(NULL, 0));
      if (biasIdx != -1)
         srcs.push_back(getSrc(&insn->src[biasIdx].src, 0));
      if (lodIdx != -1)
         srcs.push_back(getSrc(&insn->src[lodIdx].src, 0));
      else if (op == OP_TXF)
         lz = true;
      if (msIdx != -1)
         srcs.push_back(getSrc(&insn->src[msIdx].src, 0));
      if (offsetIdx != -1)
         offsets.push_back(&insn->src[offsetIdx].src);
      if (compIdx != -1)
         srcs.push_back(getSrc(&insn->src[compIdx].src, 0));
      if (texOffIdx != -1) {
         srcs.push_back(getSrc(&insn->src[texOffIdx].src, 0));
         texOffIdx = srcs.size() - 1;
      }
      if (sampOffIdx != -1) {
         srcs.push_back(getSrc(&insn->src[sampOffIdx].src, 0));
         sampOffIdx = srcs.size() - 1;
      }
      if (bindless) {
         // currently we use the lower bits
         Value *split[2];
         Value *handle = getSrc(&insn->src[sampHandleIdx].src, 0);

         mkSplit(split, 4, handle);

         srcs.push_back(split[0]);
         texOffIdx = srcs.size() - 1;
      }

      r = bindless ? 0xff : insn->texture_index;
      s = bindless ? 0x1f : insn->sampler_index;

      defs.resize(newDefs.size());
      for (uint8_t d = 0u; d < newDefs.size(); ++d) {
         defs[d] = newDefs[d];
         mask |= 1 << d;
      }
      if (target.isMS() || (op == OP_TEX && prog->getType() != Program::TYPE_FRAGMENT))
         lz = true;

      TexInstruction *texi = mkTex(op, target.getEnum(), r, s, defs, srcs);
      texi->tex.levelZero = lz;
      texi->tex.mask = mask;
      texi->tex.bindless = bindless;

      if (texOffIdx != -1)
         texi->tex.rIndirectSrc = texOffIdx;
      if (sampOffIdx != -1)
         texi->tex.sIndirectSrc = sampOffIdx;

      switch (insn->op) {
      case nir_texop_tg4:
         if (!target.isShadow())
            texi->tex.gatherComp = insn->component;
         break;
      case nir_texop_txs:
         texi->tex.query = TXQ_DIMS;
         break;
      case nir_texop_texture_samples:
         texi->tex.mask = 0x4;
         texi->tex.query = TXQ_TYPE;
         break;
      case nir_texop_query_levels:
         texi->tex.mask = 0x8;
         texi->tex.query = TXQ_DIMS;
         break;
      default:
         break;
      }

      texi->tex.useOffsets = offsets.size();
      if (texi->tex.useOffsets) {
         for (uint8_t s = 0; s < texi->tex.useOffsets; ++s) {
            for (uint32_t c = 0u; c < 3; ++c) {
               uint8_t s2 = std::min(c, target.getDim() - 1);
               texi->offset[s][c].set(getSrc(offsets[s], s2));
               texi->offset[s][c].setInsn(texi);
            }
         }
      }

      if (op == OP_TXG && offsetIdx == -1) {
         if (nir_tex_instr_has_explicit_tg4_offsets(insn)) {
            texi->tex.useOffsets = 4;
            setPosition(texi, false);
            for (uint8_t i = 0; i < 4; ++i) {
               for (uint8_t j = 0; j < 2; ++j) {
                  texi->offset[i][j].set(loadImm(NULL, insn->tg4_offsets[i][j]));
                  texi->offset[i][j].setInsn(texi);
               }
            }
            setPosition(texi, true);
         }
      }

      if (ddxIdx != -1 && ddyIdx != -1) {
         for (uint8_t c = 0u; c < target.getDim() + target.isCube(); ++c) {
            texi->dPdx[c].set(getSrc(&insn->src[ddxIdx].src, c));
            texi->dPdy[c].set(getSrc(&insn->src[ddyIdx].src, c));
         }
      }

      break;
   }
   default:
      ERROR("unknown nir_texop %u\n", insn->op);
      return false;
   }
   return true;
}

bool
Converter::run()
{
   bool progress;

   if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
      nir_print_shader(nir, stderr);

   struct nir_lower_subgroups_options subgroup_options = {};
   subgroup_options.subgroup_size = 32;
   subgroup_options.ballot_bit_size = 32;
   subgroup_options.ballot_components = 1;
   subgroup_options.lower_elect = true;

   /* prepare for IO lowering */
   NIR_PASS_V(nir, nir_opt_deref);
   NIR_PASS_V(nir, nir_lower_regs_to_ssa);
   NIR_PASS_V(nir, nir_lower_vars_to_ssa);

   /* codegen assumes vec4 alignment for memory */
   NIR_PASS_V(nir, nir_lower_vars_to_explicit_types, nir_var_function_temp, function_temp_type_info);
   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_function_temp, nir_address_format_32bit_offset);
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_function_temp, NULL);

   NIR_PASS_V(nir, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
              type_size, (nir_lower_io_options)0);

   NIR_PASS_V(nir, nir_lower_subgroups, &subgroup_options);

   struct nir_lower_tex_options tex_options = {};
   tex_options.lower_txp = ~0;

   NIR_PASS_V(nir, nir_lower_tex, &tex_options);

   NIR_PASS_V(nir, nir_lower_load_const_to_scalar);
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS_V(nir, nir_lower_phis_to_scalar, false);

   /*TODO: improve this lowering/optimisation loop so that we can use
    *      nir_opt_idiv_const effectively before this.
    */
   nir_lower_idiv_options idiv_options = {
      .imprecise_32bit_lowering = false,
      .allow_fp16 = true,
   };
   NIR_PASS(progress, nir, nir_lower_idiv, &idiv_options);

   do {
      progress = false;
      NIR_PASS(progress, nir, nir_copy_prop);
      NIR_PASS(progress, nir, nir_opt_remove_phis);
      NIR_PASS(progress, nir, nir_opt_trivial_continues);
      NIR_PASS(progress, nir, nir_opt_cse);
      NIR_PASS(progress, nir, nir_opt_algebraic);
      NIR_PASS(progress, nir, nir_opt_constant_folding);
      NIR_PASS(progress, nir, nir_copy_prop);
      NIR_PASS(progress, nir, nir_opt_dce);
      NIR_PASS(progress, nir, nir_opt_dead_cf);
   } while (progress);

   NIR_PASS_V(nir, nir_lower_bool_to_int32);
   NIR_PASS_V(nir, nir_convert_from_ssa, true);

   // Garbage collect dead instructions
   nir_sweep(nir);

   if (!parseNIR()) {
      ERROR("Couldn't prase NIR!\n");
      return false;
   }

   if (!assignSlots()) {
      ERROR("Couldn't assign slots!\n");
      return false;
   }

   if (prog->dbgFlags & NV50_IR_DEBUG_BASIC)
      nir_print_shader(nir, stderr);

   nir_foreach_function(function, nir) {
      if (!visit(function))
         return false;
   }

   return true;
}

} // unnamed namespace

namespace nv50_ir {

bool
Program::makeFromNIR(struct nv50_ir_prog_info *info,
                     struct nv50_ir_prog_info_out *info_out)
{
   nir_shader *nir = (nir_shader*)info->bin.source;
   Converter converter(this, nir, info, info_out);
   bool result = converter.run();
   if (!result)
      return result;
   LoweringHelper lowering;
   lowering.run(this);
   tlsSize = info_out->bin.tlsSpace;
   return result;
}

} // namespace nv50_ir

static nir_shader_compiler_options
nvir_nir_shader_compiler_options(int chipset)
{
   nir_shader_compiler_options op = {};
   op.lower_fdiv = (chipset >= NVISA_GV100_CHIPSET);
   op.lower_ffma16 = false;
   op.lower_ffma32 = false;
   op.lower_ffma64 = false;
   op.fuse_ffma16 = false; /* nir doesn't track mad vs fma */
   op.fuse_ffma32 = false; /* nir doesn't track mad vs fma */
   op.fuse_ffma64 = false; /* nir doesn't track mad vs fma */
   op.lower_flrp16 = (chipset >= NVISA_GV100_CHIPSET);
   op.lower_flrp32 = true;
   op.lower_flrp64 = true;
   op.lower_fpow = false; // TODO: nir's lowering is broken, or we could use it
   op.lower_fsat = false;
   op.lower_fsqrt = false; // TODO: only before gm200
   op.lower_sincos = false;
   op.lower_fmod = true;
   op.lower_bitfield_extract = false;
   op.lower_bitfield_extract_to_shifts = (chipset >= NVISA_GV100_CHIPSET);
   op.lower_bitfield_insert = false;
   op.lower_bitfield_insert_to_shifts = (chipset >= NVISA_GV100_CHIPSET);
   op.lower_bitfield_insert_to_bitfield_select = false;
   op.lower_bitfield_reverse = false;
   op.lower_bit_count = false;
   op.lower_ifind_msb = false;
   op.lower_find_lsb = false;
   op.lower_uadd_carry = true; // TODO
   op.lower_usub_borrow = true; // TODO
   op.lower_mul_high = false;
   op.lower_fneg = false;
   op.lower_ineg = false;
   op.lower_scmp = true; // TODO: not implemented yet
   op.lower_vector_cmp = false;
   op.lower_bitops = false;
   op.lower_isign = (chipset >= NVISA_GV100_CHIPSET);
   op.lower_fsign = (chipset >= NVISA_GV100_CHIPSET);
   op.lower_fdph = false;
   op.lower_fdot = false;
   op.fdot_replicates = false; // TODO
   op.lower_ffloor = false; // TODO
   op.lower_ffract = true;
   op.lower_fceil = false; // TODO
   op.lower_ftrunc = false;
   op.lower_ldexp = true;
   op.lower_pack_half_2x16 = true;
   op.lower_pack_unorm_2x16 = true;
   op.lower_pack_snorm_2x16 = true;
   op.lower_pack_unorm_4x8 = true;
   op.lower_pack_snorm_4x8 = true;
   op.lower_unpack_half_2x16 = true;
   op.lower_unpack_unorm_2x16 = true;
   op.lower_unpack_snorm_2x16 = true;
   op.lower_unpack_unorm_4x8 = true;
   op.lower_unpack_snorm_4x8 = true;
   op.lower_pack_split = false;
   op.lower_extract_byte = (chipset < NVISA_GM107_CHIPSET);
   op.lower_extract_word = (chipset < NVISA_GM107_CHIPSET);
   op.lower_insert_byte = true;
   op.lower_insert_word = true;
   op.lower_all_io_to_temps = false;
   op.lower_all_io_to_elements = false;
   op.vertex_id_zero_based = false;
   op.lower_base_vertex = false;
   op.lower_helper_invocation = false;
   op.optimize_sample_mask_in = false;
   op.lower_cs_local_index_from_id = true;
   op.lower_cs_local_id_from_index = false;
   op.lower_device_index_to_zero = false; // TODO
   op.lower_wpos_pntc = false; // TODO
   op.lower_hadd = true; // TODO
   op.lower_uadd_sat = true; // TODO
   op.lower_iadd_sat = true; // TODO
   op.vectorize_io = false;
   op.lower_to_scalar = false;
   op.unify_interfaces = false;
   op.use_interpolated_input_intrinsics = true;
   op.lower_mul_2x32_64 = true; // TODO
   op.lower_rotate = (chipset < NVISA_GV100_CHIPSET);
   op.has_imul24 = false;
   op.intel_vec4 = false;
   op.max_unroll_iterations = 32;
   op.lower_int64_options = (nir_lower_int64_options) (
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_imul64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_isign64 : 0) |
      nir_lower_divmod64 |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_imul_high64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_mov64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_icmp64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_iabs64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_ineg64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_logic64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_minmax64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_shift64 : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_imul_2x32_64 : 0) |
      ((chipset >= NVISA_GM107_CHIPSET) ? nir_lower_extract64 : 0) |
      nir_lower_ufind_msb64
   );
   op.lower_doubles_options = (nir_lower_doubles_options) (
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_drcp : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_dsqrt : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_drsq : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_dfract : 0) |
      nir_lower_dmod |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_dsub : 0) |
      ((chipset >= NVISA_GV100_CHIPSET) ? nir_lower_ddiv : 0)
   );
   return op;
}

static const nir_shader_compiler_options gf100_nir_shader_compiler_options =
nvir_nir_shader_compiler_options(NVISA_GF100_CHIPSET);
static const nir_shader_compiler_options gm107_nir_shader_compiler_options =
nvir_nir_shader_compiler_options(NVISA_GM107_CHIPSET);
static const nir_shader_compiler_options gv100_nir_shader_compiler_options =
nvir_nir_shader_compiler_options(NVISA_GV100_CHIPSET);

const nir_shader_compiler_options *
nv50_ir_nir_shader_compiler_options(int chipset)
{
   if (chipset >= NVISA_GV100_CHIPSET)
      return &gv100_nir_shader_compiler_options;
   if (chipset >= NVISA_GM107_CHIPSET)
      return &gm107_nir_shader_compiler_options;
   return &gf100_nir_shader_compiler_options;
}
