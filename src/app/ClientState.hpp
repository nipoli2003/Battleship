#ifndef CLIENT_STATE_HPP
#define CLIENT_STATE_HPP

#include "controller/GameState.hpp"
#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include "view/AppScene.hpp"
#include <optional>
#include <string>
#include <vector>

struct ClientState {
    // --- Scene / app ---
    AppScene scene = AppScene::TitleScreen;
    bool shouldExit = false;

    // --- Lobby ---
    LobbySubState lobbySubState = LobbySubState::Setup;
    std::string lobbyCode;
    std::string lobbyError;

    // --- Game (server-authoritative) ---
    // yourBoard / enemyBoard are raw CellState values from StateUpdate.
    Protocol::UserState serverState = Protocol::UserState::None;
    std::string statusMessage;
    std::vector<int> yourBoard;
    std::vector<int> enemyBoard;
    std::optional<Protocol::GameOver> gameOver;

    // --- Settings ---
    int musicVolume = 70;
    int sfxVolume = 80;

    std::vector<int> abilityCharges;
    int localPlayerIdx{-1};   // set when LobbyJoined arrives
    int lastShooterSeen{-1};  // for SFX diffing
    int lastResultSeen{-1};
    int turnNumberSeen{-1};

    int lastShooter{-1};
    int lastAbilityType{-1};
    int lastAbilityX{-1};
    int lastAbilityY{-1};

    // --- Side Panel helper ---
    std::vector<std::string> abilityLogs;
};

#endif