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

BEGIN_TEST(to_hw_instr.swap_subdword)
   PhysReg v0_lo{256};
   PhysReg v0_hi{256};
   PhysReg v0_b1{256};
   PhysReg v0_b3{256};
   PhysReg v1_lo{257};
   PhysReg v1_hi{257};
   PhysReg v1_b1{257};
   PhysReg v1_b3{257};
   PhysReg v2_lo{258};
   PhysReg v3_lo{259};
   v0_hi.reg_b += 2;
   v1_hi.reg_b += 2;
   v0_b1.reg_b += 1;
   v1_b1.reg_b += 1;
   v0_b3.reg_b += 3;
   v1_b3.reg_b += 3;

   for (unsigned i = GFX6; i <= GFX7; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

      //~gfx[67]>>  p_unit_test 0
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v2b), Definition(v1_lo, v2b),
                 Operand(v1_lo, v2b), Operand(v0_lo, v2b));

      //~gfx[67]! p_unit_test 1
      //~gfx[67]! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[1][0:16], %0:v[0][16:32], 2
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[0][0:16], %0:v[0][16:32], 2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(1u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v1),
                 Operand(v1_lo, v2b), Operand(v0_lo, v2b));

      //~gfx[67]! p_unit_test 2
      //~gfx[67]! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[1][0:16], %0:v[0][16:32], 2
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[0][0:16], %0:v[0][16:32], 2
      //~gfx[67]! v2b: %0:v[1][0:16] = v_mov_b32 %0:v[2][0:16]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v6b), Operand(v1_lo, v2b),
                 Operand(v0_lo, v2b), Operand(v2_lo, v2b));

      //~gfx[67]! p_unit_test 3
      //~gfx[67]! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[1][0:16], %0:v[0][16:32], 2
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[0][0:16], %0:v[0][16:32], 2
      //~gfx[67]! v2b: %0:v[1][16:32] = v_lshlrev_b32 16, %0:v[2][0:16]
      //~gfx[67]! v1: %0:v[1] = v_alignbyte_b32 %0:v[3][0:16], %0:v[1][16:32], 2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(3u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v2),
                 Operand(v1_lo, v2b), Operand(v0_lo, v2b),
                 Operand(v2_lo, v2b), Operand(v3_lo, v2b));

      //~gfx[67]! p_unit_test 4
      //~gfx[67]! v2b: %0:v[1][16:32] = v_lshlrev_b32 16, %0:v[1][0:16]
      //~gfx[67]! v1: %0:v[1] = v_alignbyte_b32 %0:v[2][0:16], %0:v[1][16:32], 2
      //~gfx[67]! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[3][0:16], %0:v[0][16:32], 2
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(4u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v2),
                 Operand(v1_lo, v2b), Operand(v2_lo, v2b),
                 Operand(v0_lo, v2b), Operand(v3_lo, v2b));

      //~gfx[67]! p_unit_test 5
      //~gfx[67]! v2b: %0:v[1][0:16] = v_mov_b32 %0:v[0][0:16]
      //~gfx[67]! v2b: %0:v[0][0:16] = v_lshrrev_b32 16, %0:v[1][16:32]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(5u));
      bld.pseudo(aco_opcode::p_split_vector,
                 Definition(v1_lo, v2b), Definition(v0_lo, v2b),
                 Operand(v0_lo, v1));

      //~gfx[67]! p_unit_test 6
      //~gfx[67]! v2b: %0:v[2][0:16] = v_mov_b32 %0:v[1][0:16]
      //~gfx[67]! v2b: %0:v[1][0:16] = v_mov_b32 %0:v[0][0:16]
      //~gfx[67]! v2b: %0:v[0][0:16] = v_lshrrev_b32 16, %0:v[1][16:32]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(6u));
      bld.pseudo(aco_opcode::p_split_vector,
                 Definition(v1_lo, v2b), Definition(v0_lo, v2b),
                 Definition(v2_lo, v2b), Operand(v0_lo, v6b));

      //~gfx[67]! p_unit_test 7
      //~gfx[67]! v2b: %0:v[2][0:16] = v_mov_b32 %0:v[1][0:16]
      //~gfx[67]! v2b: %0:v[1][0:16] = v_mov_b32 %0:v[0][0:16]
      //~gfx[67]! v2b: %0:v[0][0:16] = v_lshrrev_b32 16, %0:v[1][16:32]
      //~gfx[67]! v2b: %0:v[3][0:16] = v_lshrrev_b32 16, %0:v[2][16:32]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(7u));
      bld.pseudo(aco_opcode::p_split_vector,
                 Definition(v1_lo, v2b), Definition(v0_lo, v2b),
                 Definition(v2_lo, v2b), Definition(v3_lo, v2b),
                 Operand(v0_lo, v2));

      //~gfx[67]! p_unit_test 8
      //~gfx[67]! v2b: %0:v[2][0:16] = v_lshrrev_b32 16, %0:v[0][16:32]
      //~gfx[67]! v2b: %0:v[3][0:16] = v_lshrrev_b32 16, %0:v[1][16:32]
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(8u));
      bld.pseudo(aco_opcode::p_split_vector,
                 Definition(v1_lo, v2b), Definition(v2_lo, v2b),
                 Definition(v0_lo, v2b), Definition(v3_lo, v2b),
                 Operand(v0_lo, v2));

      //~gfx[67]! p_unit_test 9
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx[67]! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(9u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v1b), Definition(v1_lo, v1b),
                 Operand(v1_lo, v1b), Operand(v0_lo, v1b));

      //~gfx[67]! p_unit_test 10
      //~gfx[67]! v1b: %0:v[1][24:32] = v_lshlrev_b32 24, %0:v[1][0:8]
      //~gfx[67]! v2b: %0:v[1][0:16] = v_alignbyte_b32 %0:v[0][0:8], %0:v[1][24:32], 3
      //~gfx[67]! v2b: %0:v[0][0:16] = v_mov_b32 %0:v[1][0:16]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(10u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v2b),
                 Operand(v1_lo, v1b), Operand(v0_lo, v1b));

      //~gfx[67]! p_unit_test 11
      //~gfx[67]! v1b: %0:v[1][24:32] = v_lshlrev_b32 24, %0:v[1][0:8]
      //~gfx[67]! v2b: %0:v[1][0:16] = v_alignbyte_b32 %0:v[0][0:8], %0:v[1][24:32], 3
      //~gfx[67]! v2b: %0:v[0][0:16] = v_mov_b32 %0:v[1][0:16]
      //~gfx[67]! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx[67]! v3b: %0:v[0][0:24] = v_alignbyte_b32 %0:v[2][0:8], %0:v[0][16:32], 2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(11u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v3b), Operand(v1_lo, v1b),
                 Operand(v0_lo, v1b), Operand(v2_lo, v1b));

      //~gfx[67]! p_unit_test 12
      //~gfx[67]! v1b: %0:v[1][24:32] = v_lshlrev_b32 24, %0:v[1][0:8]
      //~gfx[67]! v2b: %0:v[1][0:16] = v_alignbyte_b32 %0:v[0][0:8], %0:v[1][24:32], 3
      //~gfx[67]! v2b: %0:v[0][0:16] = v_mov_b32 %0:v[1][0:16]
      //~gfx[67]! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx[67]! v3b: %0:v[0][0:24] = v_alignbyte_b32 %0:v[2][0:8], %0:v[0][16:32], 2
      //~gfx[67]! v3b: %0:v[0][8:32] = v_lshlrev_b32 8, %0:v[0][0:24]
      //~gfx[67]! v1: %0:v[0] = v_alignbyte_b32 %0:v[3][0:8], %0:v[0][8:32], 1
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(12u));
      bld.pseudo(aco_opcode::p_create_vector,
                 Definition(v0_lo, v1),
                 Operand(v1_lo, v1b), Operand(v0_lo, v1b),
                 Operand(v2_lo, v1b), Operand(v3_lo, v1b));

      //~gfx[67]! p_unit_test 13
      //~gfx[67]! v1b: %0:v[0][0:8] = v_and_b32 0xff, %0:v[0][0:8]
      //~gfx[67]! v2b: %0:v[0][0:16] = v_mul_u32_u24 0x101, %0:v[0][0:8]
      //~gfx[67]! v2b: %0:v[0][0:16] = v_and_b32 0xffff, %0:v[0][0:16]
      //~gfx[67]! v3b: %0:v[0][0:24] = v_cvt_pk_u16_u32 %0:v[0][0:16], %0:v[0][0:8]
      //~gfx[67]! v3b: %0:v[0][0:24] = v_and_b32 0xffffff, %0:v[0][0:24]
      //~gfx[67]! s1: %0:m0 = s_mov_b32 0x1000001
      //~gfx[67]! v1: %0:v[0] = v_mul_lo_u32 %0:m0, %0:v[0][0:8]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(13u));
      Instruction* pseudo = bld.pseudo(aco_opcode::p_create_vector,
                                       Definition(v0_lo, v1),
                                       Operand(v0_lo, v1b), Operand(v0_lo, v1b),
                                       Operand(v0_lo, v1b), Operand(v0_lo, v1b));
      pseudo->pseudo().scratch_sgpr = m0;

      //~gfx[67]! p_unit_test 14
      //~gfx[67]! v1b: %0:v[1][0:8] = v_mov_b32 %0:v[0][0:8]
      //~gfx[67]! v1b: %0:v[0][0:8] = v_lshrrev_b32 8, %0:v[1][8:16]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(14u));
      bld.pseudo(aco_opcode::p_split_vector,
                 Definition(v1_lo, v1b), Definition(v0_lo, v1b),
                 Operand(v0_lo, v2b));

      //~gfx[67]! p_unit_test 15
      //~gfx[67]! v1b: %0:v[1][0:8] = v_mov_b32 %0:v[0][0:8]
      //~gfx[67]! v1b: %0:v[0][0:8] = v_lshrrev_b32 8, %0:v[1][8:16]
      //~gfx[67]! v1b: %0:v[2][0:8] = v_lshrrev_b32 16, %0:v[1][16:24]
      //~gfx[67]! v1b: %0:v[3][0:8] = v_lshrrev_b32 24, %0:v[1][24:32]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(15u));
      bld.pseudo(aco_opcode::p_split_vector,
                 Definition(v1_lo, v1b), Definition(v0_lo, v1b),
                 Definition(v2_lo, v1b), Definition(v3_lo, v1b),
                 Operand(v0_lo, v1));

      //~gfx[67]! s_endpgm

      finish_to_hw_instr_test();
   }

   for (unsigned i = GFX8; i <= GFX9; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

      //~gfx[89]>> p_unit_test 0
      //~gfx8! v1: %0:v[0] = v_alignbyte_b32 %0:v[0][0:16], %0:v[0][16:32], 2
      //~gfx9! v1: %0:v[0] = v_pack_b32_f16 hi(%0:v[0][16:32]), %0:v[0][0:16]
      bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand(v0_hi, v2b), Operand(v0_lo, v2b));

      //~gfx[89]! p_unit_test 1
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx9! v1: %0:v[0],  v1: %0:v[1] = v_swap_b32 %0:v[1], %0:v[0]
      //~gfx[89]! v2b: %0:v[1][16:32] = v_mov_b32 %0:v[0][16:32] dst_sel:uword1 dst_preserve src0_sel:uword1
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(1u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v1), Definition(v1_lo, v2b),
                 Operand(v1_lo, v1), Operand(v0_lo, v2b));

      //~gfx[89]! p_unit_test 2
      //~gfx[89]! v2b: %0:v[0][16:32] = v_mov_b32 %0:v[1][16:32] dst_sel:uword1 dst_preserve src0_sel:uword1
      //~gfx[89]! v2b: %0:v[1][16:32] = v_mov_b32 %0:v[0][0:16] dst_sel:uword1 dst_preserve src0_sel:uword0
      //~gfx[89]! v2b: %0:v[1][0:16] = v_xor_b32 %0:v[1][0:16], %0:v[0][0:16] dst_sel:uword0 dst_preserve src0_sel:uword0 src1_sel:uword0
      //~gfx[89]! v2b: %0:v[0][0:16] = v_xor_b32 %0:v[1][0:16], %0:v[0][0:16] dst_sel:uword0 dst_preserve src0_sel:uword0 src1_sel:uword0
      //~gfx[89]! v2b: %0:v[1][0:16] = v_xor_b32 %0:v[1][0:16], %0:v[0][0:16] dst_sel:uword0 dst_preserve src0_sel:uword0 src1_sel:uword0
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v1), Definition(v1_lo, v2b), Definition(v1_hi, v2b),
                 Operand(v1_lo, v1), Operand(v0_lo, v2b), Operand(v0_lo, v2b));

      //~gfx[89]! p_unit_test 3
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx9! v1: %0:v[0],  v1: %0:v[1] = v_swap_b32 %0:v[1], %0:v[0]
      //~gfx[89]! v2b: %0:v[1][0:16] = v_mov_b32 %0:v[0][0:16] dst_sel:uword0 dst_preserve src0_sel:uword0
      //~gfx[89]! v1b: %0:v[1][16:24] = v_mov_b32 %0:v[0][16:24] dst_sel:ubyte2 dst_preserve src0_sel:ubyte2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(3u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v1), Definition(v1_b3, v1b),
                 Operand(v1_lo, v1), Operand(v0_b3, v1b));

      //~gfx[89]! p_unit_test 4
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx9! v1: %0:v[0],  v1: %0:v[1] = v_swap_b32 %0:v[1], %0:v[0]
      //~gfx[89]! v1b: %0:v[1][8:16] = v_mov_b32 %0:v[0][8:16] dst_sel:ubyte1 dst_preserve src0_sel:ubyte1
      //~gfx[89]! v2b: %0:v[1][16:32] = v_mov_b32 %0:v[0][16:32] dst_sel:uword1 dst_preserve src0_sel:uword1
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(4u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v1), Definition(v1_lo, v1b),
                 Operand(v1_lo, v1), Operand(v0_lo, v1b));

      //~gfx[89]! p_unit_test 5
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[0], %0:v[1]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[0], %0:v[1]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[0], %0:v[1]
      //~gfx9! v1: %0:v[1],  v1: %0:v[0] = v_swap_b32 %0:v[0], %0:v[1]
      //~gfx[89]! v1b: %0:v[0][8:16] = v_mov_b32 %0:v[1][8:16] dst_sel:ubyte1 dst_preserve src0_sel:ubyte1
      //~gfx[89]! v1b: %0:v[0][24:32] = v_mov_b32 %0:v[1][24:32] dst_sel:ubyte3 dst_preserve src0_sel:ubyte3
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(5u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v1b), Definition(v0_hi, v1b), Definition(v1_lo, v1),
                 Operand(v1_lo, v1b), Operand(v1_hi, v1b), Operand(v0_lo, v1));

      //~gfx[89]! p_unit_test 6
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx9! v1: %0:v[0],  v1: %0:v[1] = v_swap_b32 %0:v[1], %0:v[0]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(6u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v2b), Definition(v0_hi, v2b), Definition(v1_lo, v1),
                 Operand(v1_lo, v2b), Operand(v1_hi, v2b), Operand(v0_lo, v1));

      //~gfx[89]! p_unit_test 7
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[0], %0:v[1]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[0], %0:v[1]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[0], %0:v[1]
      //~gfx9! v1: %0:v[1],  v1: %0:v[0] = v_swap_b32 %0:v[0], %0:v[1]
      //~gfx[89]! v1: %0:v[0] = v_alignbyte_b32 %0:v[0][0:16], %0:v[0][16:32], 2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(7u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v2b), Definition(v0_hi, v2b), Definition(v1_lo, v1),
                 Operand(v1_hi, v2b), Operand(v1_lo, v2b), Operand(v0_lo, v1));

      //~gfx[89]! p_unit_test 8
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx9! v1: %0:v[0],  v1: %0:v[1] = v_swap_b32 %0:v[1], %0:v[0]
      //~gfx[89]! v1b: %0:v[1][24:32] = v_xor_b32 %0:v[1][24:32], %0:v[0][24:32] dst_sel:ubyte3 dst_preserve src0_sel:ubyte3 src1_sel:ubyte3
      //~gfx[89]! v1b: %0:v[0][24:32] = v_xor_b32 %0:v[1][24:32], %0:v[0][24:32] dst_sel:ubyte3 dst_preserve src0_sel:ubyte3 src1_sel:ubyte3
      //~gfx[89]! v1b: %0:v[1][24:32] = v_xor_b32 %0:v[1][24:32], %0:v[0][24:32] dst_sel:ubyte3 dst_preserve src0_sel:ubyte3 src1_sel:ubyte3
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(8u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v3b), Definition(v1_lo, v3b),
                 Operand(v1_lo, v3b), Operand(v0_lo, v3b));

      //~gfx[89]! p_unit_test 9
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[0] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx8! v1: %0:v[1] = v_xor_b32 %0:v[1], %0:v[0]
      //~gfx9! v1: %0:v[0],  v1: %0:v[1] = v_swap_b32 %0:v[1], %0:v[0]
      //~gfx[89]! v1b: %0:v[1][24:32] = v_mov_b32 %0:v[0][24:32] dst_sel:ubyte3 dst_preserve src0_sel:ubyte3
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(9u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_lo, v3b), Definition(v1_lo, v3b), Definition(v0_b3, v1b),
                 Operand(v1_lo, v3b), Operand(v0_lo, v3b), Operand(v1_b3, v1b));

      //~gfx[89]! p_unit_test 10
      //~gfx[89]! v1b: %0:v[1][8:16] = v_xor_b32 %0:v[1][8:16], %0:v[0][8:16] dst_sel:ubyte1 dst_preserve src0_sel:ubyte1 src1_sel:ubyte1
      //~gfx[89]! v1b: %0:v[0][8:16] = v_xor_b32 %0:v[1][8:16], %0:v[0][8:16] dst_sel:ubyte1 dst_preserve src0_sel:ubyte1 src1_sel:ubyte1
      //~gfx[89]! v1b: %0:v[1][8:16] = v_xor_b32 %0:v[1][8:16], %0:v[0][8:16] dst_sel:ubyte1 dst_preserve src0_sel:ubyte1 src1_sel:ubyte1
      //~gfx[89]! v1b: %0:v[1][16:24] = v_xor_b32 %0:v[1][16:24], %0:v[0][16:24] dst_sel:ubyte2 dst_preserve src0_sel:ubyte2 src1_sel:ubyte2
      //~gfx[89]! v1b: %0:v[0][16:24] = v_xor_b32 %0:v[1][16:24], %0:v[0][16:24] dst_sel:ubyte2 dst_preserve src0_sel:ubyte2 src1_sel:ubyte2
      //~gfx[89]! v1b: %0:v[1][16:24] = v_xor_b32 %0:v[1][16:24], %0:v[0][16:24] dst_sel:ubyte2 dst_preserve src0_sel:ubyte2 src1_sel:ubyte2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(10u));
      bld.pseudo(aco_opcode::p_parallelcopy,
                 Definition(v0_b1, v2b), Definition(v1_b1, v2b),
                 Operand(v1_b1, v2b), Operand(v0_b1, v2b));

      //~gfx[89]! p_unit_test 11
      //~gfx[89]! v2b: %0:v[1][0:16] = v_mov_b32 %0:v[0][16:32] dst_sel:uword0 dst_preserve src0_sel:uword1
      //~gfx[89]! v1: %0:v[0] = v_mov_b32 42
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(11u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v1), Definition(v1_lo, v2b),
                 Operand::c32(42u), Operand(v0_hi, v2b));

      //~gfx[89]! s_endpgm

      finish_to_hw_instr_test();
   }
