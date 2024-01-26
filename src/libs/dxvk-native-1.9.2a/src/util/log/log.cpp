#ifdef VBOX
#include <iprt/log.h>
#endif

#include "log.h"

#include "../util_env.h"

namespace dxvk {
  
  Logger::Logger(const std::string& file_name)
  : m_minLevel(getMinLogLevel()) {
    if (m_minLevel != LogLevel::None) {
      auto path = getFileName(file_name);

      if (!path.empty()) {
#ifdef _WIN32
        m_fileStream = std::ofstream(str::tows(path.c_str()).c_str());
#else
        m_fileStream = std::ofstream(path.c_str());
#endif
      }
    }
  }
  
  
  Logger::~Logger() { }
  
  
  void Logger::trace(const std::string& message) {
#ifndef VBOX
    s_instance.emitMsg(LogLevel::Trace, message);
#else
    LogRel2(("%s", message.c_str()));
#endif
  }
  
  
  void Logger::debug(const std::string& message) {
#ifndef VBOX
    s_instance.emitMsg(LogLevel::Debug, message);
#else
    LogFlow(("%s", message.c_str()));
#endif
  }
  
  
  void Logger::info(const std::string& message) {
#ifndef VBOX
    s_instance.emitMsg(LogLevel::Info, message);
#else
    Log(("%s", message.c_str()));
#endif
  }
  
  
  void Logger::warn(const std::string& message) {
#ifndef VBOX
    s_instance.emitMsg(LogLevel::Warn, message);
#else
    LogRel(("%s", message.c_str()));
#endif
  }
  
  
  void Logger::err(const std::string& message) {
#ifndef VBOX
    s_instance.emitMsg(LogLevel::Error, message);
#else
    LogRel(("%s", message.c_str()));
#endif
  }
  
  
  void Logger::log(LogLevel level, const std::string& message) {
#ifndef VBOX
    s_instance.emitMsg(level, message);
#else
    Log(("%s", message.c_str()));
#endif
  }
  
#ifndef VBOX
  void Logger::emitMsg(LogLevel level, const std::string& message) {
    if (level >= m_minLevel) {
      std::lock_guard<dxvk::mutex> lock(m_mutex);
      
      static std::array<const char*, 5> s_prefixes
        = {{ "trace: ", "debug: ", "info:  ", "warn:  ", "err:   " }};
      
      const char* prefix = s_prefixes.at(static_cast<uint32_t>(level));

      std::stringstream stream(message);
      std::string       line;

      while (std::getline(stream, line, '\n')) {
        std::cerr << prefix << line << std::endl;

        if (m_fileStream)
          m_fileStream << prefix << line << std::endl;
      }
    }
  }
#endif
  
  LogLevel Logger::getMinLogLevel() {
#ifndef VBOX
    const std::array<std::pair<const char*, LogLevel>, 6> logLevels = {{
      { "trace", LogLevel::Trace },
      { "debug", LogLevel::Debug },
      { "info",  LogLevel::Info  },
      { "warn",  LogLevel::Warn  },
      { "error", LogLevel::Error },
      { "none",  LogLevel::None  },
    }};
    
    const std::string logLevelStr = env::getEnvVar("DXVK_LOG_LEVEL");
    
    for (const auto& pair : logLevels) {
      if (logLevelStr == pair.first)
        return pair.second;
    }
#endif
    return LogLevel::Info;
  }
  
  
  std::string Logger::getFileName(const std::string& base) {
#ifndef VBOX
    std::string path = env::getEnvVar("DXVK_LOG_PATH");
    
    if (path == "none")
      return "";

    if (!path.empty() && *path.rbegin() != '/')
      path += '/';

    std::string exeName = env::getExeBaseName();
    path += exeName + "_" + base;
    return path;
#else
    return "";
#endif
  }
  
}
