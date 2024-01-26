#pragma once

#include <array>
#include <fstream>
#include <iostream>
#include <string>

#include "../thread.h"

namespace dxvk {
  
  enum class LogLevel : uint32_t {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    None  = 5,
  };
  
  /**
   * \brief Logger
   * 
   * Logger for one DLL. Creates a text file and
   * writes all log messages to that file.
   */
  class Logger {
    
  public:
    
    Logger(const std::string& file_name);
    ~Logger();
    
    static void trace(const std::string& message);
    static void debug(const std::string& message);
    static void info (const std::string& message);
    static void warn (const std::string& message);
    static void err  (const std::string& message);
    static void log  (LogLevel level, const std::string& message);
    
    static LogLevel logLevel() {
#ifndef VBOX
      return s_instance.m_minLevel;
#else
      return LogLevel::Info;
#endif
    }
    
  private:

#ifndef VBOX
    static Logger s_instance;
#endif

    const LogLevel m_minLevel;
    
    dxvk::mutex   m_mutex;
    std::ofstream m_fileStream;

#ifndef VBOX
    void emitMsg(LogLevel level, const std::string& message);
#endif
    
    static LogLevel getMinLogLevel();
    
    static std::string getFileName(
      const std::string& base);

  };
  
}