END_TEST

BEGIN_TEST(to_hw_instr.subdword_constant)
   PhysReg v0_lo{256};
   PhysReg v0_hi{256};
   PhysReg v0_b1{256};
   PhysReg v1_lo{257};
   PhysReg v1_hi{257};
   v0_hi.reg_b += 2;
   v0_b1.reg_b += 1;
   v1_hi.reg_b += 2;

   for (unsigned i = GFX9; i <= GFX10; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

      /* 16-bit pack */
      //>> p_unit_test 0
      //! v1: %_:v[0] = v_pack_b32_f16 0.5, hi(%_:v[1][16:32])
      bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand::c16(0x3800), Operand(v1_hi, v2b));

      //! p_unit_test 1
      //~gfx9! v2b: %0:v[0][16:32] = v_and_b32 0xffff0000, %0:v[1][16:32]
      //~gfx9! v1: %0:v[0] = v_or_b32 0x4205, %0:v[0]
      //~gfx10! v1: %_:v[0] = v_pack_b32_f16 0x4205, hi(%_:v[1][16:32])
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(1u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand::c16(0x4205), Operand(v1_hi, v2b));

      //! p_unit_test 2
      //~gfx9! v2b: %0:v[0][16:32] = v_lshlrev_b32 16, %0:v[0][0:16]
      //~gfx9! v1: %_:v[0] = v_or_b32 0x4205, %_:v[0]
      //~gfx10! v1: %0:v[0] = v_pack_b32_f16 0x4205, %0:v[0][0:16]
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand::c16(0x4205), Operand(v0_lo, v2b));

      //! p_unit_test 3
      //! v1: %_:v[0] = v_mov_b32 0x3c003800
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(3u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand::c16(0x3800), Operand::c16(0x3c00));

      //! p_unit_test 4
      //! v1: %_:v[0] = v_mov_b32 0x43064205
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(4u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand::c16(0x4205), Operand::c16(0x4306));

      //! p_unit_test 5
      //! v1: %_:v[0] = v_mov_b32 0x38004205
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(5u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Definition(v0_hi, v2b),
                 Operand::c16(0x4205), Operand::c16(0x3800));

      /* 16-bit copy */
      //! p_unit_test 6
      //! v2b: %_:v[0][0:16] = v_add_f16 0.5, 0 dst_sel:uword0 dst_preserve src0_sel:uword0 src1_sel:dword
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(6u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Operand::c16(0x3800));

      //! p_unit_test 7
      //~gfx9! v1: %_:v[0] = v_and_b32 0xffff0000, %_:v[0]
      //~gfx9! v1: %_:v[0] = v_or_b32 0x4205, %_:v[0]
      //~gfx10! v2b: %_:v[0][0:16] = v_pack_b32_f16 0x4205, hi(%_:v[0][16:32])
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(7u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v2b), Operand::c16(0x4205));

      //! p_unit_test 8
      //~gfx9! v1: %_:v[0] = v_and_b32 0xffff, %_:v[0]
      //~gfx9! v1: %_:v[0] = v_or_b32 0x42050000, %_:v[0]
      //~gfx10! v2b: %_:v[0][16:32] = v_pack_b32_f16 %_:v[0][0:16], 0x4205
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(8u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_hi, v2b), Operand::c16(0x4205));

      //! p_unit_test 9
      //! v1b: %_:v[0][8:16] = v_mov_b32 0 dst_sel:ubyte1 dst_preserve src0_sel:dword
      //! v1b: %_:v[0][16:24] = v_mov_b32 56 dst_sel:ubyte2 dst_preserve src0_sel:dword
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(9u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_b1, v2b), Operand::c16(0x3800));

      //! p_unit_test 10
      //! v1b: %_:v[0][8:16] = v_mov_b32 5 dst_sel:ubyte1 dst_preserve src0_sel:dword
      //! v1b: %_:v[0][16:24] = v_mul_u32_u24 2, 33 dst_sel:ubyte2 dst_preserve src0_sel:dword src1_sel:dword
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(10u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_b1, v2b), Operand::c16(0x4205));

      /* 8-bit copy */
      //! p_unit_test 11
      //! v1b: %_:v[0][0:8] = v_mul_u32_u24 2, 33 dst_sel:ubyte0 dst_preserve src0_sel:dword src1_sel:dword
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(11u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v1b), Operand::c8(0x42));

      /* 32-bit and 8-bit copy */
      //! p_unit_test 12
      //! v1: %_:v[0] = v_mov_b32 0
      //! v1b: %_:v[1][0:8] = v_mov_b32 0 dst_sel:ubyte0 dst_preserve src0_sel:dword
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(12u));
      bld.pseudo(aco_opcode::p_parallelcopy, Definition(v0_lo, v1), Definition(v1_lo, v1b),
                 Operand::zero(), Operand::zero(1));

      //! s_endpgm

      finish_to_hw_instr_test();
   }
