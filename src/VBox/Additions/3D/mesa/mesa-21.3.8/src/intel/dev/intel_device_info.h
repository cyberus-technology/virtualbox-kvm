 /*
  * Copyright © 2013 Intel Corporation
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
  *
  */

#ifndef INTEL_DEVICE_INFO_H
#define INTEL_DEVICE_INFO_H

#include <stdbool.h>
#include <stdint.h>

#include "util/macros.h"
#include "compiler/shader_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

struct drm_i915_query_topology_info;

#define INTEL_DEVICE_MAX_NAME_SIZE        64
#define INTEL_DEVICE_MAX_SLICES           (6)  /* Maximum on gfx10 */
#define INTEL_DEVICE_MAX_SUBSLICES        (8)  /* Maximum on gfx11 */
#define INTEL_DEVICE_MAX_EUS_PER_SUBSLICE (16) /* Maximum on gfx12 */
#define INTEL_DEVICE_MAX_PIXEL_PIPES      (3)  /* Maximum on gfx12 */

/**
 * Intel hardware information and quirks
 */
struct intel_device_info
{
   /* Driver internal numbers used to differentiate platforms. */
   int ver;
   int verx10;
   int display_ver;
   int revision;
   int gt;

   bool is_g4x;
   bool is_ivybridge;
   bool is_baytrail;
   bool is_haswell;
   bool is_broadwell;
   bool is_cherryview;
   bool is_skylake;
   bool is_broxton;
   bool is_kabylake;
   bool is_geminilake;
   bool is_coffeelake;
   bool is_elkhartlake;
   bool is_tigerlake;
   bool is_rocketlake;
   bool is_dg1;
   bool is_alderlake;
   bool is_dg2;

   bool has_hiz_and_separate_stencil;
   bool must_use_separate_stencil;
   bool has_sample_with_hiz;
   bool has_llc;

   bool has_pln;
   bool has_64bit_float;
   bool has_64bit_int;
   bool has_integer_dword_mul;
   bool has_compr4;
   bool has_surface_tile_offset;
   bool supports_simd16_3src;
   bool disable_ccs_repack;
   bool has_aux_map;
   bool has_tiling_uapi;
   bool has_ray_tracing;
   bool has_local_mem;
   bool has_lsc;

   /**
    * \name Intel hardware quirks
    *  @{
    */
   bool has_negative_rhw_bug;

   /**
    * Some versions of Gen hardware don't do centroid interpolation correctly
    * on unlit pixels, causing incorrect values for derivatives near triangle
    * edges.  Enabling this flag causes the fragment shader to use
    * non-centroid interpolation for unlit pixels, at the expense of two extra
    * fragment shader instructions.
    */
   bool needs_unlit_centroid_workaround;
   /** @} */

   /**
    * \name GPU hardware limits
    *
    * In general, you can find shader thread maximums by looking at the "Maximum
    * Number of Threads" field in the Intel PRM description of the 3DSTATE_VS,
    * 3DSTATE_GS, 3DSTATE_HS, 3DSTATE_DS, and 3DSTATE_PS commands. URB entry
    * limits come from the "Number of URB Entries" field in the
    * 3DSTATE_URB_VS command and friends.
    *
    * These fields are used to calculate the scratch space to allocate.  The
    * amount of scratch space can be larger without being harmful on modern
    * GPUs, however, prior to Haswell, programming the maximum number of threads
    * to greater than the hardware maximum would cause GPU performance to tank.
    *
    *  @{
    */
   /**
    * Total number of slices present on the device whether or not they've been
    * fused off.
    *
    * XXX: CS thread counts are limited by the inability to do cross subslice
    * communication. It is the effectively the number of logical threads which
    * can be executed in a subslice. Fuse configurations may cause this number
    * to change, so we program @max_cs_threads as the lower maximum.
    */
   unsigned num_slices;

   /**
    * Maximum number of slices present on this device (can be more than
    * num_slices if some slices are fused).
    */
   unsigned max_slices;

   /**
    * Number of subslices for each slice (used to be uniform until CNL).
    */
   unsigned num_subslices[INTEL_DEVICE_MAX_SUBSLICES];

   /**
    * Maximum number of subslices per slice present on this device (can be
    * more than the maximum value in the num_subslices[] array if some
    * subslices are fused).
    */
   unsigned max_subslices_per_slice;

