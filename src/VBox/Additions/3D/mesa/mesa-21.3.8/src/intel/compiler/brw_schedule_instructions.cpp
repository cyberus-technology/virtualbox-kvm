/*
 * Copyright © 2010 Intel Corporation
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
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_eu.h"
#include "brw_fs.h"
#include "brw_fs_live_variables.h"
#include "brw_vec4.h"
#include "brw_cfg.h"
#include "brw_shader.h"

using namespace brw;

/** @file brw_fs_schedule_instructions.cpp
 *
 * List scheduling of FS instructions.
 *
 * The basic model of the list scheduler is to take a basic block,
 * compute a DAG of the dependencies (RAW ordering with latency, WAW
 * ordering with latency, WAR ordering), and make a list of the DAG heads.
 * Heuristically pick a DAG head, then put all the children that are
 * now DAG heads into the list of things to schedule.
 *
 * The heuristic is the important part.  We're trying to be cheap,
 * since actually computing the optimal scheduling is NP complete.
 * What we do is track a "current clock".  When we schedule a node, we
 * update the earliest-unblocked clock time of its children, and
 * increment the clock.  Then, when trying to schedule, we just pick
 * the earliest-unblocked instruction to schedule.
 *
 * Note that often there will be many things which could execute
 * immediately, and there are a range of heuristic options to choose
 * from in picking among those.
 */

static bool debug = false;

class instruction_scheduler;

class schedule_node : public exec_node
{
public:
   schedule_node(backend_instruction *inst, instruction_scheduler *sched);
   void set_latency_gfx4();
   void set_latency_gfx7(bool is_haswell);

   const struct intel_device_info *devinfo;
   backend_instruction *inst;
   schedule_node **children;
   int *child_latency;
   int child_count;
   int parent_count;
   int child_array_size;
   int unblocked_time;
   int latency;

   /**
    * Which iteration of pushing groups of children onto the candidates list
    * this node was a part of.
    */
   unsigned cand_generation;

   /**
    * This is the sum of the instruction's latency plus the maximum delay of
    * its children, or just the issue_time if it's a leaf node.
    */
   int delay;

   /**
    * Preferred exit node among the (direct or indirect) successors of this
    * node.  Among the scheduler nodes blocked by this node, this will be the
    * one that may cause earliest program termination, or NULL if none of the
    * successors is an exit node.
    */
   schedule_node *exit;
};

/**
 * Lower bound of the scheduling time after which one of the instructions
 * blocked by this node may lead to program termination.
 *
 * exit_unblocked_time() determines a strict partial ordering relation '«' on
 * the set of scheduler nodes as follows:
 *
 *   n « m <-> exit_unblocked_time(n) < exit_unblocked_time(m)
 *
 * which can be used to heuristically order nodes according to how early they
 * can unblock an exit node and lead to program termination.
 */
static inline int
exit_unblocked_time(const schedule_node *n)
{
   return n->exit ? n->exit->unblocked_time : INT_MAX;
}

void
schedule_node::set_latency_gfx4()
{
   int chans = 8;
   int math_latency = 22;

   switch (inst->opcode) {
   case SHADER_OPCODE_RCP:
      this->latency = 1 * chans * math_latency;
      break;
   case SHADER_OPCODE_RSQ:
      this->latency = 2 * chans * math_latency;
      break;
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_LOG2:
      /* full precision log.  partial is 2. */
      this->latency = 3 * chans * math_latency;
      break;
   case SHADER_OPCODE_INT_REMAINDER:
   case SHADER_OPCODE_EXP2:
      /* full precision.  partial is 3, same throughput. */
      this->latency = 4 * chans * math_latency;
      break;
   case SHADER_OPCODE_POW:
      this->latency = 8 * chans * math_latency;
      break;
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      /* minimum latency, max is 12 rounds. */
      this->latency = 5 * chans * math_latency;
      break;
   default:
      this->latency = 2;
      break;
   }
}

