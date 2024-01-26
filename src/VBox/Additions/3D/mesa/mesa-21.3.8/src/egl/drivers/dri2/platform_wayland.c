/*
 * Copyright © 2011-2012 Intel Corporation
 * Copyright © 2012 Collabora, Ltd.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <xf86drm.h>
#include "drm-uapi/drm_fourcc.h"
#include <sys/mman.h>

#include "egl_dri2.h"
#include "loader_dri_helper.h"
#include "loader.h"
#include "util/u_vector.h"
#include "util/anon_file.h"
#include "eglglobals.h"

#include <wayland-egl-backend.h>
#include <wayland-client.h>
#include "wayland-drm-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"

/*
 * The index of entries in this table is used as a bitmask in
 * dri2_dpy->formats, which tracks the formats supported by our server.
 */
static const struct dri2_wl_visual {
   const char *format_name;
   uint32_t wl_drm_format;
   uint32_t wl_shm_format;
   int dri_image_format;
   /* alt_dri_image_format is a substitute wl_buffer format to use for a
    * wl-server unsupported dri_image_format, ie. some other dri_image_format in
    * the table, of the same precision but with different channel ordering, or
    * __DRI_IMAGE_FORMAT_NONE if an alternate format is not needed or supported.
    * The code checks if alt_dri_image_format can be used as a fallback for a
    * dri_image_format for a given wl-server implementation.
    */
   int alt_dri_image_format;
   int bpp;
   int rgba_shifts[4];
   unsigned int rgba_sizes[4];
} dri2_wl_visuals[] = {
   {
      "ABGR16F",
      WL_DRM_FORMAT_ABGR16F, WL_SHM_FORMAT_ABGR16161616F,
      __DRI_IMAGE_FORMAT_ABGR16161616F, 0, 64,
      { 0, 16, 32, 48 },
      { 16, 16, 16, 16 },
   },
   {
      "XBGR16F",
      WL_DRM_FORMAT_XBGR16F, WL_SHM_FORMAT_XBGR16161616F,
      __DRI_IMAGE_FORMAT_XBGR16161616F, 0, 64,
      { 0, 16, 32, -1 },
      { 16, 16, 16, 0 },
   },
   {
      "XRGB2101010",
      WL_DRM_FORMAT_XRGB2101010, WL_SHM_FORMAT_XRGB2101010,
      __DRI_IMAGE_FORMAT_XRGB2101010, __DRI_IMAGE_FORMAT_XBGR2101010, 32,
      { 20, 10, 0, -1 },
      { 10, 10, 10, 0 },
   },
   {
      "ARGB2101010",
      WL_DRM_FORMAT_ARGB2101010, WL_SHM_FORMAT_ARGB2101010,
      __DRI_IMAGE_FORMAT_ARGB2101010, __DRI_IMAGE_FORMAT_ABGR2101010, 32,
      { 20, 10, 0, 30 },
      { 10, 10, 10, 2 },
   },
   {
      "XBGR2101010",
      WL_DRM_FORMAT_XBGR2101010, WL_SHM_FORMAT_XBGR2101010,
      __DRI_IMAGE_FORMAT_XBGR2101010, __DRI_IMAGE_FORMAT_XRGB2101010, 32,
      { 0, 10, 20, -1 },
      { 10, 10, 10, 0 },
   },
   {
      "ABGR2101010",
      WL_DRM_FORMAT_ABGR2101010, WL_SHM_FORMAT_ABGR2101010,
      __DRI_IMAGE_FORMAT_ABGR2101010, __DRI_IMAGE_FORMAT_ARGB2101010, 32,
      { 0, 10, 20, 30 },
      { 10, 10, 10, 2 },
   },
   {
      "XRGB8888",
      WL_DRM_FORMAT_XRGB8888, WL_SHM_FORMAT_XRGB8888,
      __DRI_IMAGE_FORMAT_XRGB8888, __DRI_IMAGE_FORMAT_NONE, 32,
      { 16, 8, 0, -1 },
      { 8, 8, 8, 0 },
   },
   {
      "ARGB8888",
      WL_DRM_FORMAT_ARGB8888, WL_SHM_FORMAT_ARGB8888,
      __DRI_IMAGE_FORMAT_ARGB8888, __DRI_IMAGE_FORMAT_NONE, 32,
      { 16, 8, 0, 24 },
      { 8, 8, 8, 8 },
   },
   {
      "ABGR8888",
      WL_DRM_FORMAT_ABGR8888, WL_SHM_FORMAT_ABGR8888,
      __DRI_IMAGE_FORMAT_ABGR8888, __DRI_IMAGE_FORMAT_NONE, 32,
      { 0, 8, 16, 24 },
      { 8, 8, 8, 8 },
   },
   {
      "XBGR8888",
      WL_DRM_FORMAT_XBGR8888, WL_SHM_FORMAT_XBGR8888,
      __DRI_IMAGE_FORMAT_XBGR8888, __DRI_IMAGE_FORMAT_NONE, 32,
      { 0, 8, 16, -1 },
      { 8, 8, 8, 0 },
   },
   {
      "RGB565",
      WL_DRM_FORMAT_RGB565, WL_SHM_FORMAT_RGB565,
      __DRI_IMAGE_FORMAT_RGB565, __DRI_IMAGE_FORMAT_NONE, 16,
      { 11, 5, 0, -1 },
      { 5, 6, 5, 0 },
   },
};

static_assert(ARRAY_SIZE(dri2_wl_visuals) <= EGL_DRI2_MAX_FORMATS,
              "dri2_egl_display::formats is not large enough for "
              "the formats in dri2_wl_visuals");

static int
dri2_wl_visual_idx_from_config(struct dri2_egl_display *dri2_dpy,
                               const __DRIconfig *config,
                               bool force_opaque)
{
   int shifts[4];
   unsigned int sizes[4];

   dri2_get_shifts_and_sizes(dri2_dpy->core, config, shifts, sizes);

   for (unsigned int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      const struct dri2_wl_visual *wl_visual = &dri2_wl_visuals[i];

      int cmp_rgb_shifts = memcmp(shifts, wl_visual->rgba_shifts,
                                  3 * sizeof(shifts[0]));
      int cmp_rgb_sizes = memcmp(sizes, wl_visual->rgba_sizes,
                                 3 * sizeof(sizes[0]));

      if (cmp_rgb_shifts == 0 && cmp_rgb_sizes == 0 &&
          wl_visual->rgba_shifts[3] == (force_opaque ? -1 : shifts[3]) &&
          wl_visual->rgba_sizes[3] == (force_opaque ? 0 : sizes[3])) {
         return i;
      }
   }

   return -1;
}

static int
dri2_wl_visual_idx_from_fourcc(uint32_t fourcc)
{
   for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      /* wl_drm format codes overlap with DRIImage FourCC codes for all formats
       * we support. */
      if (dri2_wl_visuals[i].wl_drm_format == fourcc)
         return i;
   }

   return -1;
}

static int
dri2_wl_visual_idx_from_dri_image_format(uint32_t dri_image_format)
{
   for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      if (dri2_wl_visuals[i].dri_image_format == dri_image_format)
         return i;
   }

   return -1;
}

static int
dri2_wl_visual_idx_from_shm_format(uint32_t shm_format)
{
   for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      if (dri2_wl_visuals[i].wl_shm_format == shm_format)
         return i;
   }

   return -1;
}

bool
dri2_wl_is_format_supported(void* user_data, uint32_t format)
{
   _EGLDisplay *disp = (_EGLDisplay *) user_data;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   int j = dri2_wl_visual_idx_from_fourcc(format);

   if (j == -1)
      return false;

   for (int i = 0; dri2_dpy->driver_configs[i]; i++)
      if (j == dri2_wl_visual_idx_from_config(dri2_dpy,
                                              dri2_dpy->driver_configs[i],
                                              false))
         return true;

   return false;
}

static int
roundtrip(struct dri2_egl_display *dri2_dpy)
{
   return wl_display_roundtrip_queue(dri2_dpy->wl_dpy, dri2_dpy->wl_queue);
}

static void
wl_buffer_release(void *data, struct wl_buffer *buffer)
{
   struct dri2_egl_surface *dri2_surf = data;
   int i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); ++i)
      if (dri2_surf->color_buffers[i].wl_buffer == buffer)
         break;

   assert (i < ARRAY_SIZE(dri2_surf->color_buffers));

   if (dri2_surf->color_buffers[i].wl_release) {
      wl_buffer_destroy(buffer);
      dri2_surf->color_buffers[i].wl_release = false;
      dri2_surf->color_buffers[i].wl_buffer = NULL;
   }

   dri2_surf->color_buffers[i].locked = false;
}

static const struct wl_buffer_listener wl_buffer_listener = {
   .release = wl_buffer_release
};

