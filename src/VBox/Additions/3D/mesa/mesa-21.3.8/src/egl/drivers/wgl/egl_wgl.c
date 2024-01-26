/*
 * Copyright © Microsoft Corporation
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <egldriver.h>
#include <egllog.h>
#include <eglcurrent.h>
#include <eglcontext.h>
#include <eglsurface.h>

#include "egl_wgl.h"

#include <stw_device.h>
#include <stw_pixelformat.h>
#include <stw_context.h>
#include <stw_framebuffer.h>

#include <GL/wglext.h>

#include <pipe/p_screen.h>

#include <mapi/glapi/glapi.h>

static EGLBoolean
wgl_match_config(const _EGLConfig *conf, const _EGLConfig *criteria)
{
   if (_eglCompareConfigs(conf, criteria, NULL, EGL_FALSE) != 0)
      return EGL_FALSE;

   if (!_eglMatchConfig(conf, criteria))
      return EGL_FALSE;

   return EGL_TRUE;
}

static struct wgl_egl_config *
wgl_add_config(_EGLDisplay *disp, const struct stw_pixelformat_info *stw_config, int id, EGLint surface_type)
{
   struct wgl_egl_config *conf;
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   _EGLConfig base;
   unsigned int double_buffer;
   int wgl_shifts[4] = { -1, -1, -1, -1 };
   unsigned int wgl_sizes[4] = { 0, 0, 0, 0 };
   _EGLConfig *matching_config;
   EGLint num_configs = 0;
   EGLint config_id;

   _eglInitConfig(&base, disp, id);

   double_buffer = (stw_config->pfd.dwFlags & PFD_DOUBLEBUFFER) != 0;

   if (stw_config->pfd.iPixelType != PFD_TYPE_RGBA)
      return NULL;

   wgl_sizes[0] = stw_config->pfd.cRedBits;
   wgl_sizes[1] = stw_config->pfd.cGreenBits;
   wgl_sizes[2] = stw_config->pfd.cBlueBits;
   wgl_sizes[3] = stw_config->pfd.cAlphaBits;

   base.RedSize = stw_config->pfd.cRedBits;
   base.GreenSize = stw_config->pfd.cGreenBits;
   base.BlueSize = stw_config->pfd.cBlueBits;
   base.AlphaSize = stw_config->pfd.cAlphaBits;
   base.BufferSize = stw_config->pfd.cColorBits;

   wgl_shifts[0] = stw_config->pfd.cRedShift;
   wgl_shifts[1] = stw_config->pfd.cGreenShift;
   wgl_shifts[2] = stw_config->pfd.cBlueShift;
   wgl_shifts[3] = stw_config->pfd.cAlphaShift;

   if (stw_config->pfd.cAccumBits) {
      /* Don't expose visuals with the accumulation buffer. */
      return NULL;
   }

   base.MaxPbufferWidth = _EGL_MAX_PBUFFER_WIDTH;
   base.MaxPbufferHeight = _EGL_MAX_PBUFFER_HEIGHT;

   base.DepthSize = stw_config->pfd.cDepthBits;
   base.StencilSize = stw_config->pfd.cStencilBits;
   base.Samples = stw_config->stvis.samples;
   base.SampleBuffers = base.Samples > 1;

   base.NativeRenderable = EGL_TRUE;

   if (surface_type & EGL_PBUFFER_BIT) {
      base.BindToTextureRGB = stw_config->bindToTextureRGB;
      if (base.AlphaSize > 0)
         base.BindToTextureRGBA = stw_config->bindToTextureRGBA;
   }

   if (double_buffer) {
      surface_type &= ~EGL_PIXMAP_BIT;
   }

   if (!(stw_config->pfd.dwFlags & PFD_DRAW_TO_WINDOW)) {
      surface_type &= ~EGL_WINDOW_BIT;
   }

   if (!surface_type)
      return NULL;

   base.SurfaceType = surface_type;
   base.RenderableType = disp->ClientAPIs;
   base.Conformant = disp->ClientAPIs;

   base.MinSwapInterval = 0;
   base.MaxSwapInterval = 1;

   if (!_eglValidateConfig(&base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "wgl: failed to validate config %d", id);
      return NULL;
   }

   config_id = base.ConfigID;
   base.ConfigID = EGL_DONT_CARE;
   base.SurfaceType = EGL_DONT_CARE;
   num_configs = _eglFilterArray(disp->Configs, (void **)&matching_config, 1,
      (_EGLArrayForEach)wgl_match_config, &base);

   if (num_configs == 1) {
      conf = (struct wgl_egl_config *)matching_config;

      if (!conf->stw_config[double_buffer])
         conf->stw_config[double_buffer] = stw_config;
      else
         /* a similar config type is already added (unlikely) => discard */
         return NULL;
   }
   else if (num_configs == 0) {
      conf = calloc(1, sizeof * conf);
      if (conf == NULL)
         return NULL;

      conf->stw_config[double_buffer] = stw_config;

      memcpy(&conf->base, &base, sizeof base);
      conf->base.SurfaceType = 0;
      conf->base.ConfigID = config_id;

      _eglLinkConfig(&conf->base);
   }
   else {
      unreachable("duplicates should not be possible");
      return NULL;
   }

   conf->base.SurfaceType |= surface_type;

   return conf;
}

