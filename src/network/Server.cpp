#include "network/Server.hpp"
#include <iostream>
#include <random>
#include <thread>
#include "helpers/Log.hpp"

void GameServer::run(int port) {
    m_acceptor.open(sockpp::inet_address(port), 4, SO_REUSEPORT); // Use m_acceptor
    if (!m_acceptor) {
        std::cerr << "[server] error: could not create tcp acceptor\n";
        return;
    }
    std::cout << "[server] listening on port " << port << "\n";
    m_running = true;

    while (m_running) { // Check m_running
        auto res = m_acceptor.accept();
        if (!res)
            continue;
        std::thread([this, sock = res.release()]() mutable {
            handle_client(std::move(sock));
        }).detach();
    }
    std::cout << "[server] stopped.\n";
}

void GameServer::stop() {
    m_running = false;
    if (m_acceptor) {
        m_acceptor.shutdown(SHUT_RDWR);
        m_acceptor.close();
    }

    // close all active client sockets
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    for (const auto &weak_client : m_all_clients) {
        if (auto client = weak_client.lock()) {
            if (client && client->socket.is_open()) {
                client->socket.shutdown(SHUT_RDWR);
                client->socket.close();
            }
        }
    }
    m_all_clients.clear();
}

std::string GameServer::make_code() {
    static const char chars[] = "0123456789";
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

std::string GameServer::readLine(sockpp::tcp_socket &sock) {
    std::string line;
    char buf[4096];
    while (true) {
        auto res = sock.read(buf, sizeof(buf));
        if (!res || res.value() <= 0)
            break;

        for (ssize_t i = 0; i < res.value(); ++i) {
            if (buf[i] == '\n') {
                line.append(buf, i);
                return line;
            }
        }
        line.append(buf, res.value());
    }
    return line;
}

void GameServer::handle_client(sockpp::tcp_socket sock) {
    auto client = std::make_shared<Client>();
    client->socket = std::move(sock);

    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        // remove expired weak pointers
        m_all_clients.erase(
            std::remove_if(m_all_clients.begin(), m_all_clients.end(),
                           [](const auto &weak) { return weak.expired(); }),
            m_all_clients.end());
        // track the new client s.t. stop() can close it
        m_all_clients.push_back(client);
    }

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
        m_registry[code] = client;
    }
    sendLine(*client, Protocol::to_json(Protocol::LobbyCode{.code = code}));
    LOG_SRV_INFO("[+] Lobby created: ", code);

    // Wait up to TTL seconds without touching the socket
    std::unique_lock<std::mutex> lock(m_lobby_mutex);
    m_lobby_cv.wait_for(lock, std::chrono::seconds(LOBBY_TTL_S), [&] {
        return m_registry.count(code) == 0;
    });

    if (m_registry.count(code)) {
        // Timed out
        m_registry.erase(code);
        sendLine(*client, Protocol::to_json(Protocol::LobbyError{.error = "Lobby expired"}));
        LOG_SRV_INFO("[-] Lobby ", code, " expired");
    }
}

void GameServer::handle_join(ClientPtr client, const std::string &code) {
    ClientPtr p0 = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_lobby_mutex);
        auto it = m_registry.find(code);
        if (it != m_registry.end()) {
            p0 = it->second;
            m_registry.erase(it);
            m_lobby_cv.notify_all(); // Wake up handle_new
        }
    }

    if (!p0) {
        LOG_SRV_WARN("[!] Join failed: Lobby ", code, " not found");
        sendLine(*client, Protocol::to_json(Protocol::LobbyError{.error = "Lobby not found"}));
        return;
    }

    LOG_SRV_INFO("[+] Lobby joined: ", code);
    std::thread(&GameServer::runSession, this, p0, std::move(client)).detach();
}

void GameServer::runSession(ClientPtr p0,  ClientPtr p1) {
    static std::atomic<int> next_session_id{1};
    int sessionId = next_session_id++;

    LOG_SRV_INFO("[session ", sessionId, "] Started");

    GameSession session;
    ClientPtr clients[2] = {p0, p1};

    sendLine(*clients[0], Protocol::to_json(Protocol::LobbyJoined{.playerIdx = 0}));
    sendLine(*clients[1], Protocol::to_json(Protocol::LobbyJoined{.playerIdx = 1}));
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
                std::lock_guard<std::mutex> lock(sessionMutex);
                if (!session.isOver()) {
                    // tell the survivor their opponent left
                    sendLine(*clients[other], Protocol::to_json(Protocol::GameOver{
                        .result = Protocol::UserState::Victory,  // or add an Abandoned state
                        .message = "Your opponent disconnected. You win by default."}));
                }
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

    LOG_SRV_INFO("[session ", sessionId, "] Ended");
}

