/*
 * Copyright © 2020 Intel Corporation
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

#include "intel_uuid.h"
#include "git_sha1.h"
#include "util/mesa-sha1.h"

void
intel_uuid_compute_device_id(uint8_t *uuid,
                             const struct isl_device *isldev,
                             size_t size)
{
   struct mesa_sha1 sha1_ctx;
   uint8_t sha1[20];
   const struct intel_device_info *devinfo = isldev->info;

   assert(size <= sizeof(sha1));

   /* The device UUID uniquely identifies the given device within the machine.
    * Since we never have more than one device, this doesn't need to be a real
    * UUID.  However, on the off-chance that someone tries to use this to
    * cache pre-tiled images or something of the like, we use the PCI ID and
    * some bits of ISL info to ensure that this is safe.
    */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, &devinfo->chipset_id,
                     sizeof(devinfo->chipset_id));
   _mesa_sha1_update(&sha1_ctx, &isldev->has_bit6_swizzling,
                     sizeof(isldev->has_bit6_swizzling));
   _mesa_sha1_final(&sha1_ctx, sha1);
   memcpy(uuid, sha1, size);
}

void
intel_uuid_compute_driver_id(uint8_t *uuid,
                             const struct intel_device_info *devinfo,
                             size_t size)
{
   const char* intelDriver = PACKAGE_VERSION MESA_GIT_SHA1;
   struct mesa_sha1 sha1_ctx;
   uint8_t sha1[20];

   assert(size <= sizeof(sha1));

   /* The driver UUID is used for determining sharability of images and memory
    * between two Vulkan instances in separate processes, but also to
    * determining memory objects and sharability between Vulkan and OpenGL
    * driver. People who want to share memory need to also check the device
    * UUID.
    */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, intelDriver, strlen(intelDriver) * sizeof(char));
   _mesa_sha1_final(&sha1_ctx, sha1);
   memcpy(uuid, sha1, size);
}