END_TEST

BEGIN_TEST(to_hw_instr.self_intersecting_swap)
   if (!setup_cs(NULL, GFX9))
      return;

   PhysReg reg_v1{257};
   PhysReg reg_v2{258};
   PhysReg reg_v3{259};
   PhysReg reg_v7{263};

   //>> p_unit_test 0
   //! v1: %0:v[1],  v1: %0:v[2] = v_swap_b32 %0:v[2], %0:v[1]
   //! v1: %0:v[2],  v1: %0:v[3] = v_swap_b32 %0:v[3], %0:v[2]
   //! v1: %0:v[3],  v1: %0:v[7] = v_swap_b32 %0:v[7], %0:v[3]
   //! s_endpgm
   bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
   //v[1:2] = v[2:3]
   //v3 = v7
   //v7 = v1
   bld.pseudo(aco_opcode::p_parallelcopy,
              Definition(reg_v1, v2), Definition(reg_v3, v1), Definition(reg_v7, v1),
              Operand(reg_v2, v2), Operand(reg_v7, v1), Operand(reg_v1, v1));

   finish_to_hw_instr_test();
END_TEST

BEGIN_TEST(to_hw_instr.extract)
   PhysReg s0_lo{0};
   PhysReg s1_lo{1};
   PhysReg v0_lo{256};
   PhysReg v1_lo{257};

   for (unsigned i = GFX7; i <= GFX9; i++) {
   for (unsigned is_signed = 0; is_signed <= 1; is_signed++) {
      if (!setup_cs(NULL, (chip_class)i, CHIP_UNKNOWN, is_signed ? "_signed" : "_unsigned"))
         continue;

#define EXT(idx, size)                                                                             \
   bld.pseudo(aco_opcode::p_extract, Definition(v0_lo, v1), Operand(v1_lo, v1), Operand::c32(idx), \
              Operand::c32(size), Operand::c32(is_signed));

      //; funcs['v_bfe'] = lambda _: 'v_bfe_i32' if variant.endswith('_signed') else 'v_bfe_u32'
      //; funcs['v_shr'] = lambda _: 'v_ashrrev_i32' if variant.endswith('_signed') else 'v_lshrrev_b32'
      //; funcs['s_bfe'] = lambda _: 's_bfe_i32' if variant.endswith('_signed') else 's_bfe_u32'
      //; funcs['s_shr'] = lambda _: 's_ashr_i32' if variant.endswith('_signed') else 's_lshr_b32'
      //; funcs['byte'] = lambda n: '%cbyte%s' % ('s' if variant.endswith('_signed') else 'u', n)

      //>> p_unit_test 0
      bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
      //! v1: %_:v[0] = @v_bfe %_:v[1], 0, 8
      EXT(0, 8)
      //! v1: %_:v[0] = @v_bfe %_:v[1], 8, 8
      EXT(1, 8)
      //! v1: %_:v[0] = @v_bfe %_:v[1], 16, 8
      EXT(2, 8)
      //! v1: %_:v[0] = @v_shr 24, %_:v[1]
      EXT(3, 8)
      //! v1: %_:v[0] = @v_bfe %_:v[1], 0, 16
      EXT(0, 16)
      //! v1: %_:v[0] = @v_shr 16, %_:v[1]
      EXT(1, 16)

      #undef EXT

#define EXT(idx, size)                                                                             \
   bld.pseudo(aco_opcode::p_extract, Definition(s0_lo, s1), Definition(scc, s1),                   \
              Operand(s1_lo, s1), Operand::c32(idx), Operand::c32(size), Operand::c32(is_signed));

      //>> p_unit_test 2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
      //~gfx._unsigned! s1: %_:s[0],  s1: %_:scc = @s_bfe %_:s[1], 0x80000
      //~gfx._signed! s1: %_:s[0] = s_sext_i32_i8 %_:s[1]
      EXT(0, 8)
      //! s1: %_:s[0],  s1: %_:scc = @s_bfe %_:s[1], 0x80008
      EXT(1, 8)
      //! s1: %_:s[0],  s1: %_:scc = @s_bfe %_:s[1], 0x80010
      EXT(2, 8)
      //! s1: %_:s[0],  s1: %_:scc = @s_shr %_:s[1], 24
      EXT(3, 8)
      //~gfx._unsigned! s1: %_:s[0],  s1: %_:scc = @s_bfe %_:s[1], 0x100000
      //~gfx._signed! s1: %_:s[0] = s_sext_i32_i16 %_:s[1]
      EXT(0, 16)
      //! s1: %_:s[0],  s1: %_:scc = @s_shr %_:s[1], 16
      EXT(1, 16)

      #undef EXT

#define EXT(idx, src_b)                                                                            \
   bld.pseudo(aco_opcode::p_extract, Definition(v0_lo, v2b), Operand(v1_lo.advance(src_b), v2b),   \
              Operand::c32(idx), Operand::c32(8u), Operand::c32(is_signed));

      //>> p_unit_test 4
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(4u));
      //~gfx7.*! v2b: %_:v[0][0:16] = @v_bfe %_:v[1][0:16], 0, 8
      //~gfx[^7].*! v2b: %_:v[0][0:16] = v_mov_b32 %_:v[1][0:16] dst_sel:uword0 dst_preserve src0_sel:@byte(0)
      EXT(0, 0)
      //~gfx[^7].*! v2b: %_:v[0][0:16] = v_mov_b32 %_:v[1][16:32] dst_sel:uword0 dst_preserve src0_sel:@byte(2)
      if (i != GFX7)
         EXT(0, 2)
      //~gfx7.*! v2b: %_:v[0][0:16] = @v_bfe %_:v[1][0:16], 8, 8
      //~gfx[^7].*! v2b: %_:v[0][0:16] = v_mov_b32 %_:v[1][0:16] dst_sel:uword0 dst_preserve src0_sel:@byte(1)
      EXT(1, 0)
      //~gfx[^7].*! v2b: %_:v[0][0:16] = v_mov_b32 %_:v[1][16:32] dst_sel:uword0 dst_preserve src0_sel:@byte(3)
      if (i != GFX7)
         EXT(1, 2)

      #undef EXT

      finish_to_hw_instr_test();

      //! s_endpgm
   }
   }