static void
resize_callback(struct wl_egl_window *wl_win, void *data)
{
   struct dri2_egl_surface *dri2_surf = data;
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   if (dri2_surf->base.Width == wl_win->width &&
       dri2_surf->base.Height == wl_win->height)
      return;

   dri2_surf->resized = true;

   /* Update the surface size as soon as native window is resized; from user
    * pov, this makes the effect that resize is done immediately after native
    * window resize, without requiring to wait until the first draw.
    *
    * A more detailed and lengthy explanation can be found at
    * https://lists.freedesktop.org/archives/mesa-dev/2018-June/196474.html
    */
   if (!dri2_surf->back) {
      dri2_surf->base.Width = wl_win->width;
      dri2_surf->base.Height = wl_win->height;
   }
   dri2_dpy->flush->invalidate(dri2_surf->dri_drawable);
}

static void
destroy_window_callback(void *data)
{
   struct dri2_egl_surface *dri2_surf = data;
   dri2_surf->wl_win = NULL;
}

static struct wl_surface *
get_wl_surface_proxy(struct wl_egl_window *window)
{
    /* Version 3 of wl_egl_window introduced a version field at the same
     * location where a pointer to wl_surface was stored. Thus, if
     * window->version is dereferenceable, we've been given an older version of
     * wl_egl_window, and window->version points to wl_surface */
   if (_eglPointerIsDereferencable((void *)(window->version))) {
      return wl_proxy_create_wrapper((void *)(window->version));
   }
   return wl_proxy_create_wrapper(window->surface);
}

/**
 * Called via eglCreateWindowSurface(), drv->CreateWindowSurface().
 */
static _EGLSurface *
dri2_wl_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
                              void *native_window, const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct wl_egl_window *window = native_window;
   struct dri2_egl_surface *dri2_surf;
   int visual_idx;
   const __DRIconfig *config;

   if (!window) {
      _eglError(EGL_BAD_NATIVE_WINDOW, "dri2_create_surface");
      return NULL;
   }

   if (window->driver_private) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }

   dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }

   if (!dri2_init_surface(&dri2_surf->base, disp, EGL_WINDOW_BIT, conf,
                          attrib_list, false, native_window))
      goto cleanup_surf;

   config = dri2_get_dri_config(dri2_conf, EGL_WINDOW_BIT,
                                dri2_surf->base.GLColorspace);

   if (!config) {
      _eglError(EGL_BAD_MATCH, "Unsupported surfacetype/colorspace configuration");
      goto cleanup_surf;
   }

   dri2_surf->base.Width = window->width;
   dri2_surf->base.Height = window->height;

#ifndef NDEBUG
   /* Enforce that every visual has an opaque variant (requirement to support
    * EGL_EXT_present_opaque)
    */
   for (unsigned int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      const struct dri2_wl_visual *transparent_visual = &dri2_wl_visuals[i];
      if (transparent_visual->rgba_sizes[3] == 0) {
         continue;
      }

      bool found_opaque_equivalent = false;
      for (unsigned int j = 0; j < ARRAY_SIZE(dri2_wl_visuals); j++) {
         const struct dri2_wl_visual *opaque_visual = &dri2_wl_visuals[j];
         if (opaque_visual->rgba_sizes[3] != 0) {
            continue;
         }

         int cmp_rgb_shifts = memcmp(transparent_visual->rgba_shifts,
                                     opaque_visual->rgba_shifts,
                                     3 * sizeof(opaque_visual->rgba_shifts[0]));
         int cmp_rgb_sizes = memcmp(transparent_visual->rgba_sizes,
                                    opaque_visual->rgba_sizes,
                                    3 * sizeof(opaque_visual->rgba_sizes[0]));

         if (cmp_rgb_shifts == 0 && cmp_rgb_sizes == 0) {
            found_opaque_equivalent = true;
            break;
         }
      }

      assert(found_opaque_equivalent);
   }
#endif

   visual_idx = dri2_wl_visual_idx_from_config(dri2_dpy, config,
                                               dri2_surf->base.PresentOpaque);
   assert(visual_idx != -1);

   if (dri2_dpy->wl_dmabuf || dri2_dpy->wl_drm) {
      dri2_surf->format = dri2_wl_visuals[visual_idx].wl_drm_format;
   } else {
      assert(dri2_dpy->wl_shm);
      dri2_surf->format = dri2_wl_visuals[visual_idx].wl_shm_format;
   }

   dri2_surf->wl_queue = wl_display_create_queue(dri2_dpy->wl_dpy);
   if (!dri2_surf->wl_queue) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      goto cleanup_surf;
   }

   if (dri2_dpy->wl_drm) {
      dri2_surf->wl_drm_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_drm);
      if (!dri2_surf->wl_drm_wrapper) {
         _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
         goto cleanup_queue;
      }
      wl_proxy_set_queue((struct wl_proxy *)dri2_surf->wl_drm_wrapper,
                         dri2_surf->wl_queue);
   }

   dri2_surf->wl_dpy_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dpy);
   if (!dri2_surf->wl_dpy_wrapper) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      goto cleanup_drm;
   }
   wl_proxy_set_queue((struct wl_proxy *)dri2_surf->wl_dpy_wrapper,
                      dri2_surf->wl_queue);

   dri2_surf->wl_surface_wrapper = get_wl_surface_proxy(window);
   if (!dri2_surf->wl_surface_wrapper) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      goto cleanup_dpy_wrapper;
   }
   wl_proxy_set_queue((struct wl_proxy *)dri2_surf->wl_surface_wrapper,
                      dri2_surf->wl_queue);

   dri2_surf->wl_win = window;
   dri2_surf->wl_win->driver_private = dri2_surf;
   dri2_surf->wl_win->destroy_window_callback = destroy_window_callback;
   if (dri2_dpy->flush)
      dri2_surf->wl_win->resize_callback = resize_callback;

   if (!dri2_create_drawable(dri2_dpy, config, dri2_surf, dri2_surf))
       goto cleanup_surf_wrapper;

   dri2_surf->base.SwapInterval = dri2_dpy->default_swap_interval;

   return &dri2_surf->base;

 cleanup_surf_wrapper:
   wl_proxy_wrapper_destroy(dri2_surf->wl_surface_wrapper);
 cleanup_dpy_wrapper:
   wl_proxy_wrapper_destroy(dri2_surf->wl_dpy_wrapper);
 cleanup_drm:
   if (dri2_surf->wl_drm_wrapper)
      wl_proxy_wrapper_destroy(dri2_surf->wl_drm_wrapper);
 cleanup_queue:
   wl_event_queue_destroy(dri2_surf->wl_queue);
 cleanup_surf:
   free(dri2_surf);

   return NULL;
}

static _EGLSurface *
dri2_wl_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
                              void *native_window, const EGLint *attrib_list)
{
   /* From the EGL_EXT_platform_wayland spec, version 3:
    *
    *   It is not valid to call eglCreatePlatformPixmapSurfaceEXT with a <dpy>
    *   that belongs to Wayland. Any such call fails and generates
    *   EGL_BAD_PARAMETER.
    */
   _eglError(EGL_BAD_PARAMETER, "cannot create EGL pixmap surfaces on "
             "Wayland");
   return NULL;
}

/**
 * Called via eglDestroySurface(), drv->DestroySurface().
 */
static EGLBoolean
dri2_wl_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);

   dri2_dpy->core->destroyDrawable(dri2_surf->dri_drawable);

   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].wl_buffer)
         wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
      if (dri2_surf->color_buffers[i].dri_image)
         dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].dri_image);
      if (dri2_surf->color_buffers[i].linear_copy)
         dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].linear_copy);
      if (dri2_surf->color_buffers[i].data)
         munmap(dri2_surf->color_buffers[i].data,
                dri2_surf->color_buffers[i].data_size);
   }

   if (dri2_dpy->dri2)
      dri2_egl_surface_free_local_buffers(dri2_surf);

   if (dri2_surf->throttle_callback)
      wl_callback_destroy(dri2_surf->throttle_callback);

   if (dri2_surf->wl_win) {
      dri2_surf->wl_win->driver_private = NULL;
      dri2_surf->wl_win->resize_callback = NULL;
      dri2_surf->wl_win->destroy_window_callback = NULL;
   }

   wl_proxy_wrapper_destroy(dri2_surf->wl_surface_wrapper);
   wl_proxy_wrapper_destroy(dri2_surf->wl_dpy_wrapper);
   if (dri2_surf->wl_drm_wrapper)
      wl_proxy_wrapper_destroy(dri2_surf->wl_drm_wrapper);
   wl_event_queue_destroy(dri2_surf->wl_queue);

   dri2_fini_surface(surf);
   free(surf);

   return EGL_TRUE;
}

