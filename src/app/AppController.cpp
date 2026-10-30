#include "app/AppController.hpp"
#include "app/AudioManager.hpp"
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
        if (m_mode == Mode::Local && m_localMatch) {
            m_localMatch->tick();
            if (auto snap = m_localMatch->consumeUpdate())
                applyLocalSnapshot(*snap);
        } else {
            drainNetEvents();
        }
        m_view.render(m_state);
        AudioManager::instance().update();
    }

    m_view.close();
    AudioManager::instance().close();
}

void AppController::applyLocalSnapshot(const Protocol::StateUpdate &snap) {
    m_state.serverState = snap.state;
    m_state.statusMessage = snap.statusMessage;
    m_state.yourBoard = snap.yourBoard;
    m_state.enemyBoard = snap.enemyBoard;

    if ((snap.state == Protocol::UserState::Victory || snap.state == Protocol::UserState::Defeat) && !m_state.gameOver) {
        m_state.gameOver = Protocol::GameOver{.result = snap.state, .message = snap.statusMessage};
    }
}

void AppController::drainNetEvents() {
    if (!m_net) return;
    while (auto ev = m_net->poll()) {
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
            m_state.lobbySubState = LobbySubState::Error;
            m_state.lobbyError = msg.reason;
        }
        else if constexpr (std::is_same_v<T, Network::ConnectionResult>) {
            if (msg.success) {
                switch (m_pendingAction) {
                case PendingAction::CreateLobby:
                    m_net->createLobby();
                    break;
                case PendingAction::JoinLobby:
                    m_net->joinLobby(m_pendingCode);
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

void AppController::connectAndCreate(const std::string& ip, int port) {
    if (!m_net) m_net = std::make_unique<NetworkClient>();

    m_state.lobbySubState = LobbySubState::Connecting;
    m_state.lobbyError.clear();
    m_pendingAction = PendingAction::CreateLobby;
    m_pendingCode.clear();

    m_net->connectAsync(ip, port);
}

void AppController::connectAndJoin(const std::string& ip, int port, const std::string& code) {
    if (!m_net) m_net = std::make_unique<NetworkClient>();

    m_state.lobbySubState = LobbySubState::Connecting;
    m_state.lobbyError.clear();
    m_pendingAction = PendingAction::JoinLobby;
    m_pendingCode = code;

    m_net->connectAsync(ip, port);
}

bool AppController::sendPlacement(const std::vector<Protocol::ShipPlacement> &ships) {
    if (m_mode == Mode::Local)
        return m_localMatch && m_localMatch->submitPlacement(ships);
    if (!m_net || !m_net->isConnected())
        return false;
    m_net->sendPlaceShips(ships);
    return true;
}

void AppController::sendShot(int x, int y) {
    if (m_mode == Mode::Local) {
        if (m_localMatch)
            m_localMatch->submitShot(x, y);
        return;
    }
    if (!m_net || !m_net->isConnected())
        return;
    m_net->sendFireShot(x, y);
}

void AppController::resetToMenu() {
    m_localMatch.reset();
    m_mode = Mode::Online;
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

void AppController::startLocalBotMatch() {
    if (m_net) {
        m_net->disconnect();
        m_net.reset();
    }
    m_mode = Mode::Local;
    m_localMatch = std::make_unique<LocalMatch>(0);
    m_localMatch->start();

    const int music = m_state.musicVolume;
    const int sfx = m_state.sfxVolume;
    m_state = ClientState{};
    m_state.musicVolume = music;
    m_state.sfxVolume = sfx;
    m_state.scene = AppScene::InGame;
    m_state.serverState = Protocol::UserState::PlacementPhase;

    // NOTE: first snapshot is handled by run()'s consumeUpdate() on the next frame
    // if (auto snap = m_localMatch->consumeUpdate())
    // applyLocalSnapshot(*snap);
}
