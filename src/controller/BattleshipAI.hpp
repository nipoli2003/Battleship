#pragma once

#include "model/Board.hpp"
#include <vector>
#include <random>

class BattleshipAI {
public:
    BattleshipAI();

    Coordinate getNextShot();
    void recordShotResult(Coordinate coord, AttackResult result);
    void reset();

    // populate ships during match setup
    const FleetPlacement generateLegalPlacement();

private:
    std::vector<Coordinate> m_remainingTargets;
    std::vector<Coordinate> m_priorityTargets; // Target mode (neighbors of recent hits)
    std::mt19937 m_rng;

    void addNeighbors(Coordinate center);
};