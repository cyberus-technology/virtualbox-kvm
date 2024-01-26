/*
 * Copyright © 2011 Intel Corporation
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

#ifndef BRW_PROGRAM_H
#define BRW_PROGRAM_H

#include "compiler/brw_compiler.h"
#include "nir.h"

#ifdef __cplusplus
extern "C" {
#endif

struct brw_context;
struct blob;
struct blob_reader;

enum brw_param_domain {
   BRW_PARAM_DOMAIN_BUILTIN = 0,
   BRW_PARAM_DOMAIN_PARAMETER,
   BRW_PARAM_DOMAIN_UNIFORM,
   BRW_PARAM_DOMAIN_IMAGE,
};

#define BRW_PARAM(domain, val)   (BRW_PARAM_DOMAIN_##domain << 24 | (val))
#define BRW_PARAM_DOMAIN(param)  ((uint32_t)(param) >> 24)
#define BRW_PARAM_VALUE(param)   ((uint32_t)(param) & 0x00ffffff)

#define BRW_PARAM_PARAMETER(idx, comp) \
   BRW_PARAM(PARAMETER, ((idx) << 2) | (comp))
#define BRW_PARAM_PARAMETER_IDX(param)    (BRW_PARAM_VALUE(param) >> 2)
#define BRW_PARAM_PARAMETER_COMP(param)   (BRW_PARAM_VALUE(param) & 0x3)

#define BRW_PARAM_UNIFORM(idx)            BRW_PARAM(UNIFORM, (idx))
#define BRW_PARAM_UNIFORM_IDX(param)      BRW_PARAM_VALUE(param)

#define BRW_PARAM_IMAGE(idx, offset) BRW_PARAM(IMAGE, ((idx) << 8) | (offset))
#define BRW_PARAM_IMAGE_IDX(value)        (BRW_PARAM_VALUE(value) >> 8)
#define BRW_PARAM_IMAGE_OFFSET(value)     (BRW_PARAM_VALUE(value) & 0xf)

struct nir_shader *brw_create_nir(struct brw_context *brw,
                                  const struct gl_shader_program *shader_prog,
                                  struct gl_program *prog,
                                  gl_shader_stage stage,
                                  bool is_scalar);

void brw_nir_lower_resources(nir_shader *nir,
                             struct gl_shader_program *shader_prog,
                             struct gl_program *prog,
                             const struct intel_device_info *devinfo);

void brw_shader_gather_info(nir_shader *nir, struct gl_program *prog);

void brw_setup_tex_for_precompile(const struct intel_device_info *devinfo,
                                  struct brw_sampler_prog_key_data *tex,
                                  const struct gl_program *prog);

void brw_populate_base_prog_key(struct gl_context *ctx,
                                const struct brw_program *prog,
                                struct brw_base_prog_key *key);
void brw_populate_default_base_prog_key(const struct intel_device_info *devinfo,
                                        const struct brw_program *prog,
                                        struct brw_base_prog_key *key);
void brw_debug_recompile(struct brw_context *brw, gl_shader_stage stage,
                         unsigned api_id, struct brw_base_prog_key *key);

uint32_t
brw_assign_common_binding_table_offsets(const struct intel_device_info *devinfo,
                                        const struct gl_program *prog,
                                        struct brw_stage_prog_data *stage_prog_data,
                                        uint32_t next_binding_table_offset);

void
brw_populate_default_key(const struct brw_compiler *compiler,
                         union brw_any_prog_key *prog_key,
                         struct gl_shader_program *sh_prog,
                         struct gl_program *prog);

void
brw_stage_prog_data_free(const void *prog_data);

void
brw_dump_arb_asm(const char *stage, struct gl_program *prog);

bool brw_vs_precompile(struct gl_context *ctx, struct gl_program *prog);
bool brw_tcs_precompile(struct gl_context *ctx,
                        struct gl_shader_program *shader_prog,
                        struct gl_program *prog);
bool brw_tes_precompile(struct gl_context *ctx,
                        struct gl_shader_program *shader_prog,
                        struct gl_program *prog);
bool brw_gs_precompile(struct gl_context *ctx, struct gl_program *prog);
bool brw_fs_precompile(struct gl_context *ctx, struct gl_program *prog);
bool brw_cs_precompile(struct gl_context *ctx, struct gl_program *prog);

GLboolean brw_link_shader(struct gl_context *ctx, struct gl_shader_program *prog);

void brw_upload_tcs_prog(struct brw_context *brw);
void brw_tcs_populate_key(struct brw_context *brw,
                          struct brw_tcs_prog_key *key);
void brw_tcs_populate_default_key(const struct brw_compiler *compiler,
                                  struct brw_tcs_prog_key *key,
                                  struct gl_shader_program *sh_prog,
                                  struct gl_program *prog);
void brw_upload_tes_prog(struct brw_context *brw);
void brw_tes_populate_key(struct brw_context *brw,
                          struct brw_tes_prog_key *key);
void brw_tes_populate_default_key(const struct brw_compiler *compiler,
                                  struct brw_tes_prog_key *key,
                                  struct gl_shader_program *sh_prog,
                                  struct gl_program *prog);

void brw_write_blob_program_data(struct blob *binary, gl_shader_stage stage,
                                 const void *program,
                                 struct brw_stage_prog_data *prog_data);
bool brw_read_blob_program_data(struct blob_reader *binary,
                                struct gl_program *prog, gl_shader_stage stage,
                                const uint8_t **program,
                                struct brw_stage_prog_data *prog_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
