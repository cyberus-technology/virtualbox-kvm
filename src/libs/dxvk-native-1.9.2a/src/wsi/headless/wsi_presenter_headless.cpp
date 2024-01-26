#include "../wsi_presenter.h"

#include <wsi/native_wsi.h>

namespace dxvk::wsi {

  VkResult createSurface(
          HWND                hWindow,
    const Rc<vk::InstanceFn>& vki,
          VkSurfaceKHR*       pSurface) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

}