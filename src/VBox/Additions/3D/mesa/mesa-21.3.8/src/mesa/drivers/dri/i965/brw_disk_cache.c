/*
 * Copyright © 2014 Intel Corporation
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

#include "compiler/glsl/ir_uniform.h"
#include "compiler/glsl/shader_cache.h"
#include "main/mtypes.h"
#include "util/blob.h"
#include "util/build_id.h"
#include "util/debug.h"
#include "util/disk_cache.h"
#include "util/macros.h"
#include "util/mesa-sha1.h"

#include "compiler/brw_eu.h"
#include "dev/intel_debug.h"

#include "brw_context.h"
#include "brw_program.h"
#include "brw_cs.h"
#include "brw_gs.h"
#include "brw_state.h"
#include "brw_vs.h"
#include "brw_wm.h"

static bool
debug_enabled_for_stage(gl_shader_stage stage)
{
   static const uint64_t stage_debug_flags[] = {
      DEBUG_VS, DEBUG_TCS, DEBUG_TES, DEBUG_GS, DEBUG_WM, DEBUG_CS,
   };
   assert((int)stage >= 0 && stage < ARRAY_SIZE(stage_debug_flags));
   return INTEL_DEBUG(stage_debug_flags[stage]);
}

static void
intel_shader_sha1(struct gl_program *prog, gl_shader_stage stage,
                void *key, unsigned char *out_sha1)
{
   char sha1_buf[41];
   unsigned char sha1[20];
   char manifest[256];
   int offset = 0;

   _mesa_sha1_format(sha1_buf, prog->sh.data->sha1);
   offset += snprintf(manifest, sizeof(manifest), "program: %s\n", sha1_buf);

   _mesa_sha1_compute(key, brw_prog_key_size(stage), sha1);
   _mesa_sha1_format(sha1_buf, sha1);
   offset += snprintf(manifest + offset, sizeof(manifest) - offset,
                      "%s_key: %s\n", _mesa_shader_stage_to_abbrev(stage),
                      sha1_buf);

   _mesa_sha1_compute(manifest, strlen(manifest), out_sha1);
}

static bool
read_blob_program_data(struct blob_reader *binary, struct gl_program *prog,
                       gl_shader_stage stage, const uint8_t **program,
                       struct brw_stage_prog_data *prog_data)
{
   return
      brw_read_blob_program_data(binary, prog, stage, program, prog_data) &&
      (binary->current == binary->end);
}

static bool
read_and_upload(struct brw_context *brw, struct disk_cache *cache,
                struct gl_program *prog, gl_shader_stage stage)
{
   unsigned char binary_sha1[20];

   union brw_any_prog_key prog_key;

   switch (stage) {
   case MESA_SHADER_VERTEX:
      brw_vs_populate_key(brw, &prog_key.vs);
      break;
   case MESA_SHADER_TESS_CTRL:
      brw_tcs_populate_key(brw, &prog_key.tcs);
      break;
   case MESA_SHADER_TESS_EVAL:
      brw_tes_populate_key(brw, &prog_key.tes);
      break;
   case MESA_SHADER_GEOMETRY:
      brw_gs_populate_key(brw, &prog_key.gs);
      break;
   case MESA_SHADER_FRAGMENT:
      brw_wm_populate_key(brw, &prog_key.wm);
      break;
   case MESA_SHADER_COMPUTE:
      brw_cs_populate_key(brw, &prog_key.cs);
      break;
   default:
      unreachable("Unsupported stage!");
   }

   /* We don't care what instance of the program it is for the disk cache hash
    * lookup, so set the id to 0 for the sha1 hashing. program_string_id will
    * be set below.
    */
   prog_key.base.program_string_id = 0;

   intel_shader_sha1(prog, stage, &prog_key, binary_sha1);

   size_t buffer_size;
   uint8_t *buffer = disk_cache_get(cache, binary_sha1, &buffer_size);
   if (buffer == NULL) {
      if (brw->ctx._Shader->Flags & GLSL_CACHE_INFO) {
         char sha1_buf[41];
         _mesa_sha1_format(sha1_buf, binary_sha1);
         fprintf(stderr, "No cached %s binary found for: %s\n",
                 _mesa_shader_stage_to_abbrev(stage), sha1_buf);
      }
      return false;
   }

   if (brw->ctx._Shader->Flags & GLSL_CACHE_INFO) {
      char sha1_buf[41];
      _mesa_sha1_format(sha1_buf, binary_sha1);
      fprintf(stderr, "attempting to populate bo cache with binary: %s\n",
              sha1_buf);
   }

   struct blob_reader binary;
   blob_reader_init(&binary, buffer, buffer_size);

   const uint8_t *program;
   struct brw_stage_prog_data *prog_data =
      ralloc_size(NULL, sizeof(union brw_any_prog_data));
   if (!read_blob_program_data(&binary, prog, stage, &program, prog_data)) {
      /* Something very bad has gone wrong discard the item from the cache and
       * rebuild from source.
       */
      if (brw->ctx._Shader->Flags & GLSL_CACHE_INFO) {
         fprintf(stderr, "Error reading program from cache (invalid i965 "
                 "cache item)\n");
      }

      disk_cache_remove(cache, binary_sha1);
      ralloc_free(prog_data);
      free(buffer);
      return false;
   }

   enum brw_cache_id cache_id;
   struct brw_stage_state *stage_state;

   switch (stage) {
   case MESA_SHADER_VERTEX:
      cache_id = BRW_CACHE_VS_PROG;
      stage_state = &brw->vs.base;
      break;
   case MESA_SHADER_TESS_CTRL:
      cache_id = BRW_CACHE_TCS_PROG;
      stage_state = &brw->tcs.base;
      break;
   case MESA_SHADER_TESS_EVAL:
      cache_id = BRW_CACHE_TES_PROG;
      stage_state = &brw->tes.base;
      break;
   case MESA_SHADER_GEOMETRY:
      cache_id = BRW_CACHE_GS_PROG;
      stage_state = &brw->gs.base;
      break;
   case MESA_SHADER_FRAGMENT:
      cache_id = BRW_CACHE_FS_PROG;
      stage_state = &brw->wm.base;
      break;
   case MESA_SHADER_COMPUTE:
      cache_id = BRW_CACHE_CS_PROG;
      stage_state = &brw->cs.base;
      break;
   default:
      unreachable("Unsupported stage!");
   }

   prog_key.base.program_string_id = brw_program(prog)->id;

   brw_alloc_stage_scratch(brw, stage_state, prog_data->total_scratch);

   if (unlikely(debug_enabled_for_stage(stage))) {
      fprintf(stderr, "NIR for %s program %d loaded from disk shader cache:\n",
              _mesa_shader_stage_to_abbrev(stage), brw_program(prog)->id);
      brw_program_deserialize_driver_blob(&brw->ctx, prog, stage);
      nir_shader *nir = prog->nir;
      nir_print_shader(nir, stderr);
      fprintf(stderr, "Native code for %s %s shader %s from disk cache:\n",
              nir->info.label ? nir->info.label : "unnamed",
              _mesa_shader_stage_to_string(nir->info.stage), nir->info.name);
      brw_disassemble_with_labels(&brw->screen->devinfo, program, 0,
                                  prog_data->program_size, stderr);
   }

   brw_upload_cache(&brw->cache, cache_id, &prog_key, brw_prog_key_size(stage),
                    program, prog_data->program_size, prog_data,
                    brw_prog_data_size(stage), &stage_state->prog_offset,
                    &stage_state->prog_data);

   prog->program_written_to_cache = true;

   ralloc_free(prog_data);
   free(buffer);

   return true;
}

