/**
 * @file Protocol.hpp
 * @brief JSON protocol definitions used by the Battleship client/server.
 */

#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <glaze/glaze.hpp>
#include <iostream>
#include <string>

namespace Protocol {

/**
 * @brief Message type identifiers used in the network protocol.
 *
 * Values are serialized as integers in the JSON payload header.
 */
enum MsgType : int {
    // client -> server
    CREATE_LOBBY = 0, ///< Request creation of a new lobby.
    JOIN_LOBBY,       ///< Request joining an existing lobby.
    PLACE_SHIPS,     ///< Send a fleet placement payload.
    FIRE_SHOT,       ///< Fire a shot at a target coordinate.
    USE_ABILITY,     ///< Trigger a special ability.
    // server -> client
    LOBBY_CODE,      ///< Server returns a newly generated lobby code.
    LOBBY_JOINED,    ///< Server confirms a successful join.
    LOBBY_ERROR,     ///< Server reports a lobby-related error.
    STATE_UPDATE,    ///< Server sends the current game snapshot.
    GAME_OVER,       ///< Server reports the result of the match.
    SERVER_ERROR     ///< Generic server error payload.
};

/**
 * @brief Supported special abilities.
 */
enum class AbilityType : int {
    Sonar = 0, ///< Reveal an enemy cell or area.
};

/**
 * @brief JSON header used to detect the message type during deserialization.
 *
 * Glaze requires a small wrapper to inspect the serialized `type` field.
 */
struct Header {
    int type{-1}; ///< Numeric message type identifier.
};

/**
 * @brief Inspects a JSON payload and extracts its protocol message type.
 * @param json Serialized JSON text.
 * @return The message type encoded in the payload.
 */
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
/**
 * @brief Request to create a fresh lobby.
 */
struct CreateLobby {
    MsgType type{CREATE_LOBBY}; ///< Message type identifier.
};

/**
 * @brief Request to join an existing lobby.
 */
struct JoinLobby {
    MsgType type{JOIN_LOBBY}; ///< Message type identifier.
    std::string code;        ///< Lobby code provided by the client.
};

/**
 * @brief Single ship placement entry.
 */
struct ShipPlacement {
    int typeID{};      ///< Ship type ID, matching the board model.
    int x{};           ///< Column coordinate of the ship start.
    int y{};           ///< Row coordinate of the ship start.
    int orientation{}; ///< Orientation encoded as an integer.
};

/**
 * @brief Request to submit a full fleet placement.
 */
struct PlaceShips {
    MsgType type{PLACE_SHIPS}; ///< Message type identifier.
    std::vector<ShipPlacement> ships; ///< Placement entries for all ships.
};

/**
 * @brief Request to fire at a target cell.
 */
struct FireShot {
    MsgType type{FIRE_SHOT}; ///< Message type identifier.
    int x{};                 ///< Target column.
    int y{};                 ///< Target row.
};

/**
 * @brief Request to trigger a special ability.
 */
struct UseAbility {
    MsgType type{USE_ABILITY}; ///< Message type identifier.
    int abilityType{};         ///< Ability encoded as a Protocol::AbilityType value.
    int x{};                  ///< Target column.
    int y{};                  ///< Target row.
};

/**
 * @brief Server response containing a generated lobby code.
 */
struct LobbyCode {
    MsgType type{LOBBY_CODE}; ///< Message type identifier.
    std::string code;         ///< Newly assigned lobby code.
};

/**
 * @brief Confirmation that the client successfully joined a lobby.
 */
struct LobbyJoined {
    MsgType type{LOBBY_JOINED}; ///< Message type identifier.
};

/**
 * @brief Error payload for lobby operations.
 */
struct LobbyError {
    MsgType type{LOBBY_ERROR}; ///< Message type identifier.
    std::string error;        ///< Error text returned by the server.
};

/**
 * @brief Player-visible game state during a match.
 */
enum UserState : int {
    None,
    PlacementPhase,
    YourTurn,
    OpponentTurn,
    Victory,
    Defeat
};

/**
 * @brief Full state snapshot sent to a player after each turn or phase change.
 */
struct StateUpdate {
    MsgType type{STATE_UPDATE}; ///< Message type identifier.
    UserState state;            ///< The player's current match state.
    int turnNumber{};           ///< Current turn counter.
    std::string statusMessage;  ///< Human-readable status for the UI.
    std::vector<int> yourBoard; ///< Board state visible to the player.
    std::vector<int> enemyBoard; ///< Opponent board state visible to the player.
};

/**
 * @brief Final match result emitted when the game ends.
 */
struct GameOver {
    MsgType type{GAME_OVER}; ///< Message type identifier.
    UserState result;        ///< Final player result.
    std::string message;     ///< Summary text for the UI.
};

/**
 * @brief Generic server-side error message.
 */
struct ServerError {
    MsgType type{SERVER_ERROR}; ///< Message type identifier.
    std::string error;         ///< Error description.
};

// Glaze helpers
/**
 * @brief Serializes a protocol message to JSON.
 * @tparam T Message type.
 * @param msg Message instance.
 * @return JSON payload with a trailing newline.
 */
template <typename T>
std::string to_json(const T &msg) {
    std::string out;
    auto ec = glz::write_json(msg, out);
    if (ec)
        std::cerr << "[protocol] failed to serialize json: " << glz::format_error(ec, out) << std::endl;
    out += "\n"; // since we're sending in plaintext over TCP
    return out;
}

/**
 * @brief Deserializes a protocol message from JSON.
 * @tparam T Message type.
 * @param json JSON payload.
 * @return Parsed message instance.
 */
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