END_TEST

BEGIN_TEST(to_hw_instr.insert)
   PhysReg s0_lo{0};
   PhysReg s1_lo{1};
   PhysReg v0_lo{256};
   PhysReg v1_lo{257};

   for (unsigned i = GFX7; i <= GFX9; i++) {
      if (!setup_cs(NULL, (chip_class)i))
         continue;

#define INS(idx, size)                                                                             \
   bld.pseudo(aco_opcode::p_insert, Definition(v0_lo, v1), Operand(v1_lo, v1), Operand::c32(idx),  \
              Operand::c32(size));

      //>> p_unit_test 0
      bld.pseudo(aco_opcode::p_unit_test, Operand::zero());
      //! v1: %_:v[0] = v_bfe_u32 %_:v[1], 0, 8
      INS(0, 8)
      //~gfx7! v1: %0:v[0] = v_bfe_u32 %0:v[1], 0, 8
      //~gfx7! v1: %0:v[0] = v_lshlrev_b32 8, %0:v[0]
      //~gfx[^7]! v1: %0:v[0] = v_mov_b32 %0:v[1] dst_sel:ubyte1 src0_sel:dword
      INS(1, 8)
      //~gfx7! v1: %0:v[0] = v_bfe_u32 %0:v[1], 0, 8
      //~gfx7! v1: %0:v[0] = v_lshlrev_b32 16, %0:v[0]
      //~gfx[^7]! v1: %0:v[0] = v_mov_b32 %0:v[1] dst_sel:ubyte2 src0_sel:dword
      INS(2, 8)
      //! v1: %0:v[0] = v_lshlrev_b32 24, %0:v[1]
      INS(3, 8)
      //! v1: %0:v[0] = v_bfe_u32 %0:v[1], 0, 16
      INS(0, 16)
      //! v1: %0:v[0] = v_lshlrev_b32 16, %0:v[1]
      INS(1, 16)

      #undef INS

#define INS(idx, size)                                                                             \
   bld.pseudo(aco_opcode::p_insert, Definition(s0_lo, s1), Definition(scc, s1),                    \
              Operand(s1_lo, s1), Operand::c32(idx), Operand::c32(size));

      //>> p_unit_test 1
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(1u));
      //! s1: %_:s[0],  s1: %_:scc = s_bfe_u32 %_:s[1], 0x80000
      INS(0, 8)
      //! s1: %_:s[0],  s1: %_:scc = s_bfe_u32 %_:s[1], 0x80000
      //! s1: %_:s[0],  s1: %_:scc = s_lshl_b32 %_:s[0], 8
      INS(1, 8)
      //! s1: %_:s[0],  s1: %_:scc = s_bfe_u32 %_:s[1], 0x80000
      //! s1: %_:s[0],  s1: %_:scc = s_lshl_b32 %_:s[0], 16
      INS(2, 8)
      //! s1: %_:s[0],  s1: %_:scc = s_lshl_b32 %_:s[1], 24
      INS(3, 8)
      //! s1: %_:s[0],  s1: %_:scc = s_bfe_u32 %_:s[1], 0x100000
      INS(0, 16)
      //! s1: %_:s[0],  s1: %_:scc = s_lshl_b32 %_:s[1], 16
      INS(1, 16)

      #undef INS

