/*
 * Copyright © 2021 Valve Corporation
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

BEGIN_TEST(optimizer_postRA.vcmp)
    PhysReg reg_v0(256);
    PhysReg reg_s0(0);
    PhysReg reg_s2(2);
    PhysReg reg_s4(4);

    //>> v1: %a:v[0] = p_startpgm
    ASSERTED bool setup_ok = setup_cs("v1", GFX8);
    assert(setup_ok);

    auto &startpgm = bld.instructions->at(0);
    assert(startpgm->opcode == aco_opcode::p_startpgm);
    startpgm->definitions[0].setFixed(reg_v0);

    Temp v_in = inputs[0];

    {
        /* Recognize when the result of VOPC goes to VCC, and use that for the branching then. */

        //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
        //! s2: %e:s[2-3] = p_cbranch_z %b:vcc
        //! p_unit_test 0, %e:s[2-3]
        auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
                             Operand(v_in, reg_v0));
        auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp), Operand(exec, bld.lm));
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, reg_s2), bld.scc(sand.def(1).getTemp()));
        writeout(0, Operand(br, reg_s2));
    }

    //; del b, e

    {
        /* When VCC is overwritten inbetween, don't optimize. */

        //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
        //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
        //! s2: %f:vcc = s_mov_b64 0
        //! s2: %e:s[2-3] = p_cbranch_z %d:scc
        //! p_unit_test 1, %e:s[2-3], %f:vcc
        auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
                             Operand(v_in, reg_v0));
        auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp), Operand(exec, bld.lm));
        auto ovrwr = bld.sop1(Builder::s_mov, bld.def(bld.lm, vcc), Operand::zero());
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, reg_s2), bld.scc(sand.def(1).getTemp()));
        writeout(1, Operand(br, reg_s2), Operand(ovrwr, vcc));
    }

    //; del b, c, d, e, f

    {
        /* When the result of VOPC goes to an SGPR pair other than VCC, don't optimize */

        //! s2: %b:s[4-5] = v_cmp_eq_u32 0, %a:v[0]
        //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:s[4-5], %x:exec
        //! s2: %e:s[2-3] = p_cbranch_z %d:scc
        //! p_unit_test 2, %e:s[2-3]
        auto vcmp = bld.vopc_e64(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, reg_s4), Operand::zero(),
                                 Operand(v_in, reg_v0));
        auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), Operand(vcmp, reg_s4), Operand(exec, bld.lm));
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, reg_s2), bld.scc(sand.def(1).getTemp()));
        writeout(2, Operand(br, reg_s2));
    }

    //; del b, c, d, e

    {
        /* When the VCC isn't written by VOPC, don't optimize */

        //! s2: %b:vcc, s1: %f:scc = s_or_b64 1, %0:s[4-5]
        //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
        //! s2: %e:s[2-3] = p_cbranch_z %d:scc
        //! p_unit_test 2, %e:s[2-3]
        auto salu = bld.sop2(Builder::s_or, bld.def(bld.lm, vcc), bld.def(s1, scc),
                             Operand::c32(1u), Operand(reg_s4, bld.lm));
        auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), Operand(salu, vcc), Operand(exec, bld.lm));
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, reg_s2), bld.scc(sand.def(1).getTemp()));
        writeout(2, Operand(br, reg_s2));
    }

    //; del b, c, d, e, f, x

    {
        /* When EXEC is overwritten inbetween, don't optimize. */

        //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
        //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
        //! s2: %f:exec = s_mov_b64 42
        //! s2: %e:s[2-3] = p_cbranch_z %d:scc
        //! p_unit_test 4, %e:s[2-3], %f:exec
        auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
                             Operand(v_in, reg_v0));
        auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp), Operand(exec, bld.lm));
        auto ovrwr = bld.sop1(Builder::s_mov, bld.def(bld.lm, exec), Operand::c32(42u));
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, reg_s2), bld.scc(sand.def(1).getTemp()));
        writeout(4, Operand(br, reg_s2), Operand(ovrwr, exec));
    }

    //; del b, c, d, e, f, x

    finish_optimizer_postRA_test();
END_TEST

