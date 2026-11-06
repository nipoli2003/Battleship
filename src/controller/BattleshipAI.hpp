#pragma once

#include "model/Board.hpp"
#include <array>
#include <optional>
#include <random>
#include <vector>

enum class CellKnowledge {
    Unknown,
    Miss,
    Hit,
    Sunk
};

class BattleshipAI {
  public:
    BattleshipAI();

    // populate ships during match setup
    const FleetPlacement generateLegalPlacement();

    Coordinate getNextShot();
    void recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk);
    void reset();

  private:
    std::mt19937 m_rng;
    std::vector<ShipType> m_remainingShips;
    std::array<std::array<CellKnowledge, Board::SIZE>, Board::SIZE> m_knowledge{};
    std::array<std::array<int, Board::SIZE>, Board::SIZE> m_score{};

    // shot selection
    Coordinate huntMode();
    Coordinate targetMode(std::vector<Coordinate> hits);
    std::vector<Coordinate> pickLineShots(std::vector<Coordinate> hits);

    // score evaluation
    void evaluateScores();
    void scoreShipPlacements(int shipLength);
    std::vector<Coordinate> getBestCells();

    // knowledge update
    void updateSunkCells(Coordinate coord, std::optional<ShipType> sunk);

    // grid helpers
    std::vector<Coordinate> findCellsByKnowledge(CellKnowledge value) const;
    std::vector<Coordinate> getNeighbors(Coordinate center);
    std::vector<Coordinate> getUnknownNeighborsOfHits(std::vector<Coordinate> hits);
    [[nodiscard]] static bool inBounds(int x, int y) noexcept;
    [[nodiscard]] static bool inBounds(Coordinate c) noexcept;
};
