#pragma once

#include <optional>
#include <string>

namespace PlayerName {

constexpr int MAX_LEN = 16;

// Returns an error message if the name is not acceptable, std::nullopt if it is valid.
// Used by both client (before connecting) and server (never trust the client).
[[nodiscard]] std::optional<std::string> validate(const std::string& name);

// Where the chosen name is remembered between runs (relative to the working directory).
constexpr const char* SAVE_FILE = "player_name.txt";

// Returns the remembered name, or std::nullopt if there is none or it is no longer valid.
[[nodiscard]] std::optional<std::string> loadSaved(const std::string& path = SAVE_FILE);

// Remembers the name for the next run. Returns false if the file could not be written.
bool save(const std::string& name, const std::string& path = SAVE_FILE);

// Case-insensitive comparison, used to detect duplicate names inside a lobby.
[[nodiscard]] bool sameName(const std::string& a, const std::string& b);

} // namespace PlayerName
