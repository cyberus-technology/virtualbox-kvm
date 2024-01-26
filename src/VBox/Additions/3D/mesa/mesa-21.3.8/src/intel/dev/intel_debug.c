/*
 * Copyright 2003 VMware, Inc.
 * Copyright © 2006 Intel Corporation
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

/**
 * \file intel_debug.c
 *
 * Support for the INTEL_DEBUG environment variable, along with other
 * miscellaneous debugging code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dev/intel_debug.h"
#include "git_sha1.h"
#include "util/macros.h"
#include "util/debug.h"
#include "c11/threads.h"

uint64_t intel_debug = 0;

static const struct debug_control debug_control[] = {
   { "tex",         DEBUG_TEXTURE},
   { "state",       DEBUG_STATE},
   { "blit",        DEBUG_BLIT},
   { "mip",         DEBUG_MIPTREE},
   { "fall",        DEBUG_PERF},
   { "perf",        DEBUG_PERF},
   { "perfmon",     DEBUG_PERFMON},
   { "bat",         DEBUG_BATCH},
   { "pix",         DEBUG_PIXEL},
   { "buf",         DEBUG_BUFMGR},
   { "fbo",         DEBUG_FBO},
   { "fs",          DEBUG_WM },
   { "gs",          DEBUG_GS},
   { "sync",        DEBUG_SYNC},
   { "prim",        DEBUG_PRIMS },
   { "vert",        DEBUG_VERTS },
   { "dri",         DEBUG_DRI },
   { "sf",          DEBUG_SF },
   { "submit",      DEBUG_SUBMIT },
   { "wm",          DEBUG_WM },
   { "urb",         DEBUG_URB },
   { "vs",          DEBUG_VS },
   { "clip",        DEBUG_CLIP },
   { "shader_time", DEBUG_SHADER_TIME },
   { "no16",        DEBUG_NO16 },
   { "blorp",       DEBUG_BLORP },
   { "nodualobj",   DEBUG_NO_DUAL_OBJECT_GS },
   { "optimizer",   DEBUG_OPTIMIZER },
   { "ann",         DEBUG_ANNOTATION },
   { "no8",         DEBUG_NO8 },
   { "no-oaconfig", DEBUG_NO_OACONFIG },
   { "spill_fs",    DEBUG_SPILL_FS },
   { "spill_vec4",  DEBUG_SPILL_VEC4 },
   { "cs",          DEBUG_CS },
   { "hex",         DEBUG_HEX },
   { "nocompact",   DEBUG_NO_COMPACTION },
   { "hs",          DEBUG_TCS },
   { "tcs",         DEBUG_TCS },
   { "ds",          DEBUG_TES },
   { "tes",         DEBUG_TES },
   { "l3",          DEBUG_L3 },
   { "do32",        DEBUG_DO32 },
   { "norbc",       DEBUG_NO_RBC },
   { "nohiz",       DEBUG_NO_HIZ },
   { "color",       DEBUG_COLOR },
   { "reemit",      DEBUG_REEMIT },
   { "soft64",      DEBUG_SOFT64 },
   { "tcs8",        DEBUG_TCS_EIGHT_PATCH },
   { "bt",          DEBUG_BT },
   { "pc",          DEBUG_PIPE_CONTROL },
   { "nofc",        DEBUG_NO_FAST_CLEAR },
   { "no32",        DEBUG_NO32 },
   { "shaders",     DEBUG_WM | DEBUG_VS | DEBUG_TCS |
                    DEBUG_TES | DEBUG_GS | DEBUG_CS |
                    DEBUG_RT },
   { "rt",          DEBUG_RT },
   { NULL,    0 }
};

uint64_t
intel_debug_flag_for_shader_stage(gl_shader_stage stage)
{
   uint64_t flags[] = {
      [MESA_SHADER_VERTEX] = DEBUG_VS,
      [MESA_SHADER_TESS_CTRL] = DEBUG_TCS,
      [MESA_SHADER_TESS_EVAL] = DEBUG_TES,
      [MESA_SHADER_GEOMETRY] = DEBUG_GS,
      [MESA_SHADER_FRAGMENT] = DEBUG_WM,
      [MESA_SHADER_COMPUTE] = DEBUG_CS,

      [MESA_SHADER_RAYGEN]       = DEBUG_RT,
      [MESA_SHADER_ANY_HIT]      = DEBUG_RT,
      [MESA_SHADER_CLOSEST_HIT]  = DEBUG_RT,
      [MESA_SHADER_MISS]         = DEBUG_RT,
      [MESA_SHADER_INTERSECTION] = DEBUG_RT,
      [MESA_SHADER_CALLABLE]     = DEBUG_RT,
   };
   return flags[stage];
}

static void
brw_process_intel_debug_variable_once(void)
{
   intel_debug = parse_debug_string(getenv("INTEL_DEBUG"), debug_control);
}

void
brw_process_intel_debug_variable(void)
{
   static once_flag process_intel_debug_variable_flag = ONCE_FLAG_INIT;

   call_once(&process_intel_debug_variable_flag,
             brw_process_intel_debug_variable_once);
}

static uint64_t debug_identifier[4] = {
   0xffeeddccbbaa9988,
   0x7766554433221100,
   0xffeeddccbbaa9988,
   0x7766554433221100,
};

void *
intel_debug_identifier(void)
{
   return debug_identifier;
}

uint32_t
intel_debug_identifier_size(void)
{
   return sizeof(debug_identifier);
}

uint32_t
intel_debug_write_identifiers(void *_output,
                              uint32_t output_size,
                              const char *driver_name)
{
   void *output = _output, *output_end = _output + output_size;

   assert(output_size > intel_debug_identifier_size());

   memcpy(output, intel_debug_identifier(), intel_debug_identifier_size());
   output += intel_debug_identifier_size();

   for (uint32_t id = INTEL_DEBUG_BLOCK_TYPE_DRIVER; id < INTEL_DEBUG_BLOCK_TYPE_MAX; id++) {
      switch (id) {
      case INTEL_DEBUG_BLOCK_TYPE_DRIVER: {
         struct intel_debug_block_driver driver_desc = {
            .base = {
               .type = id,
            },
         };
         int len = snprintf(output + sizeof(driver_desc),
                            output_end - (output + sizeof(driver_desc)),
                            "%s " PACKAGE_VERSION " build " MESA_GIT_SHA1,
                            driver_name);
         driver_desc.base.length = sizeof(driver_desc) + len + 1;
         memcpy(output, &driver_desc, sizeof(driver_desc));
         output += driver_desc.base.length;
         break;
      }

      case INTEL_DEBUG_BLOCK_TYPE_FRAME: {
         struct intel_debug_block_frame frame_desc = {
            .base = {
               .type = INTEL_DEBUG_BLOCK_TYPE_FRAME,
               .length = sizeof(frame_desc),
            },
         };
         memcpy(output, &frame_desc, sizeof(frame_desc));
         output += sizeof(frame_desc);
         break;
      }

      default:
         unreachable("Missing identifier write");
      }

      assert(output < output_end);
   }

   struct intel_debug_block_base end = {
      .type = INTEL_DEBUG_BLOCK_TYPE_END,
      .length = sizeof(end),
   };
   memcpy(output, &end, sizeof(end));
   output += sizeof(end);

   assert(output < output_end);

   /* Return the how many bytes where written, so that the rest of the buffer
    * can be used for other things.
    */
   return output - _output;
}

void *
intel_debug_get_identifier_block(void *_buffer,
                                 uint32_t buffer_size,
                                 enum intel_debug_block_type type)
{
   void *buffer = _buffer + intel_debug_identifier_size(),
      *end_buffer = _buffer + buffer_size;

   while (buffer < end_buffer) {
      struct intel_debug_block_base *item = buffer;

      if (item->type == type)
         return item;
      if (item->type == INTEL_DEBUG_BLOCK_TYPE_END)
         return NULL;

      buffer += item->length;
   }

   return NULL;
}
