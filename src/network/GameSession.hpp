/**
 * @file GameSession.hpp
 * @brief Wrapper around the game engine for a single multiplayer match.
 */

#ifndef GAME_SESSION_HPP
#define GAME_SESSION_HPP

#include "controller/BattleshipEngine.hpp"
#include "model/Protocol.hpp"
#include "model/Ship.hpp"

/**
 * @brief Server-side game state for one match session.
 *
 * This class owns the underlying BattleshipEngine and exposes the actions
 * needed by the server to accept placements, resolve turns, and report state.
 */
class GameSession {
  public:
    /**
     * @brief Creates an empty game session.
     */
    GameSession() = default;

    /**
     * @brief Submits a player’s fleet placement for the current match.
     * @param playerIdx Index of the player submitting the ships.
     * @param ships Placement data for the ship layout.
     * @return true if the placement was accepted, false if already submitted or invalid.
     */
    bool placeShips(int playerIdx, const std::vector<Protocol::ShipPlacement> &ships);

    /**
     * @brief Checks whether both players are ready to begin battle.
     * @return true when both players have submitted valid fleets.
     */
    bool bothReady() const noexcept { return m_engine.bothReady(); }

    /**
     * @brief Fires a shot for the specified player.
     * @param playerIdx Index of the player taking the shot.
     * @param coord Target coordinate.
     * @return true if the shot was legal and processed, false otherwise.
     */
    bool fire(int playerIdx, Coordinate coord);

    /**
     * @brief Checks whether the match has ended.
     * @return true if the current player has won or lost the match.
     */
    bool isOver() const noexcept {
        auto s = m_engine.getSnapshot().state;
        return s == MatchState::Victory || s == MatchState::Defeat;
    }

    /**
     * @brief Builds the state snapshot visible to one player.
     * @param playerIdx Player whose perspective should be encoded.
     * @return Snapshot containing only that player's view of the match.
     *         enemyBoard masks present ships as Empty
     */
    inline Protocol::StateUpdate buildSnapshot(int playerIdx) const {
        return m_engine.buildPlayerSnapshot(playerIdx);
    }

    int useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target, Orientation orientation = Orientation::Horizontal);
    [[nodiscard]] bool hasAbility(int playerIdx, Protocol::AbilityType ability) const noexcept;

  private:
    BattleshipEngine m_engine; ///< Core match engine backing this session.
};

#endif