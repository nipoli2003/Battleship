#pragma once

#include "controller/GameState.hpp"
#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include <vector>

using ShiPlacement = std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>>;

class BattleshipEngine {
  public:
    BattleshipEngine();

    void startNewGame();

    // Placement phase methods
    bool placeShips(int playerIdx, const ShiPlacement &ships);
    bool isPlacementDone(int playerIdx) const noexcept { return m_placement_done[playerIdx]; }
    bool bothReady() const noexcept { return m_placement_done[0] && m_placement_done[1]; }
    void finishPlacement();

    // Combat phase methods
    bool fire(int playerIdx, Coordinate target);

    [[nodiscard]] const Board &getBoard(int playerIdx) const noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] Board &accessBoard(int playerIdx) noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] const GameSnapshot &getSnapshot() const noexcept { return m_snapshot; }

    // builds a tailored StateUpdate for playerIdx:
    // - yourBoard shows ships; enemyBoard masks present ships as Empty.
    // - Victory/Defeat are flipped correctly per perspective.
    Protocol::StateUpdate buildPlayerSnapshot(int playerIdx) const;

  private:
    Board m_boards[2];
    bool m_placement_done[2]{false, false};
    GameSnapshot m_snapshot;
};