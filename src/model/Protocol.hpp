#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <glaze/glaze.hpp>
#include <iostream>
#include <string>

namespace Protocol {

enum MsgType : int {
    // client -> server
    CREATE_LOBBY = 0,
    JOIN_LOBBY,
    PLACE_SHIPS,
    FIRE_SHOT,
    USE_ABILITY,
    // server -> client
    LOBBY_CODE,
    LOBBY_JOINED,
    LOBBY_ERROR,
    // START_GAME,
    STATE_UPDATE,
    GAME_OVER,
    SERVER_ERROR
};

// enum wrapper for Glaze (glaze needs a struct for serialization)
struct Header {
    int type{-1};
};

inline MsgType peek_type(const std::string &json) {
    Header h;
    constexpr static auto opts = glz::opts{.error_on_unknown_keys = false};
    auto err = glz::read<opts>(h, json);
    // TODO: we need to decide upon unified error handling
    if (err)
        throw std::runtime_error("[protocol]: failed to get type");
    return static_cast<MsgType>(h.type);
}

// messages
struct CreateLobby {
    MsgType type{CREATE_LOBBY};
};

struct JoinLobby {
    MsgType type{JOIN_LOBBY};
    std::string code;
};

struct ShipPlacement {
    int typeID{};
    int x{};
    int y{};
    int orientation{};
};

struct PlaceShips {
    MsgType type{PLACE_SHIPS};
    std::vector<ShipPlacement> ships;
};

struct FireShot {
    MsgType type{FIRE_SHOT};
    int x{};
    int y{};
};

enum class AbilityType : int {
    Sonar = 0,        // Destroyer
    Torpedo = 1,      // Submarine
    BombingRun = 2    // Carrier
};

inline constexpr int ABILITY_COUNT = 3;

inline constexpr int START_CHARGES[ABILITY_COUNT] = {1, 1, 3};

inline constexpr int TORPEDO_RANGE = 10; // SIZE = 10

inline constexpr int BOMBINGRUN_RANGE = 3;

struct UseAbility {
    MsgType type{USE_ABILITY};
    int abilityType{};  // Protocol::AbilityType as int
    int x{};
    int y{};
    int orientation{0}; // 0 = Horizontal, 1 = Vertical
};

struct LobbyCode {
    MsgType type{LOBBY_CODE};
    std::string code;
};

struct LobbyJoined {
    MsgType type{LOBBY_JOINED};
    int playerIdx{-1};   // 0 or 1, assigned by the server
};

struct LobbyError {
    MsgType type{LOBBY_ERROR};
    std::string error;
};

enum UserState : int{
    None,
    PlacementPhase,
    YourTurn,
    OpponentTurn,
    Victory,
    Defeat
};

struct StateUpdate {
    MsgType type{STATE_UPDATE};
    UserState state;
    int turnNumber{};
    std::string statusMessage;
    std::vector<int> yourBoard;
    std::vector<int> enemyBoard;
    std::vector<int> ability_charges;    // abilities left for the player

    std::vector<std::string> abilityLogs; // side panel helper for ability log

    int lastShooter{-1};   // -1 = no shot yet
    int lastResult{-1};    // cast from AttackResult

    int lastAbilityType{-1};
    int lastAbilityX{-1};
    int lastAbilityY{-1};
    int lastAbilityResult{0};
};

struct GameOver {
    MsgType type{GAME_OVER};
    UserState result;
    std::string message;
};

struct ServerError {
    MsgType type{SERVER_ERROR};
    std::string error;
};

// Glaze helpers
template <typename T>
std::string to_json(const T &msg) {
    std::string out;
    auto ec = glz::write_json(msg, out);
    if (ec)
        std::cerr << "[protocol] failed to serialize json: " << glz::format_error(ec, out) << std::endl;
    out += "\n"; // since we're sending in plaintext over TCP
    return out;
}

template <typename T>
T from_json(const std::string &json) {
    T out;
    auto ec = glz::read_json(out, json);
    if (ec)
        std::cerr << "[protocol] failed to parse json: " << glz::format_error(ec, json) << std::endl;
    return out;
}

} // namespace Protocol

#endif