#define INS(idx, def_b)                                                                            \
   bld.pseudo(aco_opcode::p_insert, Definition(v0_lo.advance(def_b), v2b), Operand(v1_lo, v2b),    \
              Operand::c32(idx), Operand::c32(8u));

      //>> p_unit_test 2
      bld.pseudo(aco_opcode::p_unit_test, Operand::c32(2u));
      //~gfx7! v2b: %_:v[0][0:16] = v_bfe_u32 %_:v[1][0:16], 0, 8
      //~gfx[^7]! v2b: %0:v[0][0:16] = v_lshlrev_b32 0, %0:v[1][0:16] dst_sel:uword0 dst_preserve src0_sel:dword src1_sel:ubyte0
      INS(0, 0)
      //~gfx[^7]! v2b: %0:v[0][16:32] = v_lshlrev_b32 0, %0:v[1][0:16] dst_sel:uword1 dst_preserve src0_sel:dword src1_sel:ubyte0
      if (i != GFX7)
         INS(0, 2)
      //~gfx7! v2b: %_:v[0][0:16] = v_lshlrev_b32 8, %_:v[1][0:16]
      //~gfx[^7]! v2b: %0:v[0][0:16] = v_lshlrev_b32 8, %0:v[1][0:16] dst_sel:uword0 dst_preserve src0_sel:dword src1_sel:ubyte0
      INS(1, 0)
      //~gfx[^7]! v2b: %0:v[0][16:32] = v_lshlrev_b32 8, %0:v[1][0:16] dst_sel:uword1 dst_preserve src0_sel:dword src1_sel:ubyte0
      if (i != GFX7)
         INS(1, 2)

      #undef INS

      finish_to_hw_instr_test();

      //! s_endpgm
   }
