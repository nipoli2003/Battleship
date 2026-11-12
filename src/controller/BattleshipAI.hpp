/**
 * @file BattleshipAI.hpp
 * @brief AI opponent used for local bot matches.
 */

#pragma once

#include "model/Board.hpp"
#include <vector>
#include <random>

/**
 * @brief Simple Battleship AI that targets remaining cells and follows hits.
 */
class BattleshipAI {
public:
    /**
     * @brief Creates a bot with a seeded random generator.
     */
    BattleshipAI();

    /**
     * @brief Selects the next coordinate to attack.
     * @return A legal target cell for the bot.
     */
    Coordinate getNextShot();

    /**
     * @brief Records the result of a shot so the bot can update its targeting strategy.
     * @param coord Target coordinate that was shot.
     * @param result Outcome of that shot.
     */
    void recordShotResult(Coordinate coord, AttackResult result);

    /**
     * @brief Resets the AI to its initial state.
     */
    void reset();

    /**
     * @brief Generates a legal fleet placement for bot setup.
     * @return A valid fleet placement for a local match.
     */
    const FleetPlacement generateLegalPlacement();

private:
    std::vector<Coordinate> m_remainingTargets; ///< Untried target cells.
    std::vector<Coordinate> m_priorityTargets; ///< Cells adjacent to recent successful hits.
    std::mt19937 m_rng; ///< Random source for placement and target selection.

    /**
     * @brief Adds neighboring cells around a hit coordinate to the priority list.
     * @param center Cell whose neighbors should be considered.
     */
    void addNeighbors(Coordinate center);
};