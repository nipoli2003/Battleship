#include <gtest/gtest.h>
#include "model/Board.hpp"
#include "model/Ship.hpp"
#include "controller/BattleshipEngine.hpp"

TEST(EngineTest, TurnProgression) {
    BattleshipEngine engine(OpponentType::LocalAI);
    EXPECT_EQ(engine.getSnapshot().state, MatchState::PlayerTurn);

    // Fire at (0, 0)
    bool shotFired = engine.humanFire({0, 0});
    EXPECT_TRUE(shotFired);
    EXPECT_EQ(engine.getSnapshot().state, MatchState::OpponentTurn);

    // Let AI take its turn
    engine.processAITurn();
    EXPECT_EQ(engine.getSnapshot().state, MatchState::PlayerTurn);
}

TEST(AITest, RandomShipPlacementValid) {
    Board board;
    BattleshipAI::placeShipsRandomly(board);

    // Count occupied cells (Carrier=5 + Battleship=4 + Cruiser=3 + Sub=3 + Destroyer=2 = 17)
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

    // Length 5 starting at x=7 extends out of bounds (7, 8, 9, 10, 11)
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

    // Attacking the same cell again is invalid
    EXPECT_EQ(board.receiveAttack({5, 5}), AttackResult::Invalid);
}