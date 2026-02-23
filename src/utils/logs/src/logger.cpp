#include "logger.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <syncstream>

static const char* level_to_cstr(LogLevel l) noexcept {
    switch (l) {
        case LogLevel::trace:
            return "TRACE";
        case LogLevel::debug:
            return "DEBUG";
        case LogLevel::info:
            return "INFO";
        case LogLevel::warn:
            return "WARN";
        case LogLevel::error:
            return "ERROR";
        case LogLevel::fatal:
            return "FATAL";
    }
    return "UNK";
}

static std::atomic<int> g_min_level{static_cast<int>(LogLevel::info)};

Logger& Logger::instance() {
    static Logger g;
    return g;
}

void Logger::set_min_level(LogLevel level) noexcept {
    g_min_level.store(static_cast<int>(level), std::memory_order_relaxed);
}

LogLevel Logger::min_level() const noexcept {
    return static_cast<LogLevel>(g_min_level.load(std::memory_order_relaxed));
}

static void write_timestamp(std::ostream& os) {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto sec = time_point_cast<seconds>(now);
    const auto ms = duration_cast<milliseconds>(now - sec).count();

    std::time_t tt = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0')
       << ms;
}

void Logger::log(LogLevel level, std::string_view tag, std::string_view message) noexcept {
    if (static_cast<int>(level) < g_min_level.load(std::memory_order_relaxed))
        return;

    // osyncstream groups output into a single atomic commit to the underlying stream
    std::osyncstream out(std::cerr);
    write_timestamp(out);
    out << " [" << level_to_cstr(level) << "] [" << tag << "] " << message << '\n';
}
