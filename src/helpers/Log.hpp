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
    inline std::string get_time() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
        std::stringstream ss;
        // Format the time as HH:MM:SS.mmm
        ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    // Get the color code for the log level
    inline const char* get_color(Level level){
        switch(level){
            case Level::INFO:  return "\033[36m"; // Cyan
            case Level::WARN:  return "\033[33m"; // Yellow
            case Level::ERROR: return "\033[31m"; // Red
            case Level::GAME:  return "\033[32m"; // Green
        }
        return "\033[0m";
    }

    // Log a message with the specified log level
    template <typename... Args>
    void log(Level lvl, Args&&... args){
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << "[" << get_time() << "] " << get_color(lvl);
        // Use fold expression to print all arguments
        // What is a fold expression? 
        //      A fold expression is a feature introduced in C++17 that allows
        //      you to apply a binary operator to a parameter pack. It simplifies
        //      the process of performing operations on multiple arguments passed
        //      to a function template. In this case, it is used to print all the
        //      arguments passed to the log function.
        (std::cout << ... << args); // No idea why this works, AI is magic
        std::cout << "\033[0m\n";
    }
} // namespace Log

// Use a unique prefix to avoid collision with raylib's LOG_INFO/LOG_ERROR 
// and syslog.h's LOG_WARNING/LOG_ERR.
#define LOG_SRV_INFO(...)  Log::log(Log::Level::INFO,  __VA_ARGS__)
#define LOG_SRV_WARN(...)  Log::log(Log::Level::WARN,  __VA_ARGS__)
#define LOG_SRV_ERR(...)   Log::log(Log::Level::ERROR, __VA_ARGS__)
#define LOG_SRV_GAME(...)  Log::log(Log::Level::GAME,  __VA_ARGS__)