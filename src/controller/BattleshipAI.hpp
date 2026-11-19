/**
 * @file BattleshipAI.hpp
 * @brief AI opponent used for local bot matches.
 */

#pragma once

#include "model/Board.hpp"
#include <array>
#include <optional>
#include <random>
#include <vector>

/**
 * @brief What the AI knows about a single cell of the opponent's board.
 */
enum class CellKnowledge {
    Unknown,
    Miss,
    Hit,
    Sunk
};

enum class BotDifficulty {
    Easy,
    Medium,
    Hard
};

/**
 * @brief Computer opponent that places its fleet and chooses where to shoot.
 *
 * Shot selection uses a probability-density approach: for every remaining ship,
 * all placements compatible with the current knowledge are counted and each
 * cell is scored by how many placements cover it. With no open hits the AI is in
 * hunt mode; with open hits it switches to target mode to finish the ship.
 */
class BattleshipAI {
  public:
    /**
     * @brief Seeds the random generator from the steady clock and resets the AI state.
     */
    BattleshipAI();

    /**
     * @brief Generates a legal placement for the AI's fleet during match setup.
     * @return A random fleet placement that respects the board rules.
     */
    [[nodiscard]] FleetPlacement generateLegalPlacement();

    /**
     * @brief Chooses the next cell to shoot at.
     *
     * Clears stale hits first, then uses hunt mode if there are no open hits,
     * target mode otherwise.
     * @return Coordinate of an unknown cell to attack.
     */
    [[nodiscard]] Coordinate getNextShot();

    /**
     * @brief Updates the AI's knowledge with the outcome of its last shot.
     * @param coord  Cell that was shot.
     * @param result Outcome of the shot; results other than Hit, Miss and Sunk are ignored.
     * @param sunk   Type of the ship sunk by this shot, if any.
     */
    void recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk);

    /**
     * @brief Sets how strong the AI plays by choosing its shot threshold.
     * @param difficulty Easy (0.1), Medium (0.4) or Hard (1.0, always the best cell).
     */
    void setDifficulty(BotDifficulty difficulty);

    /**
     * @brief Restores the initial state: full fleet remaining, all cells unknown, scores cleared.
     *
     * The difficulty is a setting, not game state, so it is kept.
     */
    void reset();

  private:
    std::mt19937 m_rng;
    std::vector<ShipType> m_remainingShips;
    std::array<std::array<CellKnowledge, Board::SIZE>, Board::SIZE> m_knowledge{};
    std::array<std::array<int, Board::SIZE>, Board::SIZE> m_score{};
    int m_maxScore{0};
    double m_threshold{1.0}; ///< Fraction of the best score a cell needs to be a shot candidate (1.0 = best cells only).

    // shot selection
    /** @brief Shoots a random cell among the unknown ones covered by the most legal ship placements. */
    [[nodiscard]] Coordinate huntMode();
    /** @brief Shoots next to the open hits, choosing the candidate covered by the most hit-weighted placements. */
    [[nodiscard]] Coordinate targetMode(const std::vector<Coordinate> &hits);
    /** @brief Returns the unknown cells right past the ends of each line of adjacent hits (falls back to all unknown neighbours of the hits). */
    [[nodiscard]] std::vector<Coordinate> pickLineShots(const std::vector<Coordinate> &hits) const;

    // score evaluation
    /** @brief Recomputes m_score from scratch over all remaining ships. */
    void evaluateScores(bool includeHits);
    /** @brief Adds to m_score every legal placement of a ship; with @p includeHits, Hit cells are allowed and each placement weighs as many hits as it covers. */
    void scoreShipPlacements(int shipLength, bool includeHits);
    /** @brief Returns all unknown cells scoring at least m_threshold * m_maxScore. */
    [[nodiscard]] std::vector<Coordinate> getBestCells() const;
    /** @brief Returns the candidates scoring at least m_threshold * m_maxScore (all candidates if none does). */
    [[nodiscard]] std::vector<Coordinate> getBestCandidates(const std::vector<Coordinate> &candidates) const;

    // knowledge update
    /** @brief Removes the sunk ship from m_remainingShips and marks the line of hits matching its length as Sunk. */
    void updateSunkCells(Coordinate coord, std::optional<ShipType> sunk);
    /** @brief Marks all hits as Sunk when none of them has an unknown neighbour left, since they can no longer lead to a ship. */
    void clearStaleHits();

    // grid helpers
    [[nodiscard]] std::vector<Coordinate> findCellsByKnowledge(CellKnowledge value) const;
    [[nodiscard]] static std::vector<Coordinate> getNeighbors(Coordinate center);
    /** @brief Returns the unknown orthogonal neighbours of the given hits (may contain duplicates). */
    [[nodiscard]] std::vector<Coordinate> getUnknownNeighborsOfHits(const std::vector<Coordinate> &hits) const;
    [[nodiscard]] static bool inBounds(int x, int y) noexcept;
    [[nodiscard]] static bool inBounds(Coordinate c) noexcept;
};
