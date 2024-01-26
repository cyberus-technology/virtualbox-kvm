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
 */

#ifndef EGL_DRI2_INCLUDED
#define EGL_DRI2_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#ifdef HAVE_X11_PLATFORM
#include <xcb/xcb.h>
#include <xcb/dri2.h>
#include <xcb/xfixes.h>
#include <X11/Xlib-xcb.h>

#ifdef HAVE_DRI3
#include "loader_dri3_helper.h"
#endif
#endif

#ifdef HAVE_WAYLAND_PLATFORM
/* forward declarations to avoid pulling wayland headers everywhere */
struct wl_egl_window;
struct wl_event_queue;
struct wl_callback;
struct wl_display;
struct wl_drm;
struct wl_registry;
struct wl_shm;
struct wl_surface;
struct zwp_linux_dmabuf_v1;
#endif

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#ifdef HAVE_DRM_PLATFORM
#include <gbm_driint.h>
#endif

#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "EGL-DRI2"

#include <hardware/gralloc.h>

#if ANDROID_API_LEVEL >= 26
#include <vndk/window.h>
#else
#include <system/window.h>
#endif

#endif /* HAVE_ANDROID_PLATFORM */

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldevice.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"
#include "eglimage.h"
#include "eglsync.h"

#include "util/u_vector.h"
#include "util/bitset.h"

#define EGL_DRI2_MAX_FORMATS 11

struct wl_buffer;

struct dri2_egl_display_vtbl {
   /* mandatory on Wayland, unused otherwise */
   int (*authenticate)(_EGLDisplay *disp, uint32_t id);

   /* mandatory */
   _EGLSurface* (*create_window_surface)(_EGLDisplay *disp, _EGLConfig *config,
                                         void *native_window,
                                         const EGLint *attrib_list);

   /* optional */
   _EGLSurface* (*create_pixmap_surface)(_EGLDisplay *disp, _EGLConfig *config,
                                         void *native_pixmap,
                                         const EGLint *attrib_list);

   /* optional */
   _EGLSurface* (*create_pbuffer_surface)(_EGLDisplay *disp, _EGLConfig *config,
                                          const EGLint *attrib_list);

   /* mandatory */
   EGLBoolean (*destroy_surface)(_EGLDisplay *disp, _EGLSurface *surface);

   /* optional */
   EGLBoolean (*swap_interval)(_EGLDisplay *disp, _EGLSurface *surf,
                               EGLint interval);

   /* mandatory */
   _EGLImage* (*create_image)(_EGLDisplay *disp, _EGLContext *ctx,
                              EGLenum target, EGLClientBuffer buffer,
                              const EGLint *attr_list);

   /* mandatory */
   EGLBoolean (*swap_buffers)(_EGLDisplay *disp, _EGLSurface *surf);

   /* optional - falls back to .swap_buffers */
   EGLBoolean (*swap_buffers_with_damage)(_EGLDisplay *disp, _EGLSurface *surface,
                                          const EGLint *rects, EGLint n_rects);

   /* optional */
   EGLBoolean (*swap_buffers_region)(_EGLDisplay *disp, _EGLSurface *surf,
                                     EGLint numRects, const EGLint *rects);

   /* optional */
   EGLBoolean (*post_sub_buffer)(_EGLDisplay *disp, _EGLSurface *surf,
                                 EGLint x, EGLint y,
                                 EGLint width, EGLint height);

   /* optional */
   EGLBoolean (*copy_buffers)(_EGLDisplay *disp, _EGLSurface *surf,
                              void *native_pixmap_target);

   /* optional */
   EGLint (*query_buffer_age)(_EGLDisplay *disp, _EGLSurface *surf);

   /* optional */
   EGLBoolean (*query_surface)(_EGLDisplay *disp, _EGLSurface *surf,
                               EGLint attribute, EGLint *value);

   /* optional */
   struct wl_buffer* (*create_wayland_buffer_from_image)(_EGLDisplay *disp,
                                                         _EGLImage *img);

   /* optional */
   EGLBoolean (*get_sync_values)(_EGLDisplay *display, _EGLSurface *surface,
                                 EGLuint64KHR *ust, EGLuint64KHR *msc,
                                 EGLuint64KHR *sbc);

   /* mandatory */
   __DRIdrawable *(*get_dri_drawable)(_EGLSurface *surf);

   /* optional */
   void (*close_screen_notify)(_EGLDisplay *disp);

