#pragma once

#include "model/Board.hpp"
#include <array>
#include <optional>
#include <vector>
#include <random>

enum class Knowledge {Unknown, Miss, Hit, Sunk};

class BattleshipAI {
public:
    BattleshipAI();

    Coordinate getNextShot();
    
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
    Coordinate huntMode();
    Coordinate targetMode(std::vector<Coordinate> hits);
    void evaluateCells();
    void evaluateCellsSingleShip(int shipLength);
    std::vector<Coordinate> getMaximalScorePositions();
    void updateSunkedCells(Coordinate coord, std::optional<ShipType> sunk);
    std::vector<Coordinate> getNeighbors(Coordinate center);
    std::vector<Coordinate> multipleHitsTarget(std::vector<Coordinate> hits);
};