static EGLBoolean
wgl_add_configs(_EGLDisplay *disp, HDC hdc)
{
   unsigned int config_count = 0;
   unsigned surface_type = EGL_PBUFFER_BIT | (hdc ? EGL_WINDOW_BIT : 0);

   // This is already a filtered set of what the driver supports,
   // and there's no further filtering needed per-visual
   for (unsigned i = 1; stw_pixelformat_get_info(i) != NULL; i++) {

      struct wgl_egl_config *wgl_conf = wgl_add_config(disp, stw_pixelformat_get_info(i),
         config_count + 1, surface_type);

      if (wgl_conf) {
         if (wgl_conf->base.ConfigID == config_count + 1)
            config_count++;
      }
   }

   return (config_count != 0);
}

static void
wgl_display_destroy(_EGLDisplay *disp)
{
   free(disp);
}

static EGLBoolean
wgl_initialize_impl(_EGLDisplay *disp, HDC hdc)
{
   struct wgl_egl_display *wgl_dpy;
   const char* err;

   wgl_dpy = calloc(1, sizeof * wgl_dpy);
   if (!wgl_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   disp->DriverData = (void *)wgl_dpy;

   if (!stw_init_screen(hdc)) {
      err = "wgl: failed to initialize screen";
      goto cleanup;
   }

   wgl_dpy->screen = stw_get_device()->screen;

   disp->ClientAPIs = 0;
   if (_eglIsApiValid(EGL_OPENGL_API))
      disp->ClientAPIs |= EGL_OPENGL_BIT;
   if (_eglIsApiValid(EGL_OPENGL_ES_API))
      disp->ClientAPIs |= EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT_KHR;

   disp->Extensions.KHR_no_config_context = EGL_TRUE;
   disp->Extensions.KHR_surfaceless_context = EGL_TRUE;
   disp->Extensions.MESA_query_driver = EGL_TRUE;

   /* Report back to EGL the bitmask of priorities supported */
   disp->Extensions.IMG_context_priority =
      wgl_dpy->screen->get_param(wgl_dpy->screen, PIPE_CAP_CONTEXT_PRIORITY_MASK);

   disp->Extensions.EXT_pixel_format_float = EGL_TRUE;

   if (wgl_dpy->screen->is_format_supported(wgl_dpy->screen,
         PIPE_FORMAT_B8G8R8A8_SRGB,
         PIPE_TEXTURE_2D, 0, 0,
         PIPE_BIND_RENDER_TARGET))
      disp->Extensions.KHR_gl_colorspace = EGL_TRUE;

   disp->Extensions.KHR_create_context = EGL_TRUE;
   disp->Extensions.KHR_reusable_sync = EGL_TRUE;

#if 0
   disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   if (wgl_dpy->image->base.version >= 5 &&
      wgl_dpy->image->createImageFromTexture) {
      disp->Extensions.KHR_gl_texture_2D_image = EGL_TRUE;
      disp->Extensions.KHR_gl_texture_cubemap_image = EGL_TRUE;

      if (wgl_renderer_query_integer(wgl_dpy,
         __wgl_RENDERER_HAS_TEXTURE_3D))
         disp->Extensions.KHR_gl_texture_3D_image = EGL_TRUE;
   }
#endif

   if (!wgl_add_configs(disp, hdc)) {
      err = "wgl: failed to add configs";
      goto cleanup;
   }

   return EGL_TRUE;

cleanup:
   wgl_display_destroy(disp);
   return _eglError(EGL_NOT_INITIALIZED, err);
}

static EGLBoolean
wgl_initialize(_EGLDisplay *disp)
{
   EGLBoolean ret = EGL_FALSE;
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);

   /* In the case where the application calls eglMakeCurrent(context1),
    * eglTerminate, then eglInitialize again (without a call to eglReleaseThread
    * or eglMakeCurrent(NULL) before that), wgl_dpy structure is still
    * initialized, as we need it to be able to free context1 correctly.
    *
    * It would probably be safest to forcibly release the display with
    * wgl_display_release, to make sure the display is reinitialized correctly.
    * However, the EGL spec states that we need to keep a reference to the
    * current context (so we cannot call wgl_make_current(NULL)), and therefore
    * we would leak context1 as we would be missing the old display connection
    * to free it up correctly.
    */
   if (wgl_dpy) {
      wgl_dpy->ref_count++;
      return EGL_TRUE;
   }

   switch (disp->Platform) {
   case _EGL_PLATFORM_SURFACELESS:
      ret = wgl_initialize_impl(disp, NULL);
      break;
   case _EGL_PLATFORM_WINDOWS:
      ret = wgl_initialize_impl(disp, disp->PlatformDisplay);
      break;
   default:
      unreachable("Callers ensure we cannot get here.");
      return EGL_FALSE;
   }

   if (!ret)
      return EGL_FALSE;

   wgl_dpy = wgl_egl_display(disp);
   wgl_dpy->ref_count++;

   return EGL_TRUE;
}

