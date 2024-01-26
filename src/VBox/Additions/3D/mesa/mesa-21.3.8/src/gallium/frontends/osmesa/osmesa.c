/*
 * Copyright (c) 2013  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Off-Screen rendering into client memory.
 * OpenGL gallium frontend for softpipe and llvmpipe.
 *
 * Notes:
 *
 * If Gallium is built with LLVM support we use the llvmpipe driver.
 * Otherwise we use softpipe.  The GALLIUM_DRIVER environment variable
 * may be set to "softpipe" or "llvmpipe" to override.
 *
 * With softpipe we could render directly into the user's buffer by using a
 * display target resource.  However, softpipe doesn't support "upside-down"
 * rendering which would be needed for the OSMESA_Y_UP=TRUE case.
 *
 * With llvmpipe we could only render directly into the user's buffer when its
 * width and height is a multiple of the tile size (64 pixels).
 *
 * Because of these constraints we always render into ordinary resources then
 * copy the results to the user's buffer in the flush_front() function which
 * is called when the app calls glFlush/Finish.
 *
 * In general, the OSMesa interface is pretty ugly and not a good match
 * for Gallium.  But we're interested in doing the best we can to preserve
 * application portability.  With a little work we could come up with a
 * much nicer, new off-screen Gallium interface...
 */


#include <stdio.h>
#include <c11/threads.h>
#include "GL/osmesa.h"

#include "glapi/glapi.h"  /* for OSMesaGetProcAddress below */

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_atomic.h"
#include "util/u_box.h"
#include "util/u_debug.h"
#include "util/format/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "postprocess/filters.h"
#include "postprocess/postprocess.h"

#include "frontend/api.h"
#include "state_tracker/st_gl_api.h"



extern struct pipe_screen *
osmesa_create_screen(void);



struct osmesa_buffer
{
   struct st_framebuffer_iface *stfb;
   struct st_visual visual;
   unsigned width, height;

   struct pipe_resource *textures[ST_ATTACHMENT_COUNT];

   void *map;

   struct osmesa_buffer *next;  /**< next in linked list */
};


struct osmesa_context
{
   struct st_context_iface *stctx;

   boolean ever_used;     /*< Has this context ever been current? */

   struct osmesa_buffer *current_buffer;

   /* Storage for depth/stencil, if the user has requested access.  The backing
    * driver always has its own storage for the actual depth/stencil, which we
    * have to transfer in and out.
    */
   void *zs;
   unsigned zs_stride;

   enum pipe_format depth_stencil_format, accum_format;

   GLenum format;         /*< User-specified context format */
   GLenum type;           /*< Buffer's data type */
   GLint user_row_length; /*< user-specified number of pixels per row */
   GLboolean y_up;        /*< TRUE  -> Y increases upward */
                          /*< FALSE -> Y increases downward */

   /** Which postprocessing filters are enabled. */
   unsigned pp_enabled[PP_FILTERS];
   struct pp_queue_t *pp;
};

/**
 * Called from the ST manager.
 */
static int
osmesa_st_get_param(struct st_manager *smapi, enum st_manager_param param)
{
   /* no-op */
   return 0;
}

static struct st_manager *stmgr = NULL;
static struct st_api *stapi = NULL;

static void
destroy_st_manager(void)
{
   if (stmgr) {
      if (stmgr->screen)
         stmgr->screen->destroy(stmgr->screen);
      FREE(stmgr);
   }

   if (stapi && stapi->destroy) {
      stapi->destroy(stapi);
   }
}

static void
create_st_manager(void)
{
   if (atexit(destroy_st_manager) != 0)
      return;

   stmgr = CALLOC_STRUCT(st_manager);
   if (stmgr) {
      stmgr->screen = osmesa_create_screen();
      stmgr->get_param = osmesa_st_get_param;
      stmgr->get_egl_image = NULL;
   }

   stapi = st_gl_api_create();
}

/**
 * Create/return a singleton st_manager object.
 */
