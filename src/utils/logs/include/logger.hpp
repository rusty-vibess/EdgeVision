#pragma once

#include <string_view>

enum class LogLevel : int { trace = 0, debug, info, warn, error, fatal };

class Logger {
  public:
    static Logger& instance();

    void set_min_level(LogLevel level) noexcept;
    LogLevel min_level() const noexcept;

    // Thread-safe. Writes one complete line atomically (C++20 osyncstream).
    void log(LogLevel level, std::string_view tag, std::string_view message) noexcept;

  private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

#define LOG(level, tag, message) ::Logger::instance().log((level), (tag), (message))
