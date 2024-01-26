#include "util_monitor.h"
#include "util_string.h"

#include "../wsi/wsi_mode.h"
#include "../wsi/wsi_monitor.h"
#include "../wsi/wsi_window.h"

#include "./log/log.h"

namespace dxvk {
  
  HMONITOR GetDefaultMonitor() {
    return wsi::getDefaultMonitor();
  }


  void GetWindowClientSize(
          HWND                    hWnd,
          UINT*                   pWidth,
          UINT*                   pHeight) {
    wsi::getWindowSize(hWnd, pWidth, pHeight);
  }


  void GetMonitorClientSize(
          HMONITOR                hMonitor,
          UINT*                   pWidth,
          UINT*                   pHeight) {
    RECT rect;

    if (!wsi::getDesktopCoordinates(hMonitor, &rect)) {
      Logger::err("D3D9: Failed to query monitor info");
      return;
    }

    if (pWidth)
      *pWidth = rect.right - rect.left;
    
    if (pHeight)
      *pHeight = rect.bottom - rect.top;
  }


  void GetMonitorRect(
          HMONITOR                hMonitor,
          RECT*                   pRect) {
    if (!wsi::getDesktopCoordinates(hMonitor, pRect))
      Logger::err("D3D9: Failed to query monitor info");
  }

}