bool
brw_disk_cache_upload_program(struct brw_context *brw, gl_shader_stage stage)
{
   struct disk_cache *cache = brw->ctx.Cache;
   if (cache == NULL)
      return false;

   struct gl_program *prog = brw->ctx._Shader->CurrentProgram[stage];
   if (prog == NULL)
      return false;

   if (prog->sh.data->spirv)
      return false;

   if (brw->ctx._Shader->Flags & GLSL_CACHE_FALLBACK)
      goto fail;

   if (!read_and_upload(brw, cache, prog, stage))
      goto fail;

   if (brw->ctx._Shader->Flags & GLSL_CACHE_INFO) {
      fprintf(stderr, "read gen program from cache\n");
   }

   return true;

fail:
   prog->program_written_to_cache = false;
   if (brw->ctx._Shader->Flags & GLSL_CACHE_INFO) {
      fprintf(stderr, "falling back to nir %s.\n",
              _mesa_shader_stage_to_abbrev(prog->info.stage));
   }

   brw_program_deserialize_driver_blob(&brw->ctx, prog, stage);

   return false;
}

static void
write_program_data(struct brw_context *brw, struct gl_program *prog,
                   void *key, struct brw_stage_prog_data *prog_data,
                   uint32_t prog_offset, struct disk_cache *cache,
                   gl_shader_stage stage)
{
   struct blob binary;
   blob_init(&binary);

   const void *program_map = brw->cache.map + prog_offset;
   /* TODO: Improve perf for non-LLC. It would be best to save it at program
    * generation time when the program is in normal memory accessible with
    * cache to the CPU. Another easier change would be to use
    * _mesa_streaming_load_memcpy to read from the program mapped memory. */
   brw_write_blob_program_data(&binary, stage, program_map, prog_data);

   unsigned char sha1[20];
   char buf[41];
   intel_shader_sha1(prog, stage, key, sha1);
   _mesa_sha1_format(buf, sha1);
   if (brw->ctx._Shader->Flags & GLSL_CACHE_INFO) {
      fprintf(stderr, "putting binary in cache: %s\n", buf);
   }

   disk_cache_put(cache, sha1, binary.data, binary.size, NULL);

   prog->program_written_to_cache = true;
   blob_finish(&binary);
}