static struct st_manager *
get_st_manager(void)
{
   static once_flag create_once_flag = ONCE_FLAG_INIT;

   call_once(&create_once_flag, create_st_manager);

   return stmgr;
}

/**
 * Create/return singleton st_api object.
 */
static struct st_api *
get_st_api(void)
{
   get_st_manager();
   return stapi;
}

/* Reads the color or depth buffer from the backing context to either the user storage
 * (color buffer) or our temporary (z/s)
 */
static void
osmesa_read_buffer(OSMesaContext osmesa, struct pipe_resource *res, void *dst,
                   int dst_stride, bool y_up)
{
   struct pipe_context *pipe = osmesa->stctx->pipe;

   struct pipe_box box;
   u_box_2d(0, 0, res->width0, res->height0, &box);

   struct pipe_transfer *transfer = NULL;
   ubyte *src = pipe->texture_map(pipe, res, 0, PIPE_MAP_READ, &box,
                                   &transfer);

   /*
    * Copy the color buffer from the resource to the user's buffer.
    */

   if (y_up) {
      /* need to flip image upside down */
      dst = (ubyte *)dst + (res->height0 - 1) * dst_stride;
      dst_stride = -dst_stride;
   }

   unsigned bpp = util_format_get_blocksize(res->format);
   for (unsigned y = 0; y < res->height0; y++)
   {
      memcpy(dst, src, bpp * res->width0);
      dst = (ubyte *)dst + dst_stride;
      src += transfer->stride;
   }

   pipe->texture_unmap(pipe, transfer);
}


/**
 * Given an OSMESA_x format and a GL_y type, return the best
 * matching PIPE_FORMAT_z.
 * Note that we can't exactly match all user format/type combinations
 * with gallium formats.  If we find this to be a problem, we can
 * implement more elaborate format/type conversion in the flush_front()
 * function.
 */
static enum pipe_format
osmesa_choose_format(GLenum format, GLenum type)
{
   switch (format) {
   case OSMESA_RGBA:
      if (type == GL_UNSIGNED_BYTE) {
#if UTIL_ARCH_LITTLE_ENDIAN
         return PIPE_FORMAT_R8G8B8A8_UNORM;
#else
         return PIPE_FORMAT_A8B8G8R8_UNORM;
#endif
      }
      else if (type == GL_UNSIGNED_SHORT) {
         return PIPE_FORMAT_R16G16B16A16_UNORM;
      }
      else if (type == GL_FLOAT) {
         return PIPE_FORMAT_R32G32B32A32_FLOAT;
      }
      else {
         return PIPE_FORMAT_NONE;
      }
      break;
   case OSMESA_BGRA:
      if (type == GL_UNSIGNED_BYTE) {
#if UTIL_ARCH_LITTLE_ENDIAN
         return PIPE_FORMAT_B8G8R8A8_UNORM;
#else
         return PIPE_FORMAT_A8R8G8B8_UNORM;
#endif
      }
      else if (type == GL_UNSIGNED_SHORT) {
         return PIPE_FORMAT_R16G16B16A16_UNORM;
      }
      else if (type == GL_FLOAT) {
         return PIPE_FORMAT_R32G32B32A32_FLOAT;
      }
      else {
         return PIPE_FORMAT_NONE;
      }
      break;
   case OSMESA_ARGB:
      if (type == GL_UNSIGNED_BYTE) {
#if UTIL_ARCH_LITTLE_ENDIAN
         return PIPE_FORMAT_A8R8G8B8_UNORM;
#else
         return PIPE_FORMAT_B8G8R8A8_UNORM;
#endif
      }
      else if (type == GL_UNSIGNED_SHORT) {
         return PIPE_FORMAT_R16G16B16A16_UNORM;
      }
      else if (type == GL_FLOAT) {
         return PIPE_FORMAT_R32G32B32A32_FLOAT;
      }
      else {
         return PIPE_FORMAT_NONE;
      }
      break;
   case OSMESA_RGB:
      if (type == GL_UNSIGNED_BYTE) {
         return PIPE_FORMAT_R8G8B8_UNORM;
      }
      else if (type == GL_UNSIGNED_SHORT) {
         return PIPE_FORMAT_R16G16B16_UNORM;
      }
      else if (type == GL_FLOAT) {
         return PIPE_FORMAT_R32G32B32_FLOAT;
      }
      else {
         return PIPE_FORMAT_NONE;
      }
      break;
   case OSMESA_BGR:
      /* No gallium format for this one */
      return PIPE_FORMAT_NONE;
   case OSMESA_RGB_565:
      if (type != GL_UNSIGNED_SHORT_5_6_5)
         return PIPE_FORMAT_NONE;
      return PIPE_FORMAT_B5G6R5_UNORM;
   default:
      return PIPE_FORMAT_NONE;
   }
}


