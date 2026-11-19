/**
 * @file NetEvent.hpp
 * @brief Event types emitted by the network layer.
 */

#ifndef NET_EVENT_HPP
#define NET_EVENT_HPP

#include "model/Protocol.hpp"
#include <string>
#include <variant>

namespace Network {

/**
 * @brief Message describing a disconnect that occurred on the client side.
 */
struct Disconnected {
    std::string reason; ///< Disconnect cause, such as "peer closed" or "local disconnect".
};

/**
 * @brief Result of an asynchronous connection attempt.
 */
struct ConnectionResult {
    bool success{false}; ///< true when the socket connected successfully.
    std::string error;  ///< Empty on success; otherwise contains the failure reason.
};

/**
 * @brief Unified event type for all network messages received by the client.
 *
 * Each variant member corresponds to a distinct protocol payload or status event
 * that the game loop may handle.
 */
using NetEvent = std::variant<
    Protocol::LobbyCode,
    Protocol::LobbyJoined,
    Protocol::LobbyError,
    Protocol::StateUpdate,
    Protocol::GameOver,
    Protocol::ServerError,
    Disconnected,
    ConnectionResult
>;

} // namespace Network

#endif