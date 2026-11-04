#ifndef NET_EVENT_HPP
#define NET_EVENT_HPP

#include "model/Protocol.hpp"
#include <string>
#include <variant>

namespace Network {

struct Disconnected {
    std::string reason;   // "peer closed", "error", "local disconnect"
};

struct ConnectionResult {
    bool success{false};
    std::string error;    // empty on success
};

using NetEvent = std::variant<
    Protocol::LobbyCode,
    Protocol::LobbyJoined,
    Protocol::LobbyError,
    Protocol::StateUpdate,
    Protocol::GameOver,
    Protocol::ServerError,
    Protocol::NameError,
    Disconnected,
    ConnectionResult
>;

} // namespace Network

#endif