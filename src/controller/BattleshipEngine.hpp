#pragma once

#include "controller/GameState.hpp"
#include "model/Board.hpp"
#include <vector>

class BattleshipEngine {
  public:
    BattleshipEngine();

    void startNewGame();

    // Placement phase methods
    bool placeShips(int playerIdx, const std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>> &ships);
    bool isPlacementDone(int playerIdx) const noexcept { return m_placement_done[playerIdx]; }
    bool bothReady() const noexcept { return m_placement_done[0] && m_placement_done[1]; }
    void finishPlacement();

    // Combat phase methods
    bool fire(int playerIdx, Coordinate target);

    [[nodiscard]] const Board &getBoard(int playerIdx) const noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] Board &accessBoard(int playerIdx) noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] const GameSnapshot &getSnapshot() const noexcept { return m_snapshot; }

  private:
    Board m_boards[2];
    bool m_placement_done[2]{false, false};
    GameSnapshot m_snapshot;
};