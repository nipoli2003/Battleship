#include "network/Server.hpp"
#include <iostream>
#include <random>
#include <thread>

void GameServer::run(int port) {
    sockpp::tcp_acceptor acc;
    acc.open(sockpp::inet_address(port), 4, SO_REUSEPORT);
    if (!acc) {
        // TODO: how to get error
        std::cerr << "[server] error: could not create tcp acceptor\n";
        return;
    }
    std::cout << "[server] linstening on port " << port << "\n";

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
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

void GameServer::sendLine(sockpp::tcp_socket &sock, const std::string &line) {
    sock.write_n(line.c_str(), line.size());
}

void GameServer::reaper() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        for (auto it = m_registry.begin(); it != m_registry.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.created_at).count();
            if (age >= LOBBY_TTL_S) {
                std::cout << "[reaper] Expiring lobby " << it->first << "\n";
                if (it->second.socket) { // check: when initializing, socket is null
                    sendLine(*it->second.socket,
                             Protocol::to_json(Protocol::LobbyError{.error = "Lobby expired"}));
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

std::string GameServer::readLine(sockpp::tcp_socket &sock) {
    std::string line;
    char ch;
    while (true) {
        auto res = sock.read(&ch, 1);
        if (!res || res.value() <= 0)
            break;
        if (ch == '\n')
            break;
        line.push_back(ch);
    }
    return line;
}

void GameServer::handle_client(sockpp::tcp_socket sock) {
    std::string line = readLine(sock);
    if (line.empty()) {
        std::cerr << "[server] empty message\n";
        return;
    }

    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        std::cerr << "[server] bad message: " << line << std::endl;
        return;
    }

    if (type == Protocol::CREATE_LOBBY) {
        handle_new(std::move(sock));
    } else if (type == Protocol::JOIN_LOBBY) {
        auto msg = Protocol::from_json<Protocol::JoinLobby>(line);
        handle_join(std::move(sock), msg.code);
    } else {
        std::cerr << "[server] unknown message type: " << static_cast<int>(type) << std::endl;
    }
}

void GameServer::handle_new(sockpp::tcp_socket sock) {
    sockpp::tcp_socket *stored = new sockpp::tcp_socket(std::move(sock));
    std::string code;
    {
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        do {
            code = make_code();
        } while (m_registry.count(code));
        m_registry[code] = {stored, std::chrono::steady_clock::now()};
    }
    sendLine(*stored, Protocol::to_json(Protocol::LobbyCode{.code = code}));
    std::cout << "[+] Lobby created: " << code << "\n";
}

void GameServer::handle_join(sockpp::tcp_socket sock,
                             const std::string &code) {
    sockpp::tcp_socket *p0 = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        auto it = m_registry.find(code);
        if (it != m_registry.end()) {
            p0 = it->second.socket;
            m_registry.erase(it);
        }
    }

    if (!p0) {
        sendLine(sock, Protocol::to_json(Protocol::LobbyError{.error = "Lobby not found"}));
        return;
    }

    sockpp::tcp_socket *p1 = new sockpp::tcp_socket(std::move(sock));
    // sendLine(*p1, Protocol::to_json(Protocol::LobbyJoined{})); // notified later in runSession
    std::cout << "[+] Lobby joined: " << code << "\n";
    std::thread(&GameServer::runSession, this, p0, p1).detach();
}

void GameServer::runSession(sockpp::tcp_socket *p0, sockpp::tcp_socket *p1) {
    GameSession session;
    sockpp::tcp_socket *sockets[2] = {p0, p1};

    sendLine(*sockets[0], Protocol::to_json(Protocol::LobbyJoined{}));
    sendLine(*sockets[1], Protocol::to_json(Protocol::LobbyJoined{}));
    pushToAll(session, sockets);

    std::atomic<bool> done{false};
    std::mutex sessionMutex;

    auto readerFor = [&](int idx) {
        std::string buf;
        char tmp[BUFFER_SIZE];
        while (!done) {
            auto res = sockets[idx]->read(tmp, sizeof(tmp));
            if (!res || res.value() == 0) {
                done = true;
                break;
            }
            buf.append(tmp, res.value());
            std::string::size_type pos;
            while ((pos = buf.find('\n')) != std::string::npos) {
                std::string line = buf.substr(0, pos);
                buf.erase(0, pos + 1);
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(sessionMutex);
                    handleLine(session, sockets, idx, line);
                }
                if (done)
                    break;
            }
        }
    };

    std::thread t0(readerFor, 0);
    std::thread t1(readerFor, 1);
    t0.join();
    t1.join();

    std::cout << "[session] ended\n";
    delete p0;
    delete p1;
}

void GameServer::handleLine(GameSession &session, sockpp::tcp_socket *sockets[2], int playerIdx, const std::string &line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        return;
    }

    if (type == Protocol::PLACE_SHIPS) {
        auto msg = Protocol::from_json<Protocol::PlaceShips>(line);
        if (!session.placeShips(playerIdx, msg.ships)) {
            sendLine(*sockets[playerIdx],
                     Protocol::to_json(Protocol::ServerError{.error = "Invalid placement"}));
            return;
        }
        pushToAll(session, sockets);

    } else if (type == Protocol::FIRE_SHOT) {
        auto msg = Protocol::from_json<Protocol::FireShot>(line);
        if (!session.fire(playerIdx, {msg.x, msg.y})) {
            sendLine(*sockets[playerIdx],
                     Protocol::to_json(Protocol::ServerError{.error = "Invalid shot"}));
            return;
        }
        pushToAll(session, sockets);
    }
}

void GameServer::pushToAll(GameSession &session, sockpp::tcp_socket *sockets[2]) {
    for (int i = 0; i < 2; ++i)
        sendLine(*sockets[i], Protocol::to_json(session.buildSnapshot(i)));
}