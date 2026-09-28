#pragma once

#include "model/Board.hpp"
#include <vector>
#include <random>

class BattleshipAI {
public:
    BattleshipAI();

    Coordinate getNextShot();
    void recordShotResult(Coordinate coord, AttackResult result);
    void reset(int boardSize = Board::DEFAULT_SIZE);

    // Helper to randomly populate ships during match setup
    static void placeShipsRandomly(Board& board);

private:
    std::vector<Coordinate> m_remainingTargets;
    std::vector<Coordinate> m_priorityTargets; // Target mode (neighbors of recent hits)
    std::mt19937 m_rng;
    int m_boardSize{Board::DEFAULT_SIZE};

    void addNeighbors(Coordinate center);
};