void
schedule_node::set_latency_gfx7(bool is_haswell)
{
   switch (inst->opcode) {
   case BRW_OPCODE_MAD:
      /* 2 cycles
       *  (since the last two src operands are in different register banks):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 3 cycles on IVB, 4 on HSW
       *  (since the last two src operands are in the same register bank):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 18 cycles on IVB, 16 on HSW
       *  (since the last two src operands are in different register banks):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,5,1>F                     { align16 WE_normal 1Q };
       *
       * 20 cycles on IVB, 18 on HSW
       *  (since the last two src operands are in the same register bank):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,4,1>F                     { align16 WE_normal 1Q };
       */

      /* Our register allocator doesn't know about register banks, so use the
       * higher latency.
       */
      latency = is_haswell ? 16 : 18;
      break;

   case BRW_OPCODE_LRP:
      /* 2 cycles
       *  (since the last two src operands are in different register banks):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 3 cycles on IVB, 4 on HSW
       *  (since the last two src operands are in the same register bank):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 16 cycles on IVB, 14 on HSW
       *  (since the last two src operands are in different register banks):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,4,1>F                     { align16 WE_normal 1Q };
       *
       * 16 cycles
       *  (since the last two src operands are in the same register bank):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,4,1>F                     { align16 WE_normal 1Q };
       */

      /* Our register allocator doesn't know about register banks, so use the
       * higher latency.
       */
      latency = 14;
      break;

   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      /* 2 cycles:
       * math inv(8) g4<1>F g2<0,1,0>F      null       { align1 WE_normal 1Q };
       *
       * 18 cycles:
       * math inv(8) g4<1>F g2<0,1,0>F      null       { align1 WE_normal 1Q };
       * mov(8)      null   g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * Same for exp2, log2, rsq, sqrt, sin, cos.
       */
      latency = is_haswell ? 14 : 16;
      break;

   case SHADER_OPCODE_POW:
      /* 2 cycles:
       * math pow(8) g4<1>F g2<0,1,0>F   g2.1<0,1,0>F  { align1 WE_normal 1Q };
       *
       * 26 cycles:
       * math pow(8) g4<1>F g2<0,1,0>F   g2.1<0,1,0>F  { align1 WE_normal 1Q };
       * mov(8)      null   g4<8,8,1>F                 { align1 WE_normal 1Q };
       */
      latency = is_haswell ? 22 : 24;
      break;

   case SHADER_OPCODE_TEX:
   case SHADER_OPCODE_TXD:
   case SHADER_OPCODE_TXF:
   case SHADER_OPCODE_TXF_LZ:
   case SHADER_OPCODE_TXL:
   case SHADER_OPCODE_TXL_LZ:
      /* 18 cycles:
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       *
       * 697 +/-49 cycles (min 610, n=26):
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * So the latency on our first texture load of the batchbuffer takes
       * ~700 cycles, since the caches are cold at that point.
       *
       * 840 +/- 92 cycles (min 720, n=25):
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * On the second load, it takes just an extra ~140 cycles, and after
       * accounting for the 14 cycles of the MOV's latency, that makes ~130.
       *
       * 683 +/- 49 cycles (min = 602, n=47):
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * send(8) g50<1>UW   g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * The unit appears to be pipelined, since this matches up with the
       * cache-cold case, despite there being two loads here.  If you replace
       * the g4 in the MOV to null with g50, it's still 693 +/- 52 (n=39).
       *
       * So, take some number between the cache-hot 140 cycles and the
       * cache-cold 700 cycles.  No particular tuning was done on this.
       *
       * I haven't done significant testing of the non-TEX opcodes.  TXL at
       * least looked about the same as TEX.
       */
      latency = 200;
      break;

   case SHADER_OPCODE_TXS:
      /* Testing textureSize(sampler2D, 0), one load was 420 +/- 41
       * cycles (n=15):
       * mov(8)   g114<1>UD  0D                        { align1 WE_normal 1Q };
       * send(8)  g6<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 10, 1) mlen 1 rlen 4        { align1 WE_normal 1Q };
       * mov(16)  g6<1>F     g6<8,8,1>D                { align1 WE_normal 1Q };
       *
       *
       * Two loads was 535 +/- 30 cycles (n=19):
       * mov(16)   g114<1>UD  0D                       { align1 WE_normal 1H };
       * send(16)  g6<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 10, 2) mlen 2 rlen 8        { align1 WE_normal 1H };
       * mov(16)   g114<1>UD  0D                       { align1 WE_normal 1H };
       * mov(16)   g6<1>F     g6<8,8,1>D               { align1 WE_normal 1H };
       * send(16)  g8<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 10, 2) mlen 2 rlen 8        { align1 WE_normal 1H };
       * mov(16)   g8<1>F     g8<8,8,1>D               { align1 WE_normal 1H };
       * add(16)   g6<1>F     g6<8,8,1>F   g8<8,8,1>F  { align1 WE_normal 1H };
       *
       * Since the only caches that should matter are just the
       * instruction/state cache containing the surface state, assume that we
       * always have hot caches.
       */
      latency = 100;
      break;

   case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GFX4:
   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD_GFX7:
   case VS_OPCODE_PULL_CONSTANT_LOAD:
      /* testing using varying-index pull constants:
       *
       * 16 cycles:
       * mov(8)  g4<1>D  g2.1<0,1,0>F                  { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       *
       * ~480 cycles:
       * mov(8)  g4<1>D  g2.1<0,1,0>F                  { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       * mov(8)  null    g4<8,8,1>F                    { align1 WE_normal 1Q };
       *
       * ~620 cycles:
       * mov(8)  g4<1>D  g2.1<0,1,0>F                  { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       * mov(8)  null    g4<8,8,1>F                    { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       * mov(8)  null    g4<8,8,1>F                    { align1 WE_normal 1Q };
       *
       * So, if it's cache-hot, it's about 140.  If it's cache cold, it's
       * about 460.  We expect to mostly be cache hot, so pick something more
       * in that direction.
       */
      latency = 200;
      break;

   case SHADER_OPCODE_GFX7_SCRATCH_READ:
      /* Testing a load from offset 0, that had been previously written:
       *
       * send(8) g114<1>UW g0<8,8,1>F data (0, 0, 0) mlen 1 rlen 1 { align1 WE_normal 1Q };
       * mov(8)  null      g114<8,8,1>F { align1 WE_normal 1Q };
       *
       * The cycles spent seemed to be grouped around 40-50 (as low as 38),
       * then around 140.  Presumably this is cache hit vs miss.
       */
      latency = 50;
      break;

   case VEC4_OPCODE_UNTYPED_ATOMIC:
      /* See GFX7_DATAPORT_DC_UNTYPED_ATOMIC_OP */
      latency = 14000;
      break;

   case VEC4_OPCODE_UNTYPED_SURFACE_READ:
   case VEC4_OPCODE_UNTYPED_SURFACE_WRITE:
      /* See also GFX7_DATAPORT_DC_UNTYPED_SURFACE_READ */
      latency = is_haswell ? 300 : 600;
      break;

   case SHADER_OPCODE_SEND:
      switch (inst->sfid) {
      case BRW_SFID_SAMPLER: {
         unsigned msg_type = (inst->desc >> 12) & 0x1f;
         switch (msg_type) {
         case GFX5_SAMPLER_MESSAGE_SAMPLE_RESINFO:
         case GFX6_SAMPLER_MESSAGE_SAMPLE_SAMPLEINFO:
            /* See also SHADER_OPCODE_TXS */
            latency = 100;
            break;

         default:
            /* See also SHADER_OPCODE_TEX */
            latency = 200;
            break;
         }
         break;
      }

      case GFX6_SFID_DATAPORT_RENDER_CACHE:
         switch (brw_fb_desc_msg_type(devinfo, inst->desc)) {
         case GFX7_DATAPORT_RC_TYPED_SURFACE_WRITE:
         case GFX7_DATAPORT_RC_TYPED_SURFACE_READ:
            /* See also SHADER_OPCODE_TYPED_SURFACE_READ */
            assert(!is_haswell);
            latency = 600;
            break;

         case GFX7_DATAPORT_RC_TYPED_ATOMIC_OP:
            /* See also SHADER_OPCODE_TYPED_ATOMIC */
            assert(!is_haswell);
            latency = 14000;
            break;

         case GFX6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE:
            /* completely fabricated number */
            latency = 600;
            break;

         default:
            unreachable("Unknown render cache message");
         }
         break;

      case GFX7_SFID_DATAPORT_DATA_CACHE:
         switch ((inst->desc >> 14) & 0x1f) {
         case BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ:
         case GFX7_DATAPORT_DC_UNALIGNED_OWORD_BLOCK_READ:
         case GFX6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE:
            /* We have no data for this but assume it's a little faster than
             * untyped surface read/write.
             */
            latency = 200;
            break;

         case GFX7_DATAPORT_DC_DWORD_SCATTERED_READ:
         case GFX6_DATAPORT_WRITE_MESSAGE_DWORD_SCATTERED_WRITE:
         case HSW_DATAPORT_DC_PORT0_BYTE_SCATTERED_READ:
         case HSW_DATAPORT_DC_PORT0_BYTE_SCATTERED_WRITE:
            /* We have no data for this but assume it's roughly the same as
             * untyped surface read/write.
             */
            latency = 300;
            break;

         case GFX7_DATAPORT_DC_UNTYPED_SURFACE_READ:
         case GFX7_DATAPORT_DC_UNTYPED_SURFACE_WRITE:
            /* Test code:
             *   mov(8)    g112<1>UD       0x00000000UD       { align1 WE_all 1Q };
             *   mov(1)    g112.7<1>UD     g1.7<0,1,0>UD      { align1 WE_all };
             *   mov(8)    g113<1>UD       0x00000000UD       { align1 WE_normal 1Q };
             *   send(8)   g4<1>UD         g112<8,8,1>UD
             *             data (38, 6, 5) mlen 2 rlen 1      { align1 WE_normal 1Q };
             *   .
             *   . [repeats 8 times]
             *   .
             *   mov(8)    g112<1>UD       0x00000000UD       { align1 WE_all 1Q };
             *   mov(1)    g112.7<1>UD     g1.7<0,1,0>UD      { align1 WE_all };
             *   mov(8)    g113<1>UD       0x00000000UD       { align1 WE_normal 1Q };
             *   send(8)   g4<1>UD         g112<8,8,1>UD
             *             data (38, 6, 5) mlen 2 rlen 1      { align1 WE_normal 1Q };
             *
             * Running it 100 times as fragment shader on a 128x128 quad
             * gives an average latency of 583 cycles per surface read,
             * standard deviation 0.9%.
             */
            assert(!is_haswell);
            latency = 600;
            break;

         case GFX7_DATAPORT_DC_UNTYPED_ATOMIC_OP:
            /* Test code:
             *   mov(8)    g112<1>ud       0x00000000ud       { align1 WE_all 1Q };
             *   mov(1)    g112.7<1>ud     g1.7<0,1,0>ud      { align1 WE_all };
             *   mov(8)    g113<1>ud       0x00000000ud       { align1 WE_normal 1Q };
             *   send(8)   g4<1>ud         g112<8,8,1>ud
             *             data (38, 5, 6) mlen 2 rlen 1      { align1 WE_normal 1Q };
             *
             * Running it 100 times as fragment shader on a 128x128 quad
             * gives an average latency of 13867 cycles per atomic op,
             * standard deviation 3%.  Note that this is a rather
             * pessimistic estimate, the actual latency in cases with few
             * collisions between threads and favorable pipelining has been
             * seen to be reduced by a factor of 100.
             */
            assert(!is_haswell);
            latency = 14000;
            break;

         default:
            unreachable("Unknown data cache message");
         }
         break;

      case HSW_SFID_DATAPORT_DATA_CACHE_1:
         switch ((inst->desc >> 14) & 0x1f) {
         case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ:
         case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE:
         case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ:
         case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE:
         case GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_SURFACE_WRITE:
         case GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_SURFACE_READ:
         case GFX8_DATAPORT_DC_PORT1_A64_SCATTERED_WRITE:
         case GFX9_DATAPORT_DC_PORT1_A64_SCATTERED_READ:
         case GFX9_DATAPORT_DC_PORT1_A64_OWORD_BLOCK_READ:
         case GFX9_DATAPORT_DC_PORT1_A64_OWORD_BLOCK_WRITE:
            /* See also GFX7_DATAPORT_DC_UNTYPED_SURFACE_READ */
            latency = 300;
            break;

         case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP:
         case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2:
         case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2:
         case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP:
         case GFX9_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_FLOAT_OP:
         case GFX8_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_OP:
         case GFX9_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_FLOAT_OP:
         case GFX12_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_HALF_INT_OP:
         case GFX12_DATAPORT_DC_PORT1_A64_UNTYPED_ATOMIC_HALF_FLOAT_OP:
            /* See also GFX7_DATAPORT_DC_UNTYPED_ATOMIC_OP */
            latency = 14000;
            break;

         default:
            unreachable("Unknown data cache message");
         }
         break;

      case GFX12_SFID_UGM:
      case GFX12_SFID_TGM:
      case GFX12_SFID_SLM:
         switch (lsc_msg_desc_opcode(devinfo, inst->desc)) {
         case LSC_OP_LOAD:
         case LSC_OP_STORE:
         case LSC_OP_LOAD_CMASK:
         case LSC_OP_STORE_CMASK:
            latency = 300;
            break;
         case LSC_OP_FENCE:
         case LSC_OP_ATOMIC_INC:
         case LSC_OP_ATOMIC_DEC:
         case LSC_OP_ATOMIC_LOAD:
         case LSC_OP_ATOMIC_STORE:
         case LSC_OP_ATOMIC_ADD:
         case LSC_OP_ATOMIC_SUB:
         case LSC_OP_ATOMIC_MIN:
         case LSC_OP_ATOMIC_MAX:
         case LSC_OP_ATOMIC_UMIN:
         case LSC_OP_ATOMIC_UMAX:
         case LSC_OP_ATOMIC_CMPXCHG:
         case LSC_OP_ATOMIC_FADD:
         case LSC_OP_ATOMIC_FSUB:
         case LSC_OP_ATOMIC_FMIN:
         case LSC_OP_ATOMIC_FMAX:
         case LSC_OP_ATOMIC_FCMPXCHG:
         case LSC_OP_ATOMIC_AND:
         case LSC_OP_ATOMIC_OR:
         case LSC_OP_ATOMIC_XOR:
            latency = 1400;
            break;
         default:
            unreachable("unsupported new data port message instruction");
         }
         break;

      case GEN_RT_SFID_BINDLESS_THREAD_DISPATCH:
      case GEN_RT_SFID_RAY_TRACE_ACCELERATOR:
         /* TODO.
          *
          * We'll assume for the moment that this is pretty quick as it
          * doesn't actually return any data.
          */
         latency = 200;
         break;

      default:
         unreachable("Unknown SFID");
      }
      break;

   default:
      /* 2 cycles:
       * mul(8) g4<1>F g2<0,1,0>F      0.5F            { align1 WE_normal 1Q };
       *
       * 16 cycles:
       * mul(8) g4<1>F g2<0,1,0>F      0.5F            { align1 WE_normal 1Q };
       * mov(8) null   g4<8,8,1>F                      { align1 WE_normal 1Q };
       */
      latency = 14;
      break;
   }
}

