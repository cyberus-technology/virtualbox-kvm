/*
 * Copyright © 2017 Gert Wollny
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "st_tests_common.h"

#include "mesa/program/prog_instruction.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_ureg.h"
#include "compiler/glsl/list.h"
#include "gtest/gtest.h"

#include <utility>
#include <algorithm>

using std::vector;
using std::pair;
using std::make_pair;
using std::transform;
using std::copy;
using std::tuple;


/* Implementation of helper and test classes */
void *FakeCodeline::mem_ctx = nullptr;

FakeCodeline::FakeCodeline(tgsi_opcode _op, const vector<int>& _dst,
                           const vector<int>& _src, const vector<int>&_to):
   op(_op),
   max_temp_id(0),
   max_array_id(0)
{
   transform(_dst.begin(), _dst.end(), std::back_inserter(dst),
             [this](int i) { return create_dst_register(i);});

   transform(_src.begin(), _src.end(), std::back_inserter(src),
             [this](int i) { return create_src_register(i);});

   transform(_to.begin(), _to.end(), std::back_inserter(tex_offsets),
             [this](int i) { return create_src_register(i);});

}

FakeCodeline::FakeCodeline(tgsi_opcode _op, const vector<pair<int,int>>& _dst,
                           const vector<pair<int, const char *>>& _src,
                           const vector<pair<int, const char *>>&_to,
                           SWZ with_swizzle):
   op(_op),
   max_temp_id(0),
   max_array_id(0)
{
   (void)with_swizzle;

   transform(_dst.begin(), _dst.end(), std::back_inserter(dst),
             [this](pair<int,int> r) {
      return create_dst_register(r.first, r.second);
   });

   transform(_src.begin(), _src.end(), std::back_inserter(src),
             [this](const pair<int,const char *>& r) {
      return create_src_register(r.first, r.second);
   });

   transform(_to.begin(), _to.end(), std::back_inserter(tex_offsets),
             [this](const pair<int,const char *>& r) {
      return create_src_register(r.first, r.second);
   });
}

FakeCodeline::FakeCodeline(tgsi_opcode _op, const vector<tuple<int,int,int>>& _dst,
                           const vector<tuple<int,int,int>>& _src,
                           const vector<tuple<int,int,int>>&_to, RA with_reladdr):
   op(_op),
   max_temp_id(0),
   max_array_id(0)
{
   (void)with_reladdr;

   transform(_dst.begin(), _dst.end(), std::back_inserter(dst),
             [this](const tuple<int,int,int>& r) {
      return create_dst_register(r);
   });

   transform(_src.begin(), _src.end(), std::back_inserter(src),
             [this](const tuple<int,int,int>& r) {
      return create_src_register(r);
   });

   transform(_to.begin(), _to.end(), std::back_inserter(tex_offsets),
             [this](const tuple<int,int,int>& r) {
      return create_src_register(r);
   });
}

FakeCodeline::FakeCodeline(tgsi_opcode _op, const vector<tuple<int,int,int>>& _dst,
			   const vector<tuple<int,int, const char*>>& _src,
			   const vector<tuple<int,int, const char*>>&_to,
			   ARR with_array):
   FakeCodeline(_op)
{
   (void)with_array;

   transform(_dst.begin(), _dst.end(), std::back_inserter(dst),
	     [this](const tuple<int,int,int>& r) {
      return create_array_dst_register(r);
   });

   transform(_src.begin(), _src.end(), std::back_inserter(src),
	     [this](const tuple<int,int,const char*>& r) {
      return create_array_src_register(r);
   });

   transform(_to.begin(), _to.end(), std::back_inserter(tex_offsets),
	     [this](const tuple<int,int,const char*>& r) {
      return create_array_src_register(r);
   });

}

FakeCodeline::FakeCodeline(const glsl_to_tgsi_instruction& instr):
   op(instr.op),
   max_temp_id(0),
   max_array_id(0)
{
   int nsrc = num_inst_src_regs(&instr);
   int ndst = num_inst_dst_regs(&instr);

   copy(instr.src, instr.src + nsrc, std::back_inserter(src));
   copy(instr.dst, instr.dst + ndst, std::back_inserter(dst));

   for (auto& s: src)
      read_reg(s);

   for (auto& d: dst)
      read_reg(d);

}

template <typename st_reg>
void FakeCodeline::read_reg(const st_reg& s)
{
   if (s.file == PROGRAM_ARRAY) {
      if (s.array_id > max_array_id)
	 max_array_id = s.array_id;
      if (s.reladdr)
	 read_reg(*s.reladdr);
      if (s.reladdr2)
	 read_reg(*s.reladdr2);
   } else  if (s.file == PROGRAM_TEMPORARY) {
      if (s.index > max_temp_id)
         max_temp_id = s.index;
   }
}

