/*
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef INTELTEX_INC
#define INTELTEX_INC

#include "main/mtypes.h"
#include "main/formats.h"
#include "brw_context.h"
#include "brw_mipmap_tree.h"

void brw_init_texture_functions(struct dd_function_table *functions);

void brw_init_texture_image_functions(struct dd_function_table *functions);

void brw_init_texture_copy_image_functions(struct dd_function_table *functs);

void brw_init_copy_image_functions(struct dd_function_table *functions);

void brw_set_texbuffer(__DRIcontext *pDRICtx,
                       GLint target, __DRIdrawable *pDraw);
void brw_set_texbuffer2(__DRIcontext *pDRICtx,
                        GLint target, GLint format, __DRIdrawable *pDraw);
void brw_release_texbuffer(__DRIcontext *pDRICtx, GLint target,
                           __DRIdrawable *dPriv);

struct brw_mipmap_tree *
brw_miptree_create_for_teximage(struct brw_context *brw,
                                struct brw_texture_object *brw_obj,
                                struct brw_texture_image *brw_image,
                                enum brw_miptree_create_flags flags);

void brw_finalize_mipmap_tree(struct brw_context *brw,
                                struct gl_texture_object *tex_obj);

#endif