class instruction_scheduler {
public:
   instruction_scheduler(const backend_shader *s, int grf_count,
                         unsigned hw_reg_count, int block_count,
                         instruction_scheduler_mode mode):
      bs(s)
   {
      this->mem_ctx = ralloc_context(NULL);
      this->grf_count = grf_count;
      this->hw_reg_count = hw_reg_count;
      this->instructions.make_empty();
      this->post_reg_alloc = (mode == SCHEDULE_POST);
      this->mode = mode;
      this->reg_pressure = 0;
      this->block_idx = 0;
      if (!post_reg_alloc) {
         this->reg_pressure_in = rzalloc_array(mem_ctx, int, block_count);

         this->livein = ralloc_array(mem_ctx, BITSET_WORD *, block_count);
         for (int i = 0; i < block_count; i++)
            this->livein[i] = rzalloc_array(mem_ctx, BITSET_WORD,
                                            BITSET_WORDS(grf_count));

         this->liveout = ralloc_array(mem_ctx, BITSET_WORD *, block_count);
         for (int i = 0; i < block_count; i++)
            this->liveout[i] = rzalloc_array(mem_ctx, BITSET_WORD,
                                             BITSET_WORDS(grf_count));

         this->hw_liveout = ralloc_array(mem_ctx, BITSET_WORD *, block_count);
         for (int i = 0; i < block_count; i++)
            this->hw_liveout[i] = rzalloc_array(mem_ctx, BITSET_WORD,
                                                BITSET_WORDS(hw_reg_count));

         this->written = rzalloc_array(mem_ctx, bool, grf_count);

         this->reads_remaining = rzalloc_array(mem_ctx, int, grf_count);

         this->hw_reads_remaining = rzalloc_array(mem_ctx, int, hw_reg_count);
      } else {
         this->reg_pressure_in = NULL;
         this->livein = NULL;
         this->liveout = NULL;
         this->hw_liveout = NULL;
         this->written = NULL;
         this->reads_remaining = NULL;
         this->hw_reads_remaining = NULL;
      }
   }

   ~instruction_scheduler()
   {
      ralloc_free(this->mem_ctx);
   }
   void add_barrier_deps(schedule_node *n);
   void add_dep(schedule_node *before, schedule_node *after, int latency);
   void add_dep(schedule_node *before, schedule_node *after);

   void run(cfg_t *cfg);
   void add_insts_from_block(bblock_t *block);
   void compute_delays();
   void compute_exits();
   virtual void calculate_deps() = 0;
   virtual schedule_node *choose_instruction_to_schedule() = 0;

   /**
    * Returns how many cycles it takes the instruction to issue.
    *
    * Instructions in gen hardware are handled one simd4 vector at a time,
    * with 1 cycle per vector dispatched.  Thus SIMD8 pixel shaders take 2
    * cycles to dispatch and SIMD16 (compressed) instructions take 4.
    */
   virtual int issue_time(backend_instruction *inst) = 0;

   virtual void count_reads_remaining(backend_instruction *inst) = 0;
   virtual void setup_liveness(cfg_t *cfg) = 0;
   virtual void update_register_pressure(backend_instruction *inst) = 0;
   virtual int get_register_pressure_benefit(backend_instruction *inst) = 0;

   void schedule_instructions(bblock_t *block);

   void *mem_ctx;

   bool post_reg_alloc;
   int grf_count;
   unsigned hw_reg_count;
   int reg_pressure;
   int block_idx;
   exec_list instructions;
   const backend_shader *bs;

   instruction_scheduler_mode mode;

   /*
    * The register pressure at the beginning of each basic block.
    */

   int *reg_pressure_in;

   /*
    * The virtual GRF's whose range overlaps the beginning of each basic block.
    */

   BITSET_WORD **livein;

   /*
    * The virtual GRF's whose range overlaps the end of each basic block.
    */

   BITSET_WORD **liveout;

   /*
    * The hardware GRF's whose range overlaps the end of each basic block.
    */

   BITSET_WORD **hw_liveout;

