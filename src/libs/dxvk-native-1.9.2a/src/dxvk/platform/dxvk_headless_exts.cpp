#include "../dxvk_platform_exts.h"

namespace dxvk {

  DxvkPlatformExts DxvkPlatformExts::s_instance;

  std::string_view DxvkPlatformExts::getName() {
    return "Headless WSI";
  }


  DxvkNameSet DxvkPlatformExts::getInstanceExtensions() {
    return DxvkNameSet();
  }


  DxvkNameSet DxvkPlatformExts::getDeviceExtensions(
          uint32_t      adapterId) {
    return DxvkNameSet();
  }
  

  void DxvkPlatformExts::initInstanceExtensions() {
  }


  void DxvkPlatformExts::initDeviceExtensions(
    const DxvkInstance* instance) {

  }

}