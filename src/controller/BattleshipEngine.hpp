/**
 * @file BattleshipEngine.hpp
 * @brief Core engine for match setup, turn resolution, and snapshots.
 */

#pragma once

#include "controller/GameState.hpp"
#include "model/Protocol.hpp"
#include "model/Board.hpp"
#include "model/Protocol.hpp"

/**
 * @brief Coordinates the gameplay logic for a two-player Battleship match.
 *
 * The engine manages fleet placement, attack resolution, and player-specific
 * state snapshots that are serialized for the network layer.
 */
class BattleshipEngine {
  public:
    /**
     * @brief Creates a fresh engine with default match state.
     */
    BattleshipEngine();

    
    void startNewGame();

    // Placement phase methods
    /**
     * @brief Validates and registers a player's fleet placement.
     * @param playerIdx Index of the player submitting the fleet.
     * @param ships Placement entries for the player's ships.
     * @return true if the placement is accepted.
     */
    bool placeShips(int playerIdx, const FleetPlacement &ships);

    /**
     * @brief Checks whether a player has already finished placement.
     * @param playerIdx Player index to query.
     * @return true if that player is ready.
     */
    bool isPlacementDone(int playerIdx) const noexcept { return m_placement_done[playerIdx]; }

    /**
     * @brief Checks whether both players have completed placement.
     * @return true when both players are ready.
     */
    bool bothReady() const noexcept { return m_placement_done[0] && m_placement_done[1]; }

    /**
     * @brief Finalizes the placement phase and transitions the match to combat.
     */
    void finishPlacement();

    // Tactical Abilities
    int useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target, Orientation oerientation = Orientation::Horizontal);  // Orientation is for Torpedo
    [[nodiscard]] int getAbilityCharges(int playerIdx, Protocol::AbilityType ability) const noexcept { return m_abilities[playerIdx][static_cast<int>(ability)]; }
    [[nodiscard]] std::string getSonarLog(int playerIdx) const { return m_sonarLog[playerIdx]; }
    [[nodiscard]] std::string getTorpedoLog(int playerIdx) const { return m_torpedoLog[playerIdx]; }
    [[nodiscard]] std::string getBombingRunLog(int playerIdx) const { return m_bombingRunLog[playerIdx]; }

    // Combat phase methods
    /**
     * @brief Resolves a shot by a player for the given target coordinate.
     * @param playerIdx Player firing the shot.
     * @param target Target coordinate.
     * @return true if the shot was valid and processed.
     */
    bool fire(int playerIdx, Coordinate target);

    /**
     * @brief Returns the immutable board for a given player.
     * @param playerIdx Player index.
     * @return The player's board.
     */
    [[nodiscard]] const Board &getBoard(int playerIdx) const noexcept { return m_boards[playerIdx]; }

    /**
     * @brief Returns a mutable board for a given player.
     * @param playerIdx Player index.
     * @return The player's board.
     */
    [[nodiscard]] Board &accessBoard(int playerIdx) noexcept { return m_boards[playerIdx]; }

    /**
     * @brief Accesses the current match snapshot.
     * @return The current engine state snapshot.
     */
    [[nodiscard]] const GameSnapshot &getSnapshot() const noexcept { return m_snapshot; }

    /**
     * @brief Builds a player-specific state update for serialization.
     *
     * - yourBoard shows ships; enemyBoard masks present ships as Empty.
     * - Victory/Defeat are flipped correctly per perspective.
     * @param playerIdx Player whose perspective should be generated.
     * @return The serialized state snapshot for that player.
     */
    Protocol::StateUpdate buildPlayerSnapshot(int playerIdx) const;

  private:
    // Returns true if the player has a non-sunk ship of this type
    bool hasActiveShip(int playerIdx, ShipType type) const;

    int useSonar(int playerIdx, Coordinate target);
    int fireTorpedo(int playerIdx, Coordinate target, Orientation orientation);
    int fireBombingRun(int playerIdx, Coordinate target, Orientation orientation);

    Board m_boards[2]; ///< Boards for both players.
    bool m_placement_done[2]{false, false}; ///< Placement completion flags for each player.
    GameSnapshot m_snapshot; ///< Current match meta-state.

    int m_abilities[2][Protocol::ABILITY_COUNT]{};
    std::string m_sonarLog[2];
    std::string m_torpedoLog[2];
    std::string m_bombingRunLog[2];
};