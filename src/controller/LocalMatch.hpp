#pragma once
#include "controller/BattleshipAI.hpp"
#include "controller/BattleshipEngine.hpp"
#include "model/Protocol.hpp"
#include <chrono>
#include <optional>
#include <vector>

class LocalMatch {
  public:
    explicit LocalMatch(int humanPlayerIdx = 0);

    // bot places its fleet; engine enters placement phase
    void start();
    bool submitPlacement(const std::vector<Protocol::ShipPlacement> &ships);
    bool submitShot(int x, int y);
    // fires bot when it's the bot's turn and the delay elapsed
    void tick();

    [[nodiscard]] bool isOver() const;

    // returns a fresh snapshot only if state changed since last call.
    std::optional<Protocol::StateUpdate> consumeUpdate();

  private:
    BattleshipEngine m_engine;
    BattleshipAI m_bot;
    int m_humanIdx;
    int m_botIdx;

    std::chrono::steady_clock::time_point m_fireTime;
    static constexpr int m_tickDelay = 1000;

    bool m_dirty{true};
    inline void markDirty() { m_dirty = true; }

    bool isHumanTurn() const;
    bool isBotTurn() const;
    bool botFireOnce();
    void addBotDelay();
};
