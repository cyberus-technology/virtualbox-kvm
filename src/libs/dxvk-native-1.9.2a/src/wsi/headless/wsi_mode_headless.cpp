#include "../wsi_mode.h"

#include "wsi_helpers_headless.h"

#include <wsi/native_wsi.h>

#include "../../util/util_string.h"
#include "../../util/log/log.h"

namespace dxvk::wsi {

  static inline void setMode(WsiMode* pMode) {
    pMode->width          = 1024;
    pMode->height         = 1024;
    pMode->refreshRate    = WsiRational{60 * 1000, 1000 };
    // BPP should always be a power of two
    // to match Windows behaviour of including padding.
    pMode->bitsPerPixel   = 8;
    pMode->interlaced     = false;
  }


  bool getDisplayMode(
          HMONITOR         hMonitor,
          uint32_t         ModeNumber,
          WsiMode*         pMode) {
    const int32_t displayId    = fromHmonitor(hMonitor);

    if (!isDisplayValid(displayId))
      return false;

    setMode(pMode);
    return true;
  }


  bool getCurrentDisplayMode(
          HMONITOR         hMonitor,
          WsiMode*         pMode) {
    const int32_t displayId    = fromHmonitor(hMonitor);

    if (!isDisplayValid(displayId))
      return false;

    setMode(pMode);
    return true;
  }


  bool getDesktopDisplayMode(
          HMONITOR         hMonitor,
          WsiMode*         pMode) {
    const int32_t displayId    = fromHmonitor(hMonitor);

    if (!isDisplayValid(displayId))
      return false;

    setMode(pMode);
    return true;
  }
  
}
