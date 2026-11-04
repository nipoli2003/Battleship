#include "model/AdmiralNames.hpp"
#include <array>
#include <random>

namespace AdmiralNames {

namespace {

// All names must stay within PlayerName::MAX_LEN and be plain ASCII (checked by the tests).
constexpr std::array<std::string_view, 20> CREATOR_NAMES = {
    "Nelson", "Nimitz", "Halsey", "Spruance", "Jellicoe",
    "Beatty", "Cunningham", "Fisher", "Hood", "Rodney",
    "Collingwood", "Anson", "Hawke", "Benbow", "Farragut",
    "Dewey", "Mahan", "Fletcher", "Kinkaid", "Mountbatten"};

constexpr std::array<std::string_view, 20> JOINER_NAMES = {
    "Yamamoto", "Togo", "Nagumo", "Yi Sun-sin", "Doenitz",
    "Raeder", "Tirpitz", "Scheer", "Hipper", "Ushakov",
    "Nakhimov", "Kolchak", "De Ruyter", "Tromp", "Villeneuve",
    "Gravina", "Barbarossa", "Doria", "Suffren", "Tourville"};

std::string pickRandom(std::span<const std::string_view> names) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<size_t> dist(0, names.size() - 1);
    return std::string(names[dist(rng)]);
}

} // namespace

std::span<const std::string_view> creatorNames() { return CREATOR_NAMES; }
std::span<const std::string_view> joinerNames() { return JOINER_NAMES; }

std::string randomCreatorName() { return pickRandom(CREATOR_NAMES); }
std::string randomJoinerName() { return pickRandom(JOINER_NAMES); }

} // namespace AdmiralNames
