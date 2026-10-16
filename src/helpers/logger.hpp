#pragma once

#include <source_location>
#include <sstream>

namespace Log {

enum class Level {
    Debug,
    Info,
    Warn,
    Error
};

void setLevel(Level lvl);

struct Line {
    Line(Level lvl, const std::source_location &loc = std::source_location::current());
    ~Line();

    template <class T>
    Line &operator<<(const T &v) {
        m_ss << v;
        return *this;
    }

  private:
    Level m_lvl;
    std::source_location m_loc;
    std::ostringstream m_ss;
};

} // namespace log

#define BS_DEBUG ::Log::Line(::Log::Level::Debug)
#define BS_INFO ::Log::Line(::Log::Level::Info)
#define BS_WARN ::Log::Line(::Log::Level::Warn)
#define BS_ERROR ::Log::Line(::Log::Level::Error)
