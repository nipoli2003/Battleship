#include <gtest/gtest.h>
#include "model/Board.hpp"
#include "model/Ship.hpp"

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