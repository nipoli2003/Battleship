#include <gtest/gtest.h>
#include "model/Board.hpp"
#include "model/Ship.hpp"
#include "controller/BattleshipEngine.hpp"

TEST(BoardTest, PlaceShipWithinBounds) {
    Board board;
    auto destroyer = std::make_shared<Ship>(ShipType::Destroyer, Orientation::Horizontal);

    EXPECT_TRUE(board.placeShip(destroyer, {0, 0}));
    EXPECT_EQ(board.getCell(0, 0), CellState::ShipPresent);
    EXPECT_EQ(board.getCell(1, 0), CellState::ShipPresent);
    EXPECT_EQ(board.getCell(2, 0), CellState::Empty);
}

TEST(BoardTest, CannotPlaceShipOutOfBounds) {
    Board board;
    auto carrier = std::make_shared<Ship>(ShipType::Carrier, Orientation::Horizontal);

    EXPECT_FALSE(board.placeShip(carrier, {7, 0}));
}

TEST(BoardTest, AttackHitAndSink) {
    Board board;
    auto destroyer = std::make_shared<Ship>(ShipType::Destroyer, Orientation::Vertical);
    ASSERT_TRUE(board.placeShip(destroyer, {3, 3}));

    EXPECT_EQ(board.receiveAttack({3, 3}), AttackResult::Hit);
    EXPECT_FALSE(destroyer->isSunk());

    EXPECT_EQ(board.receiveAttack({3, 4}), AttackResult::Sunk);
    EXPECT_TRUE(destroyer->isSunk());
    EXPECT_TRUE(board.allShipsSunk());
}

TEST(BoardTest, AttackMissAndDuplicate) {
    Board board;
    EXPECT_EQ(board.receiveAttack({5, 5}), AttackResult::Miss);
    EXPECT_EQ(board.getCell(5, 5), CellState::Miss);

    EXPECT_EQ(board.receiveAttack({5, 5}), AttackResult::Invalid);
}

TEST(AITest, RandomShipPlacementValid) {
    Board board;
    BattleshipAI::placeShipsRandomly(board);

    int occupiedCount = 0;
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            if (board.getCell(x, y) == CellState::ShipPresent) {
                occupiedCount++;
            }
        }
    }
    EXPECT_EQ(occupiedCount, 17);
}

TEST(EngineTest, PlacementPhaseAndTurnProgression) {
    BattleshipEngine engine(OpponentType::LocalAI);

    // 1. Verify engine starts in placement phase
    EXPECT_EQ(engine.getSnapshot().state, MatchState::PlacementPhase);
    EXPECT_FALSE(engine.isPlacementComplete());

    // 2. Cannot fire before finishing placement
    EXPECT_FALSE(engine.humanFire({0, 0}));

    // 3. Complete placement
    engine.randomizeHumanFleet();
    EXPECT_TRUE(engine.isPlacementComplete());
    EXPECT_EQ(engine.getSnapshot().state, MatchState::PlayerTurn);

    // 4. Fire until a miss occurs (hits retain the turn due to the consecutive hit rule)
    bool missed = false;
    for (int y = 0; y < Board::SIZE && !missed; ++y) {
        for (int x = 0; x < Board::SIZE && !missed; ++x) {
            if (engine.getOpponentBoard().getCell(x, y) == CellState::Empty) {
                EXPECT_TRUE(engine.humanFire({x, y}));
                missed = true;
            }
        }
    }

    // After a miss, turn transfers to Opponent
    EXPECT_EQ(engine.getSnapshot().state, MatchState::OpponentTurn);

    // AI timer delay simulation: update with 2.0s triggers bot turn
    engine.update(2.1f);

    // Turn should either remain OpponentTurn (if bot hit our ship) or return to PlayerTurn (if bot missed)
    MatchState stateAfterAI = engine.getSnapshot().state;
    EXPECT_TRUE(stateAfterAI == MatchState::PlayerTurn || stateAfterAI == MatchState::OpponentTurn);
}