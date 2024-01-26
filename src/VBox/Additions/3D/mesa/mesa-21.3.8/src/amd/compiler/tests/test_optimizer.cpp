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

BEGIN_TEST(optimize.neg)
   for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, s1: %c, s1: %d = p_startpgm
      if (!setup_cs("v1 v1 s1 s1", (chip_class)i))
         continue;

      //! v1: %res0 = v_mul_f32 %a, -%b
      //! p_unit_test 0, %res0
      Temp neg_b = fneg(inputs[1]);
      writeout(0, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), inputs[0], neg_b));

      //~gfx9! v1: %neg_a = v_mul_f32 -1.0, %a
      //~gfx9! v1: %res1 = v_mul_f32 0x123456, %neg_a
      //~gfx10! v1: %res1 = v_mul_f32 0x123456, -%a
      //! p_unit_test 1, %res1
      Temp neg_a = fneg(inputs[0]);
      writeout(1, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x123456u), neg_a));

      //! v1: %res2 = v_mul_f32 %a, %b
      //! p_unit_test 2, %res2
      Temp neg_neg_a = fneg(neg_a);
      writeout(2, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), neg_neg_a, inputs[1]));

      //! v1: %res3 = v_mul_f32 |%a|, %b
      //! p_unit_test 3, %res3
      Temp abs_neg_a = fabs(neg_a);
      writeout(3, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), abs_neg_a, inputs[1]));

      //! v1: %res4 = v_mul_f32 -|%a|, %b
      //! p_unit_test 4, %res4
      Temp abs_a = fabs(inputs[0]);
      Temp neg_abs_a = fneg(abs_a);
      writeout(4, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), neg_abs_a, inputs[1]));

      //! v1: %res5 = v_mul_f32 -%a, %b row_shl:1 bound_ctrl:1
      //! p_unit_test 5, %res5
      writeout(5, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), neg_a, inputs[1], dpp_row_sl(1)));

      //! v1: %res6 = v_subrev_f32 %a, %b
      //! p_unit_test 6, %res6
      writeout(6, bld.vop2(aco_opcode::v_add_f32, bld.def(v1), neg_a, inputs[1]));

      //! v1: %res7 = v_sub_f32 %b, %a
      //! p_unit_test 7, %res7
      writeout(7, bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[1], neg_a));

      //! v1: %res8 = v_mul_f32 %a, -%c
      //! p_unit_test 8, %res8
      Temp neg_c = fneg(bld.copy(bld.def(v1), inputs[2]));
      writeout(8, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), inputs[0], neg_c));

      // //! v1: %res9 = v_mul_f32 |%neg_a|, %b
      // //! p_unit_test 9, %res9
      Temp abs_neg_abs_a = fabs(neg_abs_a);
      writeout(9, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), abs_neg_abs_a, inputs[1]));

      finish_opt_test();
   }
END_TEST

BEGIN_TEST(optimize.output_modifiers)
   //>> v1: %a, v1: %b = p_startpgm
   if (!setup_cs("v1 v1", GFX9))
      return;

   program->blocks[0].fp_mode.denorm16_64 = fp_denorm_flush;

   /* 32-bit modifiers */

   //! v1: %res0 = v_add_f32 %a, %b *0.5
   //! p_unit_test 0, %res0
   Temp tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(0, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x3f000000u), tmp));

   //! v1: %res1 = v_add_f32 %a, %b *2
   //! p_unit_test 1, %res1
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(1, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));

   //! v1: %res2 = v_add_f32 %a, %b *4
   //! p_unit_test 2, %res2
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(2, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40800000u), tmp));

   //! v1: %res3 = v_add_f32 %a, %b clamp
   //! p_unit_test 3, %res3
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(3, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
                        Operand::c32(0x3f800000u), tmp));

   //! v1: %res4 = v_add_f32 %a, %b *2 clamp
   //! p_unit_test 4, %res4
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   tmp = bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp);
   writeout(4, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
                        Operand::c32(0x3f800000u), tmp));

   /* 16-bit modifiers */

   //! v2b: %res5 = v_add_f16 %a, %b *0.5
   //! p_unit_test 5, %res5
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(5, bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x3800u), tmp));

   //! v2b: %res6 = v_add_f16 %a, %b *2
   //! p_unit_test 6, %res6
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(6, bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000u), tmp));

   //! v2b: %res7 = v_add_f16 %a, %b *4
   //! p_unit_test 7, %res7
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(7, bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4400u), tmp));

   //! v2b: %res8 = v_add_f16 %a, %b clamp
   //! p_unit_test 8, %res8
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(8, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
                        Operand::c16(0x3c00u), tmp));

   //! v2b: %res9 = v_add_f16 %a, %b *2 clamp
   //! p_unit_test 9, %res9
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   tmp = bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000), tmp);
   writeout(9, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
                        Operand::c16(0x3c00u), tmp));

   /* clamping is done after omod */

   //! v1: %res10_tmp = v_add_f32 %a, %b clamp
   //! v1: %res10 = v_mul_f32 2.0, %res10_tmp
   //! p_unit_test 10, %res10
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   tmp = bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(), Operand::c32(0x3f800000u),
                  tmp);
   writeout(10, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));

   /* unsupported instructions */

   //! v1: %res11_tmp = v_xor_b32 %a, %b
   //! v1: %res11 = v_mul_f32 2.0, %res11_tmp
   //! p_unit_test 11, %res11
   tmp = bld.vop2(aco_opcode::v_xor_b32, bld.def(v1), inputs[0], inputs[1]);
   writeout(11, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));

   /* several users */

   //! v1: %res12_tmp = v_add_f32 %a, %b
   //! p_unit_test %res12_tmp
   //! v1: %res12 = v_mul_f32 2.0, %res12_tmp
   //! p_unit_test 12, %res12
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   bld.pseudo(aco_opcode::p_unit_test, tmp);
   writeout(12, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));

   //! v1: %res13 = v_add_f32 %a, %b
   //! p_unit_test 13, %res13
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp);
   writeout(13, tmp);

   /* omod has no effect if denormals are enabled but clamp is fine */

   //>> BB1
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm32 = fp_denorm_keep;
   program->next_fp_mode.denorm16_64 = fp_denorm_flush;
   bld.reset(program->create_and_insert_block());

   //! v1: %res14_tmp = v_add_f32 %a, %b
   //! v1: %res14 = v_mul_f32 2.0, %res13_tmp
   //! p_unit_test 14, %res14
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(14, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));

   //! v1: %res15 = v_add_f32 %a, %b clamp
   //! p_unit_test 15, %res15
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(15, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
                         Operand::c32(0x3f800000u), tmp));

   //>> BB2
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm32 = fp_denorm_flush;
   program->next_fp_mode.denorm16_64 = fp_denorm_keep;
   bld.reset(program->create_and_insert_block());

   //! v2b: %res16_tmp = v_add_f16 %a, %b
   //! v2b: %res16 = v_mul_f16 2.0, %res15_tmp
   //! p_unit_test 16, %res16
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(16, bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000u), tmp));

   //! v2b: %res17 = v_add_f16 %a, %b clamp
   //! p_unit_test 17, %res17
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(17, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
                         Operand::c16(0x3c00u), tmp));

   /* omod flushes -0.0 to +0.0 */

   //>> BB3
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm32 = fp_denorm_keep;
   program->next_fp_mode.denorm16_64 = fp_denorm_keep;
   program->next_fp_mode.preserve_signed_zero_inf_nan32 = true;
   program->next_fp_mode.preserve_signed_zero_inf_nan16_64 = false;
   bld.reset(program->create_and_insert_block());

   //! v1: %res18_tmp = v_add_f32 %a, %b
   //! v1: %res18 = v_mul_f32 2.0, %res18_tmp
   //! p_unit_test 18, %res18
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(18, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));
   //! v1: %res19 = v_add_f32 %a, %b clamp
   //! p_unit_test 19, %res19
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(19, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
                         Operand::c32(0x3f800000u), tmp));

   //>> BB4
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.preserve_signed_zero_inf_nan32 = false;
   program->next_fp_mode.preserve_signed_zero_inf_nan16_64 = true;
   bld.reset(program->create_and_insert_block());
   //! v2b: %res20_tmp = v_add_f16 %a, %b
   //! v2b: %res20 = v_mul_f16 2.0, %res20_tmp
   //! p_unit_test 20, %res20
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(20, bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000u), tmp));
   //! v2b: %res21 = v_add_f16 %a, %b clamp
   //! p_unit_test 21, %res21
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(21, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
                         Operand::c16(0x3c00u), tmp));

   finish_opt_test();