   /*
    * Whether we've scheduled a write for this virtual GRF yet.
    */

   bool *written;

   /*
    * How many reads we haven't scheduled for this virtual GRF yet.
    */

   int *reads_remaining;

   /*
    * How many reads we haven't scheduled for this hardware GRF yet.
    */

   int *hw_reads_remaining;
};

class fs_instruction_scheduler : public instruction_scheduler
{
public:
   fs_instruction_scheduler(const fs_visitor *v, int grf_count, int hw_reg_count,
                            int block_count,
                            instruction_scheduler_mode mode);
   void calculate_deps();
   bool is_compressed(const fs_inst *inst);
   schedule_node *choose_instruction_to_schedule();
   int issue_time(backend_instruction *inst);
   const fs_visitor *v;

   void count_reads_remaining(backend_instruction *inst);
   void setup_liveness(cfg_t *cfg);
   void update_register_pressure(backend_instruction *inst);
   int get_register_pressure_benefit(backend_instruction *inst);
};

fs_instruction_scheduler::fs_instruction_scheduler(const fs_visitor *v,
                                                   int grf_count, int hw_reg_count,
                                                   int block_count,
                                                   instruction_scheduler_mode mode)
   : instruction_scheduler(v, grf_count, hw_reg_count, block_count, mode),
     v(v)
{
}

static bool
is_src_duplicate(fs_inst *inst, int src)
{
   for (int i = 0; i < src; i++)
     if (inst->src[i].equals(inst->src[src]))
       return true;

  return false;
}

void
fs_instruction_scheduler::count_reads_remaining(backend_instruction *be)
{
   fs_inst *inst = (fs_inst *)be;

   if (!reads_remaining)
      return;

   for (int i = 0; i < inst->sources; i++) {
      if (is_src_duplicate(inst, i))
         continue;

      if (inst->src[i].file == VGRF) {
         reads_remaining[inst->src[i].nr]++;
      } else if (inst->src[i].file == FIXED_GRF) {
         if (inst->src[i].nr >= hw_reg_count)
            continue;

         for (unsigned j = 0; j < regs_read(inst, i); j++)
            hw_reads_remaining[inst->src[i].nr + j]++;
      }
   }
}

void
fs_instruction_scheduler::setup_liveness(cfg_t *cfg)
{
   const fs_live_variables &live = v->live_analysis.require();

   /* First, compute liveness on a per-GRF level using the in/out sets from
    * liveness calculation.
    */
   for (int block = 0; block < cfg->num_blocks; block++) {
      for (int i = 0; i < live.num_vars; i++) {
         if (BITSET_TEST(live.block_data[block].livein, i)) {
            int vgrf = live.vgrf_from_var[i];
            if (!BITSET_TEST(livein[block], vgrf)) {
               reg_pressure_in[block] += v->alloc.sizes[vgrf];
               BITSET_SET(livein[block], vgrf);
            }
         }

         if (BITSET_TEST(live.block_data[block].liveout, i))
            BITSET_SET(liveout[block], live.vgrf_from_var[i]);
      }
   }

   /* Now, extend the live in/live out sets for when a range crosses a block
    * boundary, which matches what our register allocator/interference code
    * does to account for force_writemask_all and incompatible exec_mask's.
    */
   for (int block = 0; block < cfg->num_blocks - 1; block++) {
      for (int i = 0; i < grf_count; i++) {
         if (live.vgrf_start[i] <= cfg->blocks[block]->end_ip &&
             live.vgrf_end[i] >= cfg->blocks[block + 1]->start_ip) {
            if (!BITSET_TEST(livein[block + 1], i)) {
                reg_pressure_in[block + 1] += v->alloc.sizes[i];
                BITSET_SET(livein[block + 1], i);
            }

            BITSET_SET(liveout[block], i);
         }
      }
   }

   int payload_last_use_ip[hw_reg_count];
   v->calculate_payload_ranges(hw_reg_count, payload_last_use_ip);

   for (unsigned i = 0; i < hw_reg_count; i++) {
      if (payload_last_use_ip[i] == -1)
         continue;

      for (int block = 0; block < cfg->num_blocks; block++) {
         if (cfg->blocks[block]->start_ip <= payload_last_use_ip[i])
            reg_pressure_in[block]++;

         if (cfg->blocks[block]->end_ip <= payload_last_use_ip[i])
            BITSET_SET(hw_liveout[block], i);
      }
   }
}

void
fs_instruction_scheduler::update_register_pressure(backend_instruction *be)
{
   fs_inst *inst = (fs_inst *)be;

   if (!reads_remaining)
      return;

   if (inst->dst.file == VGRF) {
      written[inst->dst.nr] = true;
   }

   for (int i = 0; i < inst->sources; i++) {
      if (is_src_duplicate(inst, i))
          continue;

      if (inst->src[i].file == VGRF) {
         reads_remaining[inst->src[i].nr]--;
      } else if (inst->src[i].file == FIXED_GRF &&
                 inst->src[i].nr < hw_reg_count) {
         for (unsigned off = 0; off < regs_read(inst, i); off++)
            hw_reads_remaining[inst->src[i].nr + off]--;
      }
   }
}

int
fs_instruction_scheduler::get_register_pressure_benefit(backend_instruction *be)
{
   fs_inst *inst = (fs_inst *)be;
   int benefit = 0;

   if (inst->dst.file == VGRF) {
      if (!BITSET_TEST(livein[block_idx], inst->dst.nr) &&
          !written[inst->dst.nr])
         benefit -= v->alloc.sizes[inst->dst.nr];
   }

   for (int i = 0; i < inst->sources; i++) {
      if (is_src_duplicate(inst, i))
         continue;

      if (inst->src[i].file == VGRF &&
          !BITSET_TEST(liveout[block_idx], inst->src[i].nr) &&
          reads_remaining[inst->src[i].nr] == 1)
         benefit += v->alloc.sizes[inst->src[i].nr];

      if (inst->src[i].file == FIXED_GRF &&
          inst->src[i].nr < hw_reg_count) {
         for (unsigned off = 0; off < regs_read(inst, i); off++) {
            int reg = inst->src[i].nr + off;
            if (!BITSET_TEST(hw_liveout[block_idx], reg) &&
                hw_reads_remaining[reg] == 1) {
               benefit++;
            }
         }
      }
   }

   return benefit;
}

class vec4_instruction_scheduler : public instruction_scheduler
{
public:
   vec4_instruction_scheduler(const vec4_visitor *v, int grf_count);
   void calculate_deps();
   schedule_node *choose_instruction_to_schedule();
   int issue_time(backend_instruction *inst);
   const vec4_visitor *v;

   void count_reads_remaining(backend_instruction *inst);
   void setup_liveness(cfg_t *cfg);
   void update_register_pressure(backend_instruction *inst);
   int get_register_pressure_benefit(backend_instruction *inst);
};

vec4_instruction_scheduler::vec4_instruction_scheduler(const vec4_visitor *v,
                                                       int grf_count)
   : instruction_scheduler(v, grf_count, 0, 0, SCHEDULE_POST),
     v(v)
{
}

void
vec4_instruction_scheduler::count_reads_remaining(backend_instruction *)
{
}

void
vec4_instruction_scheduler::setup_liveness(cfg_t *)
{
}

void
vec4_instruction_scheduler::update_register_pressure(backend_instruction *)
{
}

int
vec4_instruction_scheduler::get_register_pressure_benefit(backend_instruction *)
{
   return 0;
}