/**
 * Decrement display reference count, and free up display if necessary.
 */
static void
wgl_display_release(_EGLDisplay *disp)
{
   struct wgl_egl_display *wgl_dpy;

   if (!disp)
      return;

   wgl_dpy = wgl_egl_display(disp);

   assert(wgl_dpy->ref_count > 0);
   wgl_dpy->ref_count--;

   if (wgl_dpy->ref_count > 0)
      return;

   _eglCleanupDisplay(disp);
   wgl_display_destroy(disp);
}

/**
 * Called via eglTerminate(), drv->Terminate().
 *
 * This must be guaranteed to be called exactly once, even if eglTerminate is
 * called many times (without a eglInitialize in between).
 */
static EGLBoolean
wgl_terminate(_EGLDisplay *disp)
{
   /* Release all non-current Context/Surfaces. */
   _eglReleaseDisplayResources(disp);

   wgl_display_release(disp);

   return EGL_TRUE;
}

/**
 * Called via eglCreateContext(), drv->CreateContext().
 */
static _EGLContext *
wgl_create_context(_EGLDisplay *disp, _EGLConfig *conf,
   _EGLContext *share_list, const EGLint *attrib_list)
{
   struct wgl_egl_context *wgl_ctx;
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   struct wgl_egl_context *wgl_ctx_shared = wgl_egl_context(share_list);
   struct stw_context *shared =
      wgl_ctx_shared ? wgl_ctx_shared->ctx : NULL;
   struct wgl_egl_config *wgl_config = wgl_egl_config(conf);
   const struct stw_pixelformat_info *stw_config;

   wgl_ctx = malloc(sizeof * wgl_ctx);
   if (!wgl_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&wgl_ctx->base, disp, conf, attrib_list))
      goto cleanup;

   /* The EGL_EXT_create_context_robustness spec says:
    *
    *    "Add to the eglCreateContext context creation errors: [...]
    *
    *     * If the reset notification behavior of <share_context> and the
    *       newly created context are different then an EGL_BAD_MATCH error is
    *       generated."
    */
   if (share_list && share_list->ResetNotificationStrategy !=
      wgl_ctx->base.ResetNotificationStrategy) {
      _eglError(EGL_BAD_MATCH, "eglCreateContext");
      goto cleanup;
   }

   /* The EGL_KHR_create_context_no_error spec says:
    *
    *    "BAD_MATCH is generated if the value of EGL_CONTEXT_OPENGL_NO_ERROR_KHR
    *    used to create <share_context> does not match the value of
    *    EGL_CONTEXT_OPENGL_NO_ERROR_KHR for the context being created."
    */
   if (share_list && share_list->NoError != wgl_ctx->base.NoError) {
      _eglError(EGL_BAD_MATCH, "eglCreateContext");
      goto cleanup;
   }

   unsigned profile_mask = 0;
   switch (wgl_ctx->base.ClientAPI) {
   case EGL_OPENGL_ES_API:
      profile_mask = WGL_CONTEXT_ES_PROFILE_BIT_EXT;
      break;
   case EGL_OPENGL_API:
      if ((wgl_ctx->base.ClientMajorVersion >= 4
         || (wgl_ctx->base.ClientMajorVersion == 3
            && wgl_ctx->base.ClientMinorVersion >= 2))
         && wgl_ctx->base.Profile == EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR)
         profile_mask = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
      else if (wgl_ctx->base.ClientMajorVersion == 3 &&
         wgl_ctx->base.ClientMinorVersion == 1)
         profile_mask = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
      else
         profile_mask = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
      free(wgl_ctx);
      return NULL;
   }

   if (conf != NULL) {
      /* The config chosen here isn't necessarily
       * used for surfaces later.
       * A pixmap surface will use the single config.
       * This opportunity depends on disabling the
       * doubleBufferMode check in
       * src/mesa/main/context.c:check_compatible()
       */
      if (wgl_config->stw_config[1])
         stw_config = wgl_config->stw_config[1];
      else
         stw_config = wgl_config->stw_config[0];
   }
   else
      stw_config = NULL;

   unsigned flags = 0;
   if (wgl_ctx->base.Flags & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR)
      flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
   if (wgl_ctx->base.Flags & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR)
      flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
   wgl_ctx->ctx = stw_create_context_attribs(disp->PlatformDisplay, 0, shared,
      wgl_ctx->base.ClientMajorVersion,
      wgl_ctx->base.ClientMinorVersion,
      flags,
      profile_mask,
      stw_config->iPixelFormat);

   if (!wgl_ctx->ctx)
      goto cleanup;

   return &wgl_ctx->base;