END_TEST

Temp create_subbrev_co(Operand op0, Operand op1, Operand op2)
{
   return bld.vop2_e64(aco_opcode::v_subbrev_co_u32, bld.def(v1), bld.hint_vcc(bld.def(bld.lm)), op0, op1, op2);
}

BEGIN_TEST(optimize.cndmask)
   for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, s1: %b, s2: %c = p_startpgm
      if (!setup_cs("v1 s1 s2", (chip_class)i))
         continue;

      Temp subbrev;

      //! v1: %res0 = v_cndmask_b32 0, %a, %c
      //! p_unit_test 0, %res0
      subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
      writeout(0, bld.vop2(aco_opcode::v_and_b32, bld.def(v1), inputs[0], subbrev));

      //! v1: %res1 = v_cndmask_b32 0, 42, %c
      //! p_unit_test 1, %res1
      subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
      writeout(1, bld.vop2(aco_opcode::v_and_b32, bld.def(v1), Operand::c32(42u), subbrev));

      //~gfx9! v1: %subbrev, s2: %_ = v_subbrev_co_u32 0, 0, %c
      //~gfx9! v1: %res2 = v_and_b32 %b, %subbrev
      //~gfx10! v1: %res2 = v_cndmask_b32 0, %b, %c
      //! p_unit_test 2, %res2
      subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
      writeout(2, bld.vop2(aco_opcode::v_and_b32, bld.def(v1), inputs[1], subbrev));

      //! v1: %subbrev1, s2: %_ = v_subbrev_co_u32 0, 0, %c
      //! v1: %xor = v_xor_b32 %a, %subbrev1
      //! v1: %res3 = v_cndmask_b32 0, %xor, %c
      //! p_unit_test 3, %res3
      subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
      Temp xor_a = bld.vop2(aco_opcode::v_xor_b32, bld.def(v1), inputs[0], subbrev);
      writeout(3, bld.vop2(aco_opcode::v_and_b32, bld.def(v1), xor_a, subbrev));

      //! v1: %res4 = v_cndmask_b32 0, %a, %c
      //! p_unit_test 4, %res4
      Temp cndmask = bld.vop2_e64(aco_opcode::v_cndmask_b32, bld.def(v1), Operand::zero(),
                                  Operand::c32(1u), Operand(inputs[2]));
      Temp sub = bld.vsub32(bld.def(v1), Operand::zero(), cndmask);
      writeout(4, bld.vop2(aco_opcode::v_and_b32, bld.def(v1), Operand(inputs[0]), sub));

      finish_opt_test();
   }
END_TEST