void FakeCodeline::print(std::ostream& os) const
{
   const struct tgsi_opcode_info *info = tgsi_get_opcode_info(op);
   os << tgsi_get_opcode_name(info->opcode) << " ";

   for (auto d: dst) {
      os << d << " ";
   }
   os << " <- ";
   for (auto s: src) {
      os << s << " ";
   }
   os << "\n";
}

bool operator == (const FakeCodeline& lhs, const FakeCodeline& rhs)
{
   if  ((lhs.op != rhs.op) ||
        (lhs.src.size() != rhs.src.size()) ||
        (lhs.dst.size() != rhs.dst.size()))
      return false;

   return std::equal(lhs.src.begin(), lhs.src.end(), rhs.src.begin()) &&
         std::equal(lhs.dst.begin(), lhs.dst.end(), rhs.dst.begin());
}

st_src_reg FakeCodeline::create_src_register(int src_idx)
{
   return create_src_register(src_idx,
                              src_idx < 0 ? PROGRAM_INPUT : PROGRAM_TEMPORARY);
}

static int swizzle_from_char(const char *sw)
{
   int swizzle = 0;
   if (!sw || sw[0] == 0)
       return SWIZZLE_XYZW;

   const char *isw = sw;
   for (int i = 0; i < 4; ++i) {
      switch (*isw) {
      case 'x': break; /* is zero */
      case 'y': swizzle |= SWIZZLE_Y << 3 * i; break;
      case 'z': swizzle |= SWIZZLE_Z << 3 * i; break;
      case 'w': swizzle |= SWIZZLE_W << 3 * i; break;
      default:
         assert(!"This test uses an unknown swizzle character");
      }
      if (isw[1] != 0)
         ++isw;
   }
   return swizzle;
}

st_src_reg FakeCodeline::create_src_register(int src_idx, const char *sw)
{
   st_src_reg result = create_src_register(src_idx);
   result.swizzle = swizzle_from_char(sw);
   return result;
}

st_src_reg FakeCodeline::create_src_register(int src_idx, gl_register_file file)
{
   st_src_reg retval;
   retval.file = file;
   retval.index = src_idx >= 0 ? src_idx  : 1 - src_idx;

   if (file == PROGRAM_TEMPORARY) {
      if (max_temp_id < src_idx)
         max_temp_id = src_idx;
   } else if (file == PROGRAM_ARRAY) {
      retval.array_id = 1;
      if (max_array_id < 1)
	  max_array_id = 1;
   }
   retval.swizzle = SWIZZLE_XYZW;
   retval.type = GLSL_TYPE_INT;

   return retval;
}

st_src_reg *FakeCodeline::create_rel_src_register(int idx)
{
   st_src_reg *retval = ralloc(mem_ctx, st_src_reg);
   *retval = st_src_reg(PROGRAM_TEMPORARY, idx, GLSL_TYPE_INT);
   if (max_temp_id < idx)
      max_temp_id = idx;
   return retval;
}

st_src_reg FakeCodeline::create_array_src_register(const tuple<int,int, const char*>& r)
{

   int array_id = std::get<0>(r);
   int idx = std::get<1>(r);

   st_src_reg retval = create_src_register(idx, std::get<2>(r));

   if (array_id > 0) {
      retval.file = PROGRAM_ARRAY;

      retval.array_id = array_id;
      if (max_array_id < array_id)
	 max_array_id = array_id;
   } else {
      if (max_temp_id < idx)
	 max_temp_id = idx;
   }

   return retval;
}

st_dst_reg FakeCodeline::create_array_dst_register(const tuple<int,int,int>& r)
{

   int array_id = std::get<0>(r);
   int idx = std::get<1>(r);

   st_dst_reg retval = create_dst_register(idx, std::get<2>(r));

   if (array_id > 0) {
      retval.file = PROGRAM_ARRAY;
      retval.array_id = array_id;
      if (max_array_id < array_id)
	 max_array_id = array_id;
   } else {
      if (max_temp_id < idx)
	 max_temp_id = idx;
   }
   return retval;
}

st_src_reg FakeCodeline::create_src_register(const tuple<int,int,int>& src)
{
   int src_idx = std::get<0>(src);
   int relidx1 = std::get<1>(src);
   int relidx2 = std::get<2>(src);

   gl_register_file file = PROGRAM_TEMPORARY;
   if (src_idx < 0)
      file = PROGRAM_OUTPUT;
   else if (relidx1 || relidx2) {
      file = PROGRAM_ARRAY;
   }

   st_src_reg retval = create_src_register(src_idx, file);
   if (src_idx >= 0) {
      if (relidx1 || relidx2) {
         retval.array_id = 1;

         if (relidx1)
            retval.reladdr = create_rel_src_register(relidx1);
         if (relidx2) {
            retval.reladdr2 = create_rel_src_register(relidx2);
            retval.has_index2 = true;
            retval.index2D = 10;
         }
      }
   }
   return retval;
}

