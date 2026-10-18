#pragma once
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace Log {
    // Log levels for logging messages
    enum class Level {
        INFO, WARN, ERROR, GAME
    };

    // Get the current time as a string
    inline std::mutex& get_mutex() {
        static std::mutex mtx;
        return mtx;
    }

    // Get the current time as a string
    void get_time() {
        return;
    }

    // Get the color code for the log level
    void get_color(Level level){
        return;
    }

    // Log a message with the specified log level
    void log(){
        return;
    }
} // namespace Log