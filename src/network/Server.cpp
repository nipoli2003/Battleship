#include "network/Server.hpp"
#include "helpers/Log.hpp"
#include <iostream>
#include <istream>
#include <random>

GameServer::GameServer(unsigned int lobby_ttl)
    : m_acceptor(m_ioc), m_lobby_ttl(lobby_ttl) {}

GameServer::~GameServer() {
    stop();
}

void GameServer::run(int port) {
    asio::error_code ec;
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(),
                                     static_cast<asio::ip::port_type>(port));
    if (m_acceptor.open(endpoint.protocol(), ec)) {
        std::cerr << "[server] open: " << ec.message() << "\n";
        return;
    }

    (void)m_acceptor.set_option(asio::socket_base::reuse_address(true), ec);

    if (m_acceptor.bind(endpoint, ec)) {
        std::cerr << "[server] bind: " << ec.message() << "\n";
        return;
    }

    if (m_acceptor.listen(asio::socket_base::max_listen_connections, ec)) {
        std::cerr << "[server] listen: " << ec.message() << "\n";
        return;
    }

    m_bound_port = m_acceptor.local_endpoint().port();
    std::cout << "[server] listening on port " << m_bound_port << "\n";

    accept_next();
    m_ioc.run();
    std::cout << "[server] stopped.\n";
}

void GameServer::stop() {
    // Post cleanup unconditionally. Safe whether run() has started or not:
    // if it hasn't, the handler runs when ioc.run() is finally called.
    asio::post(m_ioc, [this] {
        asio::error_code ec;
        (void)m_acceptor.close(ec);
        for (auto &[code, client] : m_registry) {
            if (client->lobby_timer)
                client->lobby_timer->cancel();
        }
        m_registry.clear();
        for (auto &c : m_all_clients) {
            if (c->state != Client::State::Closed) {
                if (c->lobby_timer)
                    c->lobby_timer->cancel();
                asio::error_code e;
                (void)c->socket.close(e);
                c->state = Client::State::Closed;
            }
        }
    });
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

void GameServer::accept_next() {
    m_acceptor.async_accept(
        [this](asio::error_code ec, asio::ip::tcp::socket sock) {
            on_accept(ec, std::move(sock));
        });
}

void GameServer::on_accept(asio::error_code ec, asio::ip::tcp::socket sock) {
    if (ec == asio::error::operation_aborted)
        return;
    if (!ec) {
        auto client = std::make_shared<Client>(std::move(sock));
        m_all_clients.insert(client);
        read_message(client);
    }
    accept_next();
}

// ---- read pipeline ----

void GameServer::read_message(ClientPtr c) {
    if (c->state == Client::State::Closed)
        return;
    asio::async_read_until(c->socket, c->read_buf, '\n',
                           [this, c](asio::error_code ec, std::size_t) {
                               on_read(c, ec);
                           });
}

void GameServer::on_read(ClientPtr c, asio::error_code ec) {
    if (c->state == Client::State::Closed)
        return;

    if (ec) {
        handle_disconnect(c);
        return;
    }

    std::istream is(&c->read_buf);
    std::string line;
    std::getline(is, line);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    dispatch_message(c, line);
}

void GameServer::dispatch_message(ClientPtr c, const std::string &line) {
    if (line.empty()) {
        read_message(c);
        return;
    }

    switch (c->state) {
    case Client::State::AwaitingRequest: handle_request(c, line); break;
    case Client::State::InLobby:         handle_lobby_message(c, line); break;
    case Client::State::InSession:       handle_session_message(c, line); break;
    case Client::State::Closed:          return;
    }

    if (c->state != Client::State::Closed)
        read_message(c);
}

// ---- handshake ----

void GameServer::handle_request(ClientPtr c, const std::string &line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        send_line(c, Protocol::to_json(Protocol::ServerError{.error = "Bad message"}));
        return;
    }

    if (type == Protocol::CREATE_LOBBY) {
        handle_create(c);
    } else if (type == Protocol::JOIN_LOBBY) {
        auto msg = Protocol::from_json<Protocol::JoinLobby>(line);
        handle_join(c, msg.code);
    } else {
        send_line(c, Protocol::to_json(Protocol::ServerError{
                         .error = "Expected CREATE_LOBBY or JOIN_LOBBY"}));
    }
}

