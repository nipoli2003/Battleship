#pragma once

#include "model/Board.hpp"
#include <vector>
#include <random>

enum class Knowledge {Unknown, Miss, Hit, Sunk};

class BattleshipAI {
public:
    BattleshipAI();

    Coordinate getNextShot();
    Coordinate huntMode();
    Coordinate targetMode();
    void recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk);
    void reset();

    // populate ships during match setup
    const FleetPlacement generateLegalPlacement();

private:
    std::mt19937 m_rng;
    //New
    std::vector<ShipType> m_remainingShips;
    

    std::array<std::array<Knowledge, Board::SIZE>, Board::SIZE> m_known{};
    std::array<std::array<int, Board::SIZE>, Board::SIZE> m_score{};

    std::vector<Coordinate> findKnowledgeCells(Knowledge value) const;

};