static void
dri2_wl_release_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].wl_buffer) {
         if (dri2_surf->color_buffers[i].locked) {
            dri2_surf->color_buffers[i].wl_release = true;
         } else {
            wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
            dri2_surf->color_buffers[i].wl_buffer = NULL;
         }
      }
      if (dri2_surf->color_buffers[i].dri_image)
         dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].dri_image);
      if (dri2_surf->color_buffers[i].linear_copy)
         dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].linear_copy);
      if (dri2_surf->color_buffers[i].data)
         munmap(dri2_surf->color_buffers[i].data,
                dri2_surf->color_buffers[i].data_size);

      dri2_surf->color_buffers[i].dri_image = NULL;
      dri2_surf->color_buffers[i].linear_copy = NULL;
      dri2_surf->color_buffers[i].data = NULL;
   }

   if (dri2_dpy->dri2)
      dri2_egl_surface_free_local_buffers(dri2_surf);
}

static int
get_back_bo(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   int use_flags;
   int visual_idx;
   unsigned int dri_image_format;
   unsigned int linear_dri_image_format;
   uint64_t *modifiers;
   int num_modifiers;

   visual_idx = dri2_wl_visual_idx_from_fourcc(dri2_surf->format);
   assert(visual_idx != -1);
   dri_image_format = dri2_wl_visuals[visual_idx].dri_image_format;
   linear_dri_image_format = dri_image_format;
   modifiers = u_vector_tail(&dri2_dpy->wl_modifiers[visual_idx]);
   num_modifiers = u_vector_length(&dri2_dpy->wl_modifiers[visual_idx]);

   if (num_modifiers == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID) {
      /* For the purposes of this function, an INVALID modifier on its own
       * means the modifiers aren't supported.
       */
      num_modifiers = 0;
   }

   /* Substitute dri image format if server does not support original format */
   if (!BITSET_TEST(dri2_dpy->formats, visual_idx))
      linear_dri_image_format = dri2_wl_visuals[visual_idx].alt_dri_image_format;

   /* These asserts hold, as long as dri2_wl_visuals[] is self-consistent and
    * the PRIME substitution logic in dri2_wl_add_configs_for_visuals() is free
    * of bugs.
    */
   assert(linear_dri_image_format != __DRI_IMAGE_FORMAT_NONE);
   assert(BITSET_TEST(dri2_dpy->formats,
          dri2_wl_visual_idx_from_dri_image_format(linear_dri_image_format)));

   /* There might be a buffer release already queued that wasn't processed */
   wl_display_dispatch_queue_pending(dri2_dpy->wl_dpy, dri2_surf->wl_queue);

   while (dri2_surf->back == NULL) {
      for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
         /* Get an unlocked buffer, preferably one with a dri_buffer
          * already allocated. */
         if (dri2_surf->color_buffers[i].locked)
            continue;
         if (dri2_surf->back == NULL)
            dri2_surf->back = &dri2_surf->color_buffers[i];
         else if (dri2_surf->back->dri_image == NULL)
            dri2_surf->back = &dri2_surf->color_buffers[i];
      }

      if (dri2_surf->back)
         break;

      /* If we don't have a buffer, then block on the server to release one for
       * us, and try again. wl_display_dispatch_queue will process any pending
       * events, however not all servers flush on issuing a buffer release
       * event. So, we spam the server with roundtrips as they always cause a
       * client flush.
       */
      if (wl_display_roundtrip_queue(dri2_dpy->wl_dpy,
                                     dri2_surf->wl_queue) < 0)
          return -1;
   }

   if (dri2_surf->back == NULL)
      return -1;

   use_flags = __DRI_IMAGE_USE_SHARE | __DRI_IMAGE_USE_BACKBUFFER;

   if (dri2_surf->base.ProtectedContent) {
      /* Protected buffers can't be read from another GPU */
      if (dri2_dpy->is_different_gpu)
         return -1;
      use_flags |= __DRI_IMAGE_USE_PROTECTED;
   }

   if (dri2_dpy->is_different_gpu &&
       dri2_surf->back->linear_copy == NULL) {
      /* The LINEAR modifier should be a perfect alias of the LINEAR use
       * flag; try the new interface first before the old, then fall back. */
      uint64_t linear_mod = DRM_FORMAT_MOD_LINEAR;

      dri2_surf->back->linear_copy =
            loader_dri_create_image(dri2_dpy->dri_screen, dri2_dpy->image,
                                    dri2_surf->base.Width,
                                    dri2_surf->base.Height,
                                    linear_dri_image_format,
                                    use_flags | __DRI_IMAGE_USE_LINEAR,
                                    &linear_mod, 1, NULL);

      if (dri2_surf->back->linear_copy == NULL)
          return -1;
   }

   if (dri2_surf->back->dri_image == NULL) {
      /* If our DRIImage implementation does not support
       * createImageWithModifiers, then fall back to the old createImage,
       * and hope it allocates an image which is acceptable to the winsys.
        */
      dri2_surf->back->dri_image =
            loader_dri_create_image(dri2_dpy->dri_screen, dri2_dpy->image,
                                    dri2_surf->base.Width,
                                    dri2_surf->base.Height,
                                    dri_image_format,
                                    dri2_dpy->is_different_gpu ? 0 : use_flags,
                                    modifiers, num_modifiers, NULL);

      dri2_surf->back->age = 0;
   }
   if (dri2_surf->back->dri_image == NULL)
      return -1;

   dri2_surf->back->locked = true;

   return 0;
}


static void
back_bo_to_dri_buffer(struct dri2_egl_surface *dri2_surf, __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   __DRIimage *image;
   int name, pitch;

   image = dri2_surf->back->dri_image;

   dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_NAME, &name);
   dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_STRIDE, &pitch);

   buffer->attachment = __DRI_BUFFER_BACK_LEFT;
   buffer->name = name;
   buffer->pitch = pitch;
   buffer->cpp = 4;
   buffer->flags = 0;
}

static int
update_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   if (dri2_surf->wl_win &&
       (dri2_surf->base.Width != dri2_surf->wl_win->width ||
        dri2_surf->base.Height != dri2_surf->wl_win->height)) {

      dri2_surf->base.Width  = dri2_surf->wl_win->width;
      dri2_surf->base.Height = dri2_surf->wl_win->height;
      dri2_surf->dx = dri2_surf->wl_win->dx;
      dri2_surf->dy = dri2_surf->wl_win->dy;
   }

   if (dri2_surf->resized) {
       dri2_wl_release_buffers(dri2_surf);
       dri2_surf->resized = false;
   }

   if (get_back_bo(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "failed to allocate color buffer");
      return -1;
   }

   /* If we have an extra unlocked buffer at this point, we had to do triple
    * buffering for a while, but now can go back to just double buffering.
    * That means we can free any unlocked buffer now. */
   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked &&
          dri2_surf->color_buffers[i].wl_buffer) {
         wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
         dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].dri_image);
         if (dri2_dpy->is_different_gpu)
            dri2_dpy->image->destroyImage(dri2_surf->color_buffers[i].linear_copy);
         dri2_surf->color_buffers[i].wl_buffer = NULL;
         dri2_surf->color_buffers[i].dri_image = NULL;
         dri2_surf->color_buffers[i].linear_copy = NULL;
      }
   }

   return 0;
}

static int
update_buffers_if_needed(struct dri2_egl_surface *dri2_surf)
{
   if (dri2_surf->back != NULL)
      return 0;

   return update_buffers(dri2_surf);
}

static __DRIbuffer *
dri2_wl_get_buffers_with_format(__DRIdrawable * driDrawable,
                                int *width, int *height,
                                unsigned int *attachments, int count,
                                int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int i, j;

   if (update_buffers(dri2_surf) < 0)
      return NULL;

   for (i = 0, j = 0; i < 2 * count; i += 2, j++) {
      __DRIbuffer *local;

      switch (attachments[i]) {
      case __DRI_BUFFER_BACK_LEFT:
         back_bo_to_dri_buffer(dri2_surf, &dri2_surf->buffers[j]);
         break;
      default:
         local = dri2_egl_surface_alloc_local_buffer(dri2_surf, attachments[i],
                                                     attachments[i + 1]);

         if (!local) {
            _eglError(EGL_BAD_ALLOC, "failed to allocate local buffer");
            return NULL;
         }
         dri2_surf->buffers[j] = *local;
         break;
      }
   }

   *out_count = j;
   if (j == 0)
      return NULL;

   *width = dri2_surf->base.Width;
   *height = dri2_surf->base.Height;

   return dri2_surf->buffers;
}

static __DRIbuffer *
dri2_wl_get_buffers(__DRIdrawable * driDrawable,
                    int *width, int *height,
                    unsigned int *attachments, int count,
                    int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   unsigned int *attachments_with_format;
   __DRIbuffer *buffer;
   int visual_idx = dri2_wl_visual_idx_from_fourcc(dri2_surf->format);

   if (visual_idx == -1)
      return NULL;

   attachments_with_format = calloc(count, 2 * sizeof(unsigned int));
   if (!attachments_with_format) {
      *out_count = 0;
      return NULL;
   }

   for (int i = 0; i < count; ++i) {
      attachments_with_format[2*i] = attachments[i];
      attachments_with_format[2*i + 1] = dri2_wl_visuals[visual_idx].bpp;
   }

   buffer =
      dri2_wl_get_buffers_with_format(driDrawable,
                                      width, height,
                                      attachments_with_format, count,
                                      out_count, loaderPrivate);

   free(attachments_with_format);

   return buffer;
}