void GameServer::handle_create(ClientPtr c) {
    std::string code;
    do {
        code = make_code();
    } while (m_registry.count(code));

    c->state = Client::State::InLobby;
    c->lobby_code = code;
    m_registry[code] = c;

    send_line(c, Protocol::to_json(Protocol::LobbyCode{.code = code}));
    LOG_SRV_INFO("[+] Lobby created: ", code);

    c->lobby_timer = std::make_unique<asio::steady_timer>(m_ioc);
    c->lobby_timer->expires_after(std::chrono::seconds(m_lobby_ttl));
    c->lobby_timer->async_wait([this, c, code](asio::error_code ec) {
        if (ec)
            return;
        auto it = m_registry.find(code);
        if (it == m_registry.end())
            return;
        m_registry.erase(it);
        if (c->state == Client::State::Closed)
            return;
        send_line(c, Protocol::to_json(Protocol::LobbyError{.error = "Lobby expired"}));
        LOG_SRV_INFO("[-] Lobby ", code, " expired");
        c->state = Client::State::AwaitingRequest;
        c->lobby_code.clear();
        c->lobby_timer.reset();
        // Pending read stays active; next message routes as AwaitingRequest.
    });
}

void GameServer::handle_join(ClientPtr c, const std::string &code) {
    auto it = m_registry.find(code);
    if (it == m_registry.end()) {
        LOG_SRV_WARN("[!] Join failed: Lobby ", code, " not found");
        send_line(c, Protocol::to_json(Protocol::LobbyError{.error = "Lobby not found"}));
        return;
    }

    ClientPtr creator = it->second;
    if (creator == c) {
        LOG_SRV_WARN("[!] Join failed: cannot join own lobby");
        send_line(c, Protocol::to_json(Protocol::LobbyError{
                         .error = "Cannot join your own lobby"}));
        return;
    }

    m_registry.erase(it);
    if (creator->lobby_timer) {
        creator->lobby_timer->cancel();
        creator->lobby_timer.reset();
    }
    LOG_SRV_INFO("[+] Lobby joined: ", code);
    start_session(creator, c);
}

void GameServer::handle_lobby_message(ClientPtr c, const std::string &) {
    send_line(c, Protocol::to_json(Protocol::ServerError{
                     .error = "Cannot send messages while in lobby"}));
}

void GameServer::start_session(ClientPtr p0, ClientPtr p1) {
    auto s = std::make_shared<Session>(m_ioc);
    s->id = m_next_session_id++;
    s->clients[0] = p0;
    s->clients[1] = p1;

    p0->state = Client::State::InSession;
    p1->state = Client::State::InSession;
    p0->player_idx = 0;
    p1->player_idx = 1;

    m_client_sessions[p0.get()] = s;
    m_client_sessions[p1.get()] = s;

    LOG_SRV_INFO("[session ", s->id, "] Started");

    send_line(p0, Protocol::to_json(Protocol::LobbyJoined{.playerIdx = 0}));
    send_line(p1, Protocol::to_json(Protocol::LobbyJoined{.playerIdx = 1}));
    push_state(s);
}

// ---- session ----