BEGIN_TEST(optimize.add_lshl)
   for (unsigned i = GFX8; i <= GFX10; i++) {
      //>> s1: %a, v1: %b = p_startpgm
      if (!setup_cs("s1 v1", (chip_class)i))
         continue;

      Temp shift;

      //~gfx8! s1: %lshl0, s1: %_:scc = s_lshl_b32 %a, 3
      //~gfx8! s1: %res0, s1: %_:scc = s_add_u32 %lshl0, 4
      //~gfx(9|10)! s1: %res0, s1: %_:scc = s_lshl3_add_u32 %a, 4
      //! p_unit_test 0, %res0
      shift = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), bld.def(s1, scc), Operand(inputs[0]),
                       Operand::c32(3u));
      writeout(0, bld.sop2(aco_opcode::s_add_u32, bld.def(s1), bld.def(s1, scc), shift,
                           Operand::c32(4u)));

      //~gfx8! s1: %lshl1, s1: %_:scc = s_lshl_b32 %a, 3
      //~gfx8! s1: %add1, s1: %_:scc = s_add_u32 %lshl1, 4
      //~gfx8! v1: %add_co1, s2: %_ = v_add_co_u32 %lshl1, %b
      //~gfx8! v1: %res1, s2: %_ = v_add_co_u32 %add1, %add_co1
      //~gfx(9|10)! s1: %lshl1, s1: %_:scc = s_lshl3_add_u32 %a, 4
      //~gfx(9|10)! v1: %lshl_add = v_lshl_add_u32 %a, 3, %b
      //~gfx(9|10)! v1: %res1 = v_add_u32 %lshl1, %lshl_add
      //! p_unit_test 1, %res1
      shift = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), bld.def(s1, scc), Operand(inputs[0]),
                       Operand::c32(3u));
      Temp sadd =
         bld.sop2(aco_opcode::s_add_u32, bld.def(s1), bld.def(s1, scc), shift, Operand::c32(4u));
      Temp vadd = bld.vadd32(bld.def(v1), shift, Operand(inputs[1]));
      writeout(1, bld.vadd32(bld.def(v1), sadd, vadd));

      //~gfx8! s1: %lshl2 = s_lshl_b32 %a, 3
      //~gfx8! v1: %res2,  s2: %_ = v_add_co_u32 %lshl2, %b
      //~gfx(9|10)! v1: %res2 = v_lshl_add_u32 %a, 3, %b
      //! p_unit_test 2, %res2
      Temp lshl =
         bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), Operand(inputs[0]), Operand::c32(3u));
      writeout(2, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! s1: %lshl3 = s_lshl_b32 (is24bit)%a, 7
      //~gfx8! v1: %res3, s2: %_ = v_add_co_u32 %lshl3, %b
      //~gfx(9|10)! v1: %res3 = v_lshl_add_u32 (is24bit)%a, 7, %b
      //! p_unit_test 3, %res3
      Operand a_24bit = Operand(inputs[0]);
      a_24bit.set24bit(true);
      lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, Operand::c32(7u));
      writeout(3, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //! s1: %lshl4 = s_lshl_b32 (is24bit)%a, 3
      //~gfx(8|9)! v1: %res4, s2: %carry = v_add_co_u32 %lshl4, %b
      //~gfx10! v1: %res4, s2: %carry = v_add_co_u32_e64 %lshl4, %b
      //! p_unit_test 4, %carry
      lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, Operand::c32(3u));
      Temp carry = bld.vadd32(bld.def(v1), lshl, Operand(inputs[1]), true).def(1).getTemp();
      writeout(4, carry);

      //~gfx8! s1: %lshl5 = s_lshl_b32 (is24bit)%a, (is24bit)%a
      //~gfx8! v1: %res5, s2: %_ = v_add_co_u32 %lshl5, %b
      //~gfx(9|10)! v1: %res5 = v_lshl_add_u32 (is24bit)%a, (is24bit)%a, %b
      //! p_unit_test 5, %res5
      lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, a_24bit);
      writeout(5, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %res6 = v_mad_u32_u24 (is24bit)%a, 8, %b
      //~gfx(9|10)! v1: %res6 = v_lshl_add_u32 (is24bit)%a, 3, %b
      //! p_unit_test 6, %res6
      lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, Operand::c32(3u));
      writeout(6, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %res7 = v_mad_u32_u24 (is16bit)%a, 16, %b
      //~gfx(9|10)! v1: %res7 = v_lshl_add_u32 (is16bit)%a, 4, %b
      //! p_unit_test 7, %res7
      Operand a_16bit = Operand(inputs[0]);
      a_16bit.set16bit(true);
      lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_16bit, Operand::c32(4u));
      writeout(7, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      finish_opt_test();
   }
END_TEST

BEGIN_TEST(optimize.bcnt)
   for (unsigned i = GFX8; i <= GFX10; i++) {
      //>> v1: %a, s1: %b = p_startpgm
      if (!setup_cs("v1 s1", (chip_class)i))
         continue;

      Temp bcnt;

      //! v1: %res0 = v_bcnt_u32_b32 %a, %a
      //! p_unit_test 0, %res0
      bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
      writeout(0, bld.vadd32(bld.def(v1), bcnt, Operand(inputs[0])));

      //! v1: %res1 = v_bcnt_u32_b32 %a, %b
      //! p_unit_test 1, %res1
      bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
      writeout(1, bld.vadd32(bld.def(v1), bcnt, Operand(inputs[1])));

      //! v1: %res2 = v_bcnt_u32_b32 %a, 42
      //! p_unit_test 2, %res2
      bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
      writeout(2, bld.vadd32(bld.def(v1), bcnt, Operand::c32(42u)));

      //! v1: %bnct3 = v_bcnt_u32_b32 %b, 0
      //~gfx8! v1: %res3, s2: %_ = v_add_co_u32 %bcnt3, %a
      //~gfx(9|10)! v1: %res3 = v_add_u32 %bcnt3, %a
      //! p_unit_test 3, %res3
      bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[1]), Operand::zero());
      writeout(3, bld.vadd32(bld.def(v1), bcnt, Operand(inputs[0])));

      //! v1: %bnct4 = v_bcnt_u32_b32 %a, 0
      //~gfx(8|9)! v1: %add4, s2: %carry = v_add_co_u32 %bcnt4, %a
      //~gfx10! v1: %add4, s2: %carry = v_add_co_u32_e64 %bcnt4, %a
      //! p_unit_test 4, %carry
      bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
      Temp carry = bld.vadd32(bld.def(v1), bcnt, Operand(inputs[0]), true).def(1).getTemp();
      writeout(4, carry);

      finish_opt_test();
   }
END_TEST

struct clamp_config {
   const char *name;
   aco_opcode min, max, med3;
   Operand lb, ub;
};

