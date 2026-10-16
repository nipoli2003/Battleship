#include "app/AppController.hpp"
#include "app/AudioManager.hpp"
#include "network/Client.hpp"
#include "raylib.h"
#include <iostream>
#include <variant>

AppController::AppController()
    : m_net(std::make_unique<NetworkClient>()) {}

AppController::~AppController() = default;

void AppController::run() {
    m_view.init();
    AudioManager::instance().init();
    m_view.setOwner(this);

    while (!m_view.shouldClose() && !m_state.shouldExit) {
        drainNetEvents();
        m_view.render(m_state);
        AudioManager::instance().update();
    }

    m_view.close();
    AudioManager::instance().close();
}

void AppController::drainNetEvents() {
    if (!m_net) return;
    while (auto ev = m_net->poll()) {
        handleNetEvent(*ev);
    }
}

void AppController::onEnterGame() {
    m_state.scene = AppScene::InGame;
    m_state.serverState = Protocol::UserState::PlacementPhase;
    m_state.placementSent = false;
    m_state.localBoard.reset();
    m_state.pendingPlacements.clear();
}

void AppController::handleNetEvent(const Network::NetEvent& ev) {
    std::visit([this](auto&& msg) {
        using T = std::decay_t<decltype(msg)>;

        if constexpr (std::is_same_v<T, Protocol::LobbyCode>) {
            m_state.lobbyCode = msg.code;
            m_state.lobbySubState = LobbySubState::ShowCode;
        }
        else if constexpr (std::is_same_v<T, Protocol::LobbyJoined>) {
            m_state.lobbySubState = LobbySubState::Waiting;
            m_state.statusMessage = "Waiting for opponent...";
        }
        else if constexpr (std::is_same_v<T, Protocol::LobbyError>) {
            m_state.lobbySubState = LobbySubState::Error;
            m_state.lobbyError = msg.error;
        }
        else if constexpr (std::is_same_v<T, Protocol::StartGame>) {
            onEnterGame();
        }
        else if constexpr (std::is_same_v<T, Protocol::StateUpdate>) {
            const bool firstUpdate = (m_state.scene != AppScene::InGame);
            m_state.serverState   = msg.state;
            m_state.statusMessage = msg.statusMessage;
            m_state.yourBoard     = msg.yourBoard;
            m_state.enemyBoard    = msg.enemyBoard;
            if (firstUpdate)
                onEnterGame();
        }
        else if constexpr (std::is_same_v<T, Protocol::GameOver>) {
            m_state.gameOver = msg;
            m_state.serverState = msg.result;
            m_state.statusMessage = msg.message;
        }
        else if constexpr (std::is_same_v<T, Protocol::ServerError>) {
            std::cerr << "[app] server error: " << msg.error << "\n";
        }
        else if constexpr (std::is_same_v<T, Network::Disconnected>) {
            m_state.connected = false;
            m_state.lobbySubState = LobbySubState::Error;
            m_state.lobbyError = msg.reason;
        }
    }, ev);
}

void AppController::connectAndCreate(const std::string& ip, int port) {
    if (!m_net) m_net = std::make_unique<NetworkClient>();
    m_state.lobbySubState = LobbySubState::Connecting;
    if (!m_net->connect(ip, port)) {
        m_state.lobbySubState = LobbySubState::Error;
        m_state.lobbyError = "Failed to connect to " + ip;
        return;
    }
    m_state.connected = true;
    m_net->createLobby();
}

void AppController::connectAndJoin(const std::string& ip, int port, const std::string& code) {
    if (!m_net) m_net = std::make_unique<NetworkClient>();
    m_state.lobbySubState = LobbySubState::Connecting;
    if (!m_net->connect(ip, port)) {
        m_state.lobbySubState = LobbySubState::Error;
        m_state.lobbyError = "Failed to connect to " + ip;
        return;
    }
    m_state.connected = true;
    m_net->joinLobby(code);
}

void AppController::sendPlacement(const std::vector<Protocol::ShipPlacement>& ships) {
    if (!m_net || !m_net->isConnected()) return;
    m_net->sendPlaceShips(ships);
    m_state.placementSent = true;
}

void AppController::sendShot(int x, int y) {
    if (!m_net || !m_net->isConnected()) return;
    m_net->sendFireShot(x, y);
}

void AppController::resetToMenu() {
    if (m_net) {
        m_net->disconnect();
        m_net.reset();
    }
    m_state = ClientState{};
    m_state.scene = AppScene::MainMenu;
}