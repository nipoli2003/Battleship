#pragma once

#include <span>
#include <string>
#include <string_view>

// Default player names, taken from real admirals. The lobby creator and the player
// joining draw from two disjoint lists, so the defaults never collide.
namespace AdmiralNames {

[[nodiscard]] std::span<const std::string_view> creatorNames();
[[nodiscard]] std::span<const std::string_view> joinerNames();

[[nodiscard]] std::string randomCreatorName();
[[nodiscard]] std::string randomJoinerName();

} // namespace AdmiralNames
