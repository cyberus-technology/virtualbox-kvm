#undef NDEBUG

#include <stdint.h>
#include <assert.h>

#include "intel_device_info.h"

int
main(int argc, char *argv[])
{
   struct {
      uint32_t pci_id;
      const char *name;
   } chipsets[] = {
#undef CHIPSET
#define CHIPSET(id, family, family_str, str_name) { .pci_id = id, .name = str_name, },
#include "pci_ids/i965_pci_ids.h"
#include "pci_ids/iris_pci_ids.h"
   };

   for (uint32_t i = 0; i < ARRAY_SIZE(chipsets); i++) {
      struct intel_device_info devinfo = { 0, };

      assert(intel_get_device_info_from_pci_id(chipsets[i].pci_id, &devinfo));

      assert(devinfo.ver != 0);
      assert(devinfo.num_eu_per_subslice != 0);
      assert(devinfo.num_thread_per_eu != 0);
      assert(devinfo.timestamp_frequency != 0);
      assert(devinfo.cs_prefetch_size > 0);

      assert(devinfo.ver < 7 || devinfo.max_constant_urb_size_kb > 0);

      assert(devinfo.num_slices <= ARRAY_SIZE(devinfo.subslice_masks));

      assert(devinfo.num_slices <= devinfo.max_slices);
      assert(intel_device_info_subslice_total(&devinfo) <=
             (devinfo.max_slices * devinfo.max_subslices_per_slice));

      for (uint32_t s = 0; s < ARRAY_SIZE(devinfo.num_subslices); s++)
         assert(devinfo.num_subslices[s] <= devinfo.max_subslices_per_slice);

      assert(__builtin_popcount(devinfo.slice_masks) <= devinfo.max_slices);

      uint32_t total_subslices = 0;
      for (size_t i = 0; i < ARRAY_SIZE(devinfo.subslice_masks); i++)
         total_subslices += __builtin_popcount(devinfo.subslice_masks[i]);
      assert(total_subslices <=
             (devinfo.max_slices * devinfo.max_subslices_per_slice));

      assert(intel_device_info_eu_total(&devinfo) > 0);
      assert(intel_device_info_subslice_total(&devinfo) > 0);

      total_subslices = 0;
      for (uint32_t s = 0; s < devinfo.max_slices; s++)
         for (uint32_t ss = 0; ss < devinfo.max_subslices_per_slice; ss++)
            total_subslices += intel_device_info_subslice_available(&devinfo, s, ss);
      assert(total_subslices == intel_device_info_subslice_total(&devinfo));

      uint32_t total_eus = 0;
      for (uint32_t s = 0; s < devinfo.max_slices; s++)
         for (uint32_t ss = 0; ss < devinfo.max_subslices_per_slice; ss++)
            for (uint32_t eu = 0; eu < devinfo.max_eu_per_subslice; eu++)
               total_eus += intel_device_info_eu_available(&devinfo, s, ss, eu);
      assert(total_eus == intel_device_info_eu_total(&devinfo));
   }

   return 0;
}
