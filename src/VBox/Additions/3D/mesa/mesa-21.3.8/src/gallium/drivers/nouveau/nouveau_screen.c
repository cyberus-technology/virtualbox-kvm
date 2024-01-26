#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/format/u_format.h"
#include "util/format/u_format_s3tc.h"
#include "util/u_string.h"

#include "os/os_mman.h"
#include "util/os_time.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <nouveau_drm.h>
#include <xf86drm.h>

#include "nouveau_winsys.h"
#include "nouveau_screen.h"
#include "nouveau_context.h"
#include "nouveau_fence.h"
#include "nouveau_mm.h"
#include "nouveau_buffer.h"

#include <compiler/glsl_types.h>

/* XXX this should go away */
#include "frontend/drm_driver.h"

/* Even though GPUs might allow addresses with more bits, some engines do not.
 * Stick with 40 for compatibility.
 */
#define NV_GENERIC_VM_LIMIT_SHIFT 39

int nouveau_mesa_debug = 0;

static const char *
nouveau_screen_get_name(struct pipe_screen *pscreen)
{
   struct nouveau_screen *screen = nouveau_screen(pscreen);
   return screen->chipset_name;
}

static const char *
nouveau_screen_get_vendor(struct pipe_screen *pscreen)
{
   return "nouveau";
}

static const char *
nouveau_screen_get_device_vendor(struct pipe_screen *pscreen)
{
   return "NVIDIA";
}

static uint64_t
nouveau_screen_get_timestamp(struct pipe_screen *pscreen)
{
   int64_t cpu_time = os_time_get() * 1000;

   /* getparam of PTIMER_TIME takes about x10 as long (several usecs) */

   return cpu_time + nouveau_screen(pscreen)->cpu_gpu_time_delta;
}

static struct disk_cache *
nouveau_screen_get_disk_shader_cache(struct pipe_screen *pscreen)
{
   return nouveau_screen(pscreen)->disk_shader_cache;
}

static void
nouveau_screen_fence_ref(struct pipe_screen *pscreen,
                         struct pipe_fence_handle **ptr,
                         struct pipe_fence_handle *pfence)
{
   nouveau_fence_ref(nouveau_fence(pfence), (struct nouveau_fence **)ptr);
}

static bool
nouveau_screen_fence_finish(struct pipe_screen *screen,
                            struct pipe_context *ctx,
                            struct pipe_fence_handle *pfence,
                            uint64_t timeout)
{
   if (!timeout)
      return nouveau_fence_signalled(nouveau_fence(pfence));

   return nouveau_fence_wait(nouveau_fence(pfence), NULL);
}


struct nouveau_bo *
nouveau_screen_bo_from_handle(struct pipe_screen *pscreen,
                              struct winsys_handle *whandle,
                              unsigned *out_stride)
{
   struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nouveau_bo *bo = 0;
   int ret;

   if (whandle->offset != 0) {
      debug_printf("%s: attempt to import unsupported winsys offset %d\n",
                   __FUNCTION__, whandle->offset);
      return NULL;
   }

   if (whandle->type != WINSYS_HANDLE_TYPE_SHARED &&
       whandle->type != WINSYS_HANDLE_TYPE_FD) {
      debug_printf("%s: attempt to import unsupported handle type %d\n",
                   __FUNCTION__, whandle->type);
      return NULL;
   }

   if (whandle->type == WINSYS_HANDLE_TYPE_SHARED)
      ret = nouveau_bo_name_ref(dev, whandle->handle, &bo);
   else
      ret = nouveau_bo_prime_handle_ref(dev, whandle->handle, &bo);

   if (ret) {
      debug_printf("%s: ref name 0x%08x failed with %d\n",
                   __FUNCTION__, whandle->handle, ret);
      return NULL;
   }

   *out_stride = whandle->stride;
   return bo;
}


bool
nouveau_screen_bo_get_handle(struct pipe_screen *pscreen,
                             struct nouveau_bo *bo,
                             unsigned stride,
                             struct winsys_handle *whandle)
{
   whandle->stride = stride;

   if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      return nouveau_bo_name_get(bo, &whandle->handle) == 0;
   } else if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
      whandle->handle = bo->handle;
      return true;
   } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      return nouveau_bo_set_prime(bo, (int *)&whandle->handle) == 0;
   } else {
      return false;
   }
}

