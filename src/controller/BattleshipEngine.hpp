#pragma once

#include "controller/GameState.hpp"
#include "model/Protocol.hpp"
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

    // Tactical Abilities
    int useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target);
    [[nodiscard]] bool hasUsedAbility(int playerIdx) const noexcept { return m_abilityUsed[playerIdx]; }
    [[nodiscard]] std::string getSonarLog(int playerIdx) const { return m_sonarLog[playerIdx]; }

    // Combat phase methods
    bool fire(int playerIdx, Coordinate target);

    [[nodiscard]] const Board &getBoard(int playerIdx) const noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] Board &accessBoard(int playerIdx) noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] const GameSnapshot &getSnapshot() const noexcept { return m_snapshot; }

  private:
    // Returns true if the player has a non-sunk ship of this type
    bool hasActiveShip(int playerIdx, ShipType type) const;

    Board m_boards[2];
    bool m_placement_done[2]{false, false};
    GameSnapshot m_snapshot;

    bool m_abilityUsed[2]{false, false};
    std::string m_sonarLog[2];
};