static int
image_get_buffers(__DRIdrawable *driDrawable,
                  unsigned int format,
                  uint32_t *stamp,
                  void *loaderPrivate,
                  uint32_t buffer_mask,
                  struct __DRIimageList *buffers)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   if (update_buffers(dri2_surf) < 0)
      return 0;

   buffers->image_mask = __DRI_IMAGE_BUFFER_BACK;
   buffers->back = dri2_surf->back->dri_image;

   return 1;
}

static void
dri2_wl_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
   (void) driDrawable;
   (void) loaderPrivate;
}

static unsigned
dri2_wl_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
{
   switch (cap) {
   case DRI_LOADER_CAP_FP16:
      return 1;
   case DRI_LOADER_CAP_RGBA_ORDERING:
      return 1;
   default:
      return 0;
   }
}

static const __DRIdri2LoaderExtension dri2_loader_extension = {
   .base = { __DRI_DRI2_LOADER, 4 },

   .getBuffers           = dri2_wl_get_buffers,
   .flushFrontBuffer     = dri2_wl_flush_front_buffer,
   .getBuffersWithFormat = dri2_wl_get_buffers_with_format,
   .getCapability        = dri2_wl_get_capability,
};

static const __DRIimageLoaderExtension image_loader_extension = {
   .base = { __DRI_IMAGE_LOADER, 2 },

   .getBuffers          = image_get_buffers,
   .flushFrontBuffer    = dri2_wl_flush_front_buffer,
   .getCapability       = dri2_wl_get_capability,
};

static void
wayland_throttle_callback(void *data,
                          struct wl_callback *callback,
                          uint32_t time)
{
   struct dri2_egl_surface *dri2_surf = data;

   dri2_surf->throttle_callback = NULL;
   wl_callback_destroy(callback);
}

static const struct wl_callback_listener throttle_listener = {
   .done = wayland_throttle_callback
};

static EGLBoolean
get_fourcc(struct dri2_egl_display *dri2_dpy,
           __DRIimage *image, int *fourcc)
{
   EGLBoolean query;
   int dri_format;
   int visual_idx;

   query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FOURCC,
                                       fourcc);
   if (query)
      return true;

   query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FORMAT,
                                       &dri_format);
   if (!query)
      return false;

   visual_idx = dri2_wl_visual_idx_from_dri_image_format(dri_format);
   if (visual_idx == -1)
      return false;

   *fourcc = dri2_wl_visuals[visual_idx].wl_drm_format;
   return true;
}

static struct wl_buffer *
create_wl_buffer(struct dri2_egl_display *dri2_dpy,
                 struct dri2_egl_surface *dri2_surf,
                 __DRIimage *image)
{
   struct wl_buffer *ret;
   EGLBoolean query;
   int width, height, fourcc, num_planes;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;

   query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_WIDTH, &width);
   query &= dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_HEIGHT,
                                        &height);
   query &= get_fourcc(dri2_dpy, image, &fourcc);
   if (!query)
      return NULL;

   query = dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_NUM_PLANES,
                                       &num_planes);
   if (!query)
      num_planes = 1;

   if (dri2_dpy->image->base.version >= 15) {
      int mod_hi, mod_lo;

      query = dri2_dpy->image->queryImage(image,
                                          __DRI_IMAGE_ATTRIB_MODIFIER_UPPER,
                                          &mod_hi);
      query &= dri2_dpy->image->queryImage(image,
                                           __DRI_IMAGE_ATTRIB_MODIFIER_LOWER,
                                           &mod_lo);
      if (query) {
         modifier = combine_u32_into_u64(mod_hi, mod_lo);
      }
   }

   bool supported_modifier = false;
   bool mod_invalid_supported = false;
   int visual_idx = dri2_wl_visual_idx_from_fourcc(fourcc);
   assert(visual_idx != -1);

   uint64_t *mod;
   u_vector_foreach(mod, &dri2_dpy->wl_modifiers[visual_idx]) {
      if (*mod == DRM_FORMAT_MOD_INVALID) {
         mod_invalid_supported = true;
      }
      if (*mod == modifier) {
         supported_modifier = true;
         break;
      }
   }
   if (!supported_modifier && mod_invalid_supported) {
      /* If the server has advertised DRM_FORMAT_MOD_INVALID then we trust
       * that the client has allocated the buffer with the right implicit
       * modifier for the format, even though it's allocated a buffer the
       * server hasn't explicitly claimed to support. */
      modifier = DRM_FORMAT_MOD_INVALID;
      supported_modifier = true;
   }

   if (dri2_dpy->wl_dmabuf && supported_modifier) {
      struct zwp_linux_buffer_params_v1 *params;
      int i;

      /* We don't need a wrapper for wl_dmabuf objects, because we have to
       * create the intermediate params object; we can set the queue on this,
       * and the wl_buffer inherits it race-free. */
      params = zwp_linux_dmabuf_v1_create_params(dri2_dpy->wl_dmabuf);
      if (dri2_surf)
         wl_proxy_set_queue((struct wl_proxy *) params, dri2_surf->wl_queue);

      for (i = 0; i < num_planes; i++) {
         __DRIimage *p_image;
         int stride, offset;
         int fd = -1;

         p_image = dri2_dpy->image->fromPlanar(image, i, NULL);
         if (!p_image) {
            assert(i == 0);
            p_image = image;
         }

         query = dri2_dpy->image->queryImage(p_image,
                                             __DRI_IMAGE_ATTRIB_FD,
                                             &fd);
         query &= dri2_dpy->image->queryImage(p_image,
                                              __DRI_IMAGE_ATTRIB_STRIDE,
                                              &stride);
         query &= dri2_dpy->image->queryImage(p_image,
                                              __DRI_IMAGE_ATTRIB_OFFSET,
                                              &offset);
         if (image != p_image)
            dri2_dpy->image->destroyImage(p_image);

         if (!query) {
            if (fd >= 0)
               close(fd);
            zwp_linux_buffer_params_v1_destroy(params);
            return NULL;
         }

         zwp_linux_buffer_params_v1_add(params, fd, i, offset, stride,
                                        modifier >> 32, modifier & 0xffffffff);
         close(fd);
      }

      ret = zwp_linux_buffer_params_v1_create_immed(params, width, height,
                                                    fourcc, 0);
      zwp_linux_buffer_params_v1_destroy(params);
   } else if (dri2_dpy->capabilities & WL_DRM_CAPABILITY_PRIME) {
      struct wl_drm *wl_drm =
         dri2_surf ? dri2_surf->wl_drm_wrapper : dri2_dpy->wl_drm;
      int fd, stride;

      if (num_planes > 1)
         return NULL;

      dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FD, &fd);
      dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_STRIDE, &stride);
      ret = wl_drm_create_prime_buffer(wl_drm, fd, width, height, fourcc, 0,
                                       stride, 0, 0, 0, 0);
      close(fd);
   } else {
      struct wl_drm *wl_drm =
         dri2_surf ? dri2_surf->wl_drm_wrapper : dri2_dpy->wl_drm;
      int name, stride;

      if (num_planes > 1)
         return NULL;

      dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_NAME, &name);
      dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_STRIDE, &stride);
      ret = wl_drm_create_buffer(wl_drm, name, width, height, stride, fourcc);
   }

   return ret;
}