BEGIN_TEST(optimizer_postRA.scc_nocmp_opt)
    //>> s1: %a, s2: %y, s1: %z = p_startpgm
    ASSERTED bool setup_ok = setup_cs("s1 s2 s1", GFX6);
    assert(setup_ok);

    PhysReg reg_s0{0};
    PhysReg reg_s1{1};
    PhysReg reg_s2{2};
    PhysReg reg_s3{3};
    PhysReg reg_s4{4};
    PhysReg reg_s6{6};

    Temp in_0 = inputs[0];
    Temp in_1 = inputs[1];
    Temp in_2 = inputs[2];
    Operand op_in_0(in_0);
    op_in_0.setFixed(reg_s0);
    Operand op_in_1(in_1);
    op_in_1.setFixed(reg_s4);
    Operand op_in_2(in_2);
    op_in_2.setFixed(reg_s6);

    {
        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s2: %f:vcc = p_cbranch_nz %e:scc
        //! p_unit_test 0, %f:vcc
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
        writeout(0, Operand(br, vcc));
    }

    //; del d, e, f

    {
        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s2: %f:vcc = p_cbranch_z %e:scc
        //! p_unit_test 1, %f:vcc
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
        writeout(1, Operand(br, vcc));
    }

    //; del d, e, f

    {
        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s2: %f:vcc = p_cbranch_z %e:scc
        //! p_unit_test 2, %f:vcc
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2, vcc), bld.scc(scmp));
        writeout(2, Operand(br, vcc));
    }

    //; del d, e, f

    {
        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s2: %f:vcc = p_cbranch_nz %e:scc
        //! p_unit_test 3, %f:vcc
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2, vcc), bld.scc(scmp));
        writeout(3, Operand(br, vcc));
    }

    //; del d, e, f

    {
        //! s2: %d:s[2-3], s1: %e:scc = s_and_b64 %y:s[4-5], 0x12345
        //! s2: %f:vcc = p_cbranch_z %e:scc
        //! p_unit_test 4, %f:vcc
        auto salu = bld.sop2(aco_opcode::s_and_b64, bld.def(s2, reg_s2), bld.def(s1, scc), op_in_1,
                             Operand::c32(0x12345u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u64, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero(8));
        auto br = bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2, vcc), bld.scc(scmp));
        writeout(4, Operand(br, vcc));
    }

    //; del d, e, f

    {
        /* SCC is overwritten in between, don't optimize */

        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s1: %h:s[3], s1: %x:scc = s_add_u32 %a:s[0], 1
        //! s1: %g:scc = s_cmp_eq_u32 %d:s[2], 0
        //! s2: %f:vcc = p_cbranch_z %g:scc
        //! p_unit_test 5, %f:vcc, %h:s[3]
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto ovrw = bld.sop2(aco_opcode::s_add_u32, bld.def(s1, reg_s3), bld.def(s1, scc), op_in_0,
                             Operand::c32(1u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
        writeout(5, Operand(br, vcc), Operand(ovrw, reg_s3));
    }

    //; del d, e, f, g, h, x

    {
        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s1: %f:s[4] = s_cselect_b32 %z:s[6], %a:s[0], %e:scc
        //! p_unit_test 6, %f:s[4]
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.sop2(aco_opcode::s_cselect_b32, bld.def(s1, reg_s4), Operand(op_in_0), Operand(op_in_2), bld.scc(scmp));
        writeout(6, Operand(br, reg_s4));
    }

    //; del d, e, f

    {
        /* SCC is overwritten in between, don't optimize */

        //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
        //! s1: %h:s[3], s1: %x:scc = s_add_u32 %a:s[0], 1
        //! s1: %g:scc = s_cmp_eq_u32 %d:s[2], 0
        //! s1: %f:s[4] = s_cselect_b32 %a:s[0], %z:s[6], %g:scc
        //! p_unit_test 7, %f:s[4], %h:s[3]
        auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
                             Operand::c32(0x40018u));
        auto ovrw = bld.sop2(aco_opcode::s_add_u32, bld.def(s1, reg_s3), bld.def(s1, scc), op_in_0,
                             Operand::c32(1u));
        auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
                             Operand::zero());
        auto br = bld.sop2(aco_opcode::s_cselect_b32, bld.def(s1, reg_s4), Operand(op_in_0), Operand(op_in_2), bld.scc(scmp));
        writeout(7, Operand(br, reg_s4), Operand(ovrw, reg_s3));
    }

    //; del d, e, f, g, h, x

    finish_optimizer_postRA_test();
END_TEST

