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

enum class OpponentType {
    LocalAI,
    RemotePlayer
};

struct GameSnapshot {
    MatchState state{MatchState::PlacementPhase};
    std::string statusMessage{"Place your Carrier (Length: 5). Press [R] to rotate."};
    int turnNumber{0};
};