cleanup:
   free(wgl_ctx);
   return NULL;
}

/**
 * Called via eglDestroyContext(), drv->DestroyContext().
 */
static EGLBoolean
wgl_destroy_context(_EGLDisplay *disp, _EGLContext *ctx)
{
   struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);

   if (_eglPutContext(ctx)) {
      stw_destroy_context(wgl_ctx->ctx);
      free(wgl_ctx);
   }

   return EGL_TRUE;
}

static EGLBoolean
wgl_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
{
   struct wgl_egl_surface *wgl_surf = wgl_egl_surface(surf);

   if (!_eglPutSurface(surf))
      return EGL_TRUE;

   struct stw_context *ctx = stw_current_context();
   stw_framebuffer_lock(wgl_surf->fb);
   stw_framebuffer_release_locked(wgl_surf->fb, ctx ? ctx->st : NULL);
   return EGL_TRUE;
}

static void
wgl_gl_flush()
{
   static void (*glFlush)(void);
   static mtx_t glFlushMutex = _MTX_INITIALIZER_NP;

   mtx_lock(&glFlushMutex);
   if (!glFlush)
      glFlush = _glapi_get_proc_address("glFlush");
   mtx_unlock(&glFlushMutex);

   /* if glFlush is not available things are horribly broken */
   if (!glFlush) {
      _eglLog(_EGL_WARNING, "wgl: failed to find glFlush entry point");
      return;
   }

   glFlush();
}