void
brw_disk_cache_write_render_programs(struct brw_context *brw)
{
   struct disk_cache *cache = brw->ctx.Cache;
   if (cache == NULL)
      return;

   struct gl_program *prog;
   gl_shader_stage stage;
   for (stage = MESA_SHADER_VERTEX; stage <= MESA_SHADER_FRAGMENT; stage++) {
      prog = brw->ctx._Shader->CurrentProgram[stage];
      if (prog && prog->sh.data->spirv)
         return;
   }

   prog = brw->ctx._Shader->CurrentProgram[MESA_SHADER_VERTEX];
   if (prog && !prog->program_written_to_cache) {
      struct brw_vs_prog_key vs_key;
      brw_vs_populate_key(brw, &vs_key);
      vs_key.base.program_string_id = 0;

      write_program_data(brw, prog, &vs_key, brw->vs.base.prog_data,
                         brw->vs.base.prog_offset, cache,
                         MESA_SHADER_VERTEX);
   }

   prog = brw->ctx._Shader->CurrentProgram[MESA_SHADER_TESS_CTRL];
   if (prog && !prog->program_written_to_cache) {
      struct brw_tcs_prog_key tcs_key;
      brw_tcs_populate_key(brw, &tcs_key);
      tcs_key.base.program_string_id = 0;

      write_program_data(brw, prog, &tcs_key, brw->tcs.base.prog_data,
                         brw->tcs.base.prog_offset, cache,
                         MESA_SHADER_TESS_CTRL);
   }

   prog = brw->ctx._Shader->CurrentProgram[MESA_SHADER_TESS_EVAL];
   if (prog && !prog->program_written_to_cache) {
      struct brw_tes_prog_key tes_key;
      brw_tes_populate_key(brw, &tes_key);
      tes_key.base.program_string_id = 0;

      write_program_data(brw, prog, &tes_key, brw->tes.base.prog_data,
                         brw->tes.base.prog_offset, cache,
                         MESA_SHADER_TESS_EVAL);
   }

   prog = brw->ctx._Shader->CurrentProgram[MESA_SHADER_GEOMETRY];
   if (prog && !prog->program_written_to_cache) {
      struct brw_gs_prog_key gs_key;
      brw_gs_populate_key(brw, &gs_key);
      gs_key.base.program_string_id = 0;

      write_program_data(brw, prog, &gs_key, brw->gs.base.prog_data,
                         brw->gs.base.prog_offset, cache,
                         MESA_SHADER_GEOMETRY);
   }

   prog = brw->ctx._Shader->CurrentProgram[MESA_SHADER_FRAGMENT];
   if (prog && !prog->program_written_to_cache) {
      struct brw_wm_prog_key wm_key;
      brw_wm_populate_key(brw, &wm_key);
      wm_key.base.program_string_id = 0;

      write_program_data(brw, prog, &wm_key, brw->wm.base.prog_data,
                         brw->wm.base.prog_offset, cache,
                         MESA_SHADER_FRAGMENT);
   }
}

void
brw_disk_cache_write_compute_program(struct brw_context *brw)
{
   struct disk_cache *cache = brw->ctx.Cache;
   if (cache == NULL)
      return;

   struct gl_program *prog =
      brw->ctx._Shader->CurrentProgram[MESA_SHADER_COMPUTE];

   if (prog && prog->sh.data->spirv)
      return;

   if (prog && !prog->program_written_to_cache) {
      struct brw_cs_prog_key cs_key;
      brw_cs_populate_key(brw, &cs_key);
      cs_key.base.program_string_id = 0;

      write_program_data(brw, prog, &cs_key, brw->cs.base.prog_data,
                         brw->cs.base.prog_offset, cache,
                         MESA_SHADER_COMPUTE);
   }
}

void
brw_disk_cache_init(struct brw_screen *screen)
{
#ifdef ENABLE_SHADER_CACHE
   if (INTEL_DEBUG(DEBUG_DISK_CACHE_DISABLE_MASK))
      return;

   /* array length: print length + null char + 1 extra to verify it is unused */
   char renderer[11];
   ASSERTED int len = snprintf(renderer, sizeof(renderer), "i965_%04x",
                                   screen->deviceID);
   assert(len == sizeof(renderer) - 2);

   const struct build_id_note *note =
      build_id_find_nhdr_for_addr(brw_disk_cache_init);
   assert(note && build_id_length(note) == 20 /* sha1 */);

   const uint8_t *id_sha1 = build_id_data(note);
   assert(id_sha1);

   char timestamp[41];
   _mesa_sha1_format(timestamp, id_sha1);

   const uint64_t driver_flags =
      brw_get_compiler_config_value(screen->compiler);
   screen->disk_cache = disk_cache_create(renderer, timestamp, driver_flags);
#endif
}
