#include "model/PlayerName.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace PlayerName {

std::optional<std::string> validate(const std::string& name) {
    if (name.empty())
        return "The name cannot be empty";
    if (static_cast<int>(name.size()) > MAX_LEN)
        return "The name cannot be longer than " + std::to_string(MAX_LEN) + " characters";
    if (name.front() == ' ' || name.back() == ' ')
        return "The name cannot start or end with a space";
    for (unsigned char c : name) {
        if (c < 32 || c > 126)
            return "The name contains invalid characters";
    }
    return std::nullopt;
}

std::optional<std::string> loadSaved(const std::string& path) {
    std::ifstream in(path);
    std::string name;
    if (!in || !std::getline(in, name))
        return std::nullopt;
    if (!name.empty() && name.back() == '\r')
        name.pop_back();
    if (validate(name))
        return std::nullopt;
    return name;
}

bool save(const std::string& name, const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return false;
    out << name << '\n';
    return static_cast<bool>(out);
}

bool sameName(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](unsigned char x, unsigned char y) {
        return std::tolower(x) == std::tolower(y);
    });
}

} // namespace PlayerName
