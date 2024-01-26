/*
 * Copyright © 2015 Intel Corporation
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

#include "anv_private.h"
#include "anv_measure.h"
#include "wsi_common.h"
#include "vk_util.h"

static PFN_vkVoidFunction
anv_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
{
   ANV_FROM_HANDLE(anv_physical_device, pdevice, physicalDevice);
   return vk_instance_get_proc_addr_unchecked(&pdevice->instance->vk, pName);
}

static void
anv_wsi_signal_semaphore_for_memory(VkDevice _device,
                                    VkSemaphore _semaphore,
                                    VkDeviceMemory _memory)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_semaphore, semaphore, _semaphore);
   ANV_FROM_HANDLE(anv_device_memory, memory, _memory);

   /* Put a BO semaphore with the image BO in the temporary.  For BO binary
    * semaphores, we always set EXEC_OBJECT_WRITE so this creates a WaR
    * hazard with the display engine's read to ensure that no one writes to
    * the image before the read is complete.
    */
   anv_semaphore_reset_temporary(device, semaphore);

   struct anv_semaphore_impl *impl = &semaphore->temporary;
   impl->type = ANV_SEMAPHORE_TYPE_WSI_BO;
   impl->bo = anv_bo_ref(memory->bo);
}

static void
anv_wsi_signal_fence_for_memory(VkDevice _device,
                                VkFence _fence,
                                VkDeviceMemory _memory)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_fence, fence, _fence);
   ANV_FROM_HANDLE(anv_device_memory, memory, _memory);

   /* Put a BO fence with the image BO in the temporary.  For BO fences, we
    * always just wait until the BO isn't busy and reads from the BO should
    * count as busy.
    */
   anv_fence_reset_temporary(device, fence);

   struct anv_fence_impl *impl = &fence->temporary;
   impl->type = ANV_FENCE_TYPE_WSI_BO;
   impl->bo.bo = anv_bo_ref(memory->bo);
   impl->bo.state = ANV_BO_FENCE_STATE_SUBMITTED;
}

VkResult
anv_init_wsi(struct anv_physical_device *physical_device)
{
   VkResult result;

   result = wsi_device_init(&physical_device->wsi_device,
                            anv_physical_device_to_handle(physical_device),
                            anv_wsi_proc_addr,
                            &physical_device->instance->vk.alloc,
                            physical_device->master_fd,
                            &physical_device->instance->dri_options,
                            false);
   if (result != VK_SUCCESS)
      return result;

   physical_device->wsi_device.supports_modifiers = true;
   physical_device->wsi_device.signal_semaphore_for_memory =
      anv_wsi_signal_semaphore_for_memory;
   physical_device->wsi_device.signal_fence_for_memory =
      anv_wsi_signal_fence_for_memory;

   physical_device->vk.wsi_device = &physical_device->wsi_device;

   return VK_SUCCESS;
}

void
anv_finish_wsi(struct anv_physical_device *physical_device)
{
   physical_device->vk.wsi_device = NULL;
   wsi_device_finish(&physical_device->wsi_device,
                     &physical_device->instance->vk.alloc);
}

VkResult anv_QueuePresentKHR(
    VkQueue                                  _queue,
    const VkPresentInfoKHR*                  pPresentInfo)
{
   ANV_FROM_HANDLE(anv_queue, queue, _queue);
   struct anv_device *device = queue->device;

   if (device->debug_frame_desc) {
      device->debug_frame_desc->frame_id++;
      if (!device->info.has_llc) {
         intel_clflush_range(device->debug_frame_desc,
                           sizeof(*device->debug_frame_desc));
      }
   }

   if (device->has_thread_submit &&
       pPresentInfo->waitSemaphoreCount > 0) {
      /* Make sure all of the dependency semaphores have materialized when
       * using a threaded submission.
       */
      VK_MULTIALLOC(ma);
      VK_MULTIALLOC_DECL(&ma, uint64_t, values,
                              pPresentInfo->waitSemaphoreCount);
      VK_MULTIALLOC_DECL(&ma, uint32_t, syncobjs,
                              pPresentInfo->waitSemaphoreCount);

      if (!vk_multialloc_alloc(&ma, &device->vk.alloc,
                               VK_SYSTEM_ALLOCATION_SCOPE_COMMAND))
         return vk_error(queue, VK_ERROR_OUT_OF_HOST_MEMORY);

      uint32_t wait_count = 0;
      for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; i++) {
         ANV_FROM_HANDLE(anv_semaphore, semaphore, pPresentInfo->pWaitSemaphores[i]);
         struct anv_semaphore_impl *impl =
            semaphore->temporary.type != ANV_SEMAPHORE_TYPE_NONE ?
            &semaphore->temporary : &semaphore->permanent;

         if (impl->type == ANV_SEMAPHORE_TYPE_DUMMY)
            continue;
         assert(impl->type == ANV_SEMAPHORE_TYPE_DRM_SYNCOBJ);
         syncobjs[wait_count] = impl->syncobj;
         values[wait_count] = 0;
         wait_count++;
      }

      int ret = 0;
      if (wait_count > 0) {
         ret =
            anv_gem_syncobj_timeline_wait(device,
                                          syncobjs, values, wait_count,
                                          anv_get_absolute_timeout(INT64_MAX),
                                          true /* wait_all */,
                                          true /* wait_materialize */);
      }

      vk_free(&device->vk.alloc, values);

      if (ret)
         return vk_error(queue, VK_ERROR_DEVICE_LOST);
   }

   VkResult result = wsi_common_queue_present(&device->physical->wsi_device,
                                              anv_device_to_handle(queue->device),
                                              _queue, 0,
                                              pPresentInfo);

   for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; i++) {
      ANV_FROM_HANDLE(anv_semaphore, semaphore, pPresentInfo->pWaitSemaphores[i]);
      /* From the Vulkan 1.0.53 spec:
       *
       *    "If the import is temporary, the implementation must restore the
       *    semaphore to its prior permanent state after submitting the next
       *    semaphore wait operation."
       */
      anv_semaphore_reset_temporary(queue->device, semaphore);
   }

   return result;
}
