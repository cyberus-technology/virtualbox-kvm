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

#include "vk_format.h"
#include "vk_util.h"

static void
anv_render_pass_add_subpass_dep(struct anv_device *device,
                                struct anv_render_pass *pass,
                                const VkSubpassDependency2KHR *dep)
{
   /* From the Vulkan 1.2.195 spec:
    *
    *    "If an instance of VkMemoryBarrier2 is included in the pNext chain,
    *    srcStageMask, dstStageMask, srcAccessMask, and dstAccessMask
    *    parameters are ignored. The synchronization and access scopes instead
    *    are defined by the parameters of VkMemoryBarrier2."
    */
   const VkMemoryBarrier2KHR *barrier =
      vk_find_struct_const(dep->pNext, MEMORY_BARRIER_2_KHR);
   VkAccessFlags2KHR src_access_mask =
      barrier ? barrier->srcAccessMask : dep->srcAccessMask;
   VkAccessFlags2KHR dst_access_mask =
      barrier ? barrier->dstAccessMask : dep->dstAccessMask;

   if (dep->dstSubpass == VK_SUBPASS_EXTERNAL) {
      pass->subpass_flushes[pass->subpass_count] |=
         anv_pipe_invalidate_bits_for_access_flags(device, dst_access_mask);
   } else {
      assert(dep->dstSubpass < pass->subpass_count);
      pass->subpass_flushes[dep->dstSubpass] |=
         anv_pipe_invalidate_bits_for_access_flags(device, dst_access_mask);
   }

   if (dep->srcSubpass == VK_SUBPASS_EXTERNAL) {
      pass->subpass_flushes[0] |=
         anv_pipe_flush_bits_for_access_flags(device, src_access_mask);
   } else {
      assert(dep->srcSubpass < pass->subpass_count);
      pass->subpass_flushes[dep->srcSubpass + 1] |=
         anv_pipe_flush_bits_for_access_flags(device, src_access_mask);
   }
}

