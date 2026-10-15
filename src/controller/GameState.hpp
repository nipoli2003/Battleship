#pragma once

#include "model/Board.hpp"
#include <string>

enum class MatchState {
    PlacementPhase,
    PlayerTurn,
    OpponentTurn,
    Victory,
    Defeat
};

enum class LobbySubState {
    Setup,
    Connecting,
    ShowCode,
    Waiting,
    Error
};

struct GameSnapshot {
    MatchState state{MatchState::PlacementPhase};
    std::string statusMessage{"Waiting for both players to place ships."};
    int turnNumber{0};
    // who fired last and what happened, so each client can get a message
    // from its own point of view (-1 = no shot yet)
    int lastShooter{-1};
    AttackResult lastResult{AttackResult::Invalid};
};