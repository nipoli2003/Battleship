#include "helpers/logger.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <unistd.h> // getpid

namespace Log {



namespace {
std::mutex g_mutex;
Level g_level = Level::Info;

// constexpr const char *name(Level l) {
//     switch (l) {
//     case Level::Debug: return "DEBUG";
//     case Level::Info:  return "INFO";
//     case Level::Warn:  return "WARN";
//     case Level::Error: return "ERROR";
//     }
//     return "?";
// }

constexpr const char *name(Level l) {
    switch (l) {
    case Level::Debug: return "\033[38;5;30mDEBUG\033[0m";
    case Level::Info:  return "\033[38;5;82mINFO\033[0m";
    case Level::Warn:  return "\033[38;5;208mWARN\033[0m";
    case Level::Error: return "\033[38;5;196mERROR\033[0m";
    }
    return "?";
}


std::string basename(std::string_view path) {
    auto pos = path.find_last_of('/');
    return std::string{pos == std::string_view::npos ? path : path.substr(pos + 1)};
}
} // namespace

void setLevel(Level lvl) {
    std::lock_guard lk(g_mutex);
    g_level = lvl;
}

Line::Line(Level lvl, const std::source_location &loc) : m_lvl(lvl), m_loc(loc) {}

Line::~Line() {
    if (m_lvl < g_level)
        return; // drop before touching the mutex

    std::lock_guard lk(g_mutex);

    auto now = std::chrono::system_clock::now();
    auto &os = (m_lvl >= Level::Warn) ? std::cerr : std::cout;
    os << std::format("[{:%H:%M:%S}] [{}] [pid {}] [{}:{}] {}\n",
                      std::chrono::floor<std::chrono::seconds>(now),
                      name(m_lvl), getpid(),
                      basename(m_loc.file_name()), m_loc.line(),
                      m_ss.str());
}

} // namespace log