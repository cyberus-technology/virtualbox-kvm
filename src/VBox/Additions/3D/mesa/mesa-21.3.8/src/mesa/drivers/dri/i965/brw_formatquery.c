/*
 * Copyright © 2015 Intel Corporation
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
 */

#include "brw_context.h"
#include "brw_state.h"
#include "main/context.h"
#include "main/formatquery.h"
#include "main/glformats.h"

static size_t
brw_query_samples_for_format(struct gl_context *ctx, GLenum target,
                             GLenum internalFormat, int samples[16])
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   (void) target;
   (void) internalFormat;

   switch (devinfo->ver) {
   case 11:
   case 10:
   case 9:
      samples[0] = 16;
      samples[1] = 8;
      samples[2] = 4;
      samples[3] = 2;
      return 4;

   case 8:
      samples[0] = 8;
      samples[1] = 4;
      samples[2] = 2;
      return 3;

   case 7:
      if (internalFormat == GL_RGBA32F && _mesa_is_gles(ctx)) {
         /* For GLES, we are allowed to return a smaller number of samples for
          * GL_RGBA32F. See OpenGLES 3.2 spec, section 20.3.1 Internal Format
          * Query Parameters, under SAMPLES:
          *
          * "A value less than or equal to the value of MAX_SAMPLES, if
          *  internalformat is RGBA16F, R32F, RG32F, or RGBA32F."
          *
          * In brw_render_target_supported, we prevent formats with a size
          * greater than 8 bytes from using 8x MSAA on gfx7.
          */
         samples[0] = 4;
         return 1;
      } else {
         samples[0] = 8;
         samples[1] = 4;
         return 2;
      }

   case 6:
      samples[0] = 4;
      return 1;

   default:
      assert(devinfo->ver < 6);
      samples[0] = 1;
      return 1;
   }
}

void
brw_query_internal_format(struct gl_context *ctx, GLenum target,
                          GLenum internalFormat, GLenum pname, GLint *params)
{
   /* The Mesa layer gives us a temporary params buffer that is guaranteed
    * to be non-NULL, and have at least 16 elements.
    */
   assert(params != NULL);

   switch (pname) {
   case GL_SAMPLES:
      brw_query_samples_for_format(ctx, target, internalFormat, params);
      break;

   case GL_NUM_SAMPLE_COUNTS: {
      size_t num_samples;
      GLint dummy_buffer[16];

      num_samples = brw_query_samples_for_format(ctx, target, internalFormat,
                                                 dummy_buffer);
      params[0] = (GLint) num_samples;
      break;
   }

   default:
      /* By default, we call the driver hook's fallback function from the frontend,
       * which has generic implementation for all pnames.
       */
      _mesa_query_internal_format_default(ctx, target, internalFormat, pname,
                                          params);
      break;
   }
}