/**
 * Initialize an st_visual object.
 */
static void
osmesa_init_st_visual(struct st_visual *vis,
                      enum pipe_format color_format,
                      enum pipe_format ds_format,
                      enum pipe_format accum_format)
{
   vis->buffer_mask = ST_ATTACHMENT_FRONT_LEFT_MASK;

   if (ds_format != PIPE_FORMAT_NONE)
      vis->buffer_mask |= ST_ATTACHMENT_DEPTH_STENCIL_MASK;
   if (accum_format != PIPE_FORMAT_NONE)
      vis->buffer_mask |= ST_ATTACHMENT_ACCUM;

   vis->color_format = color_format;
   vis->depth_stencil_format = ds_format;
   vis->accum_format = accum_format;
   vis->samples = 1;
}


/**
 * Return the osmesa_buffer that corresponds to an st_framebuffer_iface.
 */
static inline struct osmesa_buffer *
stfbi_to_osbuffer(struct st_framebuffer_iface *stfbi)
{
   return (struct osmesa_buffer *) stfbi->st_manager_private;
}


/**
 * Called via glFlush/glFinish.  This is where we copy the contents
 * of the driver's color buffer into the user-specified buffer.
 */
static bool
osmesa_st_framebuffer_flush_front(struct st_context_iface *stctx,
                                  struct st_framebuffer_iface *stfbi,
                                  enum st_attachment_type statt)
{
   OSMesaContext osmesa = OSMesaGetCurrentContext();
   struct osmesa_buffer *osbuffer = stfbi_to_osbuffer(stfbi);
   struct pipe_resource *res = osbuffer->textures[statt];
   unsigned bpp;
   int dst_stride;

   if (statt != ST_ATTACHMENT_FRONT_LEFT)
      return false;

   if (osmesa->pp) {
      struct pipe_resource *zsbuf = NULL;
      unsigned i;

      /* Find the z/stencil buffer if there is one */
      for (i = 0; i < ARRAY_SIZE(osbuffer->textures); i++) {
         struct pipe_resource *res = osbuffer->textures[i];
         if (res) {
            const struct util_format_description *desc =
               util_format_description(res->format);

            if (util_format_has_depth(desc)) {
               zsbuf = res;
               break;
            }
         }
      }

      /* run the postprocess stage(s) */
      pp_run(osmesa->pp, res, res, zsbuf);
   }

   /* Snapshot the color buffer to the user's buffer. */
   bpp = util_format_get_blocksize(osbuffer->visual.color_format);
   if (osmesa->user_row_length)
      dst_stride = bpp * osmesa->user_row_length;
   else
      dst_stride = bpp * osbuffer->width;

   osmesa_read_buffer(osmesa, res, osbuffer->map, dst_stride, osmesa->y_up);

   /* If the user has requested the Z/S buffer, then snapshot that one too. */
   if (osmesa->zs) {
      osmesa_read_buffer(osmesa, osbuffer->textures[ST_ATTACHMENT_DEPTH_STENCIL],
                         osmesa->zs, osmesa->zs_stride, true);
   }

   return true;
}


/**
 * Called by the st manager to validate the framebuffer (allocate
 * its resources).
 */
