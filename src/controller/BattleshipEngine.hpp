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
    int useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target, Orientation oerientation = Orientation::Horizontal);  // Orientation is for Torpedo
    [[nodiscard]] bool hasAbility(int playerIdx, Protocol::AbilityType ability) const noexcept { return m_abilities[playerIdx][static_cast<int>(ability)]; }
    [[nodiscard]] std::string getSonarLog(int playerIdx) const { return m_sonarLog[playerIdx]; }
    [[nodiscard]] std::string getTorpedoLog(int playerIdx) const { return m_torpedoLog[playerIdx]; }

    // Combat phase methods
    bool fire(int playerIdx, Coordinate target);

    [[nodiscard]] const Board &getBoard(int playerIdx) const noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] Board &accessBoard(int playerIdx) noexcept { return m_boards[playerIdx]; }
    [[nodiscard]] const GameSnapshot &getSnapshot() const noexcept { return m_snapshot; }

  private:
    // Returns true if the player has a non-sunk ship of this type
    bool hasActiveShip(int playerIdx, ShipType type) const;

    int useSonar(int playerIdx, Coordinate target);
    int fireTorpedo(int playerIdx, Coordinate target, Orientation orientation);

    Board m_boards[2];
    bool m_placement_done[2]{false, false};
    GameSnapshot m_snapshot;

    bool m_abilities[2][Protocol::ABILITY_COUNT]{};
    std::string m_sonarLog[2];
    std::string m_torpedoLog[2];
};