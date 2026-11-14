/**
 * @file LocalMatch.hpp
 * @brief Local player-vs-bot match wrapper.
 */

#pragma once

#include "controller/BattleshipAI.hpp"
#include "controller/BattleshipEngine.hpp"
#include "model/Protocol.hpp"
#include <chrono>
#include <optional>
#include <vector>

/**
 * @brief Local single-device match against a bot opponent.
 */
class LocalMatch {
  public:
    /**
     * @brief Creates a local match with a designated human player index.
     * @param humanPlayerIdx Index of the human-controlled player.
     */
    explicit LocalMatch(int humanPlayerIdx = 0, BotDifficulty difficulty = BotDifficulty::Hard);

    /**
     * @brief Starts the match and lets the bot place its fleet.
     */
    void start();

    /**
     * @brief Submits the human player's fleet placement.
     * @param ships Placement payload for the player's ships.
     * @return true if the placement is valid and accepted.
     */
    bool submitPlacement(const std::vector<Protocol::ShipPlacement> &ships);

    /**
     * @brief Submits a shot from the human player.
     * @param x Column index.
     * @param y Row index.
     * @return true if the shot was legal and processed.
     */
    bool submitShot(int x, int y);

    /**
     * @brief Advances bot logic when the AI is supposed to act.
     */
    void tick();

    /**
     * @brief Uses a tactical ability for the human player.
     * @param ability     Ability to use (Sonar, Torpedo or BombingRun).
     * @param x           Target column.
     * @param y           Target row.
     * @param orientation Orientation, only used by Torpedo and BombingRun.
     * @return true if the ability was legal and processed.
     */
    bool submitAbility(Protocol::AbilityType ability, int x, int y, Orientation orientation = Orientation::Horizontal);

    /**
     * @brief Checks whether the match is over.
     * @return true when one side has won the match.
     */
    [[nodiscard]] bool isOver() const;

    /**
     * @brief Consumes the next snapshot only when the match state has changed.
     * @return A new protocol snapshot if the state changed; otherwise std::nullopt.
     */
    std::optional<Protocol::StateUpdate> consumeUpdate();

  private:
    BattleshipEngine m_engine; ///< Core engine responsible for rules and boards.
    BattleshipAI m_bot; ///< Bot AI used by the opposing player.
    int m_humanIdx; ///< Player index controlled by the human.
    int m_botIdx; ///< Player index controlled by the bot.

    std::chrono::steady_clock::time_point m_fireTime; ///< Time when the bot should fire next.
    static constexpr int m_tickDelay = 1000; ///< Delay in milliseconds before bot attacks.

    bool m_dirty{true}; ///< Whether a state update is pending.

    /**
     * @brief Marks the local match as having a new update to emit.
     */
    inline void markDirty() { m_dirty = true; }

    /**
     * @brief Checks whether it is the human player's turn.
     * @return true when the human is permitted to act.
     */
    bool isHumanTurn() const;

    /**
     * @brief Checks whether it is the bot player's turn.
     * @return true when the bot is allowed to act.
     */
    bool isBotTurn() const;

    /**
     * @brief Executes one bot shot if it is currently allowed.
     * @return true if the bot fired successfully.
     */
    bool botFireOnce();

    /**
     * @brief Starts the bot's turn delay timer.
     */
    void addBotDelay();
};