static bool
osmesa_st_framebuffer_validate(struct st_context_iface *stctx,
                               struct st_framebuffer_iface *stfbi,
                               const enum st_attachment_type *statts,
                               unsigned count,
                               struct pipe_resource **out)
{
   struct pipe_screen *screen = get_st_manager()->screen;
   enum st_attachment_type i;
   struct osmesa_buffer *osbuffer = stfbi_to_osbuffer(stfbi);
   struct pipe_resource templat;

   memset(&templat, 0, sizeof(templat));
   templat.target = PIPE_TEXTURE_RECT;
   templat.format = 0; /* setup below */
   templat.last_level = 0;
   templat.width0 = osbuffer->width;
   templat.height0 = osbuffer->height;
   templat.depth0 = 1;
   templat.array_size = 1;
   templat.usage = PIPE_USAGE_DEFAULT;
   templat.bind = 0; /* setup below */
   templat.flags = 0;

   for (i = 0; i < count; i++) {
      enum pipe_format format = PIPE_FORMAT_NONE;
      unsigned bind = 0;

      /*
       * At this time, we really only need to handle the front-left color
       * attachment, since that's all we specified for the visual in
       * osmesa_init_st_visual().
       */
      if (statts[i] == ST_ATTACHMENT_FRONT_LEFT) {
         format = osbuffer->visual.color_format;
         bind = PIPE_BIND_RENDER_TARGET;
      }
      else if (statts[i] == ST_ATTACHMENT_DEPTH_STENCIL) {
         format = osbuffer->visual.depth_stencil_format;
         bind = PIPE_BIND_DEPTH_STENCIL;
      }
      else if (statts[i] == ST_ATTACHMENT_ACCUM) {
         format = osbuffer->visual.accum_format;
         bind = PIPE_BIND_RENDER_TARGET;
      }
      else {
         debug_warning("Unexpected attachment type in "
                       "osmesa_st_framebuffer_validate()");
      }

      templat.format = format;
      templat.bind = bind;
      pipe_resource_reference(&out[i], NULL);
      out[i] = osbuffer->textures[statts[i]] =
         screen->resource_create(screen, &templat);
   }

   return true;
}

static uint32_t osmesa_fb_ID = 0;

static struct st_framebuffer_iface *
osmesa_create_st_framebuffer(void)
{
   struct st_framebuffer_iface *stfbi = CALLOC_STRUCT(st_framebuffer_iface);
   if (stfbi) {
      stfbi->flush_front = osmesa_st_framebuffer_flush_front;
      stfbi->validate = osmesa_st_framebuffer_validate;
      p_atomic_set(&stfbi->stamp, 1);
      stfbi->ID = p_atomic_inc_return(&osmesa_fb_ID);
      stfbi->state_manager = get_st_manager();
   }
   return stfbi;
}


/**
 * Create new buffer and add to linked list.
 */
static struct osmesa_buffer *
osmesa_create_buffer(enum pipe_format color_format,
                     enum pipe_format ds_format,
                     enum pipe_format accum_format)
{
   struct osmesa_buffer *osbuffer = CALLOC_STRUCT(osmesa_buffer);
   if (osbuffer) {
      osbuffer->stfb = osmesa_create_st_framebuffer();

      osbuffer->stfb->st_manager_private = osbuffer;
      osbuffer->stfb->visual = &osbuffer->visual;

      osmesa_init_st_visual(&osbuffer->visual, color_format,
                            ds_format, accum_format);
   }

   return osbuffer;
}


static void
osmesa_destroy_buffer(struct osmesa_buffer *osbuffer)
{
   /*
    * Notify the state manager that the associated framebuffer interface
    * is no longer valid.
    */
   stapi->destroy_drawable(stapi, osbuffer->stfb);

   FREE(osbuffer->stfb);
   FREE(osbuffer);
}



/**********************************************************************/
/*****                    Public Functions                        *****/
/**********************************************************************/


