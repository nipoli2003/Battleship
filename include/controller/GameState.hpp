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
    std::string statusMessage{"Deploy your ships to begin!"};
    int turnNumber{0};
    int humanHitsRemaining{17};    // Total ship cells (5+4+3+3+2)
    int opponentHitsRemaining{17};
};