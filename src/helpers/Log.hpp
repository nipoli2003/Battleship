#pragma once
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <source_location> // Requires C++20

namespace Log {
    // Log levels for logging messages
    enum class Level {
        INFO, WARN, ERROR, GAME, PLAYER1, PLAYER2
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
            case Level::INFO:    return "\033[36m"; // Cyan
            case Level::WARN:    return "\033[33m"; // Yellow
            case Level::ERROR:   return "\033[31m"; // Red
            case Level::GAME:    return "\033[32m"; // Green
            case Level::PLAYER1: return "\033[35m"; // Magenta (Player 1)
            case Level::PLAYER2: return "\033[34m"; // Blue (Player 2)
        }
        return "\033[0m";
    }

    // Get a tag string based on the level
    inline const char* get_tag(Level level) {
        switch(level) {
            case Level::PLAYER1: return "[P1] ";
            case Level::PLAYER2: return "[P2] ";
            default: return "";
        }
    }

    // Log a message with the specified log level and source location
    template <typename... Args>
    void log(Level lvl, std::source_location loc, Args&&... args){
        std::lock_guard<std::mutex> lock(get_mutex());
        
        // Extract just the filename from the full path
        std::string file_path = loc.file_name();
        size_t last_slash = file_path.find_last_of("/\\");
        std::string file = (last_slash == std::string::npos) ? file_path : file_path.substr(last_slash + 1);

        // Example output:
        // [12:34:56.789] [ERROR] [P1] [main.cpp:42] (int main()): Message here
        std::cout << "[" << get_time() << "] " 
                  << get_color(lvl) 
                  << "[" << (lvl == Level::INFO ? "INFO" : 
                             lvl == Level::WARN ? "WARN" : 
                             lvl == Level::ERROR ? "ERR" : 
                             lvl == Level::GAME ? "GAME" : "PLAYER") << "] "
                  << get_tag(lvl)
                  << "[" << file << ":" << loc.line() << "] "
                  << "(" << loc.function_name() << "): ";
                  
        (std::cout << ... << args); 
        std::cout << "\033[0m\n"; // Reset color
    }
} // namespace Log

// Use a unique prefix to avoid collision with raylib's LOG_INFO/LOG_ERROR 
// and syslog.h's LOG_WARNING/LOG_ERR.
#define LOG_SRV_INFO(...)  Log::log(Log::Level::INFO,  std::source_location::current(), __VA_ARGS__)
#define LOG_SRV_WARN(...)  Log::log(Log::Level::WARN,  std::source_location::current(), __VA_ARGS__)
#define LOG_SRV_ERR(...)   Log::log(Log::Level::ERROR, std::source_location::current(), __VA_ARGS__)
#define LOG_SRV_GAME(...)  Log::log(Log::Level::GAME,  std::source_location::current(), __VA_ARGS__)
// Macros for player-specific logging
#define LOG_P1(...)        Log::log(Log::Level::PLAYER1, std::source_location::current(), __VA_ARGS__)
#define LOG_P2(...)        Log::log(Log::Level::PLAYER2, std::source_location::current(), __VA_ARGS__)