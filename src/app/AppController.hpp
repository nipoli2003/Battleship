#ifndef APP_CONTROLLER_HPP
#define APP_CONTROLLER_HPP

#include "app/ClientState.hpp"
#include "model/Protocol.hpp"
#include "network/NetEvent.hpp"
#include "view/GraphicalView.hpp"
#include <memory>
#include <string>
#include <vector>

class NetworkClient;

class AppController {
  public:
    AppController();
    ~AppController();

    void run();

    // --- Called by GraphicalView (main thread only) ---
    void connectAndCreate(const std::string& ip, int port);
    void connectAndJoin(const std::string& ip, int port, const std::string& code);
    void sendPlacement(const std::vector<Protocol::ShipPlacement>& ships);
    void sendShot(int x, int y);
    void resetToMenu();

  private:
    enum class PendingAction { None, CreateLobby, JoinLobby };

    void drainNetEvents();
    void handleNetEvent(const Network::NetEvent& ev);
    void onEnterGame();

    ClientState m_state;
    std::unique_ptr<NetworkClient> m_net;
    GraphicalView m_view;
    PendingAction m_pendingAction{PendingAction::None};
    std::string m_pendingCode;
};

#endif