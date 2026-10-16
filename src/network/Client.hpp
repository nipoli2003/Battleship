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
    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    bool connect(const std::string& ip, int port);
    void disconnect();

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
    void listenLoop();                       // background thread
    void dispatch(const std::string& line);  // runs on listen thread; pushes to inbox

    void pushEvent(Network::NetEvent ev);

    sockpp::tcp_socket m_sock;
    std::atomic<bool> m_connected{false};
    std::thread m_listenThread;

    std::mutex m_sendMutex;

    std::mutex m_inboxMutex;
    std::vector<Network::NetEvent> m_inbox;
};

#endif