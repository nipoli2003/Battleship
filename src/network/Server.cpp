#include "network/Server.hpp"
#include <iostream>
#include <random>
#include <thread>

void GameServer::run(int port) {
    sockpp::tcp_acceptor acc;
    acc.open(sockpp::inet_address(port), 4, SO_REUSEPORT);
    if (!acc) {
        // TODO: how to get error
        std::cerr << "Error: could not create tcp acceptor\n";
        return;
    }
    std::cout << "Relay on port " << port << "\n";

    std::thread(&GameServer::reaper, this).detach();

    while (true) {
        auto res = acc.accept();
        if (!res)
            continue;
        std::thread([this, sock = res.release()]() mutable {
            handle_client(std::move(sock));
        }).detach();
    }
}

std::string GameServer::make_code() {
    static const char chars[] = "ABCDEFXY0123456789";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

    std::string code(6, '\0');
    for (char &c : code)
        c = chars[dist(rng)];
    return code;
}

std::string GameServer::trim(std::string s) {
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

void GameServer::send_msg(sockpp::tcp_socket &sock, const std::string &msg) {
    sock.write_n(msg.c_str(), msg.size());
}

void GameServer::relay(sockpp::tcp_socket *src, sockpp::tcp_socket *dst) {
    char buf[BUFFER_SIZE];
    sockpp::result<unsigned long> n;
    while ((n = src->read(buf, sizeof(buf))) && n.value() > 0)
        dst->write_n(buf, n.value());
    dst->close();
}

void GameServer::run_pair(sockpp::tcp_socket *p0, sockpp::tcp_socket *p1) {
    std::thread t(relay, p0, p1);
    relay(p1, p0);
    t.join();
    std::cout << "[relay] Pair disconnected\n";
    delete p0;
    delete p1;
}

void GameServer::reaper() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_registry.begin(); it != m_registry.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                           now - it->second.created_at)
                           .count();
            if (age >= LOBBY_TTL_S) {
                std::cout << "[reaper] Expiring lobby " << it->first << "\n";
                if (it->second.socket) { // check: when initializing, socket is null
                    send_msg(*it->second.socket, "BYE\n");
                    it->second.socket->close();
                    delete it->second.socket;
                }
                it = m_registry.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void GameServer::handle_client(sockpp::tcp_socket sock) {
    char buf[64] = {};
    sockpp::result<unsigned long> n = sock.read(buf, sizeof(buf) - 1);
    if (!n || n.value() <= 0)
        return;

    std::string msg = trim(std::string(buf, n.value()));

    if (msg == "NEW") {
        handle_new(std::move(sock));

    } else if (msg.rfind("JOIN ", 0) == 0) {
        handle_join(std::move(sock), trim(msg.substr(5)));

    } else if (msg == "BYE") {
        send_msg(sock, "BYE\n");
        // socket closes at end of scope

    } else {
        send_msg(sock, "ERR\n");
    }
}

void GameServer::handle_new(sockpp::tcp_socket sock) {
    std::string code;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        do {
            code = make_code();
        } while (m_registry.count(code));
        m_registry[code] = {nullptr, std::chrono::steady_clock::now()};
    }
    sockpp::tcp_socket *stored = new sockpp::tcp_socket(std::move(sock));
    send_msg(*stored, "CODE " + code + "\n");
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_registry[code].socket = stored;
    }
    std::cout << "[+] Lobby created: " << code << "\n";
}

void GameServer::handle_join(sockpp::tcp_socket sock,
                             const std::string &code) {
    sockpp::tcp_socket *p0 = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_registry.find(code);
        if (it != m_registry.end()) {
            p0 = it->second.socket;
            m_registry.erase(it);
        }
    }

    if (!p0) {
        send_msg(sock, "ERR\n");
        return;
    }

    sockpp::tcp_socket *p1 = new sockpp::tcp_socket(std::move(sock));
    send_msg(*p1, "OK\n");
    std::cout << "[+] Lobby joined: " << code << "\n";
    std::thread(run_pair, p0, p1).detach();
}