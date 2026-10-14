#pragma once
#include "network/GameSession.hpp"
#include <chrono>
#include <memory>
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
    struct Client {
        sockpp::tcp_socket socket;
        // NOTE: right now the mutex is not actually needed, since message processing, reading cannot be at the same time
        // NOTE: since all writes go through handleLine(), but might be useful so future writes don't corrupt frames
        std::mutex write_mutex;
    };
    using ClientPtr = std::shared_ptr<Client>;
    struct LobbyEntry {
        ClientPtr socket;
        std::chrono::steady_clock::time_point created_at;
    };

    std::mutex m_lobby_mutex;
    std::unordered_map<std::string, LobbyEntry> m_registry; // all unmatched (open) lobbies

    static std::string make_code();
    static std::string trim(std::string s);

    std::string readLine(sockpp::tcp_socket &sock);
    static void sendLine(Client &c, const std::string &line);

    // evicts unjoined lobbies after TTL
    void reaper();

    // client handshake
    void handle_client(sockpp::tcp_socket sock);
    void handle_new(ClientPtr client);
    void handle_join(ClientPtr client, const std::string &code);

    // matched pair session
    void runSession(ClientPtr p0, ClientPtr p1);
    void handleLine(GameSession &session, ClientPtr sockets[2], int playerIdx, const std::string &line);
    void pushToAll(GameSession &session, ClientPtr sockets[2]);
};
