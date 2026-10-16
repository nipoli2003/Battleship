#ifndef NET_EVENT_HPP
#define NET_EVENT_HPP

#include "model/Protocol.hpp"
#include <variant>

namespace Network {

// Synthetic event: emitted when the listen thread exits.
struct Disconnected {
    std::string reason;   // "peer closed", "error", "local disconnect"
};

using NetEvent = std::variant<
    Protocol::LobbyCode,
    Protocol::LobbyJoined,
    Protocol::LobbyError,
    Protocol::StartGame,
    Protocol::StateUpdate,
    Protocol::GameOver,
    Protocol::ServerError,
    Disconnected
>;

} // namespace Network

#endif