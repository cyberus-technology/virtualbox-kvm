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

#include "codegen/nv50_ir.h"
#include "codegen/nv50_ir_build_util.h"

namespace nv50_ir {

class ConverterCommon : public BuildUtil
{
public:
   ConverterCommon(Program *, nv50_ir_prog_info *, nv50_ir_prog_info_out *);
protected:
   struct Subroutine
   {
      Subroutine(Function *f) : f(f) { }
      Function *f;
      ValueMap values;
   };

   Subroutine *getSubroutine(unsigned ip);
   Subroutine *getSubroutine(Function *);

   uint8_t translateInterpMode(const struct nv50_ir_varying *var, operation& op);

   void handleUserClipPlanes();

   struct {
      std::map<unsigned, Subroutine> map;
      Subroutine *cur;
   } sub;

   struct nv50_ir_prog_info *info;
   struct nv50_ir_prog_info_out *info_out;
   Value *fragCoord[4];
   Value *clipVtx[4];
   Value *outBase; // base address of vertex out patch (for TCP)
};

} // namespace nv50_ir