   /* Used in EGL_KHR_mutable_render_buffer to update the native window's
    * shared buffer mode.
    * optional
    */
   bool (*set_shared_buffer_mode)(_EGLDisplay *disp, _EGLSurface *surf,
                                  bool mode);
};

struct dri2_egl_display
{
   const struct dri2_egl_display_vtbl *vtbl;

   int                       dri2_major;
   int                       dri2_minor;
   __DRIscreen              *dri_screen;
   bool                      own_dri_screen;
   const __DRIconfig       **driver_configs;
   void                     *driver;
   const __DRIcoreExtension       *core;
   const __DRIimageDriverExtension *image_driver;
   const __DRIdri2Extension       *dri2;
   const __DRIswrastExtension     *swrast;
   const __DRI2flushExtension     *flush;
   const __DRI2flushControlExtension *flush_control;
   const __DRItexBufferExtension  *tex_buffer;
   const __DRIimageExtension      *image;
   const __DRIrobustnessExtension *robustness;
   const __DRInoErrorExtension    *no_error;
   const __DRI2configQueryExtension *config;
   const __DRI2fenceExtension *fence;
   const __DRI2bufferDamageExtension *buffer_damage;
   const __DRI2blobExtension *blob;
   const __DRI2rendererQueryExtension *rendererQuery;
   const __DRI2interopExtension *interop;
   const __DRIconfigOptionsExtension *configOptions;
   const __DRImutableRenderBufferDriverExtension *mutable_render_buffer;
   int                       fd;

   /* dri2_initialize/dri2_terminate increment/decrement this count, so does
    * dri2_make_current (tracks if there are active contexts/surfaces). */
   int                       ref_count;

   bool                      own_device;
   bool                      invalidate_available;
   int                       min_swap_interval;
   int                       max_swap_interval;
   int                       default_swap_interval;
#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_device    *gbm_dri;
#endif

   char                     *driver_name;

   const __DRIextension    **loader_extensions;
   const __DRIextension    **driver_extensions;

#ifdef HAVE_X11_PLATFORM
   xcb_connection_t         *conn;
   xcb_screen_t             *screen;
   bool                     swap_available;
#ifdef HAVE_DRI3
   bool                     multibuffers_available;
   int                      dri3_major_version;
   int                      dri3_minor_version;
   int                      present_major_version;
   int                      present_minor_version;
   struct loader_dri3_extensions loader_dri3_ext;
#endif
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_display        *wl_dpy;
   struct wl_display        *wl_dpy_wrapper;
   struct wl_registry       *wl_registry;
   struct wl_drm            *wl_server_drm;
   struct wl_drm            *wl_drm;
   struct wl_shm            *wl_shm;
   struct wl_event_queue    *wl_queue;
   struct zwp_linux_dmabuf_v1 *wl_dmabuf;
   struct u_vector          *wl_modifiers;
   bool                      authenticated;
   BITSET_DECLARE(formats, EGL_DRI2_MAX_FORMATS);
   uint32_t                  capabilities;
   char                     *device_name;
#endif

#ifdef HAVE_ANDROID_PLATFORM
   const gralloc_module_t *gralloc;
   /* gralloc vendor usage bit for front rendering */
   uint32_t front_rendering_usage;
#endif

   bool                      is_render_node;
   bool                      is_different_gpu;
};

struct dri2_egl_context
{
   _EGLContext   base;
   __DRIcontext *dri_context;
};

#ifdef HAVE_WAYLAND_PLATFORM
enum wayland_buffer_type {
   WL_BUFFER_FRONT,
   WL_BUFFER_BACK,
   WL_BUFFER_THIRD,
   WL_BUFFER_COUNT
};
#endif

struct dri2_egl_surface
{
   _EGLSurface          base;
   __DRIdrawable       *dri_drawable;
   __DRIbuffer          buffers[5];
   bool                 have_fake_front;

#ifdef HAVE_X11_PLATFORM
   xcb_drawable_t       drawable;
   xcb_xfixes_region_t  region;
   int                  depth;
   int                  bytes_per_pixel;
   xcb_gcontext_t       gc;
   xcb_gcontext_t       swapgc;
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_egl_window  *wl_win;
   int                    dx;
   int                    dy;
   struct wl_event_queue *wl_queue;
   struct wl_surface     *wl_surface_wrapper;
   struct wl_display     *wl_dpy_wrapper;
   struct wl_drm         *wl_drm_wrapper;
   struct wl_callback    *throttle_callback;
   int                    format;
   bool                   resized;
#endif

#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_surface *gbm_surf;
#endif