static const clamp_config clamp_configs[] = {
   /* 0.0, 4.0 */
   {"_0,4f32", aco_opcode::v_min_f32, aco_opcode::v_max_f32, aco_opcode::v_med3_f32,
    Operand::zero(), Operand::c32(0x40800000u)},
   {"_0,4f16", aco_opcode::v_min_f16, aco_opcode::v_max_f16, aco_opcode::v_med3_f16,
    Operand::c16(0u), Operand::c16(0x4400)},
   /* -1.0, 0.0 */
   {"_-1,0f32", aco_opcode::v_min_f32, aco_opcode::v_max_f32, aco_opcode::v_med3_f32,
    Operand::c32(0xbf800000u), Operand::zero()},
   {"_-1,0f16", aco_opcode::v_min_f16, aco_opcode::v_max_f16, aco_opcode::v_med3_f16,
    Operand::c16(0xBC00), Operand::c16(0u)},
   /* 0, 3 */
   {"_0,3u32", aco_opcode::v_min_u32, aco_opcode::v_max_u32, aco_opcode::v_med3_u32,
    Operand::zero(), Operand::c32(3u)},
   {"_0,3u16", aco_opcode::v_min_u16, aco_opcode::v_max_u16, aco_opcode::v_med3_u16,
    Operand::c16(0u), Operand::c16(3u)},
   {"_0,3i32", aco_opcode::v_min_i32, aco_opcode::v_max_i32, aco_opcode::v_med3_i32,
    Operand::zero(), Operand::c32(3u)},
   {"_0,3i16", aco_opcode::v_min_i16, aco_opcode::v_max_i16, aco_opcode::v_med3_i16,
    Operand::c16(0u), Operand::c16(3u)},
   /* -5, 0 */
   {"_-5,0i32", aco_opcode::v_min_i32, aco_opcode::v_max_i32, aco_opcode::v_med3_i32,
    Operand::c32(0xfffffffbu), Operand::zero()},
   {"_-5,0i16", aco_opcode::v_min_i16, aco_opcode::v_max_i16, aco_opcode::v_med3_i16,
    Operand::c16(0xfffbu), Operand::c16(0u)},
};

BEGIN_TEST(optimize.clamp)
   for (clamp_config cfg : clamp_configs) {
      if (!setup_cs("v1 v1 v1", GFX9, CHIP_UNKNOWN, cfg.name))
         continue;

      //! cfg: @match_func(min max med3 lb ub)
      fprintf(output, "cfg: %s ", instr_info.name[(int)cfg.min]);
      fprintf(output, "%s ", instr_info.name[(int)cfg.max]);
      fprintf(output, "%s ", instr_info.name[(int)cfg.med3]);
      aco_print_operand(&cfg.lb, output);
      fprintf(output, " ");
      aco_print_operand(&cfg.ub, output);
      fprintf(output, "\n");

      //>> v1: %a, v1: %b, v1: %c = p_startpgm

      //! v1: %res0 = @med3 @ub, @lb, %a
      //! p_unit_test 0, %res0
      writeout(0, bld.vop2(cfg.min, bld.def(v1), cfg.ub,
                           bld.vop2(cfg.max, bld.def(v1), cfg.lb, inputs[0])));

      //! v1: %res1 = @med3 @lb, @ub, %a
      //! p_unit_test 1, %res1
      writeout(1, bld.vop2(cfg.max, bld.def(v1), cfg.lb,
                           bld.vop2(cfg.min, bld.def(v1), cfg.ub, inputs[0])));

      /* min constant must be greater than max constant */
      //! v1: %res2_tmp = @min @lb, %a
      //! v1: %res2 = @max @ub, %res2_tmp
      //! p_unit_test 2, %res2
      writeout(2, bld.vop2(cfg.max, bld.def(v1), cfg.ub,
                           bld.vop2(cfg.min, bld.def(v1), cfg.lb, inputs[0])));

      //! v1: %res3_tmp = @max @ub, %a
      //! v1: %res3 = @min @lb, %res3_tmp
      //! p_unit_test 3, %res3
      writeout(3, bld.vop2(cfg.min, bld.def(v1), cfg.lb,
                           bld.vop2(cfg.max, bld.def(v1), cfg.ub, inputs[0])));

      /* needs two constants */

      //! v1: %res4_tmp = @max @lb, %a
      //! v1: %res4 = @min %b, %res4_tmp
      //! p_unit_test 4, %res4
      writeout(4, bld.vop2(cfg.min, bld.def(v1), inputs[1],
                           bld.vop2(cfg.max, bld.def(v1), cfg.lb, inputs[0])));

      //! v1: %res5_tmp = @max %b, %a
      //! v1: %res5 = @min @ub, %res5_tmp
      //! p_unit_test 5, %res5
      writeout(5, bld.vop2(cfg.min, bld.def(v1), cfg.ub,
                           bld.vop2(cfg.max, bld.def(v1), inputs[1], inputs[0])));

      //! v1: %res6_tmp = @max %c, %a
      //! v1: %res6 = @min %b, %res6_tmp
      //! p_unit_test 6, %res6
      writeout(6, bld.vop2(cfg.min, bld.def(v1), inputs[1],
                           bld.vop2(cfg.max, bld.def(v1), inputs[2], inputs[0])));

      /* correct NaN behaviour with precise */

      //! v1: %res7 = @med3 @ub, @lb, %a
      //! p_unit_test 7, %res7
      Builder::Result max = bld.vop2(cfg.max, bld.def(v1), cfg.lb, inputs[0]);
      max.def(0).setPrecise(true);
      Builder::Result min = bld.vop2(cfg.min, bld.def(v1), cfg.ub, max);
      max.def(0).setPrecise(true);
      writeout(7, min);

      //! v1: (precise)%res8_tmp = @min @ub, %a
      //! v1: %res8 = @max @lb, %res8_tmp
      //! p_unit_test 8, %res8
      min = bld.vop2(cfg.min, bld.def(v1), cfg.ub, inputs[0]);
      min.def(0).setPrecise(true);
      writeout(8, bld.vop2(cfg.max, bld.def(v1), cfg.lb, min));

      finish_opt_test();
   }
END_TEST