schedule_node::schedule_node(backend_instruction *inst,
                             instruction_scheduler *sched)
{
   const struct intel_device_info *devinfo = sched->bs->devinfo;

   this->devinfo = devinfo;
   this->inst = inst;
   this->child_array_size = 0;
   this->children = NULL;
   this->child_latency = NULL;
   this->child_count = 0;
   this->parent_count = 0;
   this->unblocked_time = 0;
   this->cand_generation = 0;
   this->delay = 0;
   this->exit = NULL;

   /* We can't measure Gfx6 timings directly but expect them to be much
    * closer to Gfx7 than Gfx4.
    */
   if (!sched->post_reg_alloc)
      this->latency = 1;
   else if (devinfo->ver >= 6)
      set_latency_gfx7(devinfo->is_haswell);
   else
      set_latency_gfx4();
}

void
instruction_scheduler::add_insts_from_block(bblock_t *block)
{
   foreach_inst_in_block(backend_instruction, inst, block) {
      schedule_node *n = new(mem_ctx) schedule_node(inst, this);

      instructions.push_tail(n);
   }
}

/** Computation of the delay member of each node. */
void
instruction_scheduler::compute_delays()
{
   foreach_in_list_reverse(schedule_node, n, &instructions) {
      if (!n->child_count) {
         n->delay = issue_time(n->inst);
      } else {
         for (int i = 0; i < n->child_count; i++) {
            assert(n->children[i]->delay);
            n->delay = MAX2(n->delay, n->latency + n->children[i]->delay);
         }
      }
   }
}

void
instruction_scheduler::compute_exits()
{
   /* Calculate a lower bound of the scheduling time of each node in the
    * graph.  This is analogous to the node's critical path but calculated
    * from the top instead of from the bottom of the block.
    */
   foreach_in_list(schedule_node, n, &instructions) {
      for (int i = 0; i < n->child_count; i++) {
         n->children[i]->unblocked_time =
            MAX2(n->children[i]->unblocked_time,
                 n->unblocked_time + issue_time(n->inst) + n->child_latency[i]);
      }
   }

   /* Calculate the exit of each node by induction based on the exit nodes of
    * its children.  The preferred exit of a node is the one among the exit
    * nodes of its children which can be unblocked first according to the
    * optimistic unblocked time estimate calculated above.
    */
   foreach_in_list_reverse(schedule_node, n, &instructions) {
      n->exit = (n->inst->opcode == BRW_OPCODE_HALT ? n : NULL);

      for (int i = 0; i < n->child_count; i++) {
         if (exit_unblocked_time(n->children[i]) < exit_unblocked_time(n))
            n->exit = n->children[i]->exit;
      }
   }
}

/**
 * Add a dependency between two instruction nodes.
 *
 * The @after node will be scheduled after @before.  We will try to
 * schedule it @latency cycles after @before, but no guarantees there.
 */
void
instruction_scheduler::add_dep(schedule_node *before, schedule_node *after,
                               int latency)
{
   if (!before || !after)
      return;

   assert(before != after);

   for (int i = 0; i < before->child_count; i++) {
      if (before->children[i] == after) {
         before->child_latency[i] = MAX2(before->child_latency[i], latency);
         return;
      }
   }

   if (before->child_array_size <= before->child_count) {
      if (before->child_array_size < 16)
         before->child_array_size = 16;
      else
         before->child_array_size *= 2;

      before->children = reralloc(mem_ctx, before->children,
                                  schedule_node *,
                                  before->child_array_size);
      before->child_latency = reralloc(mem_ctx, before->child_latency,
                                       int, before->child_array_size);
   }

   before->children[before->child_count] = after;
   before->child_latency[before->child_count] = latency;
   before->child_count++;
   after->parent_count++;
}

void
instruction_scheduler::add_dep(schedule_node *before, schedule_node *after)
{
   if (!before)
      return;

   add_dep(before, after, before->latency);
}

static bool
is_scheduling_barrier(const backend_instruction *inst)
{
   return inst->opcode == SHADER_OPCODE_HALT_TARGET ||
          inst->is_control_flow() ||
          inst->has_side_effects();
}

/**
 * Sometimes we really want this node to execute after everything that
 * was before it and before everything that followed it.  This adds
 * the deps to do so.
 */
void
instruction_scheduler::add_barrier_deps(schedule_node *n)
{
   schedule_node *prev = (schedule_node *)n->prev;
   schedule_node *next = (schedule_node *)n->next;

   if (prev) {
      while (!prev->is_head_sentinel()) {
         add_dep(prev, n, 0);
         if (is_scheduling_barrier(prev->inst))
            break;
         prev = (schedule_node *)prev->prev;
      }
   }

   if (next) {
      while (!next->is_tail_sentinel()) {
         add_dep(n, next, 0);
         if (is_scheduling_barrier(next->inst))
            break;
         next = (schedule_node *)next->next;
      }
   }
}

/* instruction scheduling needs to be aware of when an MRF write
 * actually writes 2 MRFs.
 */
bool
fs_instruction_scheduler::is_compressed(const fs_inst *inst)
{
   return inst->exec_size == 16;
}

