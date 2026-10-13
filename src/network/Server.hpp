#pragma once
#include "network/GameSession.hpp"
#include <chrono>
#include <mutex>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_socket.h>
#include <unordered_map>

constexpr int BUFFER_SIZE = 4096;
constexpr int LOBBY_TTL_S = 60; // seconds before an unjoined lobby is reaped

class GameServer {
  public:
    GameServer() = default;
    void run(int port);

  private:
    struct LobbyEntry {
        sockpp::tcp_socket *socket;
        std::chrono::steady_clock::time_point created_at;
    };

    std::mutex m_lobby_mutex;
    std::unordered_map<std::string, LobbyEntry> m_registry; // all unmatched (open) lobbies

    static std::string make_code();
    static std::string trim(std::string s);

    std::string readLine(sockpp::tcp_socket &sock);
    static void sendLine(sockpp::tcp_socket &sock, const std::string &line);

    // evicts unjoined lobbies after TTL
    void reaper();

    // client handshake
    void handle_client(sockpp::tcp_socket sock);
    void handle_new(sockpp::tcp_socket sock);
    void handle_join(sockpp::tcp_socket sock, const std::string &code);

    // matched pair session
    void runSession(sockpp::tcp_socket *p0, sockpp::tcp_socket *p1);
    void handleLine(GameSession &session, sockpp::tcp_socket *sockets[2], int playerIdx, const std::string &line);
    void pushToAll(GameSession &session, sockpp::tcp_socket *sockets[2]);
};