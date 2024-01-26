/*
 * Copyright (c) 2017 Intel Corporation
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

#include <stdint.h>

#include "compiler/nir/nir_serialize.h"
#include "util/build_id.h"
#include "util/mesa-sha1.h"

#include "brw_context.h"
#include "brw_program.h"
#include "brw_state.h"

static uint8_t driver_sha1[20];

void
brw_program_binary_init(unsigned device_id)
{
   const struct build_id_note *note =
      build_id_find_nhdr_for_addr(brw_program_binary_init);
   assert(note);

   /**
    * With Mesa's megadrivers, taking the sha1 of i965_dri.so may not be
    * unique. Therefore, we make a sha1 of the "i965" string and the sha1
    * build id from i965_dri.so.
    */
   struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   char renderer[10];
   assert(device_id < 0x10000);
   int len = snprintf(renderer, sizeof(renderer), "i965_%04x", device_id);
   assert(len == sizeof(renderer) - 1);
   _mesa_sha1_update(&ctx, renderer, len);
   _mesa_sha1_update(&ctx, build_id_data(note), build_id_length(note));
   _mesa_sha1_final(&ctx, driver_sha1);
}

void
brw_get_program_binary_driver_sha1(struct gl_context *ctx, uint8_t *sha1)
{
   memcpy(sha1, driver_sha1, sizeof(uint8_t) * 20);
}

enum driver_cache_blob_part {
   END_PART,
   INTEL_PART,
   NIR_PART,
};

static bool
blob_parts_valid(void *blob, uint32_t size)
{
   struct blob_reader reader;
   blob_reader_init(&reader, blob, size);

   do {
      uint32_t part_type = blob_read_uint32(&reader);
      if (reader.overrun)
         return false;
      if (part_type == END_PART)
         return reader.current == reader.end;
      switch ((enum driver_cache_blob_part)part_type) {
      case INTEL_PART:
      case NIR_PART:
         /* Read the uint32_t part-size and skip over it */
         blob_skip_bytes(&reader, blob_read_uint32(&reader));
         if (reader.overrun)
            return false;
         break;
      default:
         return false;
      }
   } while (true);
}

static bool
blob_has_part(void *blob, uint32_t size, enum driver_cache_blob_part part)
{
   struct blob_reader reader;
   blob_reader_init(&reader, blob, size);

   assert(blob_parts_valid(blob, size));
   do {
      uint32_t part_type = blob_read_uint32(&reader);
      if (part_type == END_PART)
         return false;
      if (part_type == part)
         return true;
      blob_skip_bytes(&reader, blob_read_uint32(&reader));
   } while (true);
}

static bool
driver_blob_is_ready(void *blob, uint32_t size, bool with_intel_program)
{
   if (!blob) {
      return false;
   } else if (!blob_parts_valid(blob, size)) {
      unreachable("Driver blob format is bad!");
      return false;
   } else if (blob_has_part(blob, size, INTEL_PART) == with_intel_program) {
      return true;
   } else {
      return false;
   }
}

static void
serialize_nir_part(struct blob *writer, struct gl_program *prog)
{
   blob_write_uint32(writer, NIR_PART);
   intptr_t size_offset = blob_reserve_uint32(writer);
   size_t nir_start = writer->size;
   nir_serialize(writer, prog->nir, false);
   blob_overwrite_uint32(writer, size_offset, writer->size - nir_start);
}

void
brw_program_serialize_nir(struct gl_context *ctx, struct gl_program *prog)
{
   if (driver_blob_is_ready(prog->driver_cache_blob,
                            prog->driver_cache_blob_size, false))
      return;

   if (prog->driver_cache_blob)
      ralloc_free(prog->driver_cache_blob);

   struct blob writer;
   blob_init(&writer);
   serialize_nir_part(&writer, prog);
   blob_write_uint32(&writer, END_PART);
   prog->driver_cache_blob = ralloc_size(NULL, writer.size);
   memcpy(prog->driver_cache_blob, writer.data, writer.size);
   prog->driver_cache_blob_size = writer.size;
   blob_finish(&writer);
}

static bool
deserialize_intel_program(struct blob_reader *reader, struct gl_context *ctx,
                        struct gl_program *prog, gl_shader_stage stage)
{
   struct brw_context *brw = brw_context(ctx);

   union brw_any_prog_key prog_key;
   blob_copy_bytes(reader, &prog_key, brw_prog_key_size(stage));
   prog_key.base.program_string_id = brw_program(prog)->id;

   enum brw_cache_id cache_id = brw_stage_cache_id(stage);

   const uint8_t *program;
   struct brw_stage_prog_data *prog_data =
      ralloc_size(NULL, sizeof(union brw_any_prog_data));

   if (!brw_read_blob_program_data(reader, prog, stage, &program, prog_data)) {
      ralloc_free(prog_data);
      return false;
   }

   uint32_t offset;
   void *out_prog_data;
   brw_upload_cache(&brw->cache, cache_id, &prog_key, brw_prog_key_size(stage),
                    program, prog_data->program_size, prog_data,
                    brw_prog_data_size(stage), &offset, &out_prog_data);

   ralloc_free(prog_data);

   return true;
}

