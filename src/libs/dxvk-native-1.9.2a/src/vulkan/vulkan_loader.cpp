#include "vulkan_loader.h"

#ifdef VBOX
# include <iprt/ldr.h>
# include <iprt/once.h>

# ifdef RT_OS_DARWIN
#  define VBOX_VULKAN_LIBRARY_NAME "libMoltenVK"
# else
#  define VBOX_VULKAN_LIBRARY_NAME "libvulkan.so.1"
# endif
#endif

namespace dxvk::vk {

#ifndef VBOX
  static const PFN_vkGetInstanceProcAddr GetInstanceProcAddr = vkGetInstanceProcAddr;
#else
  static PFN_vkGetInstanceProcAddr GetInstanceProcAddr = NULL;
  static RTONCE s_VkLibLoadOnce = RTONCE_INITIALIZER;

  static DECLCALLBACK(int32_t) loadVkLib(void *pvUser)
  {
    RT_NOREF(pvUser);
    dxvk::vk::GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)RTLdrGetSystemSymbol(VBOX_VULKAN_LIBRARY_NAME, "vkGetInstanceProcAddr");
    return dxvk::vk::GetInstanceProcAddr ? VINF_SUCCESS : VERR_NOT_FOUND;
  }
#endif

  PFN_vkVoidFunction LibraryLoader::sym(const char* name) const {
#ifndef VBOX
    return dxvk::vk::GetInstanceProcAddr(nullptr, name);
#else
    int rc = RTOnce(&dxvk::vk::s_VkLibLoadOnce, dxvk::vk::loadVkLib, NULL /*pvUser*/);
    if (RT_SUCCESS(rc))
      return dxvk::vk::GetInstanceProcAddr(nullptr, name);

    return NULL;
#endif
  }


  InstanceLoader::InstanceLoader(bool owned, VkInstance instance)
  : m_instance(instance), m_owned(owned) { }


  PFN_vkVoidFunction InstanceLoader::sym(const char* name) const {
    return dxvk::vk::GetInstanceProcAddr(m_instance, name);
  }


  DeviceLoader::DeviceLoader(bool owned, VkInstance instance, VkDevice device)
  : m_getDeviceProcAddr(reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      dxvk::vk::GetInstanceProcAddr(instance, "vkGetDeviceProcAddr"))),
    m_device(device), m_owned(owned) { }


  PFN_vkVoidFunction DeviceLoader::sym(const char* name) const {
    return m_getDeviceProcAddr(m_device, name);
  }


  LibraryFn::LibraryFn() { }
  LibraryFn::~LibraryFn() { }


  InstanceFn::InstanceFn(bool owned, VkInstance instance)
  : InstanceLoader(owned, instance) { }
  InstanceFn::~InstanceFn() {
    if (m_owned)
      this->vkDestroyInstance(m_instance, nullptr);
  }


  DeviceFn::DeviceFn(bool owned, VkInstance instance, VkDevice device)
  : DeviceLoader(owned, instance, device) { }
  DeviceFn::~DeviceFn() {
    if (m_owned)
      this->vkDestroyDevice(m_device, nullptr);
  }

}