BEGIN_TEST(optimize.const_comparison_ordering)
   //>> v1: %a, v1: %b, v2: %c, v1: %d = p_startpgm
   if (!setup_cs("v1 v1 v2 v1", GFX9))
      return;

   /* optimize to unordered comparison */
   //! s2: %res0 = v_cmp_nge_f32 4.0, %a
   //! p_unit_test 0, %res0
   writeout(0, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]),
                        bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm),
                                 Operand::c32(0x40800000u), inputs[0])));

   //! s2: %res1 = v_cmp_nge_f32 4.0, %a
   //! p_unit_test 1, %res1
   writeout(1, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]),
                        bld.vopc(aco_opcode::v_cmp_nge_f32, bld.def(bld.lm),
                                 Operand::c32(0x40800000u), inputs[0])));

   //! s2: %res2 = v_cmp_nge_f32 0x40a00000, %a
   //! p_unit_test 2, %res2
   writeout(2, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]),
                        bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm),
                                 bld.copy(bld.def(v1), Operand::c32(0x40a00000u)), inputs[0])));

   /* optimize to ordered comparison */
   //! s2: %res3 = v_cmp_lt_f32 4.0, %a
   //! p_unit_test 3, %res3
   writeout(3, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc),
                        bld.vopc(aco_opcode::v_cmp_eq_f32, bld.def(bld.lm), inputs[0], inputs[0]),
                        bld.vopc(aco_opcode::v_cmp_nge_f32, bld.def(bld.lm),
                                 Operand::c32(0x40800000u), inputs[0])));

   //! s2: %res4 = v_cmp_lt_f32 4.0, %a
   //! p_unit_test 4, %res4
   writeout(4, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc),
                        bld.vopc(aco_opcode::v_cmp_eq_f32, bld.def(bld.lm), inputs[0], inputs[0]),
                        bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm),
                                 Operand::c32(0x40800000u), inputs[0])));

   //! s2: %res5 = v_cmp_lt_f32 0x40a00000, %a
   //! p_unit_test 5, %res5
   writeout(5, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc),
                        bld.vopc(aco_opcode::v_cmp_eq_f32, bld.def(bld.lm), inputs[0], inputs[0]),
                        bld.vopc(aco_opcode::v_cmp_nge_f32, bld.def(bld.lm),
                                 bld.copy(bld.def(v1), Operand::c32(0x40a00000u)), inputs[0])));

   /* similar but unoptimizable expressions */
   //! s2: %tmp6_0 = v_cmp_lt_f32 4.0, %a
   //! s2: %tmp6_1 = v_cmp_neq_f32 %a, %a
   //! s2: %res6, s1: %_:scc = s_and_b64 %tmp6_1, %tmp6_0
   //! p_unit_test 6, %res6
   Temp src1 =
      bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(0x40800000u), inputs[0]);
   Temp src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
   writeout(6, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   //! s2: %tmp7_0 = v_cmp_nge_f32 4.0, %a
   //! s2: %tmp7_1 = v_cmp_eq_f32 %a, %a
   //! s2: %res7, s1: %_:scc = s_or_b64 %tmp7_1, %tmp7_0
   //! p_unit_test 7, %res7
   src1 =
      bld.vopc(aco_opcode::v_cmp_nge_f32, bld.def(bld.lm), Operand::c32(0x40800000u), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_eq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
   writeout(7, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   //! s2: %tmp8_0 = v_cmp_lt_f32 4.0, %d
   //! s2: %tmp8_1 = v_cmp_neq_f32 %a, %a
   //! s2: %res8, s1: %_:scc = s_or_b64 %tmp8_1, %tmp8_0
   //! p_unit_test 8, %res8
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(0x40800000u), inputs[3]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
   writeout(8, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   //! s2: %tmp9_0 = v_cmp_lt_f32 4.0, %a
   //! s2: %tmp9_1 = v_cmp_neq_f32 %a, %d
   //! s2: %res9, s1: %_:scc = s_or_b64 %tmp9_1, %tmp9_0
   //! p_unit_test 9, %res9
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(0x40800000u), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[3]);
   writeout(9, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   /* bit sizes */
   //! s2: %res10 = v_cmp_nge_f16 4.0, %b
   //! p_unit_test 10, %res10
   Temp input1_16 =
      bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), inputs[1], Operand::zero());
   writeout(10, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                         bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), input1_16, input1_16),
                         bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), Operand::c16(0x4400u),
                                  input1_16)));

   //! s2: %res11 = v_cmp_nge_f64 4.0, %c
   //! p_unit_test 11, %res11
   writeout(11, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                         bld.vopc(aco_opcode::v_cmp_neq_f64, bld.def(bld.lm), inputs[2], inputs[2]),
                         bld.vopc(aco_opcode::v_cmp_lt_f64, bld.def(bld.lm),
                                  Operand::c64(0x4010000000000000u), inputs[2])));

   /* NaN */
   uint16_t nan16 = 0x7e00;
   uint32_t nan32 = 0x7fc00000;
   uint64_t nan64 = 0xffffffffffffffffllu;

   //! s2: %tmp12_0 = v_cmp_lt_f16 0x7e00, %a
   //! s2: %tmp12_1 = v_cmp_neq_f16 %a, %a
   //! s2: %res12, s1: %_:scc = s_or_b64 %tmp12_1, %tmp12_0
   //! p_unit_test 12, %res12
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), Operand::c16(nan16), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), inputs[0], inputs[0]);
   writeout(12, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   //! s2: %tmp13_0 = v_cmp_lt_f32 0x7fc00000, %a
   //! s2: %tmp13_1 = v_cmp_neq_f32 %a, %a
   //! s2: %res13, s1: %_:scc = s_or_b64 %tmp13_1, %tmp13_0
   //! p_unit_test 13, %res13
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(nan32), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
   writeout(13, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   //! s2: %tmp14_0 = v_cmp_lt_f64 -1, %a
   //! s2: %tmp14_1 = v_cmp_neq_f64 %a, %a
   //! s2: %res14, s1: %_:scc = s_or_b64 %tmp14_1, %tmp14_0
   //! p_unit_test 14, %res14
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f64, bld.def(bld.lm), Operand::c64(nan64), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f64, bld.def(bld.lm), inputs[0], inputs[0]);
   writeout(14, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc), src0, src1));

   finish_opt_test();
END_TEST

