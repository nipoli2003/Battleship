#include "network/Server.hpp"
#include <iostream>
#include <random>
#include <thread>
#include "network/Log.hpp"

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
    static const char chars[] = "ABCDEXY0123456789";
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
                std::cout << "[reaper] Expiring lobby " << it->first << "\n";
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
        handle_new(std::move(client));
    } else if (type == Protocol::JOIN_LOBBY) {
        auto msg = Protocol::from_json<Protocol::JoinLobby>(line);
        handle_join(std::move(client), msg.code);
    } else {
        std::cerr << "[server] unknown message type: " << static_cast<int>(type) << std::endl;
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
    LOG_INFO("[+] Lobby created: ", code);
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
        LOG_WARN("[!] Join failed: Lobby ", code, " not found");
        sendLine(*client, Protocol::to_json(Protocol::LobbyError{.error = "Lobby not found"}));
        return;
    }

    LOG_INFO("[+] Lobby joined: ", code);
    std::thread(&GameServer::runSession, this, p0, std::move(client)).detach();
}

void GameServer::runSession(ClientPtr p0,  ClientPtr p1) {
    static std::atomic<int> next_session_id{1};
    int sessionId = next_session_id++;

    LOG_INFO("[session ", sessionId, "] Started");

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
                int other = 1 - idx;
                // tell the survivor their opponent left
                sendLine(*clients[other], Protocol::to_json(Protocol::GameOver{
                    .result = Protocol::UserState::Victory,  // or add an Abandoned state
                    .message = "Your opponent disconnected. You win by default."}));
                // then wake the survivor's blocked read
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
    // clients[] go out of scope and are destroyed

    LOG_INFO("[session ", sessionId, "] Ended");
}

void GameServer::handleLine(GameSession &session, ClientPtr clients[2], int playerIdx, const std::string &line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        LOG_WARN("[session] Bad message from player ", playerIdx);
        return;
    }

    if (type == Protocol::PLACE_SHIPS) {
        auto msg = Protocol::from_json<Protocol::PlaceShips>(line);
        if (!session.placeShips(playerIdx, msg.ships)) {
            LOG_WARN("[session] Player ", playerIdx, " invalid placement");
            sendLine(*clients[playerIdx], Protocol::to_json(Protocol::ServerError{.error = "Invalid placement"}));
            return;
        }
        LOG_INFO("[session] Player ", playerIdx, " placed ships");
        pushToAll(session, clients);

    } else if (type == Protocol::FIRE_SHOT) {
        auto msg = Protocol::from_json<Protocol::FireShot>(line);
        
        if (!session.fire(playerIdx, {msg.x, msg.y})) {
            LOG_WARN("[session] Player ", playerIdx, " invalid shot at (", msg.x, ", ", msg.y, ")");
            sendLine(*clients[playerIdx], Protocol::to_json(Protocol::ServerError{.error = "Invalid shot"}));
            return;
        }
        
        auto snapshot = session.buildSnapshot(playerIdx);
        LOG_GAME("[session] Player ", playerIdx, " fired at (", msg.x, ", ", msg.y, ") -> ", snapshot.statusMessage);

        if (session.isOver()) {
            LOG_GAME("[session] Game over! Player ", playerIdx, " wins.");
            for (int i = 0; i < 2; ++i) {
                bool won = (i == playerIdx);
                sendLine(*clients[i], Protocol::to_json(Protocol::GameOver{
                                          .result = won ? Protocol::UserState::Victory : Protocol::UserState::Defeat,
                                          .message = won ? "You sank the enemy fleet!" : "Your fleet has been destroyed."}));
            }
        } else {
            pushToAll(session, clients);
        }
    }
}

void GameServer::pushToAll(GameSession &session, ClientPtr clients[2]) {
    for (int i = 0; i < 2; ++i)
        sendLine(*clients[i], Protocol::to_json(session.buildSnapshot(i)));
}