st_dst_reg FakeCodeline::create_dst_register(int dst_idx,int writemask)
{
   gl_register_file file;
   int idx = 0;
   if (dst_idx >= 0) {
      file = PROGRAM_TEMPORARY;
      idx = dst_idx;
      if (max_temp_id < idx)
         max_temp_id = idx;
   } else {
      file = PROGRAM_OUTPUT;
      idx = 1 - dst_idx;
   }
   return st_dst_reg(file, writemask, GLSL_TYPE_INT, idx);
}

st_dst_reg FakeCodeline::create_dst_register(int dst_idx)
{
   return create_dst_register(dst_idx, dst_idx < 0 ?
                                 PROGRAM_OUTPUT : PROGRAM_TEMPORARY);
}

st_dst_reg FakeCodeline::create_dst_register(int dst_idx, gl_register_file file)
{
   st_dst_reg retval;
   retval.file = file;
   retval.index = dst_idx >= 0 ? dst_idx  : 1 - dst_idx;

   if (file == PROGRAM_TEMPORARY) {
      if (max_temp_id < dst_idx)
         max_temp_id = dst_idx;
   } else if (file == PROGRAM_ARRAY) {
      retval.array_id = 1;
      if (max_array_id < 1)
	  max_array_id = 1;
   }
   retval.writemask = 0xF;
   retval.type = GLSL_TYPE_INT;

   return retval;
}

st_dst_reg FakeCodeline::create_dst_register(const tuple<int,int,int>& dst)
{
   int dst_idx = std::get<0>(dst);
   int relidx1 = std::get<1>(dst);
   int relidx2 = std::get<2>(dst);

   gl_register_file file = PROGRAM_TEMPORARY;
   if (dst_idx < 0)
      file = PROGRAM_OUTPUT;
   else if (relidx1 || relidx2) {
      file = PROGRAM_ARRAY;
   }
   st_dst_reg retval = create_dst_register(dst_idx, file);

   if (relidx1 || relidx2) {
      if (relidx1)
         retval.reladdr = create_rel_src_register(relidx1);
      if (relidx2) {
         retval.reladdr2 = create_rel_src_register(relidx2);
         retval.has_index2 = true;
         retval.index2D = 10;
      }
   }
   return retval;
}

glsl_to_tgsi_instruction *FakeCodeline::get_codeline() const
{
   glsl_to_tgsi_instruction *next_instr = new(mem_ctx) glsl_to_tgsi_instruction();
   next_instr->op = op;
   next_instr->info = tgsi_get_opcode_info(op);

   assert(src.size() == num_inst_src_regs(next_instr));
   assert(dst.size() == num_inst_dst_regs(next_instr));
   assert(tex_offsets.size() < 3);

   copy(src.begin(), src.end(), next_instr->src);
   copy(dst.begin(), dst.end(), next_instr->dst);

   next_instr->tex_offset_num_offset = tex_offsets.size();

   if (next_instr->tex_offset_num_offset > 0) {
      next_instr->tex_offsets = ralloc_array(mem_ctx, st_src_reg, tex_offsets.size());
      copy(tex_offsets.begin(), tex_offsets.end(), next_instr->tex_offsets);
   } else {
      next_instr->tex_offsets = nullptr;
   }
   return next_instr;
}

void FakeCodeline::set_mem_ctx(void *ctx)
{
   mem_ctx = ctx;
}

FakeShader::FakeShader(const vector<FakeCodeline>& source):
   program(source),
   num_temps(0),
   num_arrays(0)
{
   for (const FakeCodeline& i: source) {
      int t = i.get_max_reg_id();
      if (t > num_temps)
         num_temps = t;

      int a = i.get_max_array_id();
      if (a > num_arrays)
	 num_arrays = a;
   }
   ++num_temps;
}

FakeShader::FakeShader(exec_list *tgsi_prog):
   num_temps(0),
   num_arrays(0)
{
   FakeCodeline nop(TGSI_OPCODE_NOP);
   FakeCodeline& last = nop;

   foreach_in_list(glsl_to_tgsi_instruction, inst, tgsi_prog) {
      program.push_back(last = FakeCodeline(*inst));
      if (last.get_max_array_id() > num_arrays)
	 num_arrays = last.get_max_array_id();
      if (num_temps < last.get_max_reg_id())
         num_temps = last.get_max_reg_id();
   }
   ++num_temps;
}

int FakeShader::get_num_arrays() const
{
   return num_arrays;
}

int FakeShader::get_num_temps() const
{
   return num_temps;
}

exec_list* FakeShader::get_program(void *ctx) const
{
   exec_list *prog = new(ctx) exec_list();

   for (const FakeCodeline& i: program) {
      prog->push_tail(i.get_codeline());
   }

   return prog;
}