BEGIN_TEST(optimize.add3)
   //>> v1: %a, v1: %b, v1: %c = p_startpgm
   if (!setup_cs("v1 v1 v1", GFX9))
      return;

   //! v1: %res0 = v_add3_u32 %a, %b, %c
   //! p_unit_test 0, %res0
   Builder::Result tmp = bld.vop2(aco_opcode::v_add_u32, bld.def(v1), inputs[1], inputs[2]);
   writeout(0, bld.vop2(aco_opcode::v_add_u32, bld.def(v1), inputs[0], tmp));

   //! v1: %tmp1 = v_add_u32 %b, %c clamp
   //! v1: %res1 = v_add_u32 %a, %tmp1
   //! p_unit_test 1, %res1
   tmp = bld.vop2_e64(aco_opcode::v_add_u32, bld.def(v1), inputs[1], inputs[2]);
   tmp.instr->vop3().clamp = true;
   writeout(1, bld.vop2(aco_opcode::v_add_u32, bld.def(v1), inputs[0], tmp));

   //! v1: %tmp2 = v_add_u32 %b, %c
   //! v1: %res2 = v_add_u32 %a, %tmp2 clamp
   //! p_unit_test 2, %res2
   tmp = bld.vop2(aco_opcode::v_add_u32, bld.def(v1), inputs[1], inputs[2]);
   tmp = bld.vop2_e64(aco_opcode::v_add_u32, bld.def(v1), inputs[0], tmp);
   tmp.instr->vop3().clamp = true;
   writeout(2, tmp);

   finish_opt_test();
END_TEST

BEGIN_TEST(optimize.minmax)
   for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a = p_startpgm
      if (!setup_cs("v1", (chip_class)i))
         continue;

      //! v1: %res0 = v_max3_f32 0, -0, %a
      //! p_unit_test 0, %res0
      Temp xor0 = fneg(inputs[0]);
      Temp min = bld.vop2(aco_opcode::v_min_f32, bld.def(v1), Operand::zero(), xor0);
      Temp xor1 = fneg(min);
      writeout(0, bld.vop2(aco_opcode::v_max_f32, bld.def(v1), Operand::zero(), xor1));

      //! v1: %res1 = v_max3_f32 0, -0, -%a
      //! p_unit_test 1, %res1
      min = bld.vop2(aco_opcode::v_min_f32, bld.def(v1), Operand::zero(), Operand(inputs[0]));
      xor1 = fneg(min);
      writeout(1, bld.vop2(aco_opcode::v_max_f32, bld.def(v1), Operand::zero(), xor1));

      finish_opt_test();
   }
END_TEST

BEGIN_TEST(optimize.mad_32_24)
   for (unsigned i = GFX8; i <= GFX9; i++) {
      //>> v1: %a, v1: %b, v1: %c = p_startpgm
      if (!setup_cs("v1 v1 v1", (chip_class)i))
         continue;

      //! v1: %res0 = v_mad_u32_u24 %b, %c, %a
      //! p_unit_test 0, %res0
      Temp mul = bld.vop2(aco_opcode::v_mul_u32_u24, bld.def(v1), inputs[1], inputs[2]);
      writeout(0, bld.vadd32(bld.def(v1), inputs[0], mul));

      //! v1: %res1_tmp = v_mul_u32_u24 %b, %c
      //! v1: %_, s2: %res1 = v_add_co_u32 %a, %res1_tmp
      //! p_unit_test 1, %res1
      mul = bld.vop2(aco_opcode::v_mul_u32_u24, bld.def(v1), inputs[1], inputs[2]);
      writeout(1, bld.vadd32(bld.def(v1), inputs[0], mul, true).def(1).getTemp());

      finish_opt_test();
   }
END_TEST

BEGIN_TEST(optimize.add_lshlrev)
   for (unsigned i = GFX8; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, s1: %c = p_startpgm
      if (!setup_cs("v1 v1 s1", (chip_class)i))
         continue;

      Temp lshl;

      //~gfx8! v1: %lshl0 = v_lshlrev_b32 3, %a
      //~gfx8! v1: %res0, s2: %_ = v_add_co_u32 %lshl0, %b
      //~gfx(9|10)! v1: %res0 = v_lshl_add_u32 %a, 3, %b
      //! p_unit_test 0, %res0
      lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(3u), Operand(inputs[0]));
      writeout(0, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %lshl1 = v_lshlrev_b32 7, (is24bit)%a
      //~gfx8! v1: %res1, s2: %_ = v_add_co_u32 %lshl1, %b
      //~gfx(9|10)! v1: %res1 = v_lshl_add_u32 (is24bit)%a, 7, %b
      //! p_unit_test 1, %res1
      Operand a_24bit = Operand(inputs[0]);
      a_24bit.set24bit(true);
      lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(7u), a_24bit);
      writeout(1, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %lshl2 = v_lshlrev_b32 (is24bit)%a, (is24bit)%b
      //~gfx8! v1: %res2, s2: %_ = v_add_co_u32 %lshl2, %b
      //~gfx(9|10)! v1: %res2 = v_lshl_add_u32 (is24bit)%b, (is24bit)%a, %b
      //! p_unit_test 2, %res2
      Operand b_24bit = Operand(inputs[1]);
      b_24bit.set24bit(true);
      lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), a_24bit, b_24bit);
      writeout(2, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %res3 = v_mad_u32_u24 (is24bit)%a, 8, %b
      //~gfx(9|10)! v1: %res3 = v_lshl_add_u32 (is24bit)%a, 3, %b
      //! p_unit_test 3, %res3
      lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(3u), a_24bit);
      writeout(3, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %res4 = v_mad_u32_u24 (is16bit)%a, 16, %b
      //~gfx(9|10)! v1: %res4 = v_lshl_add_u32 (is16bit)%a, 4, %b
      //! p_unit_test 4, %res4
      Operand a_16bit = Operand(inputs[0]);
      a_16bit.set16bit(true);
      lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(4u), a_16bit);
      writeout(4, bld.vadd32(bld.def(v1), lshl, Operand(inputs[1])));

      //~gfx8! v1: %res5 = v_mad_u32_u24 (is24bit)%c, 16, %c
      //~gfx(9|10)! v1: %res5 = v_lshl_add_u32 (is24bit)%c, 4, %c
      //! p_unit_test 5, %res5
      Operand c_24bit = Operand(inputs[2]);
      c_24bit.set24bit(true);
      lshl = bld.vop2_e64(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(4u), c_24bit);
      writeout(5, bld.vadd32(bld.def(v1), lshl, Operand(inputs[2])));

      finish_opt_test();
   }