/**
 * Create an Off-Screen Mesa rendering context.  The only attribute needed is
 * an RGBA vs Color-Index mode flag.
 *
 * Input:  format - Must be GL_RGBA
 *         sharelist - specifies another OSMesaContext with which to share
 *                     display lists.  NULL indicates no sharing.
 * Return:  an OSMesaContext or 0 if error
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContext(GLenum format, OSMesaContext sharelist)
{
   return OSMesaCreateContextExt(format, 24, 8, 0, sharelist);
}


/**
 * New in Mesa 3.5
 *
 * Create context and specify size of ancillary buffers.
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContextExt(GLenum format, GLint depthBits, GLint stencilBits,
                       GLint accumBits, OSMesaContext sharelist)
{
   int attribs[100], n = 0;

   attribs[n++] = OSMESA_FORMAT;
   attribs[n++] = format;
   attribs[n++] = OSMESA_DEPTH_BITS;
   attribs[n++] = depthBits;
   attribs[n++] = OSMESA_STENCIL_BITS;
   attribs[n++] = stencilBits;
   attribs[n++] = OSMESA_ACCUM_BITS;
   attribs[n++] = accumBits;
   attribs[n++] = 0;

   return OSMesaCreateContextAttribs(attribs, sharelist);
}


/**
 * New in Mesa 11.2
 *
 * Create context with attribute list.
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContextAttribs(const int *attribList, OSMesaContext sharelist)
{
   OSMesaContext osmesa;
   struct st_context_iface *st_shared;
   enum st_context_error st_error = 0;
   struct st_context_attribs attribs;
   struct st_api *stapi = get_st_api();
   GLenum format = GL_RGBA;
   int depthBits = 0, stencilBits = 0, accumBits = 0;
   int profile = OSMESA_COMPAT_PROFILE, version_major = 1, version_minor = 0;
   int i;

   if (sharelist) {
      st_shared = sharelist->stctx;
   }
   else {
      st_shared = NULL;
   }

   for (i = 0; attribList[i]; i += 2) {
      switch (attribList[i]) {
      case OSMESA_FORMAT:
         format = attribList[i+1];
         switch (format) {
         case OSMESA_COLOR_INDEX:
         case OSMESA_RGBA:
         case OSMESA_BGRA:
         case OSMESA_ARGB:
         case OSMESA_RGB:
         case OSMESA_BGR:
         case OSMESA_RGB_565:
            /* legal */
            break;
         default:
            return NULL;
         }
         break;
      case OSMESA_DEPTH_BITS:
         depthBits = attribList[i+1];
         if (depthBits < 0)
            return NULL;
         break;
      case OSMESA_STENCIL_BITS:
         stencilBits = attribList[i+1];
         if (stencilBits < 0)
            return NULL;
         break;
      case OSMESA_ACCUM_BITS:
         accumBits = attribList[i+1];
         if (accumBits < 0)
            return NULL;
         break;
      case OSMESA_PROFILE:
         profile = attribList[i+1];
         if (profile != OSMESA_CORE_PROFILE &&
             profile != OSMESA_COMPAT_PROFILE)
            return NULL;
         break;
      case OSMESA_CONTEXT_MAJOR_VERSION:
         version_major = attribList[i+1];
         if (version_major < 1)
            return NULL;
         break;
      case OSMESA_CONTEXT_MINOR_VERSION:
         version_minor = attribList[i+1];
         if (version_minor < 0)
            return NULL;
         break;
      case 0:
         /* end of list */
         break;
      default:
         fprintf(stderr, "Bad attribute in OSMesaCreateContextAttribs()\n");
         return NULL;
      }
   }

   osmesa = (OSMesaContext) CALLOC_STRUCT(osmesa_context);
   if (!osmesa)
      return NULL;

   /* Choose depth/stencil/accum buffer formats */
   if (accumBits > 0) {
      osmesa->accum_format = PIPE_FORMAT_R16G16B16A16_SNORM;
   }
   if (depthBits > 0 && stencilBits > 0) {
      osmesa->depth_stencil_format = PIPE_FORMAT_Z24_UNORM_S8_UINT;
   }
   else if (stencilBits > 0) {
      osmesa->depth_stencil_format = PIPE_FORMAT_S8_UINT;
   }
   else if (depthBits >= 24) {
      osmesa->depth_stencil_format = PIPE_FORMAT_Z24X8_UNORM;
   }
   else if (depthBits >= 16) {
      osmesa->depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
   }

   /*
    * Create the rendering context
    */
   memset(&attribs, 0, sizeof(attribs));
   attribs.profile = (profile == OSMESA_CORE_PROFILE)
      ? ST_PROFILE_OPENGL_CORE : ST_PROFILE_DEFAULT;
   attribs.major = version_major;
   attribs.minor = version_minor;
   attribs.flags = 0;  /* ST_CONTEXT_FLAG_x */
   attribs.options.force_glsl_extensions_warn = FALSE;
   attribs.options.disable_blend_func_extended = FALSE;
   attribs.options.disable_glsl_line_continuations = FALSE;
   attribs.options.force_glsl_version = 0;

   osmesa_init_st_visual(&attribs.visual,
                         PIPE_FORMAT_NONE,
                         osmesa->depth_stencil_format,
                         osmesa->accum_format);

   osmesa->stctx = stapi->create_context(stapi, get_st_manager(),
                                         &attribs, &st_error, st_shared);
   if (!osmesa->stctx) {
      FREE(osmesa);
      return NULL;
   }

   osmesa->stctx->st_manager_private = osmesa;

   osmesa->format = format;
   osmesa->user_row_length = 0;
   osmesa->y_up = GL_TRUE;

   return osmesa;
}



