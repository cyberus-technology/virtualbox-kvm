/*
 * Copyright 2018 Red Hat Inc.
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

#include "codegen/nv50_ir_lowering_helper.h"

namespace nv50_ir {

bool
LoweringHelper::visit(Instruction *insn)
{
   switch (insn->op) {
   case OP_ABS:
      return handleABS(insn);
   case OP_CVT:
      return handleCVT(insn);
   case OP_MAX:
   case OP_MIN:
      return handleMAXMIN(insn);
   case OP_MOV:
      return handleMOV(insn);
   case OP_NEG:
      return handleNEG(insn);
   case OP_SAT:
      return handleSAT(insn);
   case OP_SLCT:
      return handleSLCT(insn->asCmp());
   case OP_AND:
   case OP_NOT:
   case OP_OR:
   case OP_XOR:
      return handleLogOp(insn);
   default:
      return true;
   }
}

bool
LoweringHelper::handleABS(Instruction *insn)
{
   DataType dTy = insn->dType;
   if (!(dTy == TYPE_U64 || dTy == TYPE_S64))
      return true;

   bld.setPosition(insn, false);

   Value *neg = bld.getSSA(8);
   Value *negComp[2], *srcComp[2];
   Value *lo = bld.getSSA(), *hi = bld.getSSA();
   bld.mkOp2(OP_SUB, dTy, neg, bld.mkImm((uint64_t)0), insn->getSrc(0));
   bld.mkSplit(negComp, 4, neg);
   bld.mkSplit(srcComp, 4, insn->getSrc(0));
   bld.mkCmp(OP_SLCT, CC_LT, TYPE_S32, lo, TYPE_S32, negComp[0], srcComp[0], srcComp[1]);
   bld.mkCmp(OP_SLCT, CC_LT, TYPE_S32, hi, TYPE_S32, negComp[1], srcComp[1], srcComp[1]);
   insn->op = OP_MERGE;
   insn->setSrc(0, lo);
   insn->setSrc(1, hi);

   return true;
}

bool
LoweringHelper::handleCVT(Instruction *insn)
{
   DataType dTy = insn->dType;
   DataType sTy = insn->sType;

   if (typeSizeof(dTy) <= 4 && typeSizeof(sTy) <= 4)
      return true;

   bld.setPosition(insn, false);

   if ((dTy == TYPE_S32 && sTy == TYPE_S64) ||
       (dTy == TYPE_U32 && sTy == TYPE_U64)) {
      Value *src[2];
      bld.mkSplit(src, 4, insn->getSrc(0));
      insn->op = OP_MOV;
      insn->setSrc(0, src[0]);
   } else if (dTy == TYPE_S64 && sTy == TYPE_S32) {
      Value *tmp = bld.getSSA();
      bld.mkOp2(OP_SHR, TYPE_S32, tmp, insn->getSrc(0), bld.loadImm(bld.getSSA(), 31));
      insn->op = OP_MERGE;
      insn->setSrc(1, tmp);
   } else if (dTy == TYPE_U64 && sTy == TYPE_U32) {
      insn->op = OP_MERGE;
      insn->setSrc(1, bld.loadImm(bld.getSSA(), 0));
   }

   return true;
}

bool
LoweringHelper::handleMAXMIN(Instruction *insn)
{
   DataType dTy = insn->dType;
   if (!(dTy == TYPE_U64 || dTy == TYPE_S64))
      return true;

   DataType sTy = typeOfSize(4, false, isSignedIntType(dTy));
   bld.setPosition(insn, false);

   Value *flag = bld.getSSA(1, FILE_FLAGS);
   Value *src0[2];
   Value *src1[2];
   Value *def[2];

   bld.mkSplit(src0, 4, insn->getSrc(0));
   bld.mkSplit(src1, 4, insn->getSrc(1));

   def[0] = bld.getSSA();
   def[1] = bld.getSSA();

   Instruction *hi = bld.mkOp2(insn->op, sTy, def[1], src0[1], src1[1]);
   hi->subOp = NV50_IR_SUBOP_MINMAX_HIGH;
   hi->setFlagsDef(1, flag);

   Instruction *lo = bld.mkOp2(insn->op, sTy, def[0], src0[0], src1[0]);
   lo->subOp = NV50_IR_SUBOP_MINMAX_LOW;
   lo->setFlagsSrc(2, flag);

   insn->op = OP_MERGE;
   insn->setSrc(0, def[0]);
   insn->setSrc(1, def[1]);

   return true;
}

bool
LoweringHelper::handleMOV(Instruction *insn)
{
   DataType dTy = insn->dType;

   if (typeSizeof(dTy) != 8)
      return true;

   Storage &reg = insn->getSrc(0)->reg;

   if (reg.file != FILE_IMMEDIATE)
      return true;

   bld.setPosition(insn, false);

   Value *hi = bld.getSSA();
   Value *lo = bld.getSSA();

   bld.loadImm(lo, (uint32_t)(reg.data.u64 & 0xffffffff));
   bld.loadImm(hi, (uint32_t)(reg.data.u64 >> 32));

   insn->op = OP_MERGE;
   insn->setSrc(0, lo);
   insn->setSrc(1, hi);

   return true;
}

bool
LoweringHelper::handleNEG(Instruction *insn)
{
   if (typeSizeof(insn->dType) != 8 || isFloatType(insn->dType))
      return true;

   bld.setPosition(insn, false);

   insn->op = OP_SUB;
   insn->setSrc(1, insn->getSrc(0));
   insn->setSrc(0, bld.mkImm((uint64_t)0));
   return true;
}

bool
LoweringHelper::handleSAT(Instruction *insn)
{
   DataType dTy = insn->dType;

   if (typeSizeof(dTy) != 8 || !isFloatType(dTy))
      return true;

   bld.setPosition(insn, false);

   Value *tmp = bld.mkOp2v(OP_MAX, dTy, bld.getSSA(8), insn->getSrc(0), bld.loadImm(bld.getSSA(8), 0.0));
   insn->op = OP_MIN;
   insn->setSrc(0, tmp);
   insn->setSrc(1, bld.loadImm(bld.getSSA(8), 1.0));
   return true;
}

bool
LoweringHelper::handleSLCT(CmpInstruction *insn)
{
   DataType dTy = insn->dType;
   DataType sTy = insn->sType;

   if (typeSizeof(dTy) != 8 || typeSizeof(sTy) == 8)
      return true;

   CondCode cc = insn->getCondition();
   DataType hdTy = typeOfSize(4, isFloatType(dTy), isSignedIntType(dTy));
   bld.setPosition(insn, false);

   Value *src0[2];
   Value *src1[2];
   Value *def[2];

   bld.mkSplit(src0, 4, insn->getSrc(0));
   bld.mkSplit(src1, 4, insn->getSrc(1));

   def[0] = bld.getSSA();
   def[1] = bld.getSSA();

   bld.mkCmp(OP_SLCT, cc, hdTy, def[0], sTy, src0[0], src1[0], insn->getSrc(2));
   bld.mkCmp(OP_SLCT, cc, hdTy, def[1], sTy, src0[1], src1[1], insn->getSrc(2));

   insn->op = OP_MERGE;
   insn->setSrc(0, def[0]);
   insn->setSrc(1, def[1]);
   insn->setSrc(2, NULL);

   return true;
}

bool
LoweringHelper::handleLogOp(Instruction *insn)
{
   DataType dTy = insn->dType;
   DataType sTy = typeOfSize(4, isFloatType(dTy), isSignedIntType(dTy));

   if (typeSizeof(dTy) != 8)
      return true;

   bld.setPosition(insn, false);

   Value *src0[2];
   Value *src1[2];
   Value *def0 = bld.getSSA();
   Value *def1 = bld.getSSA();

   bld.mkSplit(src0, 4, insn->getSrc(0));
   if (insn->srcExists(1))
      bld.mkSplit(src1, 4, insn->getSrc(1));

   Instruction *lo = bld.mkOp1(insn->op, sTy, def0, src0[0]);
   Instruction *hi = bld.mkOp1(insn->op, sTy, def1, src0[1]);
   if (insn->srcExists(1)) {
      lo->setSrc(1, src1[0]);
      hi->setSrc(1, src1[1]);
   }

   insn->op = OP_MERGE;
   insn->setSrc(0, def0);
   insn->setSrc(1, def1);

   return true;
}

} // namespace nv50_ir