void
fs_instruction_scheduler::calculate_deps()
{
   /* Pre-register-allocation, this tracks the last write per VGRF offset.
    * After register allocation, reg_offsets are gone and we track individual
    * GRF registers.
    */
   schedule_node **last_grf_write;
   schedule_node *last_mrf_write[BRW_MAX_MRF(v->devinfo->ver)];
   schedule_node *last_conditional_mod[8] = {};
   schedule_node *last_accumulator_write = NULL;
   /* Fixed HW registers are assumed to be separate from the virtual
    * GRFs, so they can be tracked separately.  We don't really write
    * to fixed GRFs much, so don't bother tracking them on a more
    * granular level.
    */
   schedule_node *last_fixed_grf_write = NULL;

   last_grf_write = (schedule_node **)calloc(sizeof(schedule_node *), grf_count * 16);
   memset(last_mrf_write, 0, sizeof(last_mrf_write));

   /* top-to-bottom dependencies: RAW and WAW. */
   foreach_in_list(schedule_node, n, &instructions) {
      fs_inst *inst = (fs_inst *)n->inst;

      if (is_scheduling_barrier(inst))
         add_barrier_deps(n);

      /* read-after-write deps. */
      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file == VGRF) {
            if (post_reg_alloc) {
               for (unsigned r = 0; r < regs_read(inst, i); r++)
                  add_dep(last_grf_write[inst->src[i].nr + r], n);
            } else {
               for (unsigned r = 0; r < regs_read(inst, i); r++) {
                  add_dep(last_grf_write[inst->src[i].nr * 16 +
                                         inst->src[i].offset / REG_SIZE + r], n);
               }
            }
         } else if (inst->src[i].file == FIXED_GRF) {
            if (post_reg_alloc) {
               for (unsigned r = 0; r < regs_read(inst, i); r++)
                  add_dep(last_grf_write[inst->src[i].nr + r], n);
            } else {
               add_dep(last_fixed_grf_write, n);
            }
         } else if (inst->src[i].is_accumulator()) {
            add_dep(last_accumulator_write, n);
         } else if (inst->src[i].file == ARF && !inst->src[i].is_null()) {
            add_barrier_deps(n);
         }
      }

      if (inst->base_mrf != -1) {
         for (int i = 0; i < inst->mlen; i++) {
            /* It looks like the MRF regs are released in the send
             * instruction once it's sent, not when the result comes
             * back.
             */
            add_dep(last_mrf_write[inst->base_mrf + i], n);
         }
      }

      if (const unsigned mask = inst->flags_read(v->devinfo)) {
         assert(mask < (1 << ARRAY_SIZE(last_conditional_mod)));

         for (unsigned i = 0; i < ARRAY_SIZE(last_conditional_mod); i++) {
            if (mask & (1 << i))
               add_dep(last_conditional_mod[i], n);
         }
      }

      if (inst->reads_accumulator_implicitly()) {
         add_dep(last_accumulator_write, n);
      }

      /* write-after-write deps. */
      if (inst->dst.file == VGRF) {
         if (post_reg_alloc) {
            for (unsigned r = 0; r < regs_written(inst); r++) {
               add_dep(last_grf_write[inst->dst.nr + r], n);
               last_grf_write[inst->dst.nr + r] = n;
            }
         } else {
            for (unsigned r = 0; r < regs_written(inst); r++) {
               add_dep(last_grf_write[inst->dst.nr * 16 +
                                      inst->dst.offset / REG_SIZE + r], n);
               last_grf_write[inst->dst.nr * 16 +
                              inst->dst.offset / REG_SIZE + r] = n;
            }
         }
      } else if (inst->dst.file == MRF) {
         int reg = inst->dst.nr & ~BRW_MRF_COMPR4;

         add_dep(last_mrf_write[reg], n);
         last_mrf_write[reg] = n;
         if (is_compressed(inst)) {
            if (inst->dst.nr & BRW_MRF_COMPR4)
               reg += 4;
            else
               reg++;
            add_dep(last_mrf_write[reg], n);
            last_mrf_write[reg] = n;
         }
      } else if (inst->dst.file == FIXED_GRF) {
         if (post_reg_alloc) {
            for (unsigned r = 0; r < regs_written(inst); r++) {
               add_dep(last_grf_write[inst->dst.nr + r], n);
               last_grf_write[inst->dst.nr + r] = n;
            }
         } else {
            add_dep(last_fixed_grf_write, n);
            last_fixed_grf_write = n;
         }
      } else if (inst->dst.is_accumulator()) {
         add_dep(last_accumulator_write, n);
         last_accumulator_write = n;
      } else if (inst->dst.file == ARF && !inst->dst.is_null()) {
         add_barrier_deps(n);
      }

      if (inst->mlen > 0 && inst->base_mrf != -1) {
         for (unsigned i = 0; i < inst->implied_mrf_writes(); i++) {
            add_dep(last_mrf_write[inst->base_mrf + i], n);
            last_mrf_write[inst->base_mrf + i] = n;
         }
      }

      if (const unsigned mask = inst->flags_written(v->devinfo)) {
         assert(mask < (1 << ARRAY_SIZE(last_conditional_mod)));

         for (unsigned i = 0; i < ARRAY_SIZE(last_conditional_mod); i++) {
            if (mask & (1 << i)) {
               add_dep(last_conditional_mod[i], n, 0);
               last_conditional_mod[i] = n;
            }
         }
      }

      if (inst->writes_accumulator_implicitly(v->devinfo) &&
          !inst->dst.is_accumulator()) {
         add_dep(last_accumulator_write, n);
         last_accumulator_write = n;
      }
   }

   /* bottom-to-top dependencies: WAR */
   memset(last_grf_write, 0, sizeof(schedule_node *) * grf_count * 16);
   memset(last_mrf_write, 0, sizeof(last_mrf_write));
   memset(last_conditional_mod, 0, sizeof(last_conditional_mod));
   last_accumulator_write = NULL;
   last_fixed_grf_write = NULL;

   foreach_in_list_reverse_safe(schedule_node, n, &instructions) {
      fs_inst *inst = (fs_inst *)n->inst;

      /* write-after-read deps. */
      for (int i = 0; i < inst->sources; i++) {
         if (inst->src[i].file == VGRF) {
            if (post_reg_alloc) {
               for (unsigned r = 0; r < regs_read(inst, i); r++)
                  add_dep(n, last_grf_write[inst->src[i].nr + r], 0);
            } else {
               for (unsigned r = 0; r < regs_read(inst, i); r++) {
                  add_dep(n, last_grf_write[inst->src[i].nr * 16 +
                                            inst->src[i].offset / REG_SIZE + r], 0);
               }
            }
         } else if (inst->src[i].file == FIXED_GRF) {
            if (post_reg_alloc) {
               for (unsigned r = 0; r < regs_read(inst, i); r++)
                  add_dep(n, last_grf_write[inst->src[i].nr + r], 0);
            } else {
               add_dep(n, last_fixed_grf_write, 0);
            }
         } else if (inst->src[i].is_accumulator()) {
            add_dep(n, last_accumulator_write, 0);
         } else if (inst->src[i].file == ARF && !inst->src[i].is_null()) {
            add_barrier_deps(n);
         }
      }

      if (inst->base_mrf != -1) {
         for (int i = 0; i < inst->mlen; i++) {
            /* It looks like the MRF regs are released in the send
             * instruction once it's sent, not when the result comes
             * back.
             */
            add_dep(n, last_mrf_write[inst->base_mrf + i], 2);
         }
      }

      if (const unsigned mask = inst->flags_read(v->devinfo)) {
         assert(mask < (1 << ARRAY_SIZE(last_conditional_mod)));

         for (unsigned i = 0; i < ARRAY_SIZE(last_conditional_mod); i++) {
            if (mask & (1 << i))
               add_dep(n, last_conditional_mod[i]);
         }
      }

      if (inst->reads_accumulator_implicitly()) {
         add_dep(n, last_accumulator_write);
      }

      /* Update the things this instruction wrote, so earlier reads
       * can mark this as WAR dependency.
       */
      if (inst->dst.file == VGRF) {
         if (post_reg_alloc) {
            for (unsigned r = 0; r < regs_written(inst); r++)
               last_grf_write[inst->dst.nr + r] = n;
         } else {
            for (unsigned r = 0; r < regs_written(inst); r++) {
               last_grf_write[inst->dst.nr * 16 +
                              inst->dst.offset / REG_SIZE + r] = n;
            }
         }
      } else if (inst->dst.file == MRF) {
         int reg = inst->dst.nr & ~BRW_MRF_COMPR4;

         last_mrf_write[reg] = n;

         if (is_compressed(inst)) {
            if (inst->dst.nr & BRW_MRF_COMPR4)
               reg += 4;
            else
               reg++;

            last_mrf_write[reg] = n;
         }
      } else if (inst->dst.file == FIXED_GRF) {
         if (post_reg_alloc) {
            for (unsigned r = 0; r < regs_written(inst); r++)
               last_grf_write[inst->dst.nr + r] = n;
         } else {
            last_fixed_grf_write = n;
         }
      } else if (inst->dst.is_accumulator()) {
         last_accumulator_write = n;
      } else if (inst->dst.file == ARF && !inst->dst.is_null()) {
         add_barrier_deps(n);
      }

      if (inst->mlen > 0 && inst->base_mrf != -1) {
         for (unsigned i = 0; i < inst->implied_mrf_writes(); i++) {
            last_mrf_write[inst->base_mrf + i] = n;
         }
      }

      if (const unsigned mask = inst->flags_written(v->devinfo)) {
         assert(mask < (1 << ARRAY_SIZE(last_conditional_mod)));

         for (unsigned i = 0; i < ARRAY_SIZE(last_conditional_mod); i++) {
            if (mask & (1 << i))
               last_conditional_mod[i] = n;
         }
      }

      if (inst->writes_accumulator_implicitly(v->devinfo)) {
         last_accumulator_write = n;
      }
   }

   free(last_grf_write);
}