/**
 * Called via eglMakeCurrent(), drv->MakeCurrent().
 */
static EGLBoolean
wgl_make_current(_EGLDisplay *disp, _EGLSurface *dsurf,
   _EGLSurface *rsurf, _EGLContext *ctx)
{
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
   _EGLDisplay *old_disp = NULL;
   struct wgl_egl_display *old_wgl_dpy = NULL;
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   _EGLSurface *tmp_dsurf, *tmp_rsurf;
   struct stw_framebuffer *ddraw, *rdraw;
   struct stw_context *cctx;
   EGLint egl_error = EGL_SUCCESS;

   if (!wgl_dpy)
      return _eglError(EGL_NOT_INITIALIZED, "eglMakeCurrent");

   /* make new bindings, set the EGL error otherwise */
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
      return EGL_FALSE;

   if (old_ctx) {
      struct stw_context *old_cctx = wgl_egl_context(old_ctx)->ctx;
      old_disp = old_ctx->Resource.Display;
      old_wgl_dpy = wgl_egl_display(old_disp);

      /* flush before context switch */
      wgl_gl_flush();

#if 0
      if (old_dsurf)
         wgl_surf_update_fence_fd(old_ctx, disp, old_dsurf);

      /* Disable shared buffer mode */
      if (old_dsurf && _eglSurfaceInSharedBufferMode(old_dsurf) &&
         old_wgl_dpy->vtbl->set_shared_buffer_mode) {
         old_wgl_dpy->vtbl->set_shared_buffer_mode(old_disp, old_dsurf, false);
      }
#endif

      stw_unbind_context(old_cctx);
   }

   ddraw = (dsurf) ? wgl_egl_surface(dsurf)->fb : NULL;
   rdraw = (rsurf) ? wgl_egl_surface(rsurf)->fb : NULL;
   cctx = (wgl_ctx) ? wgl_ctx->ctx : NULL;

   if (cctx || ddraw || rdraw) {
      if (!stw_make_current(ddraw, rdraw, cctx)) {
         _EGLContext *tmp_ctx;

         /* stw_make_current failed. We cannot tell for sure why, but
          * setting the error to EGL_BAD_MATCH is surely better than leaving it
          * as EGL_SUCCESS.
          */
         egl_error = EGL_BAD_MATCH;

         /* undo the previous _eglBindContext */
         _eglBindContext(old_ctx, old_dsurf, old_rsurf, &ctx, &tmp_dsurf, &tmp_rsurf);
         assert(&wgl_ctx->base == ctx &&
            tmp_dsurf == dsurf &&
            tmp_rsurf == rsurf);

         _eglPutSurface(dsurf);
         _eglPutSurface(rsurf);
         _eglPutContext(ctx);

         _eglPutSurface(old_dsurf);
         _eglPutSurface(old_rsurf);
         _eglPutContext(old_ctx);

         ddraw = (old_dsurf) ? wgl_egl_surface(old_dsurf)->fb : NULL;
         rdraw = (old_rsurf) ? wgl_egl_surface(old_rsurf)->fb : NULL;
         cctx = (old_ctx) ? wgl_egl_context(old_ctx)->ctx : NULL;

         /* undo the previous wgl_dpy->core->unbindContext */
         if (stw_make_current(ddraw, rdraw, cctx)) {
#if 0
            if (old_dsurf && _eglSurfaceInSharedBufferMode(old_dsurf) &&
               old_wgl_dpy->vtbl->set_shared_buffer_mode) {
               old_wgl_dpy->vtbl->set_shared_buffer_mode(old_disp, old_dsurf, true);
            }
#endif

            return _eglError(egl_error, "eglMakeCurrent");
         }

         /* We cannot restore the same state as it was before calling
          * eglMakeCurrent() and the spec isn't clear about what to do. We
          * can prevent EGL from calling into the DRI driver with no DRI
          * context bound.
          */
         dsurf = rsurf = NULL;
         ctx = NULL;

         _eglBindContext(ctx, dsurf, rsurf, &tmp_ctx, &tmp_dsurf, &tmp_rsurf);
         assert(tmp_ctx == old_ctx && tmp_dsurf == old_dsurf &&
            tmp_rsurf == old_rsurf);

         _eglLog(_EGL_WARNING, "wgl: failed to rebind the previous context");
      }
      else {
         /* wgl_dpy->core->bindContext succeeded, so take a reference on the
          * wgl_dpy. This prevents wgl_dpy from being reinitialized when a
          * EGLDisplay is terminated and then initialized again while a
          * context is still bound. See wgl_intitialize() for a more in depth
          * explanation. */
         wgl_dpy->ref_count++;
      }
   }

   wgl_destroy_surface(disp, old_dsurf);
   wgl_destroy_surface(disp, old_rsurf);

   if (old_ctx) {
      wgl_destroy_context(disp, old_ctx);
      wgl_display_release(old_disp);
   }

   if (egl_error != EGL_SUCCESS)
      return _eglError(egl_error, "eglMakeCurrent");

#if 0
   if (dsurf && _eglSurfaceHasMutableRenderBuffer(dsurf) &&
      wgl_dpy->vtbl->set_shared_buffer_mode) {
      /* Always update the shared buffer mode. This is obviously needed when
       * the active EGL_RENDER_BUFFER is EGL_SINGLE_BUFFER. When
       * EGL_RENDER_BUFFER is EGL_BACK_BUFFER, the update protects us in the
       * case where external non-EGL API may have changed window's shared
       * buffer mode since we last saw it.
       */
      bool mode = (dsurf->ActiveRenderBuffer == EGL_SINGLE_BUFFER);
      wgl_dpy->vtbl->set_shared_buffer_mode(disp, dsurf, mode);
   }
#endif

   return EGL_TRUE;
}

