#include "util_env.h"

 #include <array>
#ifndef VBOX
#include <filesystem>
#endif
#include <unistd.h>
#include <limits.h>

#ifdef VBOX
# include <iprt/process.h>
# include <iprt/path.h>
#endif

namespace dxvk::env {

  std::string getExePath() {
#ifndef VBOX
    std::array<char, PATH_MAX> exePath = {};

    size_t count = readlink("/proc/self/exe", exePath.data(), exePath.size());

    return std::string(exePath.begin(), exePath.begin() + count);
#else
    char szExePath[RTPATH_MAX];
    RTProcGetExecutablePath(&szExePath[0], sizeof(szExePath));
    return std::string(&szExePath[0]);
#endif
  }
  
  
  void setThreadName(const std::string& name) {
  }


  bool createDirectory(const std::string& path) {
#ifndef VBOX
    return std::filesystem::create_directories(path);
#else
    return false;
#endif
  }

}