void
vec4_instruction_scheduler::calculate_deps()
{
   schedule_node *last_grf_write[grf_count];
   schedule_node *last_mrf_write[BRW_MAX_MRF(v->devinfo->ver)];
   schedule_node *last_conditional_mod = NULL;
   schedule_node *last_accumulator_write = NULL;
   /* Fixed HW registers are assumed to be separate from the virtual
    * GRFs, so they can be tracked separately.  We don't really write
    * to fixed GRFs much, so don't bother tracking them on a more
    * granular level.
    */
   schedule_node *last_fixed_grf_write = NULL;

   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));

   /* top-to-bottom dependencies: RAW and WAW. */
   foreach_in_list(schedule_node, n, &instructions) {
      vec4_instruction *inst = (vec4_instruction *)n->inst;

      if (is_scheduling_barrier(inst))
         add_barrier_deps(n);

      /* read-after-write deps. */
      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == VGRF) {
            for (unsigned j = 0; j < regs_read(inst, i); ++j)
               add_dep(last_grf_write[inst->src[i].nr + j], n);
         } else if (inst->src[i].file == FIXED_GRF) {
            add_dep(last_fixed_grf_write, n);
         } else if (inst->src[i].is_accumulator()) {
            assert(last_accumulator_write);
            add_dep(last_accumulator_write, n);
         } else if (inst->src[i].file == ARF && !inst->src[i].is_null()) {
            add_barrier_deps(n);
         }
      }

      if (inst->reads_g0_implicitly())
         add_dep(last_fixed_grf_write, n);

      if (!inst->is_send_from_grf()) {
         for (int i = 0; i < inst->mlen; i++) {
            /* It looks like the MRF regs are released in the send
             * instruction once it's sent, not when the result comes
             * back.
             */
            add_dep(last_mrf_write[inst->base_mrf + i], n);
         }
      }

      if (inst->reads_flag()) {
         assert(last_conditional_mod);
         add_dep(last_conditional_mod, n);
      }

      if (inst->reads_accumulator_implicitly()) {
         assert(last_accumulator_write);
         add_dep(last_accumulator_write, n);
      }

      /* write-after-write deps. */
      if (inst->dst.file == VGRF) {
         for (unsigned j = 0; j < regs_written(inst); ++j) {
            add_dep(last_grf_write[inst->dst.nr + j], n);
            last_grf_write[inst->dst.nr + j] = n;
         }
      } else if (inst->dst.file == MRF) {
         add_dep(last_mrf_write[inst->dst.nr], n);
         last_mrf_write[inst->dst.nr] = n;
     } else if (inst->dst.file == FIXED_GRF) {
         add_dep(last_fixed_grf_write, n);
         last_fixed_grf_write = n;
      } else if (inst->dst.is_accumulator()) {
         add_dep(last_accumulator_write, n);
         last_accumulator_write = n;
      } else if (inst->dst.file == ARF && !inst->dst.is_null()) {
         add_barrier_deps(n);
      }

      if (inst->mlen > 0 && !inst->is_send_from_grf()) {
         for (unsigned i = 0; i < inst->implied_mrf_writes(); i++) {
            add_dep(last_mrf_write[inst->base_mrf + i], n);
            last_mrf_write[inst->base_mrf + i] = n;
         }
      }

      if (inst->writes_flag(v->devinfo)) {
         add_dep(last_conditional_mod, n, 0);
         last_conditional_mod = n;
      }

      if (inst->writes_accumulator_implicitly(v->devinfo) &&
          !inst->dst.is_accumulator()) {
         add_dep(last_accumulator_write, n);
         last_accumulator_write = n;
      }
   }

   /* bottom-to-top dependencies: WAR */
   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));
   last_conditional_mod = NULL;
   last_accumulator_write = NULL;
   last_fixed_grf_write = NULL;

   foreach_in_list_reverse_safe(schedule_node, n, &instructions) {
      vec4_instruction *inst = (vec4_instruction *)n->inst;

      /* write-after-read deps. */
      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == VGRF) {
            for (unsigned j = 0; j < regs_read(inst, i); ++j)
               add_dep(n, last_grf_write[inst->src[i].nr + j]);
         } else if (inst->src[i].file == FIXED_GRF) {
            add_dep(n, last_fixed_grf_write);
         } else if (inst->src[i].is_accumulator()) {
            add_dep(n, last_accumulator_write);
         } else if (inst->src[i].file == ARF && !inst->src[i].is_null()) {
            add_barrier_deps(n);
         }
      }

      if (!inst->is_send_from_grf()) {
         for (int i = 0; i < inst->mlen; i++) {
            /* It looks like the MRF regs are released in the send
             * instruction once it's sent, not when the result comes
             * back.
             */
            add_dep(n, last_mrf_write[inst->base_mrf + i], 2);
         }
      }

      if (inst->reads_flag()) {
         add_dep(n, last_conditional_mod);
      }

      if (inst->reads_accumulator_implicitly()) {
         add_dep(n, last_accumulator_write);
      }

      /* Update the things this instruction wrote, so earlier reads
       * can mark this as WAR dependency.
       */
      if (inst->dst.file == VGRF) {
         for (unsigned j = 0; j < regs_written(inst); ++j)
            last_grf_write[inst->dst.nr + j] = n;
      } else if (inst->dst.file == MRF) {
         last_mrf_write[inst->dst.nr] = n;
      } else if (inst->dst.file == FIXED_GRF) {
         last_fixed_grf_write = n;
      } else if (inst->dst.is_accumulator()) {
         last_accumulator_write = n;
      } else if (inst->dst.file == ARF && !inst->dst.is_null()) {
         add_barrier_deps(n);
      }

      if (inst->mlen > 0 && !inst->is_send_from_grf()) {
         for (unsigned i = 0; i < inst->implied_mrf_writes(); i++) {
            last_mrf_write[inst->base_mrf + i] = n;
         }
      }

      if (inst->writes_flag(v->devinfo)) {
         last_conditional_mod = n;
      }

      if (inst->writes_accumulator_implicitly(v->devinfo)) {
         last_accumulator_write = n;
      }
   }
}

schedule_node *
fs_instruction_scheduler::choose_instruction_to_schedule()
{
   schedule_node *chosen = NULL;

   if (mode == SCHEDULE_PRE || mode == SCHEDULE_POST) {
      int chosen_time = 0;

      /* Of the instructions ready to execute or the closest to being ready,
       * choose the one most likely to unblock an early program exit, or
       * otherwise the oldest one.
       */
      foreach_in_list(schedule_node, n, &instructions) {
         if (!chosen ||
             exit_unblocked_time(n) < exit_unblocked_time(chosen) ||
             (exit_unblocked_time(n) == exit_unblocked_time(chosen) &&
              n->unblocked_time < chosen_time)) {
            chosen = n;
            chosen_time = n->unblocked_time;
         }
      }
   } else {
      int chosen_register_pressure_benefit = 0;

      /* Before register allocation, we don't care about the latencies of
       * instructions.  All we care about is reducing live intervals of
       * variables so that we can avoid register spilling, or get SIMD16
       * shaders which naturally do a better job of hiding instruction
       * latency.
       */
      foreach_in_list(schedule_node, n, &instructions) {
         fs_inst *inst = (fs_inst *)n->inst;

         if (!chosen) {
            chosen = n;
            chosen_register_pressure_benefit =
                  get_register_pressure_benefit(chosen->inst);
            continue;
         }

         /* Most important: If we can definitely reduce register pressure, do
          * so immediately.
          */
         int register_pressure_benefit = get_register_pressure_benefit(n->inst);

         if (register_pressure_benefit > 0 &&
             register_pressure_benefit > chosen_register_pressure_benefit) {
            chosen = n;
            chosen_register_pressure_benefit = register_pressure_benefit;
            continue;
         } else if (chosen_register_pressure_benefit > 0 &&
                    (register_pressure_benefit <
                     chosen_register_pressure_benefit)) {
            continue;
         }

         if (mode == SCHEDULE_PRE_LIFO) {
            /* Prefer instructions that recently became available for
             * scheduling.  These are the things that are most likely to
             * (eventually) make a variable dead and reduce register pressure.
             * Typical register pressure estimates don't work for us because
             * most of our pressure comes from texturing, where no single
             * instruction to schedule will make a vec4 value dead.
             */
            if (n->cand_generation > chosen->cand_generation) {
               chosen = n;
               chosen_register_pressure_benefit = register_pressure_benefit;
               continue;
            } else if (n->cand_generation < chosen->cand_generation) {
               continue;
            }

            /* On MRF-using chips, prefer non-SEND instructions.  If we don't
             * do this, then because we prefer instructions that just became
             * candidates, we'll end up in a pattern of scheduling a SEND,
             * then the MRFs for the next SEND, then the next SEND, then the
             * MRFs, etc., without ever consuming the results of a send.
             */
            if (v->devinfo->ver < 7) {
               fs_inst *chosen_inst = (fs_inst *)chosen->inst;

               /* We use size_written > 4 * exec_size as our test for the kind
                * of send instruction to avoid -- only sends generate many
                * regs, and a single-result send is probably actually reducing
                * register pressure.
                */
               if (inst->size_written <= 4 * inst->exec_size &&
                   chosen_inst->size_written > 4 * chosen_inst->exec_size) {
                  chosen = n;
                  chosen_register_pressure_benefit = register_pressure_benefit;
                  continue;
               } else if (inst->size_written > chosen_inst->size_written) {
                  continue;
               }
            }
         }

         /* For instructions pushed on the cands list at the same time, prefer
          * the one with the highest delay to the end of the program.  This is
          * most likely to have its values able to be consumed first (such as
          * for a large tree of lowered ubo loads, which appear reversed in
          * the instruction stream with respect to when they can be consumed).
          */
         if (n->delay > chosen->delay) {
            chosen = n;
            chosen_register_pressure_benefit = register_pressure_benefit;
            continue;
         } else if (n->delay < chosen->delay) {
            continue;
         }

         /* Prefer the node most likely to unblock an early program exit.
          */
         if (exit_unblocked_time(n) < exit_unblocked_time(chosen)) {
            chosen = n;
            chosen_register_pressure_benefit = register_pressure_benefit;
            continue;
         } else if (exit_unblocked_time(n) > exit_unblocked_time(chosen)) {
            continue;
         }

         /* If all other metrics are equal, we prefer the first instruction in
          * the list (program execution).
          */
      }
   }

   return chosen;
}

