#include "../wsi_monitor.h"

#include "../../util/log/log.h"

#include <cstring>

namespace dxvk::wsi {

  HMONITOR getDefaultMonitor() {
    return ::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
  }

  struct MonitorEnumInfo {
    UINT      iMonitorId;
    HMONITOR  oMonitor;
  };

  static BOOL CALLBACK MonitorEnumProc(
          HMONITOR                  hmon,
          HDC                       hdc,
          LPRECT                    rect,
          LPARAM                    lp) {
    auto data = reinterpret_cast<MonitorEnumInfo*>(lp);

    if (data->iMonitorId--)
      return TRUE; /* continue */

    data->oMonitor = hmon;
    return FALSE; /* stop */
  }

  HMONITOR enumMonitors(uint32_t index) {
    MonitorEnumInfo info;
    info.iMonitorId = index;
    info.oMonitor   = nullptr;

    ::EnumDisplayMonitors(
      nullptr, nullptr, &MonitorEnumProc,
      reinterpret_cast<LPARAM>(&info));

    return info.oMonitor;
  }


  bool getDisplayName(
          HMONITOR         hMonitor,
          WCHAR            (&Name)[32]) {
    // Query monitor info to get the device name
    ::MONITORINFOEXW monInfo;
    monInfo.cbSize = sizeof(monInfo);

    if (!::GetMonitorInfoW(hMonitor, reinterpret_cast<MONITORINFO*>(&monInfo))) {
      Logger::err("Win32 WSI: getDisplayName: Failed to query monitor info");
      return false;
    }

    std::memcpy(Name, monInfo.szDevice, sizeof(Name));

    return true;
  }


  bool getDesktopCoordinates(
          HMONITOR         hMonitor,
          RECT*            pRect) {
    ::MONITORINFOEXW monInfo;
    monInfo.cbSize = sizeof(monInfo);

    if (!::GetMonitorInfoW(hMonitor, reinterpret_cast<MONITORINFO*>(&monInfo))) {
      Logger::err("Win32 WSI: getDisplayName: Failed to query monitor info");
      return false;
    }

    *pRect = monInfo.rcMonitor;

    return true;
  }

}