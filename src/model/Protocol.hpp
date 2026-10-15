#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "helpers/logger.hpp"
#include <glaze/glaze.hpp>
#include <string>

namespace Protocol {

enum MsgType : int {
    // client -> server
    CREATE_LOBBY = 0,
    JOIN_LOBBY,
    PLACE_SHIPS,
    FIRE_SHOT,
    // server -> client
    LOBBY_CODE,
    LOBBY_JOINED,
    LOBBY_ERROR,
    START_GAME,
    STATE_UPDATE,
    GAME_OVER,
    SERVER_ERROR
};

inline const std::string msg_type_to_str(MsgType type) {
    switch (type) {
    case CREATE_LOBBY: return "CREATE_LOBBY";
    case JOIN_LOBBY:   return "JOIN_LOBBY";
    case PLACE_SHIPS:  return "PLACE_SHIPS";
    case FIRE_SHOT:    return "FIRE_SHOT";
    case LOBBY_CODE:   return "LOBBY_CODE";
    case LOBBY_JOINED: return "LOBBY_JOINED";
    case LOBBY_ERROR:  return "LOBBY_ERROR";
    case START_GAME:   return "START_GAME";
    case STATE_UPDATE: return "STATE_UPDATE";
    case GAME_OVER:    return "GAME_OVER";
    case SERVER_ERROR: return "SERVER_ERROR";
    default:           return "UNKNOWN_MSG_TYPE";
    }
}

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

struct LobbyCode {
    MsgType type{LOBBY_CODE};
    std::string code;
};

struct LobbyJoined {
    MsgType type{LOBBY_JOINED};
};

struct LobbyError {
    MsgType type{LOBBY_ERROR};
    std::string error;
};

struct StartGame {
    MsgType type{START_GAME};
};

enum UserState : int {
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
        BS_WARN << "[protocol] failed to serialize json: " << glz::format_error(ec, out);
    out += "\n"; // since we're sending in plaintext over TCP
    return out;
}

template <typename T>
T from_json(const std::string &json) {
    T out;
    auto ec = glz::read_json(out, json);
    if (ec)
        BS_WARN << "[protocol] failed to parse json: " << glz::format_error(ec, json);
    return out;
}

} // namespace Protocol

#endif
