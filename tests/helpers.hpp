#include "network/Client.hpp"
#include "network/Server.hpp"

#include <optional>
#include <thread>

struct ServerThread {
    GameServer server;
    std::thread thread;

    explicit ServerThread(int port, unsigned int lobby_ttl = DEFAULT_LOBBY_TTL_S)
        : server(lobby_ttl) {
        thread = std::thread([this, port] { server.run(port); });
        // give the server thread a brief moment to bind port
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ~ServerThread() {
        server.stop();
        if (thread.joinable())
            thread.join();
    }
};

inline std::optional<Network::NetEvent> waitForEvent(NetworkClient &client, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (auto ev = client.poll())
            return ev;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return std::nullopt;
}