/**
 * Destroy an Off-Screen Mesa rendering context.
 *
 * \param osmesa  the context to destroy
 */
GLAPI void GLAPIENTRY
OSMesaDestroyContext(OSMesaContext osmesa)
{
   if (osmesa) {
      pp_free(osmesa->pp);
      osmesa->stctx->destroy(osmesa->stctx);
      free(osmesa->zs);
      FREE(osmesa);
   }
}


/**
 * Bind an OSMesaContext to an image buffer.  The image buffer is just a
 * block of memory which the client provides.  Its size must be at least
 * as large as width*height*pixelSize.  Its address should be a multiple
 * of 4 if using RGBA mode.
 *
 * By default, image data is stored in the order of glDrawPixels: row-major
 * order with the lower-left image pixel stored in the first array position
 * (ie. bottom-to-top).
 *
 * If the context's viewport hasn't been initialized yet, it will now be
 * initialized to (0,0,width,height).
 *
 * Input:  osmesa - the rendering context
 *         buffer - the image buffer memory
 *         type - data type for pixel components
 *                GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT
 *                or GL_FLOAT.
 *         width, height - size of image buffer in pixels, at least 1
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid osmesa,
 *          invalid type, invalid size, etc.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaMakeCurrent(OSMesaContext osmesa, void *buffer, GLenum type,
                  GLsizei width, GLsizei height)
{
   struct st_api *stapi = get_st_api();
   enum pipe_format color_format;

   if (!osmesa && !buffer) {
      stapi->make_current(stapi, NULL, NULL, NULL);
      return GL_TRUE;
   }

   if (!osmesa || !buffer || width < 1 || height < 1) {
      return GL_FALSE;
   }

   color_format = osmesa_choose_format(osmesa->format, type);
   if (color_format == PIPE_FORMAT_NONE) {
      fprintf(stderr, "OSMesaMakeCurrent(unsupported format/type)\n");
      return GL_FALSE;
   }

   /* See if we already have a buffer that uses these pixel formats */
   if (osmesa->current_buffer &&
       (osmesa->current_buffer->visual.color_format != color_format ||
        osmesa->current_buffer->visual.depth_stencil_format != osmesa->depth_stencil_format ||
        osmesa->current_buffer->visual.accum_format != osmesa->accum_format ||
        osmesa->current_buffer->width != width ||
        osmesa->current_buffer->height != height)) {
      osmesa_destroy_buffer(osmesa->current_buffer);
      osmesa->current_buffer = NULL;
   }

   if (!osmesa->current_buffer) {
      osmesa->current_buffer = osmesa_create_buffer(color_format,
                                      osmesa->depth_stencil_format,
                                      osmesa->accum_format);
   }

   struct osmesa_buffer *osbuffer = osmesa->current_buffer;

   osbuffer->width = width;
   osbuffer->height = height;
   osbuffer->map = buffer;

   osmesa->type = type;

   stapi->make_current(stapi, osmesa->stctx, osbuffer->stfb, osbuffer->stfb);

   /* XXX: We should probably load the current color value into the buffer here
    * to match classic swrast behavior (context's fb starts with the contents of
    * your pixel buffer).
    */

   if (!osmesa->ever_used) {
      /* one-time init, just postprocessing for now */
      boolean any_pp_enabled = FALSE;
      unsigned i;

      for (i = 0; i < ARRAY_SIZE(osmesa->pp_enabled); i++) {
         if (osmesa->pp_enabled[i]) {
            any_pp_enabled = TRUE;
            break;
         }
      }

      if (any_pp_enabled) {
         osmesa->pp = pp_init(osmesa->stctx->pipe,
                              osmesa->pp_enabled,
                              osmesa->stctx->cso_context,
                              osmesa->stctx);

         pp_init_fbos(osmesa->pp, width, height);
      }

      osmesa->ever_used = TRUE;
   }

   return GL_TRUE;
}



