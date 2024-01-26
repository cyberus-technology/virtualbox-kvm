/*
 * Copyright 2011 Christoph Bumiller
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
 */

#include "codegen/nv50_ir_from_common.h"

namespace nv50_ir {

ConverterCommon::ConverterCommon(Program *prog, nv50_ir_prog_info *info,
                                 nv50_ir_prog_info_out *info_out)
   :  BuildUtil(prog),
      info(info),
      info_out(info_out) {}

ConverterCommon::Subroutine *
ConverterCommon::getSubroutine(unsigned ip)
{
   std::map<unsigned, Subroutine>::iterator it = sub.map.find(ip);

   if (it == sub.map.end())
      it = sub.map.insert(std::make_pair(
              ip, Subroutine(new Function(prog, "SUB", ip)))).first;

   return &it->second;
}

ConverterCommon::Subroutine *
ConverterCommon::getSubroutine(Function *f)
{
   unsigned ip = f->getLabel();
   std::map<unsigned, Subroutine>::iterator it = sub.map.find(ip);

   if (it == sub.map.end())
      it = sub.map.insert(std::make_pair(ip, Subroutine(f))).first;

   return &it->second;
}

uint8_t
ConverterCommon::translateInterpMode(const struct nv50_ir_varying *var, operation& op)
{
   uint8_t mode = NV50_IR_INTERP_PERSPECTIVE;

   if (var->flat)
      mode = NV50_IR_INTERP_FLAT;
   else
   if (var->linear)
      mode = NV50_IR_INTERP_LINEAR;
   else
   if (var->sc)
      mode = NV50_IR_INTERP_SC;

   op = (mode == NV50_IR_INTERP_PERSPECTIVE || mode == NV50_IR_INTERP_SC)
      ? OP_PINTERP : OP_LINTERP;

   if (var->centroid)
      mode |= NV50_IR_INTERP_CENTROID;

   return mode;
}

void
ConverterCommon::handleUserClipPlanes()
{
   Value *res[8];
   int n, i, c;

   for (c = 0; c < 4; ++c) {
      for (i = 0; i < info_out->io.genUserClip; ++i) {
         Symbol *sym = mkSymbol(FILE_MEMORY_CONST, info->io.auxCBSlot,
                                TYPE_F32, info->io.ucpBase + i * 16 + c * 4);
         Value *ucp = mkLoadv(TYPE_F32, sym, NULL);
         if (c == 0)
            res[i] = mkOp2v(OP_MUL, TYPE_F32, getScratch(), clipVtx[c], ucp);
         else
            mkOp3(OP_MAD, TYPE_F32, res[i], clipVtx[c], ucp, res[i]);
      }
   }

   const int first = info_out->numOutputs - (info_out->io.genUserClip + 3) / 4;

   for (i = 0; i < info_out->io.genUserClip; ++i) {
      n = i / 4 + first;
      c = i % 4;
      Symbol *sym =
         mkSymbol(FILE_SHADER_OUTPUT, 0, TYPE_F32, info_out->out[n].slot[c] * 4);
      mkStore(OP_EXPORT, TYPE_F32, sym, NULL, res[i]);
   }
}

} // namespace nv50_ir
