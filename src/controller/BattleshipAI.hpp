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
    [[nodiscard]] FleetPlacement generateLegalPlacement();

    [[nodiscard]] Coordinate getNextShot();
    void recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk);
    void reset();

  private:
    std::mt19937 m_rng;
    std::vector<ShipType> m_remainingShips;
    std::array<std::array<CellKnowledge, Board::SIZE>, Board::SIZE> m_knowledge{};
    std::array<std::array<int, Board::SIZE>, Board::SIZE> m_score{};

    // shot selection
    [[nodiscard]] Coordinate huntMode();
    [[nodiscard]] Coordinate targetMode(const std::vector<Coordinate> &hits);
    [[nodiscard]] std::vector<Coordinate> pickLineShots(const std::vector<Coordinate> &hits) const;

    // score evaluation
    void evaluateScores(bool includeHits);
    void scoreShipPlacements(int shipLength, bool includeHits);
    [[nodiscard]] std::vector<Coordinate> getBestCells() const;
    [[nodiscard]] std::vector<Coordinate> getBestCandidates(const std::vector<Coordinate> &candidates) const;

    // knowledge update
    void updateSunkCells(Coordinate coord, std::optional<ShipType> sunk);
    void clearStaleHits();

    // grid helpers
    [[nodiscard]] std::vector<Coordinate> findCellsByKnowledge(CellKnowledge value) const;
    [[nodiscard]] static std::vector<Coordinate> getNeighbors(Coordinate center);
    [[nodiscard]] std::vector<Coordinate> getUnknownNeighborsOfHits(const std::vector<Coordinate> &hits) const;
    [[nodiscard]] static bool inBounds(int x, int y) noexcept;
    [[nodiscard]] static bool inBounds(Coordinate c) noexcept;
};
