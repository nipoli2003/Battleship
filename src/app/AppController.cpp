#include "app/AppController.hpp"
#include "app/AudioManager.hpp"
#include "network/Client.hpp"
#include <iostream>
#include <variant>
// #include <chrono>
// #include <thread>

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
            m_state.localPlayerIdx = msg.playerIdx;
        }
        else if constexpr (std::is_same_v<T, Protocol::LobbyError>) {
            m_state.lobbySubState = LobbySubState::Error;
            m_state.lobbyError = msg.error;
        }
        else if constexpr (std::is_same_v<T, Protocol::StateUpdate>) {
            const bool hadPriorState = (m_state.scene == AppScene::InGame);

            // Did the board actually change since last update?
            bool boardChanged = false;
            if (hadPriorState &&
                msg.yourBoard.size()  == m_state.yourBoard.size() &&
                msg.enemyBoard.size() == m_state.enemyBoard.size()) {
                if (msg.yourBoard  != m_state.yourBoard)  boardChanged = true;
                if (msg.enemyBoard != m_state.enemyBoard) boardChanged = true;
            }

            if (hadPriorState && boardChanged && msg.lastShooter != -1) {
                const bool iShot = (msg.lastShooter == m_state.localPlayerIdx);
                if (!iShot) AudioManager::instance().playSFX(SoundEffect::FireSalvo);

                switch (static_cast<AttackResult>(msg.lastResult)) {
                    case AttackResult::Miss: AudioManager::instance().playSFX(SoundEffect::Miss);     break;
                    case AttackResult::Hit:  AudioManager::instance().playSFX(SoundEffect::Hit);      break;
                    case AttackResult::Sunk: AudioManager::instance().playSFX(SoundEffect::ShipSunk); break;
                    default: break;
                }
            }

            const bool firstUpdate = !hadPriorState;
            m_state.serverState    = msg.state;
            m_state.statusMessage  = msg.statusMessage;
            m_state.yourBoard      = msg.yourBoard;
            m_state.enemyBoard     = msg.enemyBoard;
            m_state.abilityCharges = msg.ability_charges;
            m_state.abilityLogs    = msg.abilityLogs;

            m_state.lastShooter = msg.lastShooter;
            m_state.lastAbilityType = msg.lastAbilityType;
            m_state.lastAbilityX = msg.lastAbilityX;
            m_state.lastAbilityY = msg.lastAbilityY;

            if (firstUpdate)
                onEnterGame();
        }
        else if constexpr (std::is_same_v<T, Protocol::GameOver>) {
            // Let the final ShipSunk sound play through before flipping the screen
            if (m_state.serverState == Protocol::UserState::YourTurn ||
                m_state.serverState == Protocol::UserState::OpponentTurn) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
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

void AppController::sendAbility(int abilityType, int x, int y, int orientation) {
    if (!m_net || !m_net->isConnected()) {
        std::cerr << "[app] cannot send ability: not connected\n";
        return;
    }

    // Play SFX immediately on the attacker's side for responsiveness
    if (abilityType == static_cast<int>(Protocol::AbilityType::Sonar)) {
        AudioManager::instance().playSFX(SoundEffect::Sonar);
    } else if (abilityType == static_cast<int>(Protocol::AbilityType::Torpedo)) {
        // To be tested
        // AudioManager::instance().playSFX(SoundEffect::Torpedo);
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        AudioManager::instance().playSFX(SoundEffect::FireSalvo);
    } else if (abilityType == static_cast<int>(Protocol::AbilityType::BombingRun)) {
        AudioManager::instance().playSFX(SoundEffect::BombingRun);
    }

    std::cerr << "[app] sending ability " << abilityType << " at (" << x << ", " << y << ") orient=" << orientation << "\n";
              
    m_net->sendUseAbility(abilityType, x, y, orientation);
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
