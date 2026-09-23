#pragma once
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_socket.h>

constexpr int PORT        = 9000;
constexpr int BUFFER_SIZE = 4096;
constexpr int LOBBY_TTL_S = 60; // seconds before an unjoined lobby is reaped

class RelayServer {
public:
    explicit RelayServer(int port) : port_(port) {}
    void run();

private:
    struct LobbyEntry {
        sockpp::tcp_socket*                          socket;
        std::chrono::steady_clock::time_point        created_at;
    };

    int                                              port_;
    std::mutex                                       mutex_;
    std::unordered_map<std::string, LobbyEntry>      registry_;

    static std::string make_code();
    static std::string trim(std::string s);
    static void send_msg(sockpp::tcp_socket& sock, const std::string& msg);

    // relay functions (relay, without inspection)
    static void relay(sockpp::tcp_socket* src, sockpp::tcp_socket* dst);
    static void run_pair(sockpp::tcp_socket* p0, sockpp::tcp_socket* p1);

    // evicts unjoined lobbies after TTL
    void reaper() ;

    // client handshake
    void handle_client(sockpp::tcp_socket sock);
    void handle_new(sockpp::tcp_socket sock);
    void handle_join(sockpp::tcp_socket sock, const std::string& code);
};