void GameServer::handleLine(GameSession &session, ClientPtr clients[2], int playerIdx, const std::string &line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        LOG_SRV_WARN("[session] Bad message from player ", playerIdx);
        return;
    }

    if (type == Protocol::PLACE_SHIPS) {
        auto msg = Protocol::from_json<Protocol::PlaceShips>(line);
        if (!session.placeShips(playerIdx, msg.ships)) {
            LOG_SRV_WARN("[session] Player ", playerIdx, " invalid placement");
            sendLine(*clients[playerIdx], Protocol::to_json(Protocol::ServerError{.error = "Invalid placement"}));
            return;
        }
        LOG_SRV_INFO("[session] Player ", playerIdx, " placed ships");
        pushToAll(session, clients);

    } else if (type == Protocol::FIRE_SHOT) {
        auto msg = Protocol::from_json<Protocol::FireShot>(line);
        
        if (!session.fire(playerIdx, {msg.x, msg.y})) {
            LOG_SRV_WARN("[session] Player ", playerIdx, " invalid shot at (", msg.x, ", ", msg.y, ")");
            sendLine(*clients[playerIdx], Protocol::to_json(Protocol::ServerError{.error = "Invalid shot"}));
            return;
        }
        
        auto snapshot = session.buildSnapshot(playerIdx);
        LOG_SRV_GAME("[session] Player ", playerIdx, " fired at (", msg.x, ", ", msg.y, ") -> ", snapshot.statusMessage);

        if (session.isOver()) {
            LOG_SRV_GAME("[session] Game over! Player ", playerIdx, " wins.");
            // Send the final state first, so both clients see the sunk ship on the board
            // AND have the chance to play the ShipSunk SFX before the game-over screen.
            pushToAll(session, clients);

            for (int i = 0; i < 2; ++i) {
                bool won = (i == playerIdx);
                sendLine(*clients[i], Protocol::to_json(Protocol::GameOver{
                                        .result = won ? Protocol::UserState::Victory : Protocol::UserState::Defeat,
                                        .message = won ? "You sank the enemy fleet!" : "Your fleet has been destroyed."}));
            }
        } else {
            pushToAll(session, clients);            // <-- add this
        }
    } else if (type == Protocol::USE_ABILITY) {
        auto msg = Protocol::from_json<Protocol::UseAbility>(line);

        Orientation orient = (msg.orientation == 0) ? Orientation::Horizontal
                                                    : Orientation::Vertical;

        int result = session.useAbility(playerIdx,
                                        static_cast<Protocol::AbilityType>(msg.abilityType),
                                        {msg.x, msg.y},
                                        orient);

        if (result < 0) {
            LOG_SRV_WARN("[session] Player ", playerIdx, " invalid ability use at (", msg.x, ", ", msg.y, ")");
            sendLine(*clients[playerIdx], Protocol::to_json(Protocol::ServerError{.error = "Cannot use ability"}));
            return;
        }

        // Success — log it. Include the ability name for readability.
        const char* abilityName = "Unknown";
        switch (static_cast<Protocol::AbilityType>(msg.abilityType)) {
            case Protocol::AbilityType::Sonar:   abilityName = "Sonar";   break;
            case Protocol::AbilityType::Torpedo: abilityName = "Torpedo"; break;
            case Protocol::AbilityType::BombingRun: abilityName = "Bombing Run"; break;
        }
        LOG_SRV_GAME("[session] Player ", playerIdx, " used ", abilityName, " at (", msg.x, ", ", msg.y, ") -> result=", result);

        // Same pattern as FIRE_SHOT: if the ability ended the game
        // (e.g., torpedo sank the final ship), send the final StateUpdate
        // first, then GameOver. Otherwise just push the new state.
        if (session.isOver()) {
            LOG_SRV_GAME("[session] Game over! Player ", playerIdx, " wins via ", abilityName, ".");
            pushToAll(session, clients);

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