schedule_node *
vec4_instruction_scheduler::choose_instruction_to_schedule()
{
   schedule_node *chosen = NULL;
   int chosen_time = 0;

   /* Of the instructions ready to execute or the closest to being ready,
    * choose the oldest one.
    */
   foreach_in_list(schedule_node, n, &instructions) {
      if (!chosen || n->unblocked_time < chosen_time) {
         chosen = n;
         chosen_time = n->unblocked_time;
      }
   }

   return chosen;
}

int
fs_instruction_scheduler::issue_time(backend_instruction *inst0)
{
   const fs_inst *inst = static_cast<fs_inst *>(inst0);
   const unsigned overhead = v->grf_used && has_bank_conflict(v->devinfo, inst) ?
      DIV_ROUND_UP(inst->dst.component_size(inst->exec_size), REG_SIZE) : 0;
   if (is_compressed(inst))
      return 4 + overhead;
   else
      return 2 + overhead;
}

int
vec4_instruction_scheduler::issue_time(backend_instruction *)
{
   /* We always execute as two vec4s in parallel. */
   return 2;
}

void
instruction_scheduler::schedule_instructions(bblock_t *block)
{
   const struct intel_device_info *devinfo = bs->devinfo;
   int time = 0;
   int instructions_to_schedule = block->end_ip - block->start_ip + 1;

   if (!post_reg_alloc)
      reg_pressure = reg_pressure_in[block->num];
   block_idx = block->num;

   /* Remove non-DAG heads from the list. */
   foreach_in_list_safe(schedule_node, n, &instructions) {
      if (n->parent_count != 0)
         n->remove();
   }

   unsigned cand_generation = 1;
   while (!instructions.is_empty()) {
      schedule_node *chosen = choose_instruction_to_schedule();

      /* Schedule this instruction. */
      assert(chosen);
      chosen->remove();
      chosen->inst->exec_node::remove();
      block->instructions.push_tail(chosen->inst);
      instructions_to_schedule--;

      if (!post_reg_alloc) {
         reg_pressure -= get_register_pressure_benefit(chosen->inst);
         update_register_pressure(chosen->inst);
      }

      /* If we expected a delay for scheduling, then bump the clock to reflect
       * that.  In reality, the hardware will switch to another hyperthread
       * and may not return to dispatching our thread for a while even after
       * we're unblocked.  After this, we have the time when the chosen
       * instruction will start executing.
       */
      time = MAX2(time, chosen->unblocked_time);

      /* Update the clock for how soon an instruction could start after the
       * chosen one.
       */
      time += issue_time(chosen->inst);

      if (debug) {
         fprintf(stderr, "clock %4d, scheduled: ", time);
         bs->dump_instruction(chosen->inst);
         if (!post_reg_alloc)
            fprintf(stderr, "(register pressure %d)\n", reg_pressure);
      }

      /* Now that we've scheduled a new instruction, some of its
       * children can be promoted to the list of instructions ready to
       * be scheduled.  Update the children's unblocked time for this
       * DAG edge as we do so.
       */
      for (int i = chosen->child_count - 1; i >= 0; i--) {
         schedule_node *child = chosen->children[i];

         child->unblocked_time = MAX2(child->unblocked_time,
                                      time + chosen->child_latency[i]);

         if (debug) {
            fprintf(stderr, "\tchild %d, %d parents: ", i, child->parent_count);
            bs->dump_instruction(child->inst);
         }

         child->cand_generation = cand_generation;
         child->parent_count--;
         if (child->parent_count == 0) {
            if (debug) {
               fprintf(stderr, "\t\tnow available\n");
            }
            instructions.push_head(child);
         }
      }
      cand_generation++;

      /* Shared resource: the mathbox.  There's one mathbox per EU on Gfx6+
       * but it's more limited pre-gfx6, so if we send something off to it then
       * the next math instruction isn't going to make progress until the first
       * is done.
       */
      if (devinfo->ver < 6 && chosen->inst->is_math()) {
         foreach_in_list(schedule_node, n, &instructions) {
            if (n->inst->is_math())
               n->unblocked_time = MAX2(n->unblocked_time,
                                        time + chosen->latency);
         }
      }
   }

   assert(instructions_to_schedule == 0);
}

void
instruction_scheduler::run(cfg_t *cfg)
{
   if (debug && !post_reg_alloc) {
      fprintf(stderr, "\nInstructions before scheduling (reg_alloc %d)\n",
              post_reg_alloc);
         bs->dump_instructions();
   }

   if (!post_reg_alloc)
      setup_liveness(cfg);

   foreach_block(block, cfg) {
      if (reads_remaining) {
         memset(reads_remaining, 0,
                grf_count * sizeof(*reads_remaining));
         memset(hw_reads_remaining, 0,
                hw_reg_count * sizeof(*hw_reads_remaining));
         memset(written, 0, grf_count * sizeof(*written));

         foreach_inst_in_block(fs_inst, inst, block)
            count_reads_remaining(inst);
      }

      add_insts_from_block(block);

      calculate_deps();

      compute_delays();
      compute_exits();

      schedule_instructions(block);
   }

   if (debug && !post_reg_alloc) {
      fprintf(stderr, "\nInstructions after scheduling (reg_alloc %d)\n",
              post_reg_alloc);
      bs->dump_instructions();
   }
}

void
fs_visitor::schedule_instructions(instruction_scheduler_mode mode)
{
   int grf_count;
   if (mode == SCHEDULE_POST)
      grf_count = grf_used;
   else
      grf_count = alloc.count;

   fs_instruction_scheduler sched(this, grf_count, first_non_payload_grf,
                                  cfg->num_blocks, mode);
   sched.run(cfg);

   invalidate_analysis(DEPENDENCY_INSTRUCTIONS);
}

void
vec4_visitor::opt_schedule_instructions()
{
   vec4_instruction_scheduler sched(this, prog_data->total_grf);
   sched.run(cfg);

   invalidate_analysis(DEPENDENCY_INSTRUCTIONS);
}