static _EGLSurface*
wgl_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
                          void *native_window, const EGLint *attrib_list)
{
   struct wgl_egl_config *wgl_conf = wgl_egl_config(conf);

   struct wgl_egl_surface *wgl_surf = calloc(1, sizeof(*wgl_surf));
   if (!wgl_surf)
      return NULL;

   if (!_eglInitSurface(&wgl_surf->base, disp, EGL_WINDOW_BIT, conf, attrib_list, native_window)) {
      free(wgl_surf);
      return NULL;
   }

   const struct stw_pixelformat_info *stw_conf = wgl_conf->stw_config[1] ?
      wgl_conf->stw_config[1] : wgl_conf->stw_config[0];
   wgl_surf->fb = stw_framebuffer_create(native_window, stw_conf->iPixelFormat, STW_FRAMEBUFFER_EGL_WINDOW);
   if (!wgl_surf->fb) {
      free(wgl_surf);
      return NULL;
   }

   stw_framebuffer_unlock(wgl_surf->fb);

   return &wgl_surf->base;
}

static EGLBoolean
wgl_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
{
   struct wgl_egl_display *wgl_disp = wgl_egl_display(disp);
   struct wgl_egl_surface *wgl_surf = wgl_egl_surface(draw);

   stw_framebuffer_lock(wgl_surf->fb);
   HDC hdc = GetDC(wgl_surf->fb->hWnd);
   BOOL ret = stw_framebuffer_swap_locked(hdc, wgl_surf->fb);
   ReleaseDC(wgl_surf->fb->hWnd, hdc);

   return ret;
}

struct _egl_driver _eglDriver = {
   .Initialize = wgl_initialize,
   .Terminate = wgl_terminate,
   .CreateContext = wgl_create_context,
   .DestroyContext = wgl_destroy_context,
   .MakeCurrent = wgl_make_current,
   .CreateWindowSurface = wgl_create_window_surface,
   .DestroySurface = wgl_destroy_surface,
   .GetProcAddress = _glapi_get_proc_address,
   .SwapBuffers = wgl_swap_buffers,
};