   /**
    * Number of subslices on each pixel pipe (ICL).
    */
   unsigned ppipe_subslices[INTEL_DEVICE_MAX_PIXEL_PIPES];

   /**
    * Upper bound of number of EU per subslice (some SKUs might have just 1 EU
    * fused across all subslices, like 47 EUs, in which case this number won't
    * be acurate for one subslice).
    */
   unsigned num_eu_per_subslice;

   /**
    * Maximum number of EUs per subslice (can be more than num_eu_per_subslice
    * if some EUs are fused off).
    */
   unsigned max_eu_per_subslice;

   /**
    * Number of threads per eu, varies between 4 and 8 between generations.
    */
   unsigned num_thread_per_eu;

   /**
    * A bit mask of the slices available.
    */
   uint8_t slice_masks;

   /**
    * An array of bit mask of the subslices available, use subslice_slice_stride
    * to access this array.
    */
   uint8_t subslice_masks[INTEL_DEVICE_MAX_SLICES *
                          DIV_ROUND_UP(INTEL_DEVICE_MAX_SUBSLICES, 8)];

   /**
    * The number of enabled subslices (considering fusing). For exactly which
    * subslices are enabled, see subslice_masks[].
    */
   unsigned subslice_total;

   /**
    * An array of bit mask of EUs available, use eu_slice_stride &
    * eu_subslice_stride to access this array.
    */
   uint8_t eu_masks[INTEL_DEVICE_MAX_SLICES *
                    INTEL_DEVICE_MAX_SUBSLICES *
                    DIV_ROUND_UP(INTEL_DEVICE_MAX_EUS_PER_SUBSLICE, 8)];

   /**
    * Stride to access subslice_masks[].
    */
   uint16_t subslice_slice_stride;

   /**
    * Strides to access eu_masks[].
    */
   uint16_t eu_slice_stride;
   uint16_t eu_subslice_stride;

   unsigned l3_banks;
   unsigned max_vs_threads;   /**< Maximum Vertex Shader threads */
   unsigned max_tcs_threads;  /**< Maximum Hull Shader threads */
   unsigned max_tes_threads;  /**< Maximum Domain Shader threads */
   unsigned max_gs_threads;   /**< Maximum Geometry Shader threads. */
   /**
    * Theoretical maximum number of Pixel Shader threads.
    *
    * PSD means Pixel Shader Dispatcher. On modern Intel GPUs, hardware will
    * automatically scale pixel shader thread count, based on a single value
    * programmed into 3DSTATE_PS.
    *
    * To calculate the maximum number of threads for Gfx8 beyond (which have
    * multiple Pixel Shader Dispatchers):
    *
    * - Look up 3DSTATE_PS and find "Maximum Number of Threads Per PSD"
    * - Usually there's only one PSD per subslice, so use the number of
    *   subslices for number of PSDs.
    * - For max_wm_threads, the total should be PSD threads * #PSDs.
    */
   unsigned max_wm_threads;

   /**
    * Maximum Compute Shader threads.
    *
    * Thread count * number of EUs per subslice
    */
   unsigned max_cs_threads;

   /**
    * Maximum number of threads per workgroup supported by the GPGPU_WALKER or
    * COMPUTE_WALKER command.
    *
    * This may be smaller than max_cs_threads as it takes into account added
    * restrictions on the GPGPU/COMPUTE_WALKER commands.  While max_cs_threads
    * expresses the total parallelism of the GPU, this expresses the maximum
    * number of threads we can dispatch in a single workgroup.
    */
   unsigned max_cs_workgroup_threads;

   /**
    * The maximum number of potential scratch ids. Due to hardware
    * implementation details, the range of scratch ids may be larger than the
    * number of subslices.
    */
   unsigned max_scratch_ids[MESA_SHADER_STAGES];

   struct {
      /**
       * Fixed size of the URB.
       *
       * On Gfx6 and DG1, this is measured in KB.  Gfx4-5 instead measure
       * this in 512b blocks, as that's more convenient there.
       *
       * On most Gfx7+ platforms, the URB is a section of the L3 cache,
       * and can be resized based on the L3 programming.  For those platforms,
       * simply leave this field blank (zero) - it isn't used.
       */
      unsigned size;

      /**
       * The minimum number of URB entries.  See the 3DSTATE_URB_<XS> docs.
       */
      unsigned min_entries[4];