GLAPI OSMesaContext GLAPIENTRY
OSMesaGetCurrentContext(void)
{
   struct st_api *stapi = get_st_api();
   struct st_context_iface *st = stapi->get_current(stapi);
   return st ? (OSMesaContext) st->st_manager_private : NULL;
}



GLAPI void GLAPIENTRY
OSMesaPixelStore(GLint pname, GLint value)
{
   OSMesaContext osmesa = OSMesaGetCurrentContext();

   switch (pname) {
   case OSMESA_ROW_LENGTH:
      osmesa->user_row_length = value;
      break;
   case OSMESA_Y_UP:
      osmesa->y_up = value ? GL_TRUE : GL_FALSE;
      break;
   default:
      fprintf(stderr, "Invalid pname in OSMesaPixelStore()\n");
      return;
   }
}


GLAPI void GLAPIENTRY
OSMesaGetIntegerv(GLint pname, GLint *value)
{
   OSMesaContext osmesa = OSMesaGetCurrentContext();
   struct osmesa_buffer *osbuffer = osmesa ? osmesa->current_buffer : NULL;

   switch (pname) {
   case OSMESA_WIDTH:
      *value = osbuffer ? osbuffer->width : 0;
      return;
   case OSMESA_HEIGHT:
      *value = osbuffer ? osbuffer->height : 0;
      return;
   case OSMESA_FORMAT:
      *value = osmesa->format;
      return;
   case OSMESA_TYPE:
      /* current color buffer's data type */
      *value = osmesa->type;
      return;
   case OSMESA_ROW_LENGTH:
      *value = osmesa->user_row_length;
      return;
   case OSMESA_Y_UP:
      *value = osmesa->y_up;
      return;
   case OSMESA_MAX_WIDTH:
      FALLTHROUGH;
   case OSMESA_MAX_HEIGHT:
      {
         struct pipe_screen *screen = get_st_manager()->screen;
         *value = screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_SIZE);
      }
      return;
   default:
      fprintf(stderr, "Invalid pname in OSMesaGetIntegerv()\n");
      return;
   }
}


