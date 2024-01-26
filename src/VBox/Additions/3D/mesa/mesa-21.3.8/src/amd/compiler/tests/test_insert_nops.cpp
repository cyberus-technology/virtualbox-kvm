/*
 * Copyright © 2020 Valve Corporation
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
#include "helpers.h"

using namespace aco;

void create_mubuf(unsigned offset)
{
   bld.mubuf(aco_opcode::buffer_load_dword, Definition(PhysReg(256), v1), Operand(PhysReg(0), s4),
             Operand(PhysReg(256), v1), Operand::zero(), offset, true);
}

void create_mimg(bool nsa, unsigned addrs, unsigned instr_dwords)
{
   aco_ptr<MIMG_instruction> mimg{create_instruction<MIMG_instruction>(
      aco_opcode::image_sample, Format::MIMG, 3 + addrs, 1)};
   mimg->definitions[0] = Definition(PhysReg(256), v1);
   mimg->operands[0] = Operand(PhysReg(0), s8);
   mimg->operands[1] = Operand(PhysReg(0), s4);
   mimg->operands[2] = Operand(v1);
   for (unsigned i = 0; i < addrs; i++)
      mimg->operands[3 + i] = Operand(PhysReg(256 + (nsa ? i * 2 : i)), v1);
   mimg->dmask = 0x1;
   mimg->dim = ac_image_2d;

   assert(get_mimg_nsa_dwords(mimg.get()) + 2 == instr_dwords);

   bld.insert(std::move(mimg));
}

BEGIN_TEST(insert_nops.nsa_to_vmem_bug)
   if (!setup_cs(NULL, GFX10))
      return;

   /* no nop needed because offset&6==0 */
   //>> p_unit_test 0
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2], %0:v[4], %0:v[6], %0:v[8], %0:v[10] 2d storage: semantics: scope:invocation
   //! v1: %0:v[0] = buffer_load_dword %0:s[0-3], %0:v[0], 0 offset:8 offen storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
   create_mimg(true, 6, 4);
   create_mubuf(8);

   /* nop needed */
   //! p_unit_test 1
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2], %0:v[4], %0:v[6], %0:v[8], %0:v[10] 2d storage: semantics: scope:invocation
   //! s_nop
   //! v1: %0:v[0] = buffer_load_dword %0:s[0-3], %0:v[0], 0 offset:4 offen storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(1u));
   create_mimg(true, 6, 4);
   create_mubuf(4);

   /* no nop needed because the MIMG is not NSA */
   //! p_unit_test 2
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[1], %0:v[2], %0:v[3], %0:v[4], %0:v[5] 2d storage: semantics: scope:invocation
   //! v1: %0:v[0] = buffer_load_dword %0:s[0-3], %0:v[0], 0 offset:4 offen storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
   create_mimg(false, 6, 2);
   create_mubuf(4);

   /* no nop needed because there's already an instruction in-between */
   //! p_unit_test 3
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2], %0:v[4], %0:v[6], %0:v[8], %0:v[10] 2d storage: semantics: scope:invocation
   //! v_nop
   //! v1: %0:v[0] = buffer_load_dword %0:s[0-3], %0:v[0], 0 offset:4 offen storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(3u));
   create_mimg(true, 6, 4);
   bld.vop1(aco_opcode::v_nop);
   create_mubuf(4);

   /* no nop needed because the NSA instruction is under 4 dwords */
   //! p_unit_test 4
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2] 2d storage: semantics: scope:invocation
   //! v1: %0:v[0] = buffer_load_dword %0:s[0-3], %0:v[0], 0 offset:4 offen storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(4u));
   create_mimg(true, 2, 3);
   create_mubuf(4);

   /* NSA instruction and MUBUF/MTBUF in a different block */
   //! p_unit_test 5
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2], %0:v[4], %0:v[6], %0:v[8], %0:v[10] 2d storage: semantics: scope:invocation
   //! BB1
   //! /* logical preds: / linear preds: BB0, / kind: uniform, */
   //! s_nop
   //! v1: %0:v[0] = buffer_load_dword %0:s[0-3], %0:v[0], 0 offset:4 offen storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(5u));
   create_mimg(true, 6, 4);
   bld.reset(program->create_and_insert_block());
   create_mubuf(4);
   program->blocks[0].linear_succs.push_back(1);
   program->blocks[1].linear_preds.push_back(0);

   finish_insert_nops_test();
END_TEST

BEGIN_TEST(insert_nops.writelane_to_nsa_bug)
   if (!setup_cs(NULL, GFX10))
      return;

   /* nop needed */
   //>> p_unit_test 0
   //! v1: %0:v[255] = v_writelane_b32_e64 0, 0, %0:v[255]
   //! s_nop
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2] 2d storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
   bld.writelane(Definition(PhysReg(511), v1), Operand::zero(), Operand::zero(),
                 Operand(PhysReg(511), v1));
   create_mimg(true, 2, 3);

   /* no nop needed because the MIMG is not NSA */
   //! p_unit_test 1
   //! v1: %0:v[255] = v_writelane_b32_e64 0, 0, %0:v[255]
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[1] 2d storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(1u));
   bld.writelane(Definition(PhysReg(511), v1), Operand::zero(), Operand::zero(),
                 Operand(PhysReg(511), v1));
   create_mimg(false, 2, 2);

   /* no nop needed because there's already an instruction in-between */
   //! p_unit_test 2
   //! v1: %0:v[255] = v_writelane_b32_e64 0, 0, %0:v[255]
   //! v_nop
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2] 2d storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
   bld.writelane(Definition(PhysReg(511), v1), Operand::zero(), Operand::zero(),
                 Operand(PhysReg(511), v1));
   bld.vop1(aco_opcode::v_nop);
   create_mimg(true, 2, 3);

   /* writelane and NSA instruction in different blocks */
   //! p_unit_test 3
   //! v1: %0:v[255] = v_writelane_b32_e64 0, 0, %0:v[255]
   //! BB1
   //! /* logical preds: / linear preds: BB0, / kind: uniform, */
   //! s_nop
   //! v1: %0:v[0] = image_sample %0:s[0-7], %0:s[0-3],  v1: undef, %0:v[0], %0:v[2] 2d storage: semantics: scope:invocation
   bld.pseudo(aco_opcode::p_unit_test, Operand::c32(3u));
   bld.writelane(Definition(PhysReg(511), v1), Operand::zero(), Operand::zero(),
                 Operand(PhysReg(511), v1));
   bld.reset(program->create_and_insert_block());
   create_mimg(true, 2, 3);
   program->blocks[0].linear_succs.push_back(1);
   program->blocks[1].linear_preds.push_back(0);

   finish_insert_nops_test();
END_TEST