void GameServer::handle_session_message(ClientPtr c, const std::string &line) {
    auto it = m_client_sessions.find(c.get());
    if (it == m_client_sessions.end())
        return;
    auto s = it->second;
    if (s->over)
        return;

    int idx = c->player_idx;
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        LOG_SRV_WARN("[session] Bad message from ", idx);
        return;
    }

    if (type == Protocol::PLACE_SHIPS) {
        auto msg = Protocol::from_json<Protocol::PlaceShips>(line);
        if (!s->game.placeShips(idx, msg.ships)) {
            send_line(c, Protocol::to_json(Protocol::ServerError{.error = "Invalid placement"}));
            return;
        }
        LOG_SRV_INFO("[session] Player ", idx, " placed ships");
        push_state(s);
    } else if (type == Protocol::FIRE_SHOT) {
        auto msg = Protocol::from_json<Protocol::FireShot>(line);
        if (!s->game.fire(idx, {msg.x, msg.y})) {
            send_line(c, Protocol::to_json(Protocol::ServerError{.error = "Invalid shot"}));
            return;
        }
        LOG_SRV_GAME("[session] Player ", idx, " fired at (", msg.x, ", ", msg.y, ")");
        if (s->game.isOver()) {
            push_state(s);
            s->over = true;
            for (int i = 0; i < 2; ++i) {
                bool won = (i == idx);
                send_line(s->clients[i], Protocol::to_json(Protocol::GameOver{
                                             .result = won ? Protocol::UserState::Victory : Protocol::UserState::Defeat,
                                             .message = won ? "You sank the enemy fleet!" : "Your fleet has been destroyed."}));
            }
        } else {
            push_state(s);
        }
    } else if (type == Protocol::USE_ABILITY) {
        auto msg = Protocol::from_json<Protocol::UseAbility>(line);
        Orientation orient = (msg.orientation == 0) ? Orientation::Horizontal
                                                    : Orientation::Vertical;
        int result = s->game.useAbility(idx,
                                        static_cast<Protocol::AbilityType>(msg.abilityType),
                                        {msg.x, msg.y}, orient);
        if (result < 0) {
            send_line(c, Protocol::to_json(Protocol::ServerError{.error = "Cannot use ability"}));
            return;
        }
        if (s->game.isOver()) {
            push_state(s);
            s->over = true;
            for (int i = 0; i < 2; ++i) {
                bool won = (i == idx);
                send_line(s->clients[i], Protocol::to_json(Protocol::GameOver{
                                             .result = won ? Protocol::UserState::Victory : Protocol::UserState::Defeat,
                                             .message = won ? "You sank the enemy fleet!" : "Your fleet has been destroyed."}));
            }
        } else {
            push_state(s);
        }
    } else {
        send_line(c, Protocol::to_json(Protocol::ServerError{
                         .error = "Unknown session message type"}));
    }
}

void GameServer::handle_disconnect(ClientPtr c) {
    if (c->state == Client::State::Closed)
        return;

    if (c->state == Client::State::InLobby) {
        auto it = m_registry.find(c->lobby_code);
        if (it != m_registry.end() && it->second == c) {
            m_registry.erase(it);
            LOG_SRV_INFO("[-] Lobby ", c->lobby_code, " removed: creator disconnected");
        }
        if (c->lobby_timer) {
            c->lobby_timer->cancel();
            c->lobby_timer.reset();
        }
    } else if (c->state == Client::State::InSession) {
        auto it = m_client_sessions.find(c.get());
        if (it != m_client_sessions.end()) {
            auto s = it->second;
            if (!s->over) {
                s->over = true;
                int other = 1 - c->player_idx;
                auto &o = s->clients[other];
                if (o && o->state == Client::State::InSession) {
                    send_line(o, Protocol::to_json(Protocol::GameOver{
                                     .result = Protocol::UserState::Victory,
                                     .message = "Your opponent disconnected. You win by default."}));
                    graceful_close(o);
                }
            }
            m_client_sessions.erase(s->clients[0].get());
            m_client_sessions.erase(s->clients[1].get());
        }
    }

    close_client(c);
}

// ---- writes ----

void GameServer::send_line(ClientPtr c, std::string line) {
    if (c->state == Client::State::Closed)
        return;
    line.push_back('\n');
    c->write_queue.push_back(std::move(line));
    if (!c->writing) {
        c->writing = true;
        write_next(c);
    }
}

void GameServer::write_next(ClientPtr c) {
    if (c->state == Client::State::Closed) {
        c->writing = false;
        return;
    }
    if (c->write_queue.empty()) {
        c->writing = false;
        if (c->close_after_writes)
            close_client(c);
        return;
    }

    auto &line = c->write_queue.front();
    asio::async_write(c->socket, asio::buffer(line),
                      [this, c](asio::error_code ec, std::size_t) {
                          if (c->state == Client::State::Closed) {
                              c->writing = false;
                              return;
                          }
                          if (ec) {
                              c->writing = false;
                              handle_disconnect(c);
                              return;
                          }
                          c->write_queue.pop_front();
                          write_next(c);
                      });
}

void GameServer::graceful_close(ClientPtr c) {
    c->close_after_writes = true;
    if (!c->writing && c->write_queue.empty())
        close_client(c);
}

void GameServer::close_client(ClientPtr c) {
    if (c->state == Client::State::Closed)
        return;
    c->state = Client::State::Closed;
    asio::error_code ec;
    (void)c->socket.close(ec);
    m_all_clients.erase(c);
}

void GameServer::push_state(SessionPtr s) {
    for (int i = 0; i < 2; ++i) {
        auto &c = s->clients[i];
        if (!c || c->state == Client::State::Closed)
            continue;
        send_line(c, Protocol::to_json(s->game.buildSnapshot(i)));
    }
}