size_t FakeShader::length() const
{
   return program.size();
}

const FakeCodeline& FakeShader::line(unsigned i) const
{
   return program[i];
}

void MesaTestWithMemCtx::SetUp()
{
   mem_ctx = ralloc_context(nullptr);
   FakeCodeline::set_mem_ctx(mem_ctx);
}

void MesaTestWithMemCtx::TearDown()
{
   ralloc_free(mem_ctx);
   FakeCodeline::set_mem_ctx(nullptr);
   mem_ctx = nullptr;
}


LifetimeEvaluatorTest::life_range_result
LifetimeEvaluatorTest::run(const vector<FakeCodeline>& code, bool& success)
{
   FakeShader shader(code);
   life_range_result result = make_pair(life_range_result::first_type(shader.get_num_temps()),
					life_range_result::second_type(shader.get_num_arrays()));

   success =
	 get_temp_registers_required_live_ranges(mem_ctx, shader.get_program(mem_ctx),
						 shader.get_num_temps(),&result.first[0],
						 shader.get_num_arrays(), &result.second[0]);
   return result;
}

void LifetimeEvaluatorTest::run(const vector<FakeCodeline>& code, const temp_lt_expect& e)
{
   bool success = false;
   auto result = run(code, success);
   ASSERT_TRUE(success);
   ASSERT_EQ(result.first.size(), e.size());
   check(result.first, e);
}

void LifetimeEvaluatorTest::run(const vector<FakeCodeline>& code, const array_lt_expect& e)
{
   bool success = false;
   auto result = run(code, success);
   ASSERT_TRUE(success);
   ASSERT_EQ(result.second.size(), e.size());
   check(result.second, e);
}

void LifetimeEvaluatorExactTest::check( const vector<register_live_range>& lifetimes,
                                        const temp_lt_expect& e)
{
   for (unsigned i = 1; i < lifetimes.size(); ++i) {
      EXPECT_EQ(lifetimes[i].begin, e[i][0]);
      EXPECT_EQ(lifetimes[i].end, e[i][1]);
   }
}

void LifetimeEvaluatorExactTest::check(const vector<array_live_range>& lifetimes,
				       const array_lt_expect& e)
{
   for (unsigned i = 0; i < lifetimes.size(); ++i) {
      EXPECT_EQ(lifetimes[i].begin(), e[i].begin());
      EXPECT_EQ(lifetimes[i].end(), e[i].end());
      EXPECT_EQ(lifetimes[i].access_mask(), e[i].access_mask());
   }
}

void LifetimeEvaluatorAtLeastTest::check( const vector<register_live_range>& lifetimes,
                                          const temp_lt_expect& e)
{
   for (unsigned i = 1; i < lifetimes.size(); ++i) {
      EXPECT_LE(lifetimes[i].begin, e[i][0]);
      EXPECT_GE(lifetimes[i].end, e[i][1]);
   }
}

void LifetimeEvaluatorAtLeastTest::check(const vector<array_live_range>& lifetimes,
					 const array_lt_expect& e)
{
   for (unsigned i = 0; i < lifetimes.size(); ++i) {
      EXPECT_LE(lifetimes[i].begin(), e[i].begin());
      EXPECT_GE(lifetimes[i].end(), e[i].end());

      /* Tests that lifetimes doesn't add unexpected swizzles */
      EXPECT_EQ(lifetimes[i].access_mask()| e[i].access_mask(),
		e[i].access_mask());
   }
}


void RegisterRemappingTest::run(const vector<register_live_range>& lt,
                                const vector<int>& expect)
{
   rename_reg_pair proto{false,0};
   vector<rename_reg_pair> result(lt.size(), proto);

   get_temp_registers_remapping(mem_ctx, lt.size(), &lt[0], &result[0]);

   vector<int> remap(lt.size());
   for (unsigned i = 0; i < lt.size(); ++i) {
      remap[i] = result[i].valid ? result[i].new_reg : i;
   }

   std::transform(remap.begin(), remap.end(), result.begin(), remap.begin(),
                  [](int x, const rename_reg_pair& rn) {
                     return rn.valid ? rn.new_reg : x;
                  });

   for(unsigned i = 1; i < remap.size(); ++i) {
      EXPECT_EQ(remap[i], expect[i]);
   }
}

void RegisterLifetimeAndRemappingTest::run(const vector<FakeCodeline>& code,
                                           const vector<int>& expect)
{
     FakeShader shader(code);
     std::vector<register_live_range> lt(shader.get_num_temps());
     std::vector<array_live_range> alt(shader.get_num_arrays());
     get_temp_registers_required_live_ranges(mem_ctx, shader.get_program(mem_ctx),
					     shader.get_num_temps(), &lt[0],
					     shader.get_num_arrays(), &alt[0]);
     this->run(lt, expect);
}
