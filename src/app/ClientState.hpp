#ifndef CLIENT_STATE_HPP
#define CLIENT_STATE_HPP

#include "controller/GameState.hpp"
#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include "view/AppScene.hpp"
#include <string>
#include <vector>

struct ClientState {
    AppScene scene = AppScene::TitleScreen;
    bool shouldExit = false;

    LobbySubState lobbySubState = LobbySubState::Setup;
    std::string lobbyCode;
    std::string lobbyError;

    Protocol::UserState serverState = Protocol::UserState::None;
    std::string statusMessage;
    std::vector<int> yourBoard;
    std::vector<int> enemyBoard;

    Board localBoard;
    std::vector<Protocol::ShipPlacement> pendingPlacements;
    Orientation placementOrientation = Orientation::Horizontal;
    bool placementSent = false;

    int musicVolume = 70;
    int sfxVolume = 80;
};

#endif
