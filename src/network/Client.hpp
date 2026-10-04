#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "model/Protocol.hpp"
#include <atomic>
#include <functional>
#include <sockpp/tcp_connector.h>
#include <string>
#include <thread>
#include <vector>

class NetworkClient {
  public:
    NetworkClient(std::function<void(std::string code)> onLobbyCode, std::function<void()> onLobbyJoined,
                  std::function<void(std::string reason)> onLobbyError, std::function<void()> onStartGame,
                  std::function<void(Protocol::StateUpdate)> onStateUpdate,
                  std::function<void(Protocol::GameOver)> onGameOver)
        : onLobbyCode(onLobbyCode), onLobbyJoined(onLobbyJoined), onLobbyError(onLobbyError),
          onStartGame(onStartGame), onStateUpdate(onStateUpdate), onGameOver(onGameOver) {}

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

    // callbacks
    std::function<void(std::string code)> onLobbyCode;
    std::function<void()> onLobbyJoined;
    std::function<void(std::string reason)> onLobbyError;
    std::function<void()> onStartGame;
    std::function<void(Protocol::StateUpdate)> onStateUpdate;
    std::function<void(Protocol::GameOver)> onGameOver;
};

#endif