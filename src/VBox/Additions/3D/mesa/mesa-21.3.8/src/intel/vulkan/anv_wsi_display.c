/*
 * Copyright © 2017 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "anv_private.h"
#include "wsi_common.h"
#include "vk_util.h"
#include "wsi_common_display.h"

/* VK_EXT_display_control */

VkResult
anv_RegisterDeviceEventEXT(VkDevice _device,
                            const VkDeviceEventInfoEXT *device_event_info,
                            const VkAllocationCallbacks *allocator,
                            VkFence *_fence)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_fence *fence;
   VkResult ret;

   fence = vk_object_zalloc(&device->vk, allocator, sizeof (*fence),
                            VK_OBJECT_TYPE_FENCE);
   if (!fence)
      return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);

   fence->permanent.type = ANV_FENCE_TYPE_WSI;

   ret = wsi_register_device_event(_device,
                                   &device->physical->wsi_device,
                                   device_event_info,
                                   allocator,
                                   &fence->permanent.fence_wsi,
                                   -1);
   if (ret == VK_SUCCESS)
      *_fence = anv_fence_to_handle(fence);
   else
      vk_free2(&device->vk.alloc, allocator, fence);
   return ret;
}

VkResult
anv_RegisterDisplayEventEXT(VkDevice _device,
                             VkDisplayKHR display,
                             const VkDisplayEventInfoEXT *display_event_info,
                             const VkAllocationCallbacks *allocator,
                             VkFence *_fence)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_fence *fence;
   VkResult ret;

   fence = vk_object_zalloc(&device->vk, allocator, sizeof (*fence),
                            VK_OBJECT_TYPE_FENCE);
   if (!fence)
      return VK_ERROR_OUT_OF_HOST_MEMORY;

   fence->permanent.type = ANV_FENCE_TYPE_WSI;

   ret = wsi_register_display_event(
      _device, &device->physical->wsi_device,
      display, display_event_info, allocator, &fence->permanent.fence_wsi, -1);

   if (ret == VK_SUCCESS)
      *_fence = anv_fence_to_handle(fence);
   else
      vk_free2(&device->vk.alloc, allocator, fence);
   return ret;
}