static EGLBoolean
try_damage_buffer(struct dri2_egl_surface *dri2_surf,
                  const EGLint *rects,
                  EGLint n_rects)
{
   if (wl_proxy_get_version((struct wl_proxy *) dri2_surf->wl_surface_wrapper)
       < WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
      return EGL_FALSE;

   for (int i = 0; i < n_rects; i++) {
      const int *rect = &rects[i * 4];

      wl_surface_damage_buffer(dri2_surf->wl_surface_wrapper,
                               rect[0],
                               dri2_surf->base.Height - rect[1] - rect[3],
                               rect[2], rect[3]);
   }
   return EGL_TRUE;
}

/**
 * Called via eglSwapBuffers(), drv->SwapBuffers().
 */
static EGLBoolean
dri2_wl_swap_buffers_with_damage(_EGLDisplay *disp,
                                 _EGLSurface *draw,
                                 const EGLint *rects,
                                 EGLint n_rects)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   if (!dri2_surf->wl_win)
      return _eglError(EGL_BAD_NATIVE_WINDOW, "dri2_swap_buffers");

   while (dri2_surf->throttle_callback != NULL)
      if (wl_display_dispatch_queue(dri2_dpy->wl_dpy,
                                    dri2_surf->wl_queue) == -1)
         return -1;

   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
      if (dri2_surf->color_buffers[i].age > 0)
         dri2_surf->color_buffers[i].age++;

   /* Make sure we have a back buffer in case we're swapping without ever
    * rendering. */
   if (update_buffers_if_needed(dri2_surf) < 0)
      return _eglError(EGL_BAD_ALLOC, "dri2_swap_buffers");

   if (draw->SwapInterval > 0) {
      dri2_surf->throttle_callback =
         wl_surface_frame(dri2_surf->wl_surface_wrapper);
      wl_callback_add_listener(dri2_surf->throttle_callback,
                               &throttle_listener, dri2_surf);
   }

   dri2_surf->back->age = 1;
   dri2_surf->current = dri2_surf->back;
   dri2_surf->back = NULL;

   if (!dri2_surf->current->wl_buffer) {
      __DRIimage *image;

      if (dri2_dpy->is_different_gpu)
         image = dri2_surf->current->linear_copy;
      else
         image = dri2_surf->current->dri_image;

      dri2_surf->current->wl_buffer =
         create_wl_buffer(dri2_dpy, dri2_surf, image);

      dri2_surf->current->wl_release = false;

      wl_buffer_add_listener(dri2_surf->current->wl_buffer,
                             &wl_buffer_listener, dri2_surf);
   }

   wl_surface_attach(dri2_surf->wl_surface_wrapper,
                     dri2_surf->current->wl_buffer,
                     dri2_surf->dx, dri2_surf->dy);

   dri2_surf->wl_win->attached_width  = dri2_surf->base.Width;
   dri2_surf->wl_win->attached_height = dri2_surf->base.Height;
   /* reset resize growing parameters */
   dri2_surf->dx = 0;
   dri2_surf->dy = 0;

   /* If the compositor doesn't support damage_buffer, we deliberately
    * ignore the damage region and post maximum damage, due to
    * https://bugs.freedesktop.org/78190 */
   if (!n_rects || !try_damage_buffer(dri2_surf, rects, n_rects))
      wl_surface_damage(dri2_surf->wl_surface_wrapper,
                        0, 0, INT32_MAX, INT32_MAX);

   if (dri2_dpy->is_different_gpu) {
      _EGLContext *ctx = _eglGetCurrentContext();
      struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
      dri2_dpy->image->blitImage(dri2_ctx->dri_context,
                                 dri2_surf->current->linear_copy,
                                 dri2_surf->current->dri_image,
                                 0, 0, dri2_surf->base.Width,
                                 dri2_surf->base.Height,
                                 0, 0, dri2_surf->base.Width,
                                 dri2_surf->base.Height, 0);
   }

   dri2_flush_drawable_for_swapbuffers(disp, draw);
   dri2_dpy->flush->invalidate(dri2_surf->dri_drawable);

   wl_surface_commit(dri2_surf->wl_surface_wrapper);

   /* If we're not waiting for a frame callback then we'll at least throttle
    * to a sync callback so that we always give a chance for the compositor to
    * handle the commit and send a release event before checking for a free
    * buffer */
   if (dri2_surf->throttle_callback == NULL) {
      dri2_surf->throttle_callback = wl_display_sync(dri2_surf->wl_dpy_wrapper);
      wl_callback_add_listener(dri2_surf->throttle_callback,
                               &throttle_listener, dri2_surf);
   }

   wl_display_flush(dri2_dpy->wl_dpy);

   return EGL_TRUE;
}

static EGLint
dri2_wl_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surface)
{
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surface);

   if (update_buffers_if_needed(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "dri2_query_buffer_age");
      return -1;
   }

   return dri2_surf->back->age;
}

static EGLBoolean
dri2_wl_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
{
   return dri2_wl_swap_buffers_with_damage(disp, draw, NULL, 0);
}

static struct wl_buffer *
dri2_wl_create_wayland_buffer_from_image(_EGLDisplay *disp, _EGLImage *img)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);
   __DRIimage *image = dri2_img->dri_image;
   struct wl_buffer *buffer;
   int format, visual_idx;

   /* Check the upstream display supports this buffer's format. */
   dri2_dpy->image->queryImage(image, __DRI_IMAGE_ATTRIB_FORMAT, &format);
   visual_idx = dri2_wl_visual_idx_from_dri_image_format(format);
   if (visual_idx == -1)
      goto bad_format;

   if (!BITSET_TEST(dri2_dpy->formats, visual_idx))
      goto bad_format;

   buffer = create_wl_buffer(dri2_dpy, NULL, image);

   /* The buffer object will have been created with our internal event queue
    * because it is using wl_dmabuf/wl_drm as a proxy factory. We want the
    * buffer to be used by the application so we'll reset it to the display's
    * default event queue. This isn't actually racy, as the only event the
    * buffer can get is a buffer release, which doesn't happen with an explicit
    * attach. */
   if (buffer)
      wl_proxy_set_queue((struct wl_proxy *) buffer, NULL);

   return buffer;

bad_format:
   _eglError(EGL_BAD_MATCH, "unsupported image format");
   return NULL;
}

static int
dri2_wl_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   int ret = 0;

   if (dri2_dpy->is_render_node) {
      _eglLog(_EGL_WARNING, "wayland-egl: client asks server to "
                            "authenticate for render-nodes");
      return 0;
   }
   dri2_dpy->authenticated = false;

   wl_drm_authenticate(dri2_dpy->wl_drm, id);
   if (roundtrip(dri2_dpy) < 0)
      ret = -1;

   if (!dri2_dpy->authenticated)
      ret = -1;

   /* reset authenticated */
   dri2_dpy->authenticated = true;

   return ret;
}

static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
   struct dri2_egl_display *dri2_dpy = data;
   drm_magic_t magic;

   dri2_dpy->device_name = strdup(device);
   if (!dri2_dpy->device_name)
      return;

   dri2_dpy->fd = loader_open_device(dri2_dpy->device_name);
   if (dri2_dpy->fd == -1) {
      _eglLog(_EGL_WARNING, "wayland-egl: could not open %s (%s)",
              dri2_dpy->device_name, strerror(errno));
      free(dri2_dpy->device_name);
      dri2_dpy->device_name = NULL;
      return;
   }

   if (drmGetNodeTypeFromFd(dri2_dpy->fd) == DRM_NODE_RENDER) {
      dri2_dpy->authenticated = true;
   } else {
      if (drmGetMagic(dri2_dpy->fd, &magic)) {
         close(dri2_dpy->fd);
         dri2_dpy->fd = -1;
         free(dri2_dpy->device_name);
         dri2_dpy->device_name = NULL;
         _eglLog(_EGL_WARNING, "wayland-egl: drmGetMagic failed");
         return;
      }
      wl_drm_authenticate(dri2_dpy->wl_drm, magic);
   }
}

static void
drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
{
   struct dri2_egl_display *dri2_dpy = data;
   int visual_idx = dri2_wl_visual_idx_from_fourcc(format);

   if (visual_idx == -1)
      return;

   BITSET_SET(dri2_dpy->formats, visual_idx);
}

static void
drm_handle_capabilities(void *data, struct wl_drm *drm, uint32_t value)
{
   struct dri2_egl_display *dri2_dpy = data;

   dri2_dpy->capabilities = value;
}

static void
drm_handle_authenticated(void *data, struct wl_drm *drm)
{
   struct dri2_egl_display *dri2_dpy = data;

   dri2_dpy->authenticated = true;
}

static const struct wl_drm_listener drm_listener = {
   .device = drm_handle_device,
   .format = drm_handle_format,
   .authenticated = drm_handle_authenticated,
   .capabilities = drm_handle_capabilities
};

static void
dmabuf_ignore_format(void *data, struct zwp_linux_dmabuf_v1 *dmabuf,
                     uint32_t format)
{
   /* formats are implicitly advertised by the 'modifier' event, so ignore */
}

static void
dmabuf_handle_modifier(void *data, struct zwp_linux_dmabuf_v1 *dmabuf,
                       uint32_t format, uint32_t modifier_hi,
                       uint32_t modifier_lo)
{
   struct dri2_egl_display *dri2_dpy = data;
   int visual_idx = dri2_wl_visual_idx_from_fourcc(format);
   uint64_t *mod;

   if (visual_idx == -1)
      return;

   BITSET_SET(dri2_dpy->formats, visual_idx);

   mod = u_vector_add(&dri2_dpy->wl_modifiers[visual_idx]);
   *mod = combine_u32_into_u64(modifier_hi, modifier_lo);
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
   .format = dmabuf_ignore_format,
   .modifier = dmabuf_handle_modifier,
};

static void
registry_handle_global_drm(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface,
                           uint32_t version)
{
   struct dri2_egl_display *dri2_dpy = data;

   if (strcmp(interface, "wl_drm") == 0) {
      dri2_dpy->wl_drm =
         wl_registry_bind(registry, name, &wl_drm_interface, MIN2(version, 2));
      wl_drm_add_listener(dri2_dpy->wl_drm, &drm_listener, dri2_dpy);
   } else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0 && version >= 3) {
      dri2_dpy->wl_dmabuf =
         wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface,
                          MIN2(version, 3));
      zwp_linux_dmabuf_v1_add_listener(dri2_dpy->wl_dmabuf, &dmabuf_listener,
                                       dri2_dpy);
   }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener_drm = {
   .global = registry_handle_global_drm,
   .global_remove = registry_handle_global_remove
};

