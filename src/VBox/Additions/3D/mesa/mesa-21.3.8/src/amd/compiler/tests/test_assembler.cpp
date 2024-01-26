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

BEGIN_TEST(assembler.s_memtime)
   for (unsigned i = GFX6; i <= GFX10; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

      //~gfx[6-7]>> c7800000
      //~gfx[6-7]!  bf810000
      //~gfx[8-9]>> s_memtime s[0:1] ; c0900000 00000000
      //~gfx10>> s_memtime s[0:1] ; f4900000 fa000000
      bld.smem(aco_opcode::s_memtime, bld.def(s2)).def(0).setFixed(PhysReg{0});

      finish_assembler_test();
   }
END_TEST

BEGIN_TEST(assembler.branch_3f)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //! BB0:
   //! s_branch BB1                                                ; bf820040
   //! s_nop 0                                                     ; bf800000
   bld.sopp(aco_opcode::s_branch, Definition(PhysReg(0), s2), 1);

   for (unsigned i = 0; i < 0x3f; i++)
      bld.vop1(aco_opcode::v_nop);

   bld.reset(program->create_and_insert_block());

   program->blocks[1].linear_preds.push_back(0u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.long_jump.unconditional_forwards)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //!BB0:
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0x20018                                  ; 8200ff00 00020018
   //! s_addc_u32 s1, s1, 0                                        ; 82018001
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
   bld.sopp(aco_opcode::s_branch, Definition(PhysReg(0), s2), 2);

   bld.reset(program->create_and_insert_block());

   //! s_nop 0                                                     ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
      bld.sopp(aco_opcode::s_nop, -1, 0);

   //! BB2:
   //! s_endpgm                                                    ; bf810000
   bld.reset(program->create_and_insert_block());

   program->blocks[2].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(1u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.long_jump.conditional_forwards)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //! BB0:
   //! s_cbranch_scc1 BB1                                          ; bf850007
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0x20018                                  ; 8200ff00 00020018
   //! s_addc_u32 s1, s1, 0                                        ; 82018001
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
   bld.sopp(aco_opcode::s_cbranch_scc0, Definition(PhysReg(0), s2), 2);

   bld.reset(program->create_and_insert_block());

   //! BB1:
   //! s_nop 0 ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
      bld.sopp(aco_opcode::s_nop, -1, 0);

   //! BB2:
   //! s_endpgm                                                    ; bf810000
   bld.reset(program->create_and_insert_block());

   program->blocks[1].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(1u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.long_jump.unconditional_backwards)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //!BB0:
   //! s_nop 0                                                     ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
      bld.sopp(aco_opcode::s_nop, -1, 0);

   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0xfffdfffc                               ; 8200ff00 fffdfffc
   //! s_addc_u32 s1, s1, -1                                       ; 8201c101
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
   bld.sopp(aco_opcode::s_branch, Definition(PhysReg(0), s2), 0);

   //! BB1:
   //! s_endpgm                                                    ; bf810000
   bld.reset(program->create_and_insert_block());

   program->blocks[0].linear_preds.push_back(0u);
   program->blocks[1].linear_preds.push_back(0u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.long_jump.conditional_backwards)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //!BB0:
   //! s_nop 0                                                     ; bf800000
   //!(then repeated 32767 times)
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
      bld.sopp(aco_opcode::s_nop, -1, 0);

   //! s_cbranch_execz BB1                                         ; bf880007
   //! s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_addc_u32 s0, s0, 0xfffdfff8                               ; 8200ff00 fffdfff8
   //! s_addc_u32 s1, s1, -1                                       ; 8201c101
   //! s_bitcmp1_b32 s0, 0                                         ; bf0d8000
   //! s_bitset0_b32 s0, 0                                         ; be801b80
   //! s_setpc_b64 s[0:1]                                          ; be802000
   bld.sopp(aco_opcode::s_cbranch_execnz, Definition(PhysReg(0), s2), 0);

   //! BB1:
   //! s_endpgm                                                    ; bf810000
   bld.reset(program->create_and_insert_block());

   program->blocks[0].linear_preds.push_back(0u);
   program->blocks[1].linear_preds.push_back(0u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.long_jump.3f)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //! BB0:
   //! s_branch BB1                                                ; bf820040
   //! s_nop 0                                                     ; bf800000
   bld.sopp(aco_opcode::s_branch, Definition(PhysReg(0), s2), 1);

   for (unsigned i = 0; i < 0x3f - 7; i++) // a unconditional long jump is 7 dwords
      bld.vop1(aco_opcode::v_nop);
   bld.sopp(aco_opcode::s_branch, Definition(PhysReg(0), s2), 2);

   bld.reset(program->create_and_insert_block());
   for (unsigned i = 0; i < INT16_MAX + 1; i++)
      bld.vop1(aco_opcode::v_nop);
   bld.reset(program->create_and_insert_block());

   program->blocks[1].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(1u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.long_jump.constaddr)
   if (!setup_cs(NULL, (chip_class)GFX10))
      return;

   //>> s_getpc_b64 s[0:1]                                          ; be801f00
   bld.sopp(aco_opcode::s_branch, Definition(PhysReg(0), s2), 2);

   bld.reset(program->create_and_insert_block());

   for (unsigned i = 0; i < INT16_MAX + 1; i++)
      bld.sopp(aco_opcode::s_nop, -1, 0);

   bld.reset(program->create_and_insert_block());

   //>> s_getpc_b64 s[0:1]                                          ; be801f00
   //! s_add_u32 s0, s0, 0xe0                                      ; 8000ff00 000000e0
   bld.sop1(aco_opcode::p_constaddr_getpc, Definition(PhysReg(0), s2), Operand::zero());
   bld.sop2(aco_opcode::p_constaddr_addlo, Definition(PhysReg(0), s1), bld.def(s1, scc),
            Operand(PhysReg(0), s1), Operand::zero());

   program->blocks[2].linear_preds.push_back(0u);
   program->blocks[2].linear_preds.push_back(1u);

   finish_assembler_test();
END_TEST

BEGIN_TEST(assembler.v_add3)
   for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

      //~gfx9>> v_add3_u32 v0, 0, 0, 0 ; d1ff0000 02010080
      //~gfx10>> v_add3_u32 v0, 0, 0, 0 ; d76d0000 02010080
      aco_ptr<VOP3_instruction> add3{create_instruction<VOP3_instruction>(aco_opcode::v_add3_u32, Format::VOP3, 3, 1)};
      add3->operands[0] = Operand::zero();
      add3->operands[1] = Operand::zero();
      add3->operands[2] = Operand::zero();
      add3->definitions[0] = Definition(PhysReg(0), v1);
      bld.insert(std::move(add3));

      finish_assembler_test();
   }
END_TEST

BEGIN_TEST(assembler.v_add3_clamp)
   for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

      //~gfx9>> integer addition + clamp ; d1ff8000 02010080
      //~gfx10>> integer addition + clamp ; d76d8000 02010080
      aco_ptr<VOP3_instruction> add3{create_instruction<VOP3_instruction>(aco_opcode::v_add3_u32, Format::VOP3, 3, 1)};
      add3->operands[0] = Operand::zero();
      add3->operands[1] = Operand::zero();
      add3->operands[2] = Operand::zero();
      add3->definitions[0] = Definition(PhysReg(0), v1);
      add3->clamp = 1;
      bld.insert(std::move(add3));

      finish_assembler_test();
   }
END_TEST
