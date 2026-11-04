#include "app/AppController.hpp"
#include "app/AudioManager.hpp"
#include "model/PlayerName.hpp"
#include "network/Client.hpp"
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
    // m_net can be dropped by an event handler (e.g. a rejected name), so re-check it every iteration
    while (m_net) {
        auto ev = m_net->poll();
        if (!ev) break;
        handleNetEvent(*ev);
    }
}

void AppController::goToMenu() {
    m_state.scene = AppScene::MainMenu;
}

void AppController::openLobbySetup() {
    m_state.scene = AppScene::OnlineLobbyWait;
    m_state.lobbySubState = LobbySubState::Setup;
    m_state.lobbyError.clear();
    m_state.nameError.clear();
}

void AppController::onEnterGame() {
    m_state.scene = AppScene::InGame;
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
        else if constexpr (std::is_same_v<T, Protocol::StateUpdate>) {
            const bool firstUpdate = (m_state.scene != AppScene::InGame);
            m_state.serverState   = msg.state;
            m_state.statusMessage = msg.statusMessage;
            m_state.yourBoard     = msg.yourBoard;
            m_state.enemyBoard    = msg.enemyBoard;
            m_state.yourName      = msg.yourName;
            m_state.opponentName  = msg.opponentName;
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
        else if constexpr (std::is_same_v<T, Protocol::NameError>) {
            // back to the setup form with the name field flagged; the server has already
            // closed this connection, so drop it and let the next attempt open a fresh one
            m_state.nameError = msg.error;
            m_state.lobbySubState = LobbySubState::Setup;
            m_pendingAction = PendingAction::None;
            m_net->disconnect();
            m_net.reset();
        }
        else if constexpr (std::is_same_v<T, Network::Disconnected>) {
            m_state.lobbySubState = LobbySubState::Error;
            m_state.lobbyError = msg.reason;
        }
        else if constexpr (std::is_same_v<T, Network::ConnectionResult>) {
            if (msg.success) {
                switch (m_pendingAction) {
                case PendingAction::CreateLobby:
                    m_net->createLobby(m_pendingName);
                    break;
                case PendingAction::JoinLobby:
                    m_net->joinLobby(m_pendingCode, m_pendingName);
                    break;
                case PendingAction::None:
                    break;
                }
                m_pendingAction = PendingAction::None;
                // scene stays OnlineLobbyWait; the server's reply drives the next transition
            } else {
                m_state.lobbySubState = LobbySubState::Error;
                m_state.lobbyError = msg.error;
            }
        }
    }, ev);
}

bool AppController::acceptName(const std::string& name) {
    if (auto err = PlayerName::validate(name)) {
        m_state.nameError = *err;
        return false;
    }
    m_state.nameError.clear();
    return true;
}

void AppController::clearNameError() {
    m_state.nameError.clear();
}

void AppController::connectAndCreate(const std::string& ip, int port, const std::string& name) {
    if (!acceptName(name)) return; // invalid name: no connection is attempted

    if (!m_net) m_net = std::make_unique<NetworkClient>();

    m_state.lobbySubState = LobbySubState::Connecting;
    m_state.lobbyError.clear();
    m_pendingAction = PendingAction::CreateLobby;
    m_pendingCode.clear();
    m_pendingName = name;

    m_net->connectAsync(ip, port);
}

void AppController::connectAndJoin(const std::string& ip, int port, const std::string& code, const std::string& name) {
    if (!acceptName(name)) return; // invalid name: no connection is attempted

    if (!m_net) m_net = std::make_unique<NetworkClient>();

    m_state.lobbySubState = LobbySubState::Connecting;
    m_state.lobbyError.clear();
    m_pendingAction = PendingAction::JoinLobby;
    m_pendingCode = code;
    m_pendingName = name;

    m_net->connectAsync(ip, port);
}

bool AppController::sendPlacement(const std::vector<Protocol::ShipPlacement>& ships) {
    if (!m_net || !m_net->isConnected()) return false;
    m_net->sendPlaceShips(ships);
    return true;
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
    const int music = m_state.musicVolume;
    const int sfx   = m_state.sfxVolume;
    m_state = ClientState{};
    m_state.scene = AppScene::MainMenu;
    m_state.musicVolume = music;
    m_state.sfxVolume   = sfx;
    m_pendingAction = PendingAction::None;
    m_pendingCode.clear();
    m_pendingName.clear();
}

void AppController::requestExit() {
    m_state.shouldExit = true;
}

void AppController::setMusicVolume(int volume) {
    m_state.musicVolume = volume;
    AudioManager::instance().setMusicVolume(volume / 100.0f);   // convert to 0.0-1.0
}

void AppController::setSFXVolume(int volume) {
    m_state.sfxVolume = volume;
    AudioManager::instance().setSFXVolume(volume / 100.0f);
}
