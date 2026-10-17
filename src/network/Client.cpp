#include "network/Client.hpp"
#include <chrono> // for connect timeout
#include <iostream>
#include <utility>

// NetworkClient implementation; will not throw exceptions from any public method, including the destructor.
NetworkClient::~NetworkClient() noexcept {
    disconnect();
}

// Blocking connect. Returns false on any failure; no exception escapes.
bool NetworkClient::connect(const std::string& ip, int port) noexcept {
    if (isConnected())
        return false;

    // If a previous connect attempt is still running, wait for it.
    // (3 s worst case; simpler than trying to cancel.)
    try {
        sockpp::inet_address addr{ip, static_cast<in_port_t>(port)};
        sockpp::tcp_connector conn;
        if (!conn.connect(addr, std::chrono::seconds(3))) { // 3 s timeout
            std::cerr << "[client] timed out connecting to " << ip << ":" << port << "\n";
            return false;
        }
        m_sock = std::move(conn);
    } catch (const std::exception& e) {
        std::cerr << "[client] connect to " << ip << ":" << port
                  << " failed: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[client] connect to " << ip << ":" << port
                  << " failed: unknown error\n";
        return false;
    }

    m_connected = true;
    m_listenThread = std::thread(&NetworkClient::listenLoop, this);
    return true;
}

// Non-blocking. Spawns a worker thread that performs the connect and
// pushes a Network::ConnectionResult into the inbox when done.
void NetworkClient::connectAsync(const std::string& ip, int port) {
    // If a previous connect attempt is still running, wait for it.
    // (3 s worst case; simpler than trying to cancel.)
    if (m_connectThread.joinable())
        m_connectThread.join();

    // Spawn a new thread to perform the connect.
    m_connectThread = std::thread([this, ip, port] {
        Network::ConnectionResult result;
        // If a previous connect attempt is still running, wait for it.
        // (3 s worst case; simpler than trying to cancel.)
        try {
            sockpp::inet_address addr{ip, static_cast<in_port_t>(port)};
            sockpp::tcp_connector conn;
            if (!conn.connect(addr, std::chrono::seconds(3))) {
                result = {false, "Timed out connecting to " + ip};
            } else {
                m_sock = std::move(conn);
                m_connected = true;
                m_listenThread = std::thread(&NetworkClient::listenLoop, this);
                result = {true, ""};
            }
        } catch (const std::exception& e) {
            result = {false, e.what()};
        } catch (...) {
            result = {false, "unknown error"};
        }
        pushEvent(result);
    });
}

// Idempotent, never throws. Safe to call from the destructor or after
// a failed or incomplete connect.
void NetworkClient::disconnect() noexcept {
    // If a previous connect attempt is still running, wait for it.
    try {
        m_connected = false;

        // Close the socket first so a blocked read() in listenLoop returns.
        if (m_sock.is_open()) {
            m_sock.shutdown(SHUT_RDWR);
            m_sock.close();
        }

        // Join the listener, if any.
        if (m_listenThread.joinable())
            m_listenThread.join();

        // Join the connect worker, if any. If it's mid-connect, this waits
        // for the sockpp timeout (≤3 s). That's acceptable for teardown.
        if (m_connectThread.joinable())
            m_connectThread.join();

        pushEvent(Network::Disconnected{"local disconnect"});
    } catch (...) {
        // Never throw from disconnect; it's called from the destructor.
    }
}

void NetworkClient::sendMsg(const std::string& line) {
    if (!isConnected())
        return;
    std::lock_guard<std::mutex> lock(m_sendMutex);
    m_sock.write_n(line.c_str(), line.size());
}

void NetworkClient::createLobby() {
    sendMsg(Protocol::to_json(Protocol::CreateLobby{}));
}

void NetworkClient::joinLobby(const std::string& code) {
    sendMsg(Protocol::to_json(Protocol::JoinLobby{.code = code}));
}

void NetworkClient::sendPlaceShips(const std::vector<Protocol::ShipPlacement>& payload) {
    sendMsg(Protocol::to_json(Protocol::PlaceShips{.ships = payload}));
}

void NetworkClient::sendFireShot(int x, int y) {
    sendMsg(Protocol::to_json(Protocol::FireShot{.x = x, .y = y}));
}

// Background thread: read from the socket, parse messages, and push events to the inbox.
void NetworkClient::listenLoop() noexcept {
    try {
        std::string buf;
        char tmp[4096];
        while (m_connected.load()) {
            auto res = m_sock.read(tmp, sizeof(tmp));
            if (!res) {
                pushEvent(Network::Disconnected{"socket error"});
                m_connected = false;
                break;
            }
            if (res.value() == 0) {
                pushEvent(Network::Disconnected{"peer closed"});
                m_connected = false;
                break;
            }
            buf.append(tmp, static_cast<size_t>(res.value()));

            std::string::size_type pos;
            while ((pos = buf.find('\n')) != std::string::npos) {
                std::string line = buf.substr(0, pos);
                buf.erase(0, pos + 1);
                if (!line.empty())
                    dispatch(line);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[client] listen loop error: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[client] listen loop error: unknown\n";
    }
}

void NetworkClient::dispatch(const std::string& line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (const std::exception& e) {
        std::cerr << "[client] bad message: " << e.what() << "\n";
        return;
    }

    using MT = Protocol::MsgType;
    switch (type) {
    case MT::LOBBY_CODE:    pushEvent(Protocol::from_json<Protocol::LobbyCode>(line));    break;
    case MT::LOBBY_JOINED:  pushEvent(Protocol::LobbyJoined{});                            break;
    case MT::LOBBY_ERROR:   pushEvent(Protocol::from_json<Protocol::LobbyError>(line));   break;
    case MT::START_GAME:    pushEvent(Protocol::StartGame{});                              break;
    case MT::STATE_UPDATE:  pushEvent(Protocol::from_json<Protocol::StateUpdate>(line));  break;
    case MT::GAME_OVER:     pushEvent(Protocol::from_json<Protocol::GameOver>(line));     break;
    case MT::SERVER_ERROR:  pushEvent(Protocol::from_json<Protocol::ServerError>(line));  break;
    default:
        std::cerr << "[client] unknown message type: " << static_cast<int>(type) << "\n";
        break;
    }
}

// Thread-safe; never throws. Pushes an event to the inbox.
void NetworkClient::pushEvent(Network::NetEvent ev) noexcept {
    try {
        std::lock_guard<std::mutex> lock(m_inboxMutex);
        m_inbox.push_back(std::move(ev));
    } catch (...) {
        // bad_alloc in the queue is unrecoverable here; drop the event.
        std::cerr << "[client] push event error: unknown\n";
    }
}

std::optional<Network::NetEvent> NetworkClient::poll() {
    std::lock_guard<std::mutex> lock(m_inboxMutex);
    if (m_inbox.empty())
        return std::nullopt;
    Network::NetEvent ev = std::move(m_inbox.front());
    m_inbox.erase(m_inbox.begin());
    return ev;
}