END_TEST

enum denorm_op {
   denorm_mul1 = 0,
   denorm_fneg = 1,
   denorm_fabs = 2,
   denorm_fnegabs = 3,
};

static const char *denorm_op_names[] = {
   "mul1",
   "fneg",
   "fabs",
   "fnegabs",
};

struct denorm_config {
   bool flush;
   unsigned op;
   aco_opcode src;
   aco_opcode dest;
};

static const char *srcdest_op_name(aco_opcode op)
{
   switch (op) {
   case aco_opcode::v_cndmask_b32:
      return "cndmask";
   case aco_opcode::v_min_f32:
      return "min";
   case aco_opcode::v_rcp_f32:
      return "rcp";
   default:
      return "none";
   }
}

static Temp emit_denorm_srcdest(aco_opcode op, Temp val)
{
   switch (op) {
   case aco_opcode::v_cndmask_b32:
      return bld.vop2(aco_opcode::v_cndmask_b32, bld.def(v1), Operand::zero(), val, inputs[1]);
   case aco_opcode::v_min_f32:
      return bld.vop2(aco_opcode::v_min_f32, bld.def(v1), Operand::zero(), val);
   case aco_opcode::v_rcp_f32:
      return bld.vop1(aco_opcode::v_rcp_f32, bld.def(v1), val);
   default:
      return val;
   }
}

BEGIN_TEST(optimize.denorm_propagation)
   for (unsigned i = GFX8; i <= GFX9; i++) {
      std::vector<denorm_config> configs;
      for (bool flush : {false, true}) {
         for (denorm_op op : {denorm_mul1, denorm_fneg, denorm_fabs, denorm_fnegabs})
            configs.push_back({flush, op, aco_opcode::num_opcodes, aco_opcode::num_opcodes});

         for (aco_opcode dest : {aco_opcode::v_min_f32, aco_opcode::v_rcp_f32}) {
            for (denorm_op op : {denorm_mul1, denorm_fneg, denorm_fabs, denorm_fnegabs})
               configs.push_back({flush, op, aco_opcode::num_opcodes, dest});
         }

         for (aco_opcode src : {aco_opcode::v_cndmask_b32, aco_opcode::v_min_f32, aco_opcode::v_rcp_f32}) {
            for (denorm_op op : {denorm_mul1, denorm_fneg, denorm_fabs, denorm_fnegabs})
               configs.push_back({flush, op, src, aco_opcode::num_opcodes});
         }
      }

      for (denorm_config cfg : configs) {
         char subvariant[128];
         sprintf(subvariant, "_%s_%s_%s_%s",
                 cfg.flush ? "flush" : "keep", srcdest_op_name(cfg.src),
                 denorm_op_names[(int)cfg.op], srcdest_op_name(cfg.dest));
         if (!setup_cs("v1 s2", (chip_class)i, CHIP_UNKNOWN, subvariant))
            continue;

         bool can_propagate = cfg.src == aco_opcode::v_rcp_f32 || (i >= GFX9 && cfg.src == aco_opcode::v_min_f32) ||
                              cfg.dest == aco_opcode::v_rcp_f32 || (i >= GFX9 && cfg.dest == aco_opcode::v_min_f32) ||
                              !cfg.flush;

         fprintf(output, "src, dest, op: %s %s %s\n",
                 srcdest_op_name(cfg.src), srcdest_op_name(cfg.dest), denorm_op_names[(int)cfg.op]);
         fprintf(output, "can_propagate: %u\n", can_propagate);
         //! src, dest, op: $src $dest $op
         //! can_propagate: #can_propagate
         //>> v1: %a, s2: %b = p_startpgm

         //; patterns = {'cndmask': 'v1: %{} = v_cndmask_b32 0, {}, %b',
         //;             'min': 'v1: %{} = v_min_f32 0, {}',
         //;             'rcp': 'v1: %{} = v_rcp_f32 {}'}
         //; ops = {'mul1': 'v1: %{} = v_mul_f32 1.0, %{}',
         //;        'fneg': 'v1: %{} = v_mul_f32 -1.0, %{}',
         //;        'fabs': 'v1: %{} = v_mul_f32 1.0, |%{}|',
         //;        'fnegabs': 'v1: %{} = v_mul_f32 -1.0, |%{}|'}
         //; inline_ops = {'mul1': '%{}', 'fneg': '-%{}', 'fabs': '|%{}|', 'fnegabs': '-|%{}|'}

         //; name = 'a'
         //; if src != 'none':
         //;    insert_pattern(patterns[src].format('src_res', '%'+name))
         //;    name = 'src_res'

         //; if can_propagate:
         //;    name = inline_ops[op].format(name)
         //; else:
         //;    insert_pattern(ops[op].format('op_res', name))
         //;    name = '%op_res'

         //; if dest != 'none':
         //;    insert_pattern(patterns[dest].format('dest_res', name))
         //;    name = '%dest_res'

         //; insert_pattern('v1: %res = v_cndmask_b32 0, {}, %b'.format(name))
         //! p_unit_test 0, %res

         program->blocks[0].fp_mode.denorm32 = cfg.flush ? fp_denorm_flush : fp_denorm_keep;

         Temp val = emit_denorm_srcdest(cfg.src, inputs[0]);
         switch (cfg.op) {
         case denorm_mul1:
            val = bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x3f800000u), val);
            break;
         case denorm_fneg:
            val = fneg(val);
            break;
         case denorm_fabs:
            val = fabs(val);
            break;
         case denorm_fnegabs:
            val = fneg(fabs(val));
            break;
         }
         val = emit_denorm_srcdest(cfg.dest, val);
         writeout(
            0, bld.vop2(aco_opcode::v_cndmask_b32, bld.def(v1), Operand::zero(), val, inputs[1]));

         finish_opt_test();
      }
   }
END_TEST

