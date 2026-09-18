#pragma once

#include "model/Board.hpp"
#include "controller/GameState.hpp"
#include "controller/BattleshipAI.hpp"

class BattleshipEngine {
public:
    explicit BattleshipEngine(OpponentType opponentType = OpponentType::LocalAI);

    void startNewGame();
    bool humanFire(Coordinate target);
    void processAITurn();

    [[nodiscard]] const Board& getHumanBoard() const noexcept { return m_humanBoard; }
    [[nodiscard]] const Board& getOpponentBoard() const noexcept { return m_opponentBoard; }
    [[nodiscard]] const GameSnapshot& getSnapshot() const noexcept { return m_snapshot; }

    Board& accessHumanBoard() noexcept { return m_humanBoard; }

private:
    Board m_humanBoard;
    Board m_opponentBoard;
    BattleshipAI m_ai;
    OpponentType m_opponentType;
    GameSnapshot m_snapshot;
};