/**
 * Return information about the depth buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetDepthBuffer(OSMesaContext c, GLint *width, GLint *height,
                     GLint *bytesPerValue, void **buffer)
{
   struct osmesa_buffer *osbuffer = c->current_buffer;
   struct pipe_resource *res = osbuffer->textures[ST_ATTACHMENT_DEPTH_STENCIL];

   if (!res) {
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = NULL;
      return GL_FALSE;
   }

   *width = res->width0;
   *height = res->height0;
   *bytesPerValue = util_format_get_blocksize(res->format);

   if (!c->zs) {
      c->zs_stride = *width * *bytesPerValue;
      c->zs = calloc(c->zs_stride, *height);
      if (!c->zs)
         return GL_FALSE;

      osmesa_read_buffer(c, res, c->zs, c->zs_stride, true);
   }

   *buffer = c->zs;

   return GL_TRUE;
}


/**
 * Return the color buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          format - the pixel format (OSMESA_FORMAT)
 *          buffer - pointer to color buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetColorBuffer(OSMesaContext osmesa, GLint *width,
                      GLint *height, GLint *format, void **buffer)
{
   struct osmesa_buffer *osbuffer = osmesa->current_buffer;

   if (osbuffer) {
      *width = osbuffer->width;
      *height = osbuffer->height;
      *format = osmesa->format;
      *buffer = osbuffer->map;
      return GL_TRUE;
   }
   else {
      *width = 0;
      *height = 0;
      *format = 0;
      *buffer = 0;
      return GL_FALSE;
   }
}


struct name_function
{
   const char *Name;
   OSMESAproc Function;
};

static struct name_function functions[] = {
   { "OSMesaCreateContext", (OSMESAproc) OSMesaCreateContext },
   { "OSMesaCreateContextExt", (OSMESAproc) OSMesaCreateContextExt },
   { "OSMesaCreateContextAttribs", (OSMESAproc) OSMesaCreateContextAttribs },
   { "OSMesaDestroyContext", (OSMESAproc) OSMesaDestroyContext },
   { "OSMesaMakeCurrent", (OSMESAproc) OSMesaMakeCurrent },
   { "OSMesaGetCurrentContext", (OSMESAproc) OSMesaGetCurrentContext },
   { "OSMesaPixelStore", (OSMESAproc) OSMesaPixelStore },
   { "OSMesaGetIntegerv", (OSMESAproc) OSMesaGetIntegerv },
   { "OSMesaGetDepthBuffer", (OSMESAproc) OSMesaGetDepthBuffer },
   { "OSMesaGetColorBuffer", (OSMESAproc) OSMesaGetColorBuffer },
   { "OSMesaGetProcAddress", (OSMESAproc) OSMesaGetProcAddress },
   { "OSMesaColorClamp", (OSMESAproc) OSMesaColorClamp },
   { "OSMesaPostprocess", (OSMESAproc) OSMesaPostprocess },
   { NULL, NULL }
};


GLAPI OSMESAproc GLAPIENTRY
OSMesaGetProcAddress(const char *funcName)
{
   int i;
   for (i = 0; functions[i].Name; i++) {
      if (strcmp(functions[i].Name, funcName) == 0)
         return functions[i].Function;
   }
   return _glapi_get_proc_address(funcName);
}


GLAPI void GLAPIENTRY
OSMesaColorClamp(GLboolean enable)
{
   extern void GLAPIENTRY _mesa_ClampColor(GLenum target, GLenum clamp);

   _mesa_ClampColor(GL_CLAMP_FRAGMENT_COLOR_ARB,
                    enable ? GL_TRUE : GL_FIXED_ONLY_ARB);
}


GLAPI void GLAPIENTRY
OSMesaPostprocess(OSMesaContext osmesa, const char *filter,
                  unsigned enable_value)
{
   if (!osmesa->ever_used) {
      /* We can only enable/disable postprocess filters before a context
       * is made current for the first time.
       */
      unsigned i;

      for (i = 0; i < PP_FILTERS; i++) {
         if (strcmp(pp_filters[i].name, filter) == 0) {
            osmesa->pp_enabled[i] = enable_value;
            return;
         }
      }
      debug_warning("OSMesaPostprocess(unknown filter)\n");
   }
   else {
      debug_warning("Calling OSMesaPostprocess() after OSMesaMakeCurrent()\n");
   }
}