      /**
       * The maximum number of URB entries.  See the 3DSTATE_URB_<XS> docs.
       */
      unsigned max_entries[4];
   } urb;

   /* Maximum size in Kb that can be allocated to constants in the URB, this
    * is usually divided among the stages for implementing push constants.
    * See 3DSTATE_PUSH_CONSTANT_ALLOC_*.
    */
   unsigned max_constant_urb_size_kb;

   /**
    * Size of the command streamer prefetch. This is important to know for
    * self modifying batches.
    */
   unsigned cs_prefetch_size;

   /**
    * For the longest time the timestamp frequency for Gen's timestamp counter
    * could be assumed to be 12.5MHz, where the least significant bit neatly
    * corresponded to 80 nanoseconds.
    *
    * Since Gfx9 the numbers aren't so round, with a a frequency of 12MHz for
    * SKL (or scale factor of 83.33333333) and a frequency of 19200000Hz for
    * BXT.
    *
    * For simplicty to fit with the current code scaling by a single constant
    * to map from raw timestamps to nanoseconds we now do the conversion in
    * floating point instead of integer arithmetic.
    *
    * In general it's probably worth noting that the documented constants we
    * have for the per-platform timestamp frequencies aren't perfect and
    * shouldn't be trusted for scaling and comparing timestamps with a large
    * delta.
    *
    * E.g. with crude testing on my system using the 'correct' scale factor I'm
    * seeing a drift of ~2 milliseconds per second.
    */
   uint64_t timestamp_frequency;

   uint64_t aperture_bytes;

   /**
    * ID to put into the .aub files.
    */
   int simulator_id;

   /**
    * holds the pci device id
    */
   uint32_t chipset_id;

   /**
    * holds the name of the device
    */
   char name[INTEL_DEVICE_MAX_NAME_SIZE];

   /**
    * no_hw is true when the chipset_id pci device id has been overridden
    */
   bool no_hw;
   /** @} */
};

#ifdef GFX_VER

#define intel_device_info_is_9lp(devinfo) \
   (GFX_VER == 9 && ((devinfo)->is_broxton || (devinfo)->is_geminilake))

#else

#define intel_device_info_is_9lp(devinfo) \
   ((devinfo)->is_broxton || (devinfo)->is_geminilake)

#endif

static inline bool
intel_device_info_subslice_available(const struct intel_device_info *devinfo,
                                     int slice, int subslice)
{
   return (devinfo->subslice_masks[slice * devinfo->subslice_slice_stride +
                                   subslice / 8] & (1U << (subslice % 8))) != 0;
}

static inline bool
intel_device_info_eu_available(const struct intel_device_info *devinfo,
                               int slice, int subslice, int eu)
{
   unsigned subslice_offset = slice * devinfo->eu_slice_stride +
      subslice * devinfo->eu_subslice_stride;

   return (devinfo->eu_masks[subslice_offset + eu / 8] & (1U << eu % 8)) != 0;
}

static inline uint32_t
intel_device_info_subslice_total(const struct intel_device_info *devinfo)
{
   uint32_t total = 0;

   for (size_t i = 0; i < ARRAY_SIZE(devinfo->subslice_masks); i++) {
      total += __builtin_popcount(devinfo->subslice_masks[i]);
   }

   return total;
}

static inline uint32_t
intel_device_info_eu_total(const struct intel_device_info *devinfo)
{
   uint32_t total = 0;

   for (uint32_t i = 0; i < ARRAY_SIZE(devinfo->eu_masks); i++)
      total += __builtin_popcount(devinfo->eu_masks[i]);

   return total;
}

static inline unsigned
intel_device_info_num_dual_subslices(UNUSED
                                     const struct intel_device_info *devinfo)
{
   unreachable("TODO");
}

int intel_device_name_to_pci_device_id(const char *name);

static inline uint64_t
intel_device_info_timebase_scale(const struct intel_device_info *devinfo,
                                 uint64_t gpu_timestamp)
{
   return (1000000000ull * gpu_timestamp) / devinfo->timestamp_frequency;
}

bool intel_get_device_info_from_fd(int fh, struct intel_device_info *devinfo);
bool intel_get_device_info_from_pci_id(int pci_id,
                                       struct intel_device_info *devinfo);
int intel_get_aperture_size(int fd, uint64_t *size);

#ifdef __cplusplus
}
#endif

#endif /* INTEL_DEVICE_INFO_H */