static void
dri2_wl_setup_swap_interval(_EGLDisplay *disp)
{
   /* We can't use values greater than 1 on Wayland because we are using the
    * frame callback to synchronise the frame and the only way we be sure to
    * get a frame callback is to attach a new buffer. Therefore we can't just
    * sit drawing nothing to wait until the next ‘n’ frame callbacks */

   dri2_setup_swap_interval(disp, 1);
}

static const struct dri2_egl_display_vtbl dri2_wl_display_vtbl = {
   .authenticate = dri2_wl_authenticate,
   .create_window_surface = dri2_wl_create_window_surface,
   .create_pixmap_surface = dri2_wl_create_pixmap_surface,
   .destroy_surface = dri2_wl_destroy_surface,
   .create_image = dri2_create_image_khr,
   .swap_buffers = dri2_wl_swap_buffers,
   .swap_buffers_with_damage = dri2_wl_swap_buffers_with_damage,
   .query_buffer_age = dri2_wl_query_buffer_age,
   .create_wayland_buffer_from_image = dri2_wl_create_wayland_buffer_from_image,
   .get_dri_drawable = dri2_surface_get_dri_drawable,
};

static const __DRIextension *dri2_loader_extensions[] = {
   &dri2_loader_extension.base,
   &image_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   NULL,
};

static const __DRIextension *image_loader_extensions[] = {
   &image_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   NULL,
};

static EGLBoolean
dri2_wl_add_configs_for_visuals(_EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   unsigned int format_count[ARRAY_SIZE(dri2_wl_visuals)] = { 0 };
   unsigned int count = 0;
   bool assigned;

   for (unsigned i = 0; dri2_dpy->driver_configs[i]; i++) {
      assigned = false;

      for (unsigned j = 0; j < ARRAY_SIZE(dri2_wl_visuals); j++) {
         struct dri2_egl_config *dri2_conf;

         if (!BITSET_TEST(dri2_dpy->formats, j))
            continue;

         dri2_conf = dri2_add_config(disp, dri2_dpy->driver_configs[i],
               count + 1, EGL_WINDOW_BIT, NULL, dri2_wl_visuals[j].rgba_shifts, dri2_wl_visuals[j].rgba_sizes);
         if (dri2_conf) {
            if (dri2_conf->base.ConfigID == count + 1)
               count++;
            format_count[j]++;
            assigned = true;
         }
      }

      if (!assigned && dri2_dpy->is_different_gpu) {
         struct dri2_egl_config *dri2_conf;
         int alt_dri_image_format, c, s;

         /* No match for config. Try if we can blitImage convert to a visual */
         c = dri2_wl_visual_idx_from_config(dri2_dpy,
                                            dri2_dpy->driver_configs[i],
                                            false);

         if (c == -1)
            continue;

         /* Find optimal target visual for blitImage conversion, if any. */
         alt_dri_image_format = dri2_wl_visuals[c].alt_dri_image_format;
         s = dri2_wl_visual_idx_from_dri_image_format(alt_dri_image_format);

         if (s == -1 || !BITSET_TEST(dri2_dpy->formats, s))
            continue;

         /* Visual s works for the Wayland server, and c can be converted into s
          * by our client gpu during PRIME blitImage conversion to a linear
          * wl_buffer, so add visual c as supported by the client renderer.
          */
         dri2_conf = dri2_add_config(disp, dri2_dpy->driver_configs[i],
                                     count + 1, EGL_WINDOW_BIT, NULL,
                                     dri2_wl_visuals[c].rgba_shifts,
                                     dri2_wl_visuals[c].rgba_sizes);
         if (dri2_conf) {
            if (dri2_conf->base.ConfigID == count + 1)
               count++;
            format_count[c]++;
            if (format_count[c] == 1)
               _eglLog(_EGL_DEBUG, "Client format %s to server format %s via "
                       "PRIME blitImage.", dri2_wl_visuals[c].format_name,
                       dri2_wl_visuals[s].format_name);
         }
      }
   }

   for (unsigned i = 0; i < ARRAY_SIZE(format_count); i++) {
      if (!format_count[i]) {
         _eglLog(_EGL_DEBUG, "No DRI config supports native format %s",
                 dri2_wl_visuals[i].format_name);
      }
   }

   return (count != 0);
}

static EGLBoolean
dri2_initialize_wayland_drm(_EGLDisplay *disp)
{
   _EGLDevice *dev;
   struct dri2_egl_display *dri2_dpy;

   dri2_dpy = calloc(1, sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   dri2_dpy->fd = -1;
   disp->DriverData = (void *) dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->wl_dpy = wl_display_connect(NULL);
      if (dri2_dpy->wl_dpy == NULL)
         goto cleanup;
      dri2_dpy->own_device = true;
   } else {
      dri2_dpy->wl_dpy = disp->PlatformDisplay;
   }

   dri2_dpy->wl_modifiers =
      calloc(ARRAY_SIZE(dri2_wl_visuals), sizeof(*dri2_dpy->wl_modifiers));
   if (!dri2_dpy->wl_modifiers)
      goto cleanup;
   for (int i = 0; i < ARRAY_SIZE(dri2_wl_visuals); i++) {
      if (!u_vector_init_pow2(&dri2_dpy->wl_modifiers[i], 4, sizeof(uint64_t)))
         goto cleanup;
   }

   dri2_dpy->wl_queue = wl_display_create_queue(dri2_dpy->wl_dpy);

   dri2_dpy->wl_dpy_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dpy);
   if (dri2_dpy->wl_dpy_wrapper == NULL)
      goto cleanup;

   wl_proxy_set_queue((struct wl_proxy *) dri2_dpy->wl_dpy_wrapper,
                      dri2_dpy->wl_queue);

   if (dri2_dpy->own_device)
      wl_display_dispatch_pending(dri2_dpy->wl_dpy);

   dri2_dpy->wl_registry = wl_display_get_registry(dri2_dpy->wl_dpy_wrapper);
   wl_registry_add_listener(dri2_dpy->wl_registry,
                            &registry_listener_drm, dri2_dpy);
   if (roundtrip(dri2_dpy) < 0 || dri2_dpy->wl_drm == NULL)
      goto cleanup;

   if (roundtrip(dri2_dpy) < 0 || dri2_dpy->fd == -1)
      goto cleanup;

   if (!dri2_dpy->authenticated &&
       (roundtrip(dri2_dpy) < 0 || !dri2_dpy->authenticated))
      goto cleanup;

   dri2_dpy->fd = loader_get_user_preferred_fd(dri2_dpy->fd,
                                               &dri2_dpy->is_different_gpu);
   dev = _eglAddDevice(dri2_dpy->fd, false);
   if (!dev) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to find EGLDevice");
      goto cleanup;
   }

   disp->Device = dev;

   if (dri2_dpy->is_different_gpu) {
      free(dri2_dpy->device_name);
      dri2_dpy->device_name = loader_get_device_name_for_fd(dri2_dpy->fd);
      if (!dri2_dpy->device_name) {
         _eglError(EGL_BAD_ALLOC, "wayland-egl: failed to get device name "
                                  "for requested GPU");
         goto cleanup;
      }
   }

   /* we have to do the check now, because loader_get_user_preferred_fd
    * will return a render-node when the requested gpu is different
    * to the server, but also if the client asks for the same gpu than
    * the server by requesting its pci-id */
   dri2_dpy->is_render_node = drmGetNodeTypeFromFd(dri2_dpy->fd) == DRM_NODE_RENDER;

   dri2_dpy->driver_name = loader_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");
      goto cleanup;
   }

   /* render nodes cannot use Gem names, and thus do not support
    * the __DRI_DRI2_LOADER extension */
   if (!dri2_dpy->is_render_node) {
      dri2_dpy->loader_extensions = dri2_loader_extensions;
      if (!dri2_load_driver(disp)) {
         _eglError(EGL_BAD_ALLOC, "DRI2: failed to load driver");
         goto cleanup;
      }
   } else {
      dri2_dpy->loader_extensions = image_loader_extensions;
      if (!dri2_load_driver_dri3(disp)) {
         _eglError(EGL_BAD_ALLOC, "DRI3: failed to load driver");
         goto cleanup;
      }
   }

   if (!dri2_create_screen(disp))
      goto cleanup;

   if (!dri2_setup_extensions(disp))
      goto cleanup;

   dri2_setup_screen(disp);

   dri2_wl_setup_swap_interval(disp);

   /* To use Prime, we must have _DRI_IMAGE v7 at least.
    * createImageFromFds support indicates that Prime export/import
    * is supported by the driver. Fall back to
    * gem names if we don't have Prime support. */

   if (dri2_dpy->image->base.version < 7 ||
       dri2_dpy->image->createImageFromFds == NULL)
      dri2_dpy->capabilities &= ~WL_DRM_CAPABILITY_PRIME;

   /* We cannot use Gem names with render-nodes, only prime fds (dma-buf).
    * The server needs to accept them */
   if (dri2_dpy->is_render_node &&
       !(dri2_dpy->capabilities & WL_DRM_CAPABILITY_PRIME)) {
      _eglLog(_EGL_WARNING, "wayland-egl: display is not render-node capable");
      goto cleanup;
   }

   if (dri2_dpy->is_different_gpu &&
       (dri2_dpy->image->base.version < 9 ||
        dri2_dpy->image->blitImage == NULL)) {
      _eglLog(_EGL_WARNING, "wayland-egl: Different GPU selected, but the "
                            "Image extension in the driver is not "
                            "compatible. Version 9 or later and blitImage() "
                            "are required");
      goto cleanup;
   }

   if (!dri2_wl_add_configs_for_visuals(disp)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to add configs");
      goto cleanup;
   }

   dri2_set_WL_bind_wayland_display(disp);
   /* When cannot convert EGLImage to wl_buffer when on a different gpu,
    * because the buffer of the EGLImage has likely a tiling mode the server
    * gpu won't support. These is no way to check for now. Thus do not support the
    * extension */
   if (!dri2_dpy->is_different_gpu)
      disp->Extensions.WL_create_wayland_buffer_from_image = EGL_TRUE;

   disp->Extensions.EXT_buffer_age = EGL_TRUE;

   disp->Extensions.EXT_swap_buffers_with_damage = EGL_TRUE;

   disp->Extensions.EXT_present_opaque = EGL_TRUE;

   /* Fill vtbl last to prevent accidentally calling virtual function during
    * initialization.
    */
   dri2_dpy->vtbl = &dri2_wl_display_vtbl;

   return EGL_TRUE;

 cleanup:
   dri2_display_destroy(disp);
   return EGL_FALSE;
}

