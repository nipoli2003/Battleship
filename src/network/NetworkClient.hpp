#pragma once
#include "network/Protocol.hpp"
#include <functional>
#include <mutex>
#include <queue>
#include <sockpp/tcp_connector.h>
#include <string>
#include <thread>

class NetworkClient {
public:
  NetworkClient() {
    m_connected = false;
    m_gamePhase = false;
  };

  ~NetworkClient();

  // Callbacks the view/engine register to react to incoming packets
  using OnCodeReceived = std::function<void(const std::string &code)>;
  using OnGameStart = std::function<void()>;
  using OnFireReceived = std::function<void(uint8_t x, uint8_t y)>;
  using OnShotResult =
      std::function<void(uint8_t x, uint8_t y, uint8_t result)>;
  using OnDisconnect = std::function<void()>;

  // ===== lobby handshake (plain text) =====
  // establish a connection; deletes existing connection
  bool connect(const std::string &host, int port);
  // create a new lobby; returns lobby code or "" on err
  std::string sendNew();
  // join an existing lobby by code
  void sendJoin(const std::string &code);
  // sends "BYE" and disconnects
  void sendBye();

  // game packets (binary)
  void sendFire(uint8_t x, uint8_t y);
  void sendShotResult(uint8_t x, uint8_t y, uint8_t result);
  void sendPlacementReady();

  // register callbacks
  void onCodeReceived(OnCodeReceived cb) { m_onCode = std::move(cb); }
  void onGameStart(OnGameStart cb) { m_onGameStart = std::move(cb); }
  void onFireReceived(OnFireReceived cb) { m_onFire = std::move(cb); }
  void onShotResult(OnShotResult cb) { m_onResult = std::move(cb); }
  void onDisconnect(OnDisconnect cb) { m_onDisconnect = std::move(cb); }

  // call once per frame from BattleshipEngine::update() — dispatches queued
  // callbacks on the main thread
  void poll();

  bool connected() const { return m_connected; }

private:
  void listenLoop(); // runs on background thread
  /**
   * sends plain text for negotiation with the server
   */
  std::vector<std::string> sendPlain(const std::string &msg);
  protocol::BinaryPacket sendBinary(const protocol::BinaryPacket &packet);

  void enqueue(std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push(std::move(fn));
  }

  sockpp::tcp_socket m_conn;
  bool m_connected;
  bool m_gamePhase; // false = text handshake, true = binary packets

  OnCodeReceived m_onCode;
  OnGameStart m_onGameStart;
  OnFireReceived m_onFire;
  OnShotResult m_onResult;
  OnDisconnect m_onDisconnect;

  std::thread m_listenThread;
  std::mutex m_queueMutex;
  std::queue<std::function<void()>> m_queue;
};
