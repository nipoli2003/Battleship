#include "network/Client.hpp"
#include <iostream>
#include <utility>

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::connect(const std::string& ip, int port) {
    if (isConnected())
        return false;

    sockpp::tcp_connector conn({ip, static_cast<in_port_t>(port)});
    if (!conn) {
        std::cerr << "[client] connection failed to " << ip << ":" << port << "\n";
        return false;
    }
    m_sock = std::move(conn);
    m_connected = true;
    m_listenThread = std::thread(&NetworkClient::listenLoop, this);
    return true;
}

void NetworkClient::disconnect() {
    const bool wasConnected = m_connected.exchange(false);
    if (!wasConnected && !m_listenThread.joinable())
        return;

    // Unblock the blocking read() in listenLoop before closing.
    // shutdown() is the portable way to do this on POSIX sockets.
    if (m_sock.is_open()) {
        m_sock.shutdown(SHUT_RDWR);
        m_sock.close();
    }

    if (m_listenThread.joinable())
        m_listenThread.join();

    pushEvent(Network::Disconnected{"local disconnect"});
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

void NetworkClient::listenLoop() {
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
    case MT::LOBBY_CODE:
        pushEvent(Protocol::from_json<Protocol::LobbyCode>(line));
        break;
    case MT::LOBBY_JOINED:
        pushEvent(Protocol::LobbyJoined{});
        break;
    case MT::LOBBY_ERROR:
        pushEvent(Protocol::from_json<Protocol::LobbyError>(line));
        break;
    case MT::START_GAME:
        pushEvent(Protocol::StartGame{});
        break;
    case MT::STATE_UPDATE:
        pushEvent(Protocol::from_json<Protocol::StateUpdate>(line));
        break;
    case MT::GAME_OVER:
        pushEvent(Protocol::from_json<Protocol::GameOver>(line));
        break;
    case MT::SERVER_ERROR:
        pushEvent(Protocol::from_json<Protocol::ServerError>(line));
        break;
    default:
        std::cerr << "[client] unknown message type: " << static_cast<int>(type) << "\n";
        break;
    }
}

void NetworkClient::pushEvent(Network::NetEvent ev) {
    std::lock_guard<std::mutex> lock(m_inboxMutex);
    m_inbox.push_back(std::move(ev));
}

std::optional<Network::NetEvent> NetworkClient::poll() {
    std::lock_guard<std::mutex> lock(m_inboxMutex);
    if (m_inbox.empty())
        return std::nullopt;
    Network::NetEvent ev = std::move(m_inbox.front());
    m_inbox.erase(m_inbox.begin());
    return ev;
}