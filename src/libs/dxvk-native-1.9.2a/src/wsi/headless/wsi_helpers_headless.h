#pragma once

#include "../wsi_monitor.h"

namespace dxvk {

  inline bool isDisplayValid(int32_t displayId) {
    return displayId == 0;
  }
  
}