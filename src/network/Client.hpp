#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "model/Protocol.hpp"
#include <atomic>
#include <mutex>
#include <sockpp/tcp_connector.h>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using NetEvent = std::variant<
    Protocol::LobbyCode,
    Protocol::LobbyJoined,
    Protocol::LobbyError,
    Protocol::StateUpdate,
    Protocol::GameOver>;

class NetworkClient {
  public:
    NetworkClient(std::vector<NetEvent> &queue, std::mutex &queueMutex);

    bool connect(std::string ip, int port);
    void disconnect();

    // lobby managment
    void createLobby();
    void joinLobby(std::string code);

    // gameplay
    void sendPlaceShips(const std::vector<Protocol::ShipPlacement> &payload);
    void sendFireShot(int x, int y);

    [[nodiscard]] bool isConnected() const {
        return m_connected.load();
    }

  private:
    void sendMsg(const std::string &line); // TODO: how to make sure we're sending a protocol.hpp struct?

    void listenLoop(); // runs on background thread
    void dispatch(const std::string &line);

    sockpp::tcp_socket m_sock;
    std::atomic<bool> m_connected;
    std::thread m_listenThread;

    std::vector<NetEvent> &m_queue;
    std::mutex &m_queueMutex;
};

#endif