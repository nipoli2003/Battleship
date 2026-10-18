#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "model/Protocol.hpp"
#include "network/NetEvent.hpp"
#include <atomic>
#include <mutex>
#include <optional>
#include <sockpp/tcp_connector.h>
#include <string>
#include <thread>
#include <vector>

class NetworkClient {
  public:
    NetworkClient() = default;
    ~NetworkClient() noexcept; // must not throw; disconnects if connected

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Blocking connect. Returns false on any failure; no exception escapes.
    bool connect(const std::string& ip, int port) noexcept;

    // Non-blocking. Spawns a worker thread that performs the connect and
    // pushes a Network::ConnectionResult into the inbox when done.
    void connectAsync(const std::string& ip, int port);

    // Idempotent, never throws. Safe to call from the destructor or after
    // a failed or incomplete connect.
    void disconnect() noexcept;

    // Lobby
    void createLobby();
    void joinLobby(const std::string& code);

    // Gameplay
    void sendPlaceShips(const std::vector<Protocol::ShipPlacement>& payload);
    void sendFireShot(int x, int y);

    [[nodiscard]] bool isConnected() const noexcept {
        return m_connected.load();
    }

    // Main thread: pop one pending event, if any. Never blocks.
    std::optional<Network::NetEvent> poll();

  private:
    void sendMsg(const std::string& line);   // thread-safe
    void listenLoop() noexcept;              // background thread; must not throw
    void dispatch(const std::string& line);  // runs on listen thread; pushes to inbox

    void pushEvent(Network::NetEvent ev) noexcept; // thread-safe; never throws

    sockpp::tcp_socket m_sock;
    std::atomic<bool> m_connected{false};
    std::thread m_listenThread;
    std::thread m_connectThread; // for connectAsync

    std::mutex m_sendMutex;

    std::mutex m_inboxMutex;
    std::vector<Network::NetEvent> m_inbox;
};

#endif