   /* EGL-owned buffers */
   __DRIbuffer           *local_buffers[__DRI_BUFFER_COUNT];

#if defined(HAVE_WAYLAND_PLATFORM) || defined(HAVE_DRM_PLATFORM)
   struct {
#ifdef HAVE_WAYLAND_PLATFORM
      struct wl_buffer   *wl_buffer;
      bool                wl_release;
      __DRIimage         *dri_image;
      /* for is_different_gpu case. NULL else */
      __DRIimage         *linear_copy;
      /* for swrast */
      void *data;
      int data_size;
#endif
#ifdef HAVE_DRM_PLATFORM
      struct gbm_bo       *bo;
#endif
      bool                locked;
      int                 age;
   } color_buffers[4], *back, *current;
#endif

#ifdef HAVE_ANDROID_PLATFORM
   struct ANativeWindow *window;
   struct ANativeWindowBuffer *buffer;
   __DRIimage *dri_image_back;
   __DRIimage *dri_image_front;

   /* Used to record all the buffers created by ANativeWindow and their ages.
    * Allocate number of color_buffers based on query to android bufferqueue
    * and save color_buffers_count.
    */
   int color_buffers_count;
   struct {
      struct ANativeWindowBuffer *buffer;
      int age;
   } *color_buffers, *back;
   uint32_t gralloc_usage;
#endif

   /* surfaceless and device */
   __DRIimage           *front;
   unsigned int         visual;

   int out_fence_fd;
   EGLBoolean enable_out_fence;

   /* swrast device */
   char *swrast_device_buffer;
};

struct dri2_egl_config
{
   _EGLConfig         base;
   const __DRIconfig *dri_config[2][2];
};

struct dri2_egl_image
{
   _EGLImage   base;
   __DRIimage *dri_image;
};

struct dri2_egl_sync {
   _EGLSync base;
   mtx_t mutex;
   cnd_t cond;
   int refcount;
   void *fence;
};

/* From driconf.h, user exposed so should be stable */
#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(dri2_egl)
_EGL_DRIVER_TYPECAST(dri2_egl_image, _EGLImage, obj)
_EGL_DRIVER_TYPECAST(dri2_egl_sync, _EGLSync, obj)

extern const __DRIimageLookupExtension image_lookup_extension;
extern const __DRIuseInvalidateExtension use_invalidate;
extern const __DRIbackgroundCallableExtension background_callable_extension;
extern const __DRIswrastLoaderExtension swrast_pbuffer_loader_extension;

EGLBoolean
dri2_load_driver(_EGLDisplay *disp);

/* Helper for platforms not using dri2_create_screen */
void
dri2_setup_screen(_EGLDisplay *disp);

void
dri2_setup_swap_interval(_EGLDisplay *disp, int max_swap_interval);

EGLBoolean
dri2_load_driver_swrast(_EGLDisplay *disp);

EGLBoolean
dri2_load_driver_dri3(_EGLDisplay *disp);

EGLBoolean
dri2_create_screen(_EGLDisplay *disp);

EGLBoolean
dri2_setup_extensions(_EGLDisplay *disp);

__DRIdrawable *
dri2_surface_get_dri_drawable(_EGLSurface *surf);

GLboolean
dri2_validate_egl_image(void *image, void *data);

__DRIimage *
dri2_lookup_egl_image_validated(void *image, void *data);

__DRIimage *
dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data);

void
dri2_get_shifts_and_sizes(const __DRIcoreExtension *core,
		          const __DRIconfig *config, int *shifts,
			  unsigned int *sizes);

void
dri2_get_render_type_float(const __DRIcoreExtension *core,
                           const __DRIconfig *config,
                           bool *is_float);

unsigned int
dri2_image_format_for_pbuffer_config(struct dri2_egl_display *dri2_dpy,
                                     const __DRIconfig *config);

struct dri2_egl_config *
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
                EGLint surface_type, const EGLint *attr_list,
                const int *rgba_shifts, const unsigned int *rgba_sizes);

EGLBoolean
dri2_add_pbuffer_configs_for_visuals(_EGLDisplay *disp);

_EGLImage *
dri2_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
                      EGLClientBuffer buffer, const EGLint *attr_list);

_EGLImage *
dri2_create_image_dma_buf(_EGLDisplay *disp, _EGLContext *ctx,
                          EGLClientBuffer buffer, const EGLint *attr_list);

_EGLImage *
dri2_create_image_from_dri(_EGLDisplay *disp, __DRIimage *dri_image);

