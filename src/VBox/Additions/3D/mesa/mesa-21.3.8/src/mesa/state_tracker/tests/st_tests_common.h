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

#ifndef mesa_st_tests_h
#define mesa_st_tests_h

#include "state_tracker/st_glsl_to_tgsi_temprename.h"
#include "state_tracker/st_glsl_to_tgsi_array_merge.h"
#include "gtest/gtest.h"

#include <utility>

#define MP(X, W) std::make_pair(X, W)
#define MT(X,Y,Z) std::make_tuple(X,Y,Z)

/* Use this to make the compiler pick the swizzle constructor below */
struct SWZ {};

/* Use this to make the compiler pick the constructor with reladdr below */
struct RA {};

/* Use this to make the compiler pick the constructor with array below */
struct ARR {};

/* A line to describe a TGSI instruction for building mock shaders. */
struct FakeCodeline {
   FakeCodeline(tgsi_opcode _op): op(_op), max_temp_id(0), max_array_id(0) {}
   FakeCodeline(tgsi_opcode _op, const std::vector<int>& _dst, const std::vector<int>& _src,
                const std::vector<int>&_to);

   FakeCodeline(tgsi_opcode _op, const std::vector<std::pair<int,int>>& _dst,
                const std::vector<std::pair<int, const char *>>& _src,
                const std::vector<std::pair<int, const char *>>&_to, SWZ with_swizzle);

   FakeCodeline(tgsi_opcode _op, const std::vector<std::tuple<int,int,int>>& _dst,
                const std::vector<std::tuple<int,int,int>>& _src,
                const std::vector<std::tuple<int,int,int>>&_to, RA with_reladdr);

   FakeCodeline(tgsi_opcode _op, const std::vector<std::tuple<int, int, int> > &_dst,
		const std::vector<std::tuple<int,int, const char*>>& _src,
		const std::vector<std::tuple<int,int, const char*>>&_to, ARR with_array);

   FakeCodeline(const glsl_to_tgsi_instruction& inst);

   int get_max_reg_id() const { return max_temp_id;}
   int get_max_array_id() const { return max_array_id;}

   glsl_to_tgsi_instruction *get_codeline() const;

   static void set_mem_ctx(void *ctx);

   friend bool operator == (const FakeCodeline& lsh, const FakeCodeline& rhs);

   void print(std::ostream& os) const;
private:
   st_src_reg create_src_register(int src_idx);
   st_src_reg create_src_register(int src_idx, const char *swizzle);
   st_src_reg create_src_register(int src_idx, gl_register_file file);
   st_src_reg create_src_register(const std::tuple<int,int,int>& src);
   st_src_reg *create_rel_src_register(int idx);
   st_src_reg create_array_src_register(const std::tuple<int,int,const char*>& r);
   st_dst_reg create_array_dst_register(const std::tuple<int,int,int>& r);

   st_dst_reg create_dst_register(int dst_idx);
   st_dst_reg create_dst_register(int dst_idx, int writemask);
   st_dst_reg create_dst_register(int dst_idx, gl_register_file file);
   st_dst_reg create_dst_register(const std::tuple<int,int,int>& dest);

   template <typename st_reg>
   void read_reg(const st_reg& s);

   tgsi_opcode op;
   std::vector<st_dst_reg> dst;
   std::vector<st_src_reg> src;
   std::vector<st_src_reg> tex_offsets;

   int max_temp_id;
   int max_array_id;
   static void *mem_ctx;
};

inline std::ostream& operator << (std::ostream& os, const FakeCodeline& line)
{
   line.print(os);
   return os;
}

/* A few constants that will not be tracked as temporary registers
   by the fake shader.
 */
const int in0 = -1;
const int in1 = -2;
const int in2 = -3;

const int out0 = -1;
const int out1 = -2;
const int out2 = -3;

class FakeShader {
public:
   FakeShader(const std::vector<FakeCodeline>& source);
   FakeShader(exec_list *tgsi_prog);

   exec_list* get_program(void *ctx) const;
   int get_num_temps() const;
   int get_num_arrays() const;

   size_t length() const;

   const FakeCodeline& line(unsigned i) const;

private:

   std::vector<FakeCodeline> program;
   int num_temps;
   int num_arrays;
};

using temp_lt_expect = std::vector<std::vector<int>>;
using array_lt_expect = std::vector<array_live_range>;

class MesaTestWithMemCtx : public testing::Test {
   void SetUp();
   void TearDown();
protected:
   void *mem_ctx;
};

class LifetimeEvaluatorTest : public MesaTestWithMemCtx {
protected:
   void run(const std::vector<FakeCodeline>& code, const temp_lt_expect& e);
   void run(const std::vector<FakeCodeline>& code, const array_lt_expect& e);
private:
   using life_range_result=std::pair<std::vector<register_live_range>,
				   std::vector<array_live_range>>;
   life_range_result run(const std::vector<FakeCodeline>& code, bool& success);

   virtual void check(const std::vector<register_live_range>& result,
		      const temp_lt_expect& e) = 0;
   virtual void check(const std::vector<array_live_range>& lifetimes,
		      const array_lt_expect& e) = 0;
};

/* This is a test class to check the exact life times of
 * registers. */
class LifetimeEvaluatorExactTest : public LifetimeEvaluatorTest {
protected:
   void check(const std::vector<register_live_range>& result,
	      const temp_lt_expect& e);

   void check(const std::vector<array_live_range>& result,
	      const array_lt_expect& e);
};

/* This test class checks that the life time covers at least
 * in the expected range. It is used for cases where we know that
 * a the implementation could be improved on estimating the minimal
 * life time.
 */
class LifetimeEvaluatorAtLeastTest : public LifetimeEvaluatorTest {
protected:
   void check(const std::vector<register_live_range>& result, const temp_lt_expect& e);
   void check(const std::vector<array_live_range>& result,
	      const array_lt_expect& e);
};

/* With this test class the renaming mapping estimation is tested */
class RegisterRemappingTest : public MesaTestWithMemCtx {
protected:
   void run(const std::vector<register_live_range>& lt,
	    const std::vector<int> &expect);
};

/* With this test class the combined lifetime estimation and renaming
 * mepping estimation is tested
 */
class RegisterLifetimeAndRemappingTest : public RegisterRemappingTest  {
protected:
   using RegisterRemappingTest::run;
   void run(const std::vector<FakeCodeline>& code, const std::vector<int> &expect);
};

#endif
