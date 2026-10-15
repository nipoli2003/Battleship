#ifndef APP_CONTROLLER_HPP
#define APP_CONTROLLER_HPP

#include "app/AudioManager.hpp"
#include "app/ClientState.hpp"
#include "network/Client.hpp"
#include "view/GraphicalView.hpp"
#include <memory>
#include <mutex>
#include <vector>

class AppController {
  public:
    AppController();
    void run();

    // called by view (main thread)
    void connectAndCreate(const std::string &ip, int port);
    void connectAndJoin(const std::string &ip, int port, const std::string &code);
    void sendPlacement(const std::vector<Protocol::ShipPlacement> &ships);
    void sendShot(int x, int y);
    void resetToMenu();

  private:
    void drainNetEvents();

    // network callbacks, called from listen thread
    void handleNetEvent(const NetEvent &e);

    ClientState m_state;
    GraphicalView m_view;
    AudioManager &m_audio;
    std::unique_ptr<NetworkClient> m_net;

    std::mutex m_queueMutex;
    std::vector<NetEvent> m_queue;
};

#endif