BEGIN_TEST(optimizer.dpp)
   //>> v1: %a, v1: %b, s2: %c, s1: %d = p_startpgm
   if (!setup_cs("v1 v1 s2 s1", GFX10_3))
      return;

   Operand a(inputs[0]);
   Operand b(inputs[1]);
   Operand c(inputs[2]);
   Operand d(inputs[3]);

   /* basic optimization */
   //! v1: %res0 = v_add_f32 %a, %b row_mirror bound_ctrl:1
   //! p_unit_test 0, %res0
   Temp tmp0 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res0 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), tmp0, b);
   writeout(0, res0);

   /* operand swapping */
   //! v1: %res1 = v_subrev_f32 %a, %b row_mirror bound_ctrl:1
   //! p_unit_test 1, %res1
   Temp tmp1 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res1 = bld.vop2(aco_opcode::v_sub_f32, bld.def(v1), b, tmp1);
   writeout(1, res1);

   //! v1: %tmp2 = v_mov_b32 %a row_mirror bound_ctrl:1
   //! v1: %res2 = v_sub_f32 %b, %tmp2 row_half_mirror bound_ctrl:1
   //! p_unit_test 2, %res2
   Temp tmp2 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res2 = bld.vop2_dpp(aco_opcode::v_sub_f32, bld.def(v1), b, tmp2, dpp_row_half_mirror);
   writeout(2, res2);

   /* modifiers */
   //! v1: %res3 = v_add_f32 -%a, %b row_mirror bound_ctrl:1
   //! p_unit_test 3, %res3
   auto tmp3 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   tmp3.instr->dpp().neg[0] = true;
   Temp res3 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), tmp3, b);
   writeout(3, res3);

   //! v1: %res4 = v_add_f32 -%a, %b row_mirror bound_ctrl:1
   //! p_unit_test 4, %res4
   Temp tmp4 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   auto res4 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp4, b);
   res4.instr->vop3().neg[0] = true;
   writeout(4, res4);

   //! v1: %tmp5 = v_mov_b32 %a row_mirror bound_ctrl:1
   //! v1: %res5 = v_add_f32 %tmp5, %b clamp
   //! p_unit_test 5, %res5
   Temp tmp5 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   auto res5 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp5, b);
   res5.instr->vop3().clamp = true;
   writeout(5, res5);

   //! v1: %res6 = v_add_f32 |%a|, %b row_mirror bound_ctrl:1
   //! p_unit_test 6, %res6
   auto tmp6 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   tmp6.instr->dpp().neg[0] = true;
   auto res6 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp6, b);
   res6.instr->vop3().abs[0] = true;
   writeout(6, res6);

   //! v1: %res7 = v_subrev_f32 %a, |%b| row_mirror bound_ctrl:1
   //! p_unit_test 7, %res7
   Temp tmp7 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   auto res7 = bld.vop2_e64(aco_opcode::v_sub_f32, bld.def(v1), b, tmp7);
   res7.instr->vop3().abs[0] = true;
   writeout(7, res7);

   /* vcc */
   //! v1: %res8 = v_cndmask_b32 %a, %b, %c:vcc row_mirror bound_ctrl:1
   //! p_unit_test 8, %res8
   Temp tmp8 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res8 = bld.vop2(aco_opcode::v_cndmask_b32, bld.def(v1), tmp8, b, c);
   writeout(8, res8);

   /* sgprs */
   //! v1: %tmp9 = v_mov_b32 %a row_mirror bound_ctrl:1
   //! v1: %res9 = v_add_f32 %tmp9, %d
   //! p_unit_test 9, %res9
   Temp tmp9 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res9 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp9, d);
   writeout(9, res9);

   //! v1: %tmp10 = v_mov_b32 %a row_mirror bound_ctrl:1
   //! v1: %res10 = v_add_f32 %d, %tmp10
   //! p_unit_test 10, %res10
   Temp tmp10 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res10 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), d, tmp10);
   writeout(10, res10);

   finish_opt_test();
END_TEST

BEGIN_TEST(optimize.dpp_prop)
   //>> v1: %a, s1: %b = p_startpgm
   if (!setup_cs("v1 s1", GFX10))
      return;

   //! v1: %one = p_parallelcopy 1
   //! v1: %res0 = v_mul_f32 1, %a
   //! p_unit_test 0, %res0
   Temp one = bld.copy(bld.def(v1), Operand::c32(1));
   writeout(0, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), one, inputs[0], dpp_row_sl(1)));

   //! v1: %res1 = v_mul_f32 %a, %one row_shl:1 bound_ctrl:1
   //! p_unit_test 1, %res1
   writeout(1, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), inputs[0], one, dpp_row_sl(1)));

   //! v1: %res2 = v_mul_f32 0x12345678, %a
   //! p_unit_test 2, %res2
   Temp literal1 = bld.copy(bld.def(v1), Operand::c32(0x12345678u));
   writeout(2, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), literal1, inputs[0], dpp_row_sl(1)));

   //! v1: %literal2 = p_parallelcopy 0x12345679
   //! v1: %res3 = v_mul_f32 %a, %literal row_shl:1 bound_ctrl:1
   //! p_unit_test 3, %res3
   Temp literal2 = bld.copy(bld.def(v1), Operand::c32(0x12345679u));
   writeout(3, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), inputs[0], literal2, dpp_row_sl(1)));

   //! v1: %b_v = p_parallelcopy %b
   //! v1: %res4 = v_mul_f32 %b, %a
   //! p_unit_test 4, %res4
   Temp b_v = bld.copy(bld.def(v1), inputs[1]);
   writeout(4, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), b_v, inputs[0], dpp_row_sl(1)));

   //! v1: %res5 = v_mul_f32 %a, %b_v row_shl:1 bound_ctrl:1
   //! p_unit_test 5, %res5
   writeout(5, bld.vop2_dpp(aco_opcode::v_mul_f32, bld.def(v1), inputs[0], b_v, dpp_row_sl(1)));

   //! v1: %res6 = v_rcp_f32 %b
   //! p_unit_test 6, %res6
   writeout(6, bld.vop1_dpp(aco_opcode::v_rcp_f32, bld.def(v1), b_v, dpp_row_sl(1)));

   finish_opt_test();
END_TEST