BEGIN_TEST(optimizer_postRA.dpp)
   //>> v1: %a:v[0], v1: %b:v[1], s2: %c:vcc, s2: %d:s[0-1] = p_startpgm
   if (!setup_cs("v1 v1 s2 s2", GFX10_3))
      return;

   bld.instructions->at(0)->definitions[0].setFixed(PhysReg(256));
   bld.instructions->at(0)->definitions[1].setFixed(PhysReg(257));
   bld.instructions->at(0)->definitions[2].setFixed(vcc);
   bld.instructions->at(0)->definitions[3].setFixed(PhysReg(0));

   PhysReg reg_v0(256);
   PhysReg reg_v2(258);
   Operand a(inputs[0], PhysReg(256));
   Operand b(inputs[1], PhysReg(257));
   Operand c(inputs[2], vcc);
   Operand d(inputs[3], PhysReg(0));

   /* basic optimization */
   //! v1: %res0:v[2] = v_add_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1
   //! p_unit_test 0, %res0:v[2]
   Temp tmp0 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res0 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp0, reg_v2), b);
   writeout(0, Operand(res0, reg_v2));

   /* operand swapping */
   //! v1: %res1:v[2] = v_subrev_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1
   //! p_unit_test 1, %res1:v[2]
   Temp tmp1 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res1 = bld.vop2(aco_opcode::v_sub_f32, bld.def(v1, reg_v2), b, Operand(tmp1, reg_v2));
   writeout(1, Operand(res1, reg_v2));

   //! v1: %tmp2:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1
   //! v1: %res2:v[2] = v_sub_f32 %b:v[1], %tmp2:v[2] row_half_mirror bound_ctrl:1
   //! p_unit_test 2, %res2:v[2]
   Temp tmp2 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res2 = bld.vop2_dpp(aco_opcode::v_sub_f32, bld.def(v1, reg_v2), b, Operand(tmp2, reg_v2), dpp_row_half_mirror);
   writeout(2, Operand(res2, reg_v2));

   /* modifiers */
   //! v1: %res3:v[2] = v_add_f32 -%a:v[0], %b:v[1] row_mirror bound_ctrl:1
   //! p_unit_test 3, %res3:v[2]
   auto tmp3 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   tmp3.instr->dpp().neg[0] = true;
   Temp res3 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp3, reg_v2), b);
   writeout(3, Operand(res3, reg_v2));

   //! v1: %res4:v[2] = v_add_f32 -%a:v[0], %b:v[1] row_mirror bound_ctrl:1
   //! p_unit_test 4, %res4:v[2]
   Temp tmp4 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   auto res4 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp4, reg_v2), b);
   res4.instr->vop3().neg[0] = true;
   writeout(4, Operand(res4, reg_v2));

   //! v1: %tmp5:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1
   //! v1: %res5:v[2] = v_add_f32 %tmp5:v[2], %b:v[1] clamp
   //! p_unit_test 5, %res5:v[2]
   Temp tmp5 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   auto res5 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp5, reg_v2), b);
   res5.instr->vop3().clamp = true;
   writeout(5, Operand(res5, reg_v2));

   //! v1: %res6:v[2] = v_add_f32 |%a:v[0]|, %b:v[1] row_mirror bound_ctrl:1
   //! p_unit_test 6, %res6:v[2]
   auto tmp6 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   tmp6.instr->dpp().neg[0] = true;
   auto res6 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp6, reg_v2), b);
   res6.instr->vop3().abs[0] = true;
   writeout(6, Operand(res6, reg_v2));

   //! v1: %res7:v[2] = v_subrev_f32 %a:v[0], |%b:v[1]| row_mirror bound_ctrl:1
   //! p_unit_test 7, %res7:v[2]
   Temp tmp7 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   auto res7 = bld.vop2_e64(aco_opcode::v_sub_f32, bld.def(v1, reg_v2), b, Operand(tmp7, reg_v2));
   res7.instr->vop3().abs[0] = true;
   writeout(7, Operand(res7, reg_v2));

   /* vcc */
   //! v1: %res8:v[2] = v_cndmask_b32 %a:v[0], %b:v[1], %c:vcc row_mirror bound_ctrl:1
   //! p_unit_test 8, %res8:v[2]
   Temp tmp8 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res8 = bld.vop2(aco_opcode::v_cndmask_b32, bld.def(v1, reg_v2), Operand(tmp8, reg_v2), b, c);
   writeout(8, Operand(res8, reg_v2));

   //! v1: %tmp9:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1
   //! v1: %res9:v[2] = v_cndmask_b32 %tmp9:v[2], %b:v[1], %d:s[0-1]
   //! p_unit_test 9, %res9:v[2]
   Temp tmp9 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res9 = bld.vop2(aco_opcode::v_cndmask_b32, bld.def(v1, reg_v2), Operand(tmp9, reg_v2), b, d);
   writeout(9, Operand(res9, reg_v2));

   /* control flow */
   //! BB1
   //! /* logical preds: / linear preds: BB0, / kind: uniform, */
   //! v1: %res10:v[2] = v_add_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1
   //! p_unit_test 10, %res10:v[2]
   Temp tmp10 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);

   bld.reset(program->create_and_insert_block());
   program->blocks[0].linear_succs.push_back(1);
   program->blocks[1].linear_preds.push_back(0);

   Temp res10 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp10, reg_v2), b);
   writeout(10, Operand(res10, reg_v2));

   /* can't combine if the v_mov_b32's operand is modified */
   //! v1: %tmp11_1:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1
   //! v1: %tmp11_2:v[0] = v_mov_b32 0
   //! v1: %res11:v[2] = v_add_f32 %tmp11_1:v[2], %b:v[1]
   //! p_unit_test 11, %res11_1:v[2], %tmp11_2:v[0]
   Temp tmp11_1 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp tmp11_2 = bld.vop1(aco_opcode::v_mov_b32, bld.def(v1, reg_v0), Operand::c32(0));
   Temp res11 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp11_1, reg_v2), b);
   writeout(11, Operand(res11, reg_v2), Operand(tmp11_2, reg_v0));

   finish_optimizer_postRA_test();
END_TEST