END_TEST

BEGIN_TEST(to_hw_instr.copy_linear_vgpr_scc)
   if (!setup_cs(NULL, GFX10))
      return;

   PhysReg reg_s0{0};
   PhysReg reg_s1{1};
   PhysReg v0_lo{256};
   PhysReg v0_b3{256};
   v0_b3.reg_b += 3;
   PhysReg v1_lo{257};

   //>> p_unit_test 0
   bld.pseudo(aco_opcode::p_unit_test, Operand::zero());

   /* It would be better if the scc=s0 copy was done later, but handle_operands() is complex
    * enough
    */

   //! s1: %0:scc = s_cmp_lg_i32 %0:s[0], 0
   //! s1: %0:m0 = s_mov_b32 %0:scc
   //! lv1: %0:v[0] = v_mov_b32 %0:v[1]
   //! s2: %0:exec,  s1: %0:scc = s_not_b64 %0:exec
   //! lv1: %0:v[0] = v_mov_b32 %0:v[1]
   //! s2: %0:exec,  s1: %0:scc = s_not_b64 %0:exec
   //! s1: %0:scc = s_cmp_lg_i32 %0:m0, 0
   Instruction *instr = bld.pseudo(
      aco_opcode::p_parallelcopy,
      Definition(scc, s1), Definition(v0_lo, v1.as_linear()),
      Operand(reg_s0, s1), Operand(v1_lo, v1.as_linear()));
   instr->pseudo().scratch_sgpr = m0;

   finish_to_hw_instr_test();
END_TEST

BEGIN_TEST(to_hw_instr.swap_linear_vgpr)
   if (!setup_cs(NULL, GFX10))
      return;

   PhysReg reg_v0{256};
   PhysReg reg_v1{257};
   RegClass v1_linear = v1.as_linear();

   //>> p_unit_test 0
   bld.pseudo(aco_opcode::p_unit_test, Operand::zero());

   Instruction *instr = bld.pseudo(
      aco_opcode::p_parallelcopy,
      Definition(reg_v0, v1_linear), Definition(reg_v1, v1_linear),
      Operand(reg_v1, v1_linear), Operand(reg_v0, v1_linear));
   instr->pseudo().scratch_sgpr = m0;

   finish_to_hw_instr_test();
END_TEST
