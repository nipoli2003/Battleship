#pragma once

#include "model/Board.hpp"
#include "controller/GameState.hpp"
#include "controller/BattleshipAI.hpp"
#include <vector>

class BattleshipEngine {
public:
    explicit BattleshipEngine(OpponentType opponentType = OpponentType::LocalAI);

    void startNewGame();
    void update(float dt); // Drives the AI thinking timer smoothly

    // Placement phase methods
    bool placeCurrentShip(Coordinate target, Orientation orientation);
    void randomizeHumanFleet();
    [[nodiscard]] bool isPlacementComplete() const noexcept;
    [[nodiscard]] std::optional<ShipType> getCurrentPlacementType() const noexcept;

    // Combat phase methods
    bool humanFire(Coordinate target);
    void processAITurn();

    [[nodiscard]] const Board& getHumanBoard() const noexcept { return m_humanBoard; }
    [[nodiscard]] const Board& getOpponentBoard() const noexcept { return m_opponentBoard; }
    [[nodiscard]] const GameSnapshot& getSnapshot() const noexcept { return m_snapshot; }

    Board& accessHumanBoard() noexcept { return m_humanBoard; }

private:
    void finishPlacement();

    Board m_humanBoard;
    Board m_opponentBoard;
    BattleshipAI m_ai;
    OpponentType m_opponentType;
    GameSnapshot m_snapshot;

    // Fleet placement tracking
    std::vector<ShipType> m_fleetToPlace;
    std::size_t m_currentPlacementIndex{0};

    // AI thinking delay timer
    float m_aiTimer{0.0f};
    static constexpr float AI_DELAY_SECONDS = 2.0f;
};