void
brw_program_deserialize_driver_blob(struct gl_context *ctx,
                                    struct gl_program *prog,
                                    gl_shader_stage stage)
{
   if (!prog->driver_cache_blob)
      return;

   struct blob_reader reader;
   blob_reader_init(&reader, prog->driver_cache_blob,
                    prog->driver_cache_blob_size);

   do {
      uint32_t part_type = blob_read_uint32(&reader);
      if ((enum driver_cache_blob_part)part_type == END_PART)
         break;
      switch ((enum driver_cache_blob_part)part_type) {
      case INTEL_PART: {
         ASSERTED uint32_t gen_size = blob_read_uint32(&reader);
         assert(!reader.overrun &&
                (uintptr_t)(reader.end - reader.current) > gen_size);
         deserialize_intel_program(&reader, ctx, prog, stage);
         break;
      }
      case NIR_PART: {
         ASSERTED uint32_t nir_size = blob_read_uint32(&reader);
         assert(!reader.overrun &&
                (uintptr_t)(reader.end - reader.current) > nir_size);
         const struct nir_shader_compiler_options *options =
            ctx->Const.ShaderCompilerOptions[stage].NirOptions;
         prog->nir = nir_deserialize(NULL, options, &reader);
         break;
      }
      default:
         unreachable("Unsupported blob part type!");
         break;
      }
   } while (true);

   ralloc_free(prog->driver_cache_blob);
   prog->driver_cache_blob = NULL;
   prog->driver_cache_blob_size = 0;
}

/* This is just a wrapper around brw_program_deserialize_nir() as i965
 * doesn't need gl_shader_program like other drivers do.
 */
void
brw_deserialize_program_binary(struct gl_context *ctx,
                               struct gl_shader_program *shProg,
                               struct gl_program *prog)
{
   brw_program_deserialize_driver_blob(ctx, prog, prog->info.stage);
}

static void
serialize_intel_part(struct blob *writer, struct gl_context *ctx,
                   struct gl_shader_program *sh_prog,
                   struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);

   union brw_any_prog_key key;
   brw_populate_default_key(brw->screen->compiler, &key, sh_prog, prog);

   const gl_shader_stage stage = prog->info.stage;
   uint32_t offset = 0;
   void *prog_data = NULL;
   if (brw_search_cache(&brw->cache, brw_stage_cache_id(stage), &key,
                        brw_prog_key_size(stage), &offset, &prog_data,
                        false)) {
      const void *program_map = brw->cache.map + offset;
      /* TODO: Improve perf for non-LLC. It would be best to save it at
       * program generation time when the program is in normal memory
       * accessible with cache to the CPU. Another easier change would be to
       * use _mesa_streaming_load_memcpy to read from the program mapped
       * memory.
       */
      blob_write_uint32(writer, INTEL_PART);
      intptr_t size_offset = blob_reserve_uint32(writer);
      size_t gen_start = writer->size;
      blob_write_bytes(writer, &key, brw_prog_key_size(stage));
      brw_write_blob_program_data(writer, stage, program_map, prog_data);
      blob_overwrite_uint32(writer, size_offset, writer->size - gen_start);
   }
}

void
brw_serialize_program_binary(struct gl_context *ctx,
                             struct gl_shader_program *sh_prog,
                             struct gl_program *prog)
{
   if (driver_blob_is_ready(prog->driver_cache_blob,
                            prog->driver_cache_blob_size, true))
      return;

   if (prog->driver_cache_blob) {
      if (!prog->nir) {
         /* If we loaded from the disk shader cache, then the nir might not
          * have been deserialized yet.
          */
         brw_program_deserialize_driver_blob(ctx, prog, prog->info.stage);
      }
      ralloc_free(prog->driver_cache_blob);
   }

   struct blob writer;
   blob_init(&writer);
   serialize_nir_part(&writer, prog);
   serialize_intel_part(&writer, ctx, sh_prog, prog);
   blob_write_uint32(&writer, END_PART);
   prog->driver_cache_blob = ralloc_size(NULL, writer.size);
   memcpy(prog->driver_cache_blob, writer.data, writer.size);
   prog->driver_cache_blob_size = writer.size;
   blob_finish(&writer);
}

void
brw_write_blob_program_data(struct blob *binary, gl_shader_stage stage,
                            const void *program,
                            struct brw_stage_prog_data *prog_data)
{
   /* Write prog_data to blob. */
   blob_write_bytes(binary, prog_data, brw_prog_data_size(stage));

   /* Write program to blob. */
   blob_write_bytes(binary, program, prog_data->program_size);

   /* Write push params */
   blob_write_bytes(binary, prog_data->param,
                    sizeof(uint32_t) * prog_data->nr_params);

   /* Write pull params */
   blob_write_bytes(binary, prog_data->pull_param,
                    sizeof(uint32_t) * prog_data->nr_pull_params);
}

bool
brw_read_blob_program_data(struct blob_reader *binary, struct gl_program *prog,
                           gl_shader_stage stage, const uint8_t **program,
                           struct brw_stage_prog_data *prog_data)
{
   /* Read shader prog_data from blob. */
   blob_copy_bytes(binary, prog_data, brw_prog_data_size(stage));
   if (binary->overrun)
      return false;

   /* Read shader program from blob. */
   *program = blob_read_bytes(binary, prog_data->program_size);

   /* Read push params */
   prog_data->param = rzalloc_array(NULL, uint32_t, prog_data->nr_params);
   blob_copy_bytes(binary, prog_data->param,
                   sizeof(uint32_t) * prog_data->nr_params);

   /* Read pull params */
   prog_data->pull_param = rzalloc_array(NULL, uint32_t,
                                         prog_data->nr_pull_params);
   blob_copy_bytes(binary, prog_data->pull_param,
                   sizeof(uint32_t) * prog_data->nr_pull_params);

   return !binary->overrun;
}
