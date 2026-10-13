#include "network/Server.hpp"
#include <random>
#include <thread>
#include <helpers/logger.hpp>

void GameServer::run(int port) {
    sockpp::tcp_acceptor acc;
    acc.open(sockpp::inet_address(port), 4, SO_REUSEPORT);
    if (!acc) {
        // TODO: how to get error
        BS_ERROR << "could not create tcp acceptor";
        return;
    }
    BS_INFO << "linstening on port " << port;

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

void GameServer::sendLine(Client &c, const std::string &line) {
    std::lock_guard<std::mutex> lock(c.write_mutex);
    c.socket.write_n(line.c_str(), line.size());
}

void GameServer::reaper() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        for (auto it = m_registry.begin(); it != m_registry.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.created_at).count();
            if (age >= LOBBY_TTL_S) {
                BS_INFO << "[reaper] Expiring lobby " << it->first;
                if (it->second.socket) { // check: when initializing, socket is null
                    sendLine(*it->second.socket,
                             Protocol::to_json(Protocol::LobbyError{.error = "Lobby expired"}));
                    // socket closes when shared_ptr has no more references
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
    auto client = std::make_shared<Client>();
    client->socket = std::move(sock);

    std::string line = readLine(client->socket);
    if (line.empty()) {
        BS_WARN << "empty message";
        return;
    }

    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        BS_WARN << "bad message: " << line;
        return;
    }

    if (type == Protocol::CREATE_LOBBY) {
        handle_new(std::move(client));
    } else if (type == Protocol::JOIN_LOBBY) {
        auto msg = Protocol::from_json<Protocol::JoinLobby>(line);
        handle_join(std::move(client), msg.code);
    } else {
        BS_WARN << "unknown message type: " << static_cast<int>(type);
    }
}

void GameServer::handle_new(ClientPtr client) {
    std::string code;
    {
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        do {
            code = make_code();
        } while (m_registry.count(code));
        m_registry[code] = {client, std::chrono::steady_clock::now()};
    }
    sendLine(*client, Protocol::to_json(Protocol::LobbyCode{.code = code}));
    BS_INFO << "[+] Lobby created: " << code;
}

void GameServer::handle_join(ClientPtr client, const std::string &code) {
    ClientPtr p0 = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        auto it = m_registry.find(code);
        if (it != m_registry.end()) {
            p0 = it->second.socket;
            m_registry.erase(it);
        }
    }

    if (!p0) {
        sendLine(*client, Protocol::to_json(Protocol::LobbyError{.error = "Lobby not found"}));
        return;
    }

    // sendLine(*p1, Protocol::to_json(Protocol::LobbyJoined{})); // notified later in runSession
    BS_INFO << "[+] Lobby joined: " << code;
    std::thread(&GameServer::runSession, this, p0, std::move(client)).detach();
}

void GameServer::runSession(ClientPtr p0,  ClientPtr p1) {
    GameSession session;
    ClientPtr clients[2] = {p0, p1};

    sendLine(*clients[0], Protocol::to_json(Protocol::LobbyJoined{}));
    sendLine(*clients[1], Protocol::to_json(Protocol::LobbyJoined{}));
    pushToAll(session, clients);

    std::atomic<bool> done{false};
    std::mutex sessionMutex;

    auto readerFor = [&](int idx) {
        std::string buf;
        char tmp[BUFFER_SIZE];
        while (!done) {
            auto res = clients[idx]->socket.read(tmp, sizeof(tmp));
            if (!res || res.value() == 0) {
                done = true;
                // wake the other readed out of its blocking() read s.t. it can observe 'done' and return
                int other = 1 - idx;
                clients[other]->socket.shutdown(SHUT_RDWR);
                break;
            }
            buf.append(tmp, res.value());
            std::string::size_type pos;
            while ((pos = buf.find('\n')) != std::string::npos) {
                std::string line = buf.substr(0, pos);
                buf.erase(0, pos + 1);
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(sessionMutex);
                    handleLine(session, clients, idx, line);
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

    BS_INFO << "[session] ended";
    // clients[] go out of scope and are destroyed
}

void GameServer::handleLine(GameSession &session, ClientPtr clients[2], int playerIdx, const std::string &line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        return;
    }

    if (type == Protocol::PLACE_SHIPS) {
        auto msg = Protocol::from_json<Protocol::PlaceShips>(line);
        if (!session.placeShips(playerIdx, msg.ships)) {
            sendLine(*clients[playerIdx],
                     Protocol::to_json(Protocol::ServerError{.error = "Invalid placement"}));
            return;
        }
        pushToAll(session, clients);

    } else if (type == Protocol::FIRE_SHOT) {
        auto msg = Protocol::from_json<Protocol::FireShot>(line);
        // invalid shot
        if (!session.fire(playerIdx, {msg.x, msg.y})) {
            sendLine(*clients[playerIdx], Protocol::to_json(Protocol::ServerError{.error = "Invalid shot"}));
            return;
        }
        // game over
        else if (session.isOver()) {
            for (int i = 0; i < 2; ++i) {
                bool won = (i == playerIdx);
                sendLine(*clients[i], Protocol::to_json(Protocol::GameOver{
                                          .result = won ? Protocol::UserState::Victory : Protocol::UserState::Defeat,
                                          .message = won ? "You sank the enemy fleet!" : "Your fleet has been destroyed."}));
            }
        }
        // normal shot
        else {
            pushToAll(session, clients);
        }
    }
}

void GameServer::pushToAll(GameSession &session, ClientPtr clients[2]) {
    for (int i = 0; i < 2; ++i)
        sendLine(*clients[i], Protocol::to_json(session.buildSnapshot(i)));
}