/* Do a second "compile" step on a render pass */
static void
anv_render_pass_compile(struct anv_render_pass *pass)
{
   /* The CreateRenderPass code zeros the entire render pass and also uses a
    * designated initializer for filling these out.  There's no need for us to
    * do it again.
    *
    * for (uint32_t i = 0; i < pass->attachment_count; i++) {
    *    pass->attachments[i].usage = 0;
    *    pass->attachments[i].first_subpass_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    * }
    */

   VkImageUsageFlags all_usage = 0;
   for (uint32_t i = 0; i < pass->subpass_count; i++) {
      struct anv_subpass *subpass = &pass->subpasses[i];

      /* We don't allow depth_stencil_attachment to be non-NULL and be
       * VK_ATTACHMENT_UNUSED.  This way something can just check for NULL
       * and be guaranteed that they have a valid attachment.
       */
      if (subpass->depth_stencil_attachment &&
          subpass->depth_stencil_attachment->attachment == VK_ATTACHMENT_UNUSED)
         subpass->depth_stencil_attachment = NULL;

      if (subpass->ds_resolve_attachment &&
          subpass->ds_resolve_attachment->attachment == VK_ATTACHMENT_UNUSED)
         subpass->ds_resolve_attachment = NULL;

      for (uint32_t j = 0; j < subpass->attachment_count; j++) {
         struct anv_subpass_attachment *subpass_att = &subpass->attachments[j];
         if (subpass_att->attachment == VK_ATTACHMENT_UNUSED)
            continue;

         struct anv_render_pass_attachment *pass_att =
            &pass->attachments[subpass_att->attachment];

         pass_att->usage |= subpass_att->usage;
         pass_att->last_subpass_idx = i;

         all_usage |= subpass_att->usage;

         /* first_subpass_layout only applies to color and depth.
          * See genX(cmd_buffer_setup_attachments)
          */
         if (vk_format_aspects(pass_att->format) != VK_IMAGE_ASPECT_STENCIL_BIT &&
             pass_att->first_subpass_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
            pass_att->first_subpass_layout = subpass_att->layout;
            assert(pass_att->first_subpass_layout != VK_IMAGE_LAYOUT_UNDEFINED);
         }

         if (subpass_att->usage == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT &&
             subpass->depth_stencil_attachment &&
             subpass_att->attachment == subpass->depth_stencil_attachment->attachment)
            subpass->has_ds_self_dep = true;
      }

      /* We have to handle resolve attachments specially */
      subpass->has_color_resolve = false;
      if (subpass->resolve_attachments) {
         for (uint32_t j = 0; j < subpass->color_count; j++) {
            struct anv_subpass_attachment *color_att =
               &subpass->color_attachments[j];
            struct anv_subpass_attachment *resolve_att =
               &subpass->resolve_attachments[j];
            if (resolve_att->attachment == VK_ATTACHMENT_UNUSED)
               continue;

            subpass->has_color_resolve = true;

            assert(color_att->attachment < pass->attachment_count);
            struct anv_render_pass_attachment *color_pass_att =
               &pass->attachments[color_att->attachment];

            assert(resolve_att->usage == VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            assert(color_att->usage == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            color_pass_att->usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
         }
      }

      if (subpass->ds_resolve_attachment) {
         struct anv_subpass_attachment *ds_att =
            subpass->depth_stencil_attachment;
         UNUSED struct anv_subpass_attachment *resolve_att =
            subpass->ds_resolve_attachment;

         assert(ds_att->attachment < pass->attachment_count);
         struct anv_render_pass_attachment *ds_pass_att =
            &pass->attachments[ds_att->attachment];

         assert(resolve_att->usage == VK_IMAGE_USAGE_TRANSFER_DST_BIT);
         assert(ds_att->usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
         ds_pass_att->usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      }

      for (uint32_t j = 0; j < subpass->attachment_count; j++)
         assert(__builtin_popcount(subpass->attachments[j].usage) == 1);
   }

   /* From the Vulkan 1.0.39 spec:
    *
    *    If there is no subpass dependency from VK_SUBPASS_EXTERNAL to the
    *    first subpass that uses an attachment, then an implicit subpass
    *    dependency exists from VK_SUBPASS_EXTERNAL to the first subpass it is
    *    used in. The subpass dependency operates as if defined with the
    *    following parameters:
    *
    *    VkSubpassDependency implicitDependency = {
    *        .srcSubpass = VK_SUBPASS_EXTERNAL;
    *        .dstSubpass = firstSubpass; // First subpass attachment is used in
    *        .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    *        .dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    *        .srcAccessMask = 0;
    *        .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
    *                         VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    *                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    *                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    *                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    *        .dependencyFlags = 0;
    *    };
    *
    *    Similarly, if there is no subpass dependency from the last subpass
    *    that uses an attachment to VK_SUBPASS_EXTERNAL, then an implicit
    *    subpass dependency exists from the last subpass it is used in to
    *    VK_SUBPASS_EXTERNAL. The subpass dependency operates as if defined
    *    with the following parameters:
    *
    *    VkSubpassDependency implicitDependency = {
    *        .srcSubpass = lastSubpass; // Last subpass attachment is used in
    *        .dstSubpass = VK_SUBPASS_EXTERNAL;
    *        .srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    *        .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    *        .srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
    *                         VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    *                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    *                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    *                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    *        .dstAccessMask = 0;
    *        .dependencyFlags = 0;
    *    };
    *
    * We could implement this by walking over all of the attachments and
    * subpasses and checking to see if any of them don't have an external
    * dependency.  Or, we could just be lazy and add a couple extra flushes.
    * We choose to be lazy.
    *
    * From the documentation for vkCmdNextSubpass:
    *
    *    "Moving to the next subpass automatically performs any multisample
    *    resolve operations in the subpass being ended. End-of-subpass
    *    multisample resolves are treated as color attachment writes for the
    *    purposes of synchronization. This applies to resolve operations for
    *    both color and depth/stencil attachments. That is, they are
    *    considered to execute in the
    *    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage and
    *    their writes are synchronized with
    *    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT."
    *
    * Therefore, the above flags concerning color attachments also apply to
    * color and depth/stencil resolve attachments.
    */
   if (all_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
      pass->subpass_flushes[0] |=
         ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT;
   }
   if (all_usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
      pass->subpass_flushes[pass->subpass_count] |=
         ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT;
   }
   if (all_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      pass->subpass_flushes[pass->subpass_count] |=
         ANV_PIPE_DEPTH_CACHE_FLUSH_BIT;
   }
}

static unsigned
num_subpass_attachments2(const VkSubpassDescription2KHR *desc)
{
   const VkSubpassDescriptionDepthStencilResolveKHR *ds_resolve =
      vk_find_struct_const(desc->pNext,
                           SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR);

   return desc->inputAttachmentCount +
          desc->colorAttachmentCount +
          (desc->pResolveAttachments ? desc->colorAttachmentCount : 0) +
          (desc->pDepthStencilAttachment != NULL) +
          (ds_resolve && ds_resolve->pDepthStencilResolveAttachment);
}

static bool
vk_image_layout_depth_only(VkImageLayout layout)
{
   switch (layout) {
   case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
      return true;

   default:
      return false;
   }
}

/* From the Vulkan Specification 1.2.166 - VkAttachmentReference2:
 *
 *   "If layout only specifies the layout of the depth aspect of the
 *    attachment, the layout of the stencil aspect is specified by the
 *    stencilLayout member of a VkAttachmentReferenceStencilLayout structure
 *    included in the pNext chain. Otherwise, layout describes the layout for
 *    all relevant image aspects."
 */
static VkImageLayout
stencil_ref_layout(const VkAttachmentReference2KHR *att_ref)
{
   if (!vk_image_layout_depth_only(att_ref->layout))
      return att_ref->layout;

   const VkAttachmentReferenceStencilLayoutKHR *stencil_ref =
      vk_find_struct_const(att_ref->pNext,
                           ATTACHMENT_REFERENCE_STENCIL_LAYOUT_KHR);
   if (!stencil_ref)
      return VK_IMAGE_LAYOUT_UNDEFINED;
   return stencil_ref->stencilLayout;
}

/* From the Vulkan Specification 1.2.166 - VkAttachmentDescription2:
 *
 *   "If format is a depth/stencil format, and initialLayout only specifies
 *    the initial layout of the depth aspect of the attachment, the initial
 *    layout of the stencil aspect is specified by the stencilInitialLayout
 *    member of a VkAttachmentDescriptionStencilLayout structure included in
 *    the pNext chain. Otherwise, initialLayout describes the initial layout
 *    for all relevant image aspects."
 */
static VkImageLayout
stencil_desc_layout(const VkAttachmentDescription2KHR *att_desc, bool final)
{
   if (!vk_format_has_stencil(att_desc->format))
      return VK_IMAGE_LAYOUT_UNDEFINED;

   const VkImageLayout main_layout =
      final ? att_desc->finalLayout : att_desc->initialLayout;
   if (!vk_image_layout_depth_only(main_layout))
      return main_layout;

   const VkAttachmentDescriptionStencilLayoutKHR *stencil_desc =
      vk_find_struct_const(att_desc->pNext,
                           ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT_KHR);
   assert(stencil_desc);
   return final ?
      stencil_desc->stencilFinalLayout :
      stencil_desc->stencilInitialLayout;
}

VkResult anv_CreateRenderPass2(
    VkDevice                                    _device,
    const VkRenderPassCreateInfo2KHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkRenderPass*                               pRenderPass)
{
   ANV_FROM_HANDLE(anv_device, device, _device);

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR);

   VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct anv_render_pass, pass, 1);
   VK_MULTIALLOC_DECL(&ma, struct anv_subpass, subpasses,
                           pCreateInfo->subpassCount);
   VK_MULTIALLOC_DECL(&ma, struct anv_render_pass_attachment, attachments,
                           pCreateInfo->attachmentCount);
   VK_MULTIALLOC_DECL(&ma, enum anv_pipe_bits, subpass_flushes,
                           pCreateInfo->subpassCount + 1);

   uint32_t subpass_attachment_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
      subpass_attachment_count +=
         num_subpass_attachments2(&pCreateInfo->pSubpasses[i]);
   }
   VK_MULTIALLOC_DECL(&ma, struct anv_subpass_attachment, subpass_attachments,
                      subpass_attachment_count);

   if (!vk_object_multizalloc(&device->vk, &ma, pAllocator,
                              VK_OBJECT_TYPE_RENDER_PASS))
      return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);

   /* Clear the subpasses along with the parent pass. This required because
    * each array member of anv_subpass must be a valid pointer if not NULL.
    */
   pass->attachment_count = pCreateInfo->attachmentCount;
   pass->subpass_count = pCreateInfo->subpassCount;
   pass->attachments = attachments;
   pass->subpass_flushes = subpass_flushes;

   for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
      pass->attachments[i] = (struct anv_render_pass_attachment) {
         .format                 = pCreateInfo->pAttachments[i].format,
         .samples                = pCreateInfo->pAttachments[i].samples,
         .load_op                = pCreateInfo->pAttachments[i].loadOp,
         .store_op               = pCreateInfo->pAttachments[i].storeOp,
         .stencil_load_op        = pCreateInfo->pAttachments[i].stencilLoadOp,
         .initial_layout         = pCreateInfo->pAttachments[i].initialLayout,
         .final_layout           = pCreateInfo->pAttachments[i].finalLayout,

         .stencil_initial_layout = stencil_desc_layout(&pCreateInfo->pAttachments[i],
                                                       false),
         .stencil_final_layout   = stencil_desc_layout(&pCreateInfo->pAttachments[i],
                                                       true),
      };
   }

   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
      const VkSubpassDescription2KHR *desc = &pCreateInfo->pSubpasses[i];
      struct anv_subpass *subpass = &pass->subpasses[i];

      subpass->input_count = desc->inputAttachmentCount;
      subpass->color_count = desc->colorAttachmentCount;
      subpass->attachment_count = num_subpass_attachments2(desc);
      subpass->attachments = subpass_attachments;
      subpass->view_mask = desc->viewMask;

      if (desc->inputAttachmentCount > 0) {
         subpass->input_attachments = subpass_attachments;
         subpass_attachments += desc->inputAttachmentCount;

         for (uint32_t j = 0; j < desc->inputAttachmentCount; j++) {
            subpass->input_attachments[j] = (struct anv_subpass_attachment) {
               .usage =          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
               .attachment =     desc->pInputAttachments[j].attachment,
               .layout =         desc->pInputAttachments[j].layout,
               .stencil_layout = stencil_ref_layout(&desc->pInputAttachments[j]),
            };
         }
      }

      if (desc->colorAttachmentCount > 0) {
         subpass->color_attachments = subpass_attachments;
         subpass_attachments += desc->colorAttachmentCount;

         for (uint32_t j = 0; j < desc->colorAttachmentCount; j++) {
            subpass->color_attachments[j] = (struct anv_subpass_attachment) {
               .usage =       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
               .attachment =  desc->pColorAttachments[j].attachment,
               .layout =      desc->pColorAttachments[j].layout,
            };
         }
      }

      if (desc->pResolveAttachments) {
         subpass->resolve_attachments = subpass_attachments;
         subpass_attachments += desc->colorAttachmentCount;

         for (uint32_t j = 0; j < desc->colorAttachmentCount; j++) {
            subpass->resolve_attachments[j] = (struct anv_subpass_attachment) {
               .usage =       VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               .attachment =  desc->pResolveAttachments[j].attachment,
               .layout =      desc->pResolveAttachments[j].layout,
            };
         }
      }

      if (desc->pDepthStencilAttachment) {
         subpass->depth_stencil_attachment = subpass_attachments++;

         *subpass->depth_stencil_attachment = (struct anv_subpass_attachment) {
            .usage =          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .attachment =     desc->pDepthStencilAttachment->attachment,
            .layout =         desc->pDepthStencilAttachment->layout,
            .stencil_layout = stencil_ref_layout(desc->pDepthStencilAttachment),
         };
      }

      const VkSubpassDescriptionDepthStencilResolveKHR *ds_resolve =
         vk_find_struct_const(desc->pNext,
                              SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR);

      if (ds_resolve && ds_resolve->pDepthStencilResolveAttachment) {
         subpass->ds_resolve_attachment = subpass_attachments++;

         *subpass->ds_resolve_attachment = (struct anv_subpass_attachment) {
            .usage =          VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .attachment =     ds_resolve->pDepthStencilResolveAttachment->attachment,
            .layout =         ds_resolve->pDepthStencilResolveAttachment->layout,
            .stencil_layout = stencil_ref_layout(ds_resolve->pDepthStencilResolveAttachment),
         };
         subpass->depth_resolve_mode = ds_resolve->depthResolveMode;
         subpass->stencil_resolve_mode = ds_resolve->stencilResolveMode;
      }
   }

   for (uint32_t i = 0; i < pCreateInfo->dependencyCount; i++) {
      anv_render_pass_add_subpass_dep(device, pass,
                                      &pCreateInfo->pDependencies[i]);
   }

   vk_foreach_struct(ext, pCreateInfo->pNext) {
      switch (ext->sType) {
      default:
         anv_debug_ignored_stype(ext->sType);
      }
   }

   anv_render_pass_compile(pass);

   *pRenderPass = anv_render_pass_to_handle(pass);

   return VK_SUCCESS;
}

void anv_DestroyRenderPass(
    VkDevice                                    _device,
    VkRenderPass                                _pass,
    const VkAllocationCallbacks*                pAllocator)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_render_pass, pass, _pass);

   if (!pass)
      return;

   vk_object_free(&device->vk, pAllocator, pass);
}

void anv_GetRenderAreaGranularity(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity)
{
   ANV_FROM_HANDLE(anv_render_pass, pass, renderPass);

   /* This granularity satisfies HiZ fast clear alignment requirements
    * for all sample counts.
    */
   for (unsigned i = 0; i < pass->subpass_count; ++i) {
      if (pass->subpasses[i].depth_stencil_attachment) {
         *pGranularity = (VkExtent2D) { .width = 8, .height = 4 };
         return;
      }
   }

   *pGranularity = (VkExtent2D) { 1, 1 };
}