static int
dri2_wl_swrast_get_stride_for_format(int format, int w)
{
   int visual_idx = dri2_wl_visual_idx_from_shm_format(format);

   assume(visual_idx != -1);

   return w * (dri2_wl_visuals[visual_idx].bpp / 8);
}

static EGLBoolean
dri2_wl_swrast_allocate_buffer(struct dri2_egl_surface *dri2_surf,
                               int format, int w, int h,
                               void **data, int *size,
                               struct wl_buffer **buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct wl_shm_pool *pool;
   int fd, stride, size_map;
   void *data_map;

   stride = dri2_wl_swrast_get_stride_for_format(format, w);
   size_map = h * stride;

   /* Create a shareable buffer */
   fd = os_create_anonymous_file(size_map, NULL);
   if (fd < 0)
      return EGL_FALSE;

   data_map = mmap(NULL, size_map, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (data_map == MAP_FAILED) {
      close(fd);
      return EGL_FALSE;
   }

   /* Share it in a wl_buffer */
   pool = wl_shm_create_pool(dri2_dpy->wl_shm, fd, size_map);
   wl_proxy_set_queue((struct wl_proxy *)pool, dri2_surf->wl_queue);
   *buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride, format);
   wl_shm_pool_destroy(pool);
   close(fd);

   *data = data_map;
   *size = size_map;
   return EGL_TRUE;
}

static int
swrast_update_buffers(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);

   /* we need to do the following operations only once per frame */
   if (dri2_surf->back)
      return 0;

   if (dri2_surf->wl_win &&
       (dri2_surf->base.Width != dri2_surf->wl_win->width ||
        dri2_surf->base.Height != dri2_surf->wl_win->height)) {

      dri2_wl_release_buffers(dri2_surf);

      dri2_surf->base.Width  = dri2_surf->wl_win->width;
      dri2_surf->base.Height = dri2_surf->wl_win->height;
      dri2_surf->dx = dri2_surf->wl_win->dx;
      dri2_surf->dy = dri2_surf->wl_win->dy;
      dri2_surf->current = NULL;
   }

   /* find back buffer */

   /* There might be a buffer release already queued that wasn't processed */
   wl_display_dispatch_queue_pending(dri2_dpy->wl_dpy, dri2_surf->wl_queue);

   /* try get free buffer already created */
   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked &&
          dri2_surf->color_buffers[i].wl_buffer) {
          dri2_surf->back = &dri2_surf->color_buffers[i];
          break;
      }
   }

   /* else choose any another free location */
   if (!dri2_surf->back) {
      for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
         if (!dri2_surf->color_buffers[i].locked) {
             dri2_surf->back = &dri2_surf->color_buffers[i];
             if (!dri2_wl_swrast_allocate_buffer(dri2_surf,
                                                 dri2_surf->format,
                                                 dri2_surf->base.Width,
                                                 dri2_surf->base.Height,
                                                 &dri2_surf->back->data,
                                                 &dri2_surf->back->data_size,
                                                 &dri2_surf->back->wl_buffer)) {
                _eglError(EGL_BAD_ALLOC, "failed to allocate color buffer");
                 return -1;
             }
             wl_buffer_add_listener(dri2_surf->back->wl_buffer,
                                    &wl_buffer_listener, dri2_surf);
             break;
         }
      }
   }

   if (!dri2_surf->back) {
      _eglError(EGL_BAD_ALLOC, "failed to find free buffer");
      return -1;
   }

   dri2_surf->back->locked = true;

   /* If we have an extra unlocked buffer at this point, we had to do triple
    * buffering for a while, but now can go back to just double buffering.
    * That means we can free any unlocked buffer now. */
   for (int i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (!dri2_surf->color_buffers[i].locked &&
          dri2_surf->color_buffers[i].wl_buffer) {
         wl_buffer_destroy(dri2_surf->color_buffers[i].wl_buffer);
         munmap(dri2_surf->color_buffers[i].data,
                dri2_surf->color_buffers[i].data_size);
         dri2_surf->color_buffers[i].wl_buffer = NULL;
         dri2_surf->color_buffers[i].data = NULL;
      }
   }

   return 0;
}

static void*
dri2_wl_swrast_get_frontbuffer_data(struct dri2_egl_surface *dri2_surf)
{
   /* if there has been a resize: */
   if (!dri2_surf->current)
      return NULL;

   return dri2_surf->current->data;
}

static void*
dri2_wl_swrast_get_backbuffer_data(struct dri2_egl_surface *dri2_surf)
{
   assert(dri2_surf->back);
   return dri2_surf->back->data;
}

static void
dri2_wl_swrast_commit_backbuffer(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(dri2_surf->base.Resource.Display);

   while (dri2_surf->throttle_callback != NULL)
      if (wl_display_dispatch_queue(dri2_dpy->wl_dpy,
                                    dri2_surf->wl_queue) == -1)
         return;

   if (dri2_surf->base.SwapInterval > 0) {
      dri2_surf->throttle_callback =
         wl_surface_frame(dri2_surf->wl_surface_wrapper);
      wl_callback_add_listener(dri2_surf->throttle_callback,
                               &throttle_listener, dri2_surf);
   }

   dri2_surf->current = dri2_surf->back;
   dri2_surf->back = NULL;

   wl_surface_attach(dri2_surf->wl_surface_wrapper,
                     dri2_surf->current->wl_buffer,
                     dri2_surf->dx, dri2_surf->dy);

   dri2_surf->wl_win->attached_width  = dri2_surf->base.Width;
   dri2_surf->wl_win->attached_height = dri2_surf->base.Height;
   /* reset resize growing parameters */
   dri2_surf->dx = 0;
   dri2_surf->dy = 0;

   wl_surface_damage(dri2_surf->wl_surface_wrapper,
                     0, 0, INT32_MAX, INT32_MAX);
   wl_surface_commit(dri2_surf->wl_surface_wrapper);

   /* If we're not waiting for a frame callback then we'll at least throttle
    * to a sync callback so that we always give a chance for the compositor to
    * handle the commit and send a release event before checking for a free
    * buffer */
   if (dri2_surf->throttle_callback == NULL) {
      dri2_surf->throttle_callback = wl_display_sync(dri2_surf->wl_dpy_wrapper);
      wl_callback_add_listener(dri2_surf->throttle_callback,
                               &throttle_listener, dri2_surf);
   }

   wl_display_flush(dri2_dpy->wl_dpy);
}

static void
dri2_wl_swrast_get_drawable_info(__DRIdrawable * draw,
                                 int *x, int *y, int *w, int *h,
                                 void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;

   (void) swrast_update_buffers(dri2_surf);
   *x = 0;
   *y = 0;
   *w = dri2_surf->base.Width;
   *h = dri2_surf->base.Height;
}

