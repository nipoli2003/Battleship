#include "network/Client.hpp"

bool NetworkClient::connect(std::string ip, int port) {
    if (isConnected())
        return false;

    sockpp::tcp_connector conn({ip, static_cast<in_port_t>(port)});
    if (!conn) {
        // TOOD: how to get error?
        std::cerr << "[client] connection failed\n";
        return false;
    }
    m_sock = std::move(conn);
    m_connected = true;
    m_listenThread = std::thread(&NetworkClient::listenLoop, this);
    return true;
}

void NetworkClient::disconnect() {
    m_connected = false;
    m_sock.close();
    // TODO: how to ensure thread really joins?
    m_listenThread.join();
}

void NetworkClient::sendMsg(const std::string &line) {
    if (!isConnected())
        return;
    m_sock.write_n(line.c_str(), line.size());
}

void NetworkClient::createLobby() {
    sendMsg(Protocol::to_json(Protocol::CreateLobby{}));
}

void NetworkClient::joinLobby(std::string code) {
    sendMsg(Protocol::to_json(Protocol::JoinLobby{.code = code}));
}

void NetworkClient::sendPlaceShips(const std::vector<Protocol::ShipPlacement> &payload) {
    sendMsg(Protocol::to_json(Protocol::PlaceShips{.ships = payload}));
}

void NetworkClient::sendFireShot(int x, int y) {
    sendMsg(Protocol::to_json(Protocol::FireShot{.x = x, .y = y}));
}

void NetworkClient::listenLoop() {
    std::string buf;
    char tmp[4096];
    while (isConnected()) {
        auto res = m_sock.read(tmp, sizeof(tmp));
        if (!res || res.value() == 0) {
            m_connected = false;
            break;
        }
        buf.append(tmp, res.value());

        // extract complete newline-delimited messages
        std::string::size_type pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            if (!line.empty())
                dispatch(line);
        }
    }
}

void NetworkClient::dispatch(const std::string &line) {
    Protocol::MsgType type;
    try {
        type = Protocol::peek_type(line);
    } catch (...) {
        std::cerr << "[client] bad message: " << line << std::endl;
        return;
    }

    using MT = Protocol::MsgType;
    if (type == MT::LOBBY_CODE) {
        auto msg = Protocol::from_json<Protocol::LobbyCode>(line);
        onLobbyCode(msg.code);
    } else if (type == MT::LOBBY_JOINED) {
        onLobbyJoined();
    } else if (type == MT::LOBBY_ERROR) {
        auto msg = Protocol::from_json<Protocol::LobbyError>(line);
        onLobbyError(msg.error);
    } else if (type == MT::START_GAME) {
        onStartGame();
    } else if (type == MT::STATE_UPDATE) {
        auto msg = Protocol::from_json<Protocol::StateUpdate>(line);
        onStateUpdate(msg);
    } else if (type == MT::GAME_OVER) {
        auto msg = Protocol::from_json<Protocol::GameOver>(line);
        onGameOver(msg);
    } else {
        std::cerr << "[client] unknown message: " << line << std::endl;
    }
}
