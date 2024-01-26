#include "../wsi_window.h"

#include "wsi_helpers_headless.h"

#include <windows.h>
#include <wsi/native_wsi.h>

#include "../../util/util_string.h"
#include "../../util/log/log.h"

namespace dxvk::wsi {

  void getWindowSize(
        HWND      hWindow,
        uint32_t* pWidth,
        uint32_t* pHeight) {
    if (pWidth)
      *pWidth = 1024;

    if (pHeight)
      *pHeight = 1024;
  }


  void resizeWindow(
          HWND             hWindow,
          DxvkWindowState* pState,
          uint32_t         Width,
          uint32_t         Height) {
  }


  bool setWindowMode(
          HMONITOR         hMonitor,
          HWND             hWindow,
    const WsiMode*         pMode,
          bool             EnteringFullscreen) {
    const int32_t displayId    = fromHmonitor(hMonitor);

    if (!isDisplayValid(displayId))
      return false;

    return true;
  }



  bool enterFullscreenMode(
          HMONITOR         hMonitor,
          HWND             hWindow,
          DxvkWindowState* pState,
          bool             ModeSwitch) {
    const int32_t displayId    = fromHmonitor(hMonitor);

    if (!isDisplayValid(displayId))
      return false;
    return true;
  }


  bool leaveFullscreenMode(
          HWND             hWindow,
          DxvkWindowState* pState) {
    return true;
  }


  bool restoreDisplayMode(HMONITOR hMonitor) {
    const int32_t displayId = fromHmonitor(hMonitor);
    return isDisplayValid(displayId);
  }


  HMONITOR getWindowMonitor(HWND hWindow) {
    return toHmonitor(0);
  }


  bool isWindow(HWND hWindow) {
    return true;
  }

}