#ifdef HAVE_X11_PLATFORM
EGLBoolean
dri2_initialize_x11(_EGLDisplay *disp);
void
dri2_teardown_x11(struct dri2_egl_display *dri2_dpy);
unsigned int
dri2_x11_get_red_mask_for_depth(struct dri2_egl_display *dri2_dpy, int depth);
#else
static inline EGLBoolean
dri2_initialize_x11(_EGLDisplay *disp)
{
   return _eglError(EGL_NOT_INITIALIZED, "X11 platform not built");
}
static inline void
dri2_teardown_x11(struct dri2_egl_display *dri2_dpy) {}
static inline unsigned int
dri2_x11_get_red_mask_for_depth(struct dri2_egl_display *dri2_dpy, int depth)
{
   return 0;
}
#endif

#ifdef HAVE_DRM_PLATFORM
EGLBoolean
dri2_initialize_drm(_EGLDisplay *disp);
void
dri2_teardown_drm(struct dri2_egl_display *dri2_dpy);
#else
static inline EGLBoolean
dri2_initialize_drm(_EGLDisplay *disp)
{
   return _eglError(EGL_NOT_INITIALIZED, "GBM/DRM platform not built");
}
static inline void
dri2_teardown_drm(struct dri2_egl_display *dri2_dpy) {}
#endif

#ifdef HAVE_WAYLAND_PLATFORM
EGLBoolean
dri2_initialize_wayland(_EGLDisplay *disp);
void
dri2_teardown_wayland(struct dri2_egl_display *dri2_dpy);
bool
dri2_wl_is_format_supported(void* user_data, uint32_t format);
#else
static inline EGLBoolean
dri2_initialize_wayland(_EGLDisplay *disp)
{
   return _eglError(EGL_NOT_INITIALIZED, "Wayland platform not built");
}
static inline void
dri2_teardown_wayland(struct dri2_egl_display *dri2_dpy) {}
#endif

#ifdef HAVE_ANDROID_PLATFORM
EGLBoolean
dri2_initialize_android(_EGLDisplay *disp);
#else
static inline EGLBoolean
dri2_initialize_android(_EGLDisplay *disp)
{
   return _eglError(EGL_NOT_INITIALIZED, "Android platform not built");
}
#endif

EGLBoolean
dri2_initialize_surfaceless(_EGLDisplay *disp);

EGLBoolean
dri2_initialize_device(_EGLDisplay *disp);
static inline void
dri2_teardown_device(struct dri2_egl_display *dri2_dpy) { /* noop */ }

void
dri2_flush_drawable_for_swapbuffers(_EGLDisplay *disp, _EGLSurface *draw);

const __DRIconfig *
dri2_get_dri_config(struct dri2_egl_config *conf, EGLint surface_type,
                    EGLenum colorspace);

static inline void
dri2_set_WL_bind_wayland_display(_EGLDisplay *disp)
{
#ifdef HAVE_WAYLAND_PLATFORM
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   if (dri2_dpy->device_name && dri2_dpy->image) {
       if (dri2_dpy->image->base.version >= 10 &&
           dri2_dpy->image->getCapabilities != NULL) {
           int capabilities;

           capabilities =
               dri2_dpy->image->getCapabilities(dri2_dpy->dri_screen);
           disp->Extensions.WL_bind_wayland_display =
               (capabilities & __DRI_IMAGE_CAP_GLOBAL_NAMES) != 0;
       } else {
           disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
       }
   }
#endif
}

void
dri2_display_destroy(_EGLDisplay *disp);

__DRIbuffer *
dri2_egl_surface_alloc_local_buffer(struct dri2_egl_surface *dri2_surf,
                                    unsigned int att, unsigned int format);

void
dri2_egl_surface_free_local_buffers(struct dri2_egl_surface *dri2_surf);

EGLBoolean
dri2_init_surface(_EGLSurface *surf, _EGLDisplay *disp, EGLint type,
        _EGLConfig *conf, const EGLint *attrib_list,
        EGLBoolean enable_out_fence, void *native_surface);

void
dri2_fini_surface(_EGLSurface *surf);

EGLBoolean
dri2_create_drawable(struct dri2_egl_display *dri2_dpy,
                     const __DRIconfig *config,
                     struct dri2_egl_surface *dri2_surf,
                     void *loaderPrivate);

static inline uint64_t
combine_u32_into_u64(uint32_t hi, uint32_t lo)
{
   return (((uint64_t) hi) << 32) | (((uint64_t) lo) & 0xffffffff);
}

#endif /* EGL_DRI2_INCLUDED */
