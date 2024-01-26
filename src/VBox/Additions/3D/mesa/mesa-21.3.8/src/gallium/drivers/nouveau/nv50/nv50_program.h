/*
 * Copyright 2010 Ben Skeggs
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

#ifndef __NV50_PROG_H__
#define __NV50_PROG_H__

struct nv50_context;

#include "pipe/p_state.h"

struct nv50_varying {
   uint8_t id; /* tgsi index */
   uint8_t hw; /* hw index, nv50 wants flat FP inputs last */

   unsigned mask   : 4;
   unsigned linear : 1;
   unsigned pad    : 3;

   ubyte sn; /* semantic name */
   ubyte si; /* semantic index */
};

struct nv50_stream_output_state
{
   uint32_t ctrl;
   uint16_t stride[4];
   uint8_t num_attribs[4];
   uint8_t map_size;
   uint8_t map[128];
};

struct nv50_gmem_state {
   unsigned valid : 1; /* whether there's something there */
   unsigned image : 1; /* buffer or image */
   unsigned slot  : 6; /* slot in the relevant resource arrays */
};

struct nv50_program {
   struct pipe_shader_state pipe;

   ubyte type;
   bool translated;

   uint32_t *code;
   unsigned code_size;
   unsigned code_base;
   uint32_t *immd;
   unsigned parm_size; /* size limit of uniform buffer */
   uint32_t tls_space; /* required local memory per thread */

   ubyte max_gpr; /* REG_ALLOC_TEMP */
   ubyte max_out; /* REG_ALLOC_RESULT or FP_RESULT_COUNT */

   ubyte in_nr;
   ubyte out_nr;
   struct nv50_varying in[16];
   struct nv50_varying out[16];

   struct {
      uint32_t attrs[3]; /* VP_ATTR_EN_0,1 and VP_GP_BUILTIN_ATTR_EN */
      ubyte psiz;        /* output slot of point size */
      ubyte bfc[2];      /* indices into varying for FFC (FP) or BFC (VP) */
      ubyte edgeflag;
      ubyte clpd[2];     /* output slot of clip distance[i]'s 1st component */
      ubyte clpd_nr;
      bool need_vertex_id;
      uint32_t clip_mode;
      uint8_t clip_enable; /* mask of defined clip planes */
      uint8_t cull_enable; /* mask of defined cull distances */
   } vp;

   struct {
      uint32_t flags[2]; /* 0x19a8, 196c */
      uint32_t interp; /* 0x1988 */
      uint32_t colors; /* 0x1904 */
      uint8_t has_samplemask;
      uint8_t force_persample_interp;
      uint8_t alphatest;
   } fp;

   struct {
      uint32_t vert_count;
      uint8_t prim_type; /* point, line strip or tri strip */
      uint8_t has_layer;
      ubyte layerid; /* hw value of layer output */
      uint8_t has_viewport;
      ubyte viewportid; /* hw value of viewport index output */
   } gp;

   struct {
      uint32_t lmem_size; /* local memory (TGSI PRIVATE resource) size */
      uint32_t smem_size; /* shared memory (TGSI LOCAL resource) size */
      struct nv50_gmem_state gmem[NV50_MAX_GLOBALS];
   } cp;

   bool mul_zero_wins;

   void *fixups; /* relocation records */
   void *interps; /* interpolation records */

   struct nouveau_heap *mem;

   struct nv50_stream_output_state *so;
};

bool nv50_program_translate(struct nv50_program *, uint16_t chipset,
                            struct pipe_debug_callback *);
bool nv50_program_upload_code(struct nv50_context *, struct nv50_program *);
void nv50_program_destroy(struct nv50_context *, struct nv50_program *);

#endif /* __NV50_PROG_H__ */