static void
nouveau_disk_cache_create(struct nouveau_screen *screen)
{
   struct mesa_sha1 ctx;
   unsigned char sha1[20];
   char cache_id[20 * 2 + 1];
   uint64_t driver_flags = 0;

   _mesa_sha1_init(&ctx);
   if (!disk_cache_get_function_identifier(nouveau_disk_cache_create,
                                           &ctx))
      return;

   _mesa_sha1_final(&ctx, sha1);
   disk_cache_format_hex_id(cache_id, sha1, 20 * 2);

   if (screen->prefer_nir)
      driver_flags |= NOUVEAU_SHADER_CACHE_FLAGS_IR_NIR;
   else
      driver_flags |= NOUVEAU_SHADER_CACHE_FLAGS_IR_TGSI;

   screen->disk_shader_cache =
      disk_cache_create(nouveau_screen_get_name(&screen->base),
                        cache_id, driver_flags);
}

static void*
reserve_vma(uintptr_t start, uint64_t reserved_size)
{
   void *reserved = os_mmap((void*)start, reserved_size, PROT_NONE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
   if (reserved == MAP_FAILED)
      return NULL;
   return reserved;
}

int
nouveau_screen_init(struct nouveau_screen *screen, struct nouveau_device *dev)
{
   struct pipe_screen *pscreen = &screen->base;
   struct nv04_fifo nv04_data = { .vram = 0xbeef0201, .gart = 0xbeef0202 };
   struct nvc0_fifo nvc0_data = { };
   uint64_t time;
   int size, ret;
   void *data;
   union nouveau_bo_config mm_config;

   char *nv_dbg = getenv("NOUVEAU_MESA_DEBUG");
   if (nv_dbg)
      nouveau_mesa_debug = atoi(nv_dbg);

   if (dev->chipset < 0x140)
      screen->prefer_nir = debug_get_bool_option("NV50_PROG_USE_NIR", false);
   else
      screen->prefer_nir = true;

   screen->force_enable_cl = debug_get_bool_option("NOUVEAU_ENABLE_CL", false);
   if (screen->force_enable_cl)
      glsl_type_singleton_init_or_ref();

   /* These must be set before any failure is possible, as the cleanup
    * paths assume they're responsible for deleting them.
    */
   screen->drm = nouveau_drm(&dev->object);
   screen->device = dev;

   /*
    * this is initialized to 1 in nouveau_drm_screen_create after screen
    * is fully constructed and added to the global screen list.
    */
   screen->refcount = -1;

   if (dev->chipset < 0xc0) {
      data = &nv04_data;
      size = sizeof(nv04_data);
   } else {
      data = &nvc0_data;
      size = sizeof(nvc0_data);
   }

   bool enable_svm = debug_get_bool_option("NOUVEAU_SVM", false);
   screen->has_svm = false;
   /* we only care about HMM with OpenCL enabled */
   if (dev->chipset > 0x130 && screen->force_enable_cl && enable_svm) {
      /* Before being able to enable SVM we need to carve out some memory for
       * driver bo allocations. Let's just base the size on the available VRAM.
       *
       * 40 bit is the biggest we care about and for 32 bit systems we don't
       * want to allocate all of the available memory either.
       *
       * Also we align the size we want to reserve to the next POT to make use
       * of hugepages.
       */
      const int vram_shift = util_logbase2_ceil64(dev->vram_size);
      const int limit_bit =
         MIN2(sizeof(void*) * 8 - 1, NV_GENERIC_VM_LIMIT_SHIFT);
      screen->svm_cutout_size =
         BITFIELD64_BIT(MIN2(sizeof(void*) == 4 ? 26 : NV_GENERIC_VM_LIMIT_SHIFT, vram_shift));

      size_t start = screen->svm_cutout_size;
      do {
         screen->svm_cutout = reserve_vma(start, screen->svm_cutout_size);
         if (!screen->svm_cutout) {
            start += screen->svm_cutout_size;
            continue;
         }

         struct drm_nouveau_svm_init svm_args = {
            .unmanaged_addr = (uintptr_t)screen->svm_cutout,
            .unmanaged_size = screen->svm_cutout_size,
         };

         ret = drmCommandWrite(screen->drm->fd, DRM_NOUVEAU_SVM_INIT,
                               &svm_args, sizeof(svm_args));
         screen->has_svm = !ret;
         if (!screen->has_svm)
            os_munmap(screen->svm_cutout, screen->svm_cutout_size);
         break;
      } while ((start + screen->svm_cutout_size) < BITFIELD64_MASK(limit_bit));
   }

   switch (dev->chipset) {
   case 0x0ea: /* TK1, GK20A */
   case 0x12b: /* TX1, GM20B */
   case 0x13b: /* TX2, GP10B */
      screen->tegra_sector_layout = true;
      break;
   default:
      /* Xavier's GPU and everything else */
      screen->tegra_sector_layout = false;
      break;
   }

   /*
    * Set default VRAM domain if not overridden
    */
   if (!screen->vram_domain) {
      if (dev->vram_size > 0)
         screen->vram_domain = NOUVEAU_BO_VRAM;
      else
         screen->vram_domain = NOUVEAU_BO_GART;
   }

   ret = nouveau_object_new(&dev->object, 0, NOUVEAU_FIFO_CHANNEL_CLASS,
                            data, size, &screen->channel);
   if (ret)
      goto err;

   ret = nouveau_client_new(screen->device, &screen->client);
   if (ret)
      goto err;
   ret = nouveau_pushbuf_new(screen->client, screen->channel,
                             4, 512 * 1024, 1,
                             &screen->pushbuf);
   if (ret)
      goto err;

   /* getting CPU time first appears to be more accurate */
   screen->cpu_gpu_time_delta = os_time_get();

   ret = nouveau_getparam(dev, NOUVEAU_GETPARAM_PTIMER_TIME, &time);
   if (!ret)
      screen->cpu_gpu_time_delta = time - screen->cpu_gpu_time_delta * 1000;

   snprintf(screen->chipset_name, sizeof(screen->chipset_name), "NV%02X", dev->chipset);
   pscreen->get_name = nouveau_screen_get_name;
   pscreen->get_vendor = nouveau_screen_get_vendor;
   pscreen->get_device_vendor = nouveau_screen_get_device_vendor;
   pscreen->get_disk_shader_cache = nouveau_screen_get_disk_shader_cache;

   pscreen->get_timestamp = nouveau_screen_get_timestamp;

   pscreen->fence_reference = nouveau_screen_fence_ref;
   pscreen->fence_finish = nouveau_screen_fence_finish;

   nouveau_disk_cache_create(screen);

   screen->transfer_pushbuf_threshold = 192;
   screen->lowmem_bindings = PIPE_BIND_GLOBAL; /* gallium limit */
   screen->vidmem_bindings =
      PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL |
      PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT |
      PIPE_BIND_CURSOR |
      PIPE_BIND_SAMPLER_VIEW |
      PIPE_BIND_SHADER_BUFFER | PIPE_BIND_SHADER_IMAGE |
      PIPE_BIND_COMPUTE_RESOURCE |
      PIPE_BIND_GLOBAL;
   screen->sysmem_bindings =
      PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_STREAM_OUTPUT |
      PIPE_BIND_COMMAND_ARGS_BUFFER;

   memset(&mm_config, 0, sizeof(mm_config));

   screen->mm_GART = nouveau_mm_create(dev,
                                       NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
                                       &mm_config);
   screen->mm_VRAM = nouveau_mm_create(dev, NOUVEAU_BO_VRAM, &mm_config);
   return 0;

err:
   if (screen->svm_cutout)
      os_munmap(screen->svm_cutout, screen->svm_cutout_size);
   return ret;
}

void
nouveau_screen_fini(struct nouveau_screen *screen)
{
   int fd = screen->drm->fd;

   if (screen->force_enable_cl)
      glsl_type_singleton_decref();
   if (screen->has_svm)
      os_munmap(screen->svm_cutout, screen->svm_cutout_size);

   nouveau_mm_destroy(screen->mm_GART);
   nouveau_mm_destroy(screen->mm_VRAM);

   nouveau_pushbuf_del(&screen->pushbuf);

   nouveau_client_del(&screen->client);
   nouveau_object_del(&screen->channel);

   nouveau_device_del(&screen->device);
   nouveau_drm_del(&screen->drm);
   close(fd);

   disk_cache_destroy(screen->disk_shader_cache);
}

static void
nouveau_set_debug_callback(struct pipe_context *pipe,
                           const struct pipe_debug_callback *cb)
{
   struct nouveau_context *context = nouveau_context(pipe);

   if (cb)
      context->debug = *cb;
   else
      memset(&context->debug, 0, sizeof(context->debug));
}

void
nouveau_context_init(struct nouveau_context *context)
{
   context->pipe.set_debug_callback = nouveau_set_debug_callback;
}