static void
dri2_wl_swrast_get_image(__DRIdrawable * read,
                         int x, int y, int w, int h,
                         char *data, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int copy_width = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, w);
   int x_offset = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, x);
   int src_stride = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, dri2_surf->base.Width);
   int dst_stride = copy_width;
   char *src, *dst;

   src = dri2_wl_swrast_get_frontbuffer_data(dri2_surf);
   if (!src) {
      memset(data, 0, copy_width * h);
      return;
   }

   assert(data != src);
   assert(copy_width <= src_stride);

   src += x_offset;
   src += y * src_stride;
   dst = data;

   if (copy_width > src_stride-x_offset)
      copy_width = src_stride-x_offset;
   if (h > dri2_surf->base.Height-y)
      h = dri2_surf->base.Height-y;

   for (; h>0; h--) {
      memcpy(dst, src, copy_width);
      src += src_stride;
      dst += dst_stride;
   }
}

static void
dri2_wl_swrast_put_image2(__DRIdrawable * draw, int op,
                         int x, int y, int w, int h, int stride,
                         char *data, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int copy_width = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, w);
   int dst_stride = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, dri2_surf->base.Width);
   int x_offset = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, x);
   char *src, *dst;

   assert(copy_width <= stride);

   (void) swrast_update_buffers(dri2_surf);
   dst = dri2_wl_swrast_get_backbuffer_data(dri2_surf);

   /* partial copy, copy old content */
   if (copy_width < dst_stride)
      dri2_wl_swrast_get_image(draw, 0, 0,
                               dri2_surf->base.Width, dri2_surf->base.Height,
                               dst, loaderPrivate);

   dst += x_offset;
   dst += y * dst_stride;

   src = data;

   /* drivers expect we do these checks (and some rely on it) */
   if (copy_width > dst_stride-x_offset)
      copy_width = dst_stride-x_offset;
   if (h > dri2_surf->base.Height-y)
      h = dri2_surf->base.Height-y;

   for (; h>0; h--) {
      memcpy(dst, src, copy_width);
      src += stride;
      dst += dst_stride;
   }
   dri2_wl_swrast_commit_backbuffer(dri2_surf);
}

static void
dri2_wl_swrast_put_image(__DRIdrawable * draw, int op,
                         int x, int y, int w, int h,
                         char *data, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int stride;

   stride = dri2_wl_swrast_get_stride_for_format(dri2_surf->format, w);
   dri2_wl_swrast_put_image2(draw, op, x, y, w, h,
                             stride, data, loaderPrivate);
}

static EGLBoolean
dri2_wl_swrast_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);

   if (!dri2_surf->wl_win)
      return _eglError(EGL_BAD_NATIVE_WINDOW, "dri2_swap_buffers");

   dri2_dpy->core->swapBuffers(dri2_surf->dri_drawable);
   return EGL_TRUE;
}

static void
shm_handle_format(void *data, struct wl_shm *shm, uint32_t format)
{
   struct dri2_egl_display *dri2_dpy = data;
   int visual_idx = dri2_wl_visual_idx_from_shm_format(format);

   if (visual_idx == -1)
      return;

   BITSET_SET(dri2_dpy->formats, visual_idx);
}

static const struct wl_shm_listener shm_listener = {
   .format = shm_handle_format
};

static void
registry_handle_global_swrast(void *data, struct wl_registry *registry,
                              uint32_t name, const char *interface,
                              uint32_t version)
{
   struct dri2_egl_display *dri2_dpy = data;

   if (strcmp(interface, "wl_shm") == 0) {
      dri2_dpy->wl_shm =
         wl_registry_bind(registry, name, &wl_shm_interface, 1);
      wl_shm_add_listener(dri2_dpy->wl_shm, &shm_listener, dri2_dpy);
   }
}

static const struct wl_registry_listener registry_listener_swrast = {
   .global = registry_handle_global_swrast,
   .global_remove = registry_handle_global_remove
};

static const struct dri2_egl_display_vtbl dri2_wl_swrast_display_vtbl = {
   .authenticate = NULL,
   .create_window_surface = dri2_wl_create_window_surface,
   .create_pixmap_surface = dri2_wl_create_pixmap_surface,
   .destroy_surface = dri2_wl_destroy_surface,
   .create_image = dri2_create_image_khr,
   .swap_buffers = dri2_wl_swrast_swap_buffers,
   .get_dri_drawable = dri2_surface_get_dri_drawable,
};

static const __DRIswrastLoaderExtension swrast_loader_extension = {
   .base = { __DRI_SWRAST_LOADER, 2 },

   .getDrawableInfo = dri2_wl_swrast_get_drawable_info,
   .putImage        = dri2_wl_swrast_put_image,
   .getImage        = dri2_wl_swrast_get_image,
   .putImage2       = dri2_wl_swrast_put_image2,
};

static const __DRIextension *swrast_loader_extensions[] = {
   &swrast_loader_extension.base,
   &image_lookup_extension.base,
   NULL,
};

static EGLBoolean
dri2_initialize_wayland_swrast(_EGLDisplay *disp)
{
   _EGLDevice *dev;
   struct dri2_egl_display *dri2_dpy;

   dri2_dpy = calloc(1, sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   dri2_dpy->fd = -1;
   disp->DriverData = (void *) dri2_dpy;
   if (disp->PlatformDisplay == NULL) {
      dri2_dpy->wl_dpy = wl_display_connect(NULL);
      if (dri2_dpy->wl_dpy == NULL)
         goto cleanup;
      dri2_dpy->own_device = true;
   } else {
      dri2_dpy->wl_dpy = disp->PlatformDisplay;
   }

   dev = _eglAddDevice(dri2_dpy->fd, true);
   if (!dev) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to find EGLDevice");
      goto cleanup;
   }

   disp->Device = dev;

   dri2_dpy->wl_queue = wl_display_create_queue(dri2_dpy->wl_dpy);

   dri2_dpy->wl_dpy_wrapper = wl_proxy_create_wrapper(dri2_dpy->wl_dpy);
   if (dri2_dpy->wl_dpy_wrapper == NULL)
      goto cleanup;

   wl_proxy_set_queue((struct wl_proxy *) dri2_dpy->wl_dpy_wrapper,
                      dri2_dpy->wl_queue);

   if (dri2_dpy->own_device)
      wl_display_dispatch_pending(dri2_dpy->wl_dpy);

   dri2_dpy->wl_registry = wl_display_get_registry(dri2_dpy->wl_dpy_wrapper);
   wl_registry_add_listener(dri2_dpy->wl_registry,
                            &registry_listener_swrast, dri2_dpy);

   if (roundtrip(dri2_dpy) < 0 || dri2_dpy->wl_shm == NULL)
      goto cleanup;

   if (roundtrip(dri2_dpy) < 0 || !BITSET_TEST_RANGE(dri2_dpy->formats,
                                                     0, EGL_DRI2_MAX_FORMATS))
      goto cleanup;

   dri2_dpy->driver_name = strdup("swrast");
   if (!dri2_load_driver_swrast(disp))
      goto cleanup;

   dri2_dpy->loader_extensions = swrast_loader_extensions;

   if (!dri2_create_screen(disp))
      goto cleanup;

   if (!dri2_setup_extensions(disp))
      goto cleanup;

   dri2_setup_screen(disp);

   dri2_wl_setup_swap_interval(disp);

   if (!dri2_wl_add_configs_for_visuals(disp)) {
      _eglError(EGL_NOT_INITIALIZED, "DRI2: failed to add configs");
      goto cleanup;
   }

   /* Fill vtbl last to prevent accidentally calling virtual function during
    * initialization.
    */
   dri2_dpy->vtbl = &dri2_wl_swrast_display_vtbl;

   return EGL_TRUE;

 cleanup:
   dri2_display_destroy(disp);
   return EGL_FALSE;
}

EGLBoolean
dri2_initialize_wayland(_EGLDisplay *disp)
{
   if (disp->Options.ForceSoftware)
      return dri2_initialize_wayland_swrast(disp);
   else
      return dri2_initialize_wayland_drm(disp);
}

void
dri2_teardown_wayland(struct dri2_egl_display *dri2_dpy)
{
   if (dri2_dpy->wl_drm)
      wl_drm_destroy(dri2_dpy->wl_drm);
   if (dri2_dpy->wl_dmabuf)
      zwp_linux_dmabuf_v1_destroy(dri2_dpy->wl_dmabuf);
   if (dri2_dpy->wl_shm)
      wl_shm_destroy(dri2_dpy->wl_shm);
   if (dri2_dpy->wl_registry)
      wl_registry_destroy(dri2_dpy->wl_registry);
   if (dri2_dpy->wl_queue)
      wl_event_queue_destroy(dri2_dpy->wl_queue);
   if (dri2_dpy->wl_dpy_wrapper)
      wl_proxy_wrapper_destroy(dri2_dpy->wl_dpy_wrapper);

   for (int i = 0; dri2_dpy->wl_modifiers && i < ARRAY_SIZE(dri2_wl_visuals); i++)
      u_vector_finish(&dri2_dpy->wl_modifiers[i]);
   free(dri2_dpy->wl_modifiers);

   if (dri2_dpy->own_device)
      wl_display_disconnect(dri2_dpy->wl_dpy);
}
