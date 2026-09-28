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
    for (int y = 0; y < board.size(); ++y) {
        for (int x = 0; x < board.size(); ++x) {
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
    const int n = engine.getOpponentBoard().size();
    for (int y = 0; y < n && !missed; ++y) {
        for (int x = 0; x < n && !missed; ++x) {
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

// ---------------------------------------------------------
// Dynamic board size
// ---------------------------------------------------------

TEST(BoardSizeTest, DefaultIsTen) {
    Board board;
    EXPECT_EQ(board.size(), Board::DEFAULT_SIZE);
    EXPECT_EQ(board.size(), 10);
}

TEST(BoardSizeTest, CustomSizeBounds) {
    Board board(12);
    EXPECT_EQ(board.size(), 12);

    // Carrier (5) at x=7 overflows a 10x10 board but fits on 12x12 (cells 7..11)
    auto carrier = std::make_shared<Ship>(ShipType::Carrier, Orientation::Horizontal);
    EXPECT_TRUE(board.placeShip(carrier, {7, 0}));
    EXPECT_EQ(board.getCell(11, 0), CellState::ShipPresent);

    EXPECT_EQ(board.receiveAttack({11, 11}), AttackResult::Miss);
    EXPECT_EQ(board.receiveAttack({12, 0}), AttackResult::Invalid);
}

TEST(BoardSizeTest, SizeIsClamped) {
    EXPECT_EQ(Board(3).size(), Board::MIN_SIZE);
    EXPECT_EQ(Board(99).size(), Board::MAX_SIZE);
}

TEST(BoardSizeTest, ResizeClearsBoard) {
    Board board;
    auto destroyer = std::make_shared<Ship>(ShipType::Destroyer, Orientation::Horizontal);
    ASSERT_TRUE(board.placeShip(destroyer, {0, 0}));

    board.reset(14);
    EXPECT_EQ(board.size(), 14);
    EXPECT_EQ(board.getCell(0, 0), CellState::Empty);
}

TEST(BoardSizeTest, RandomPlacementWorksOnEverySize) {
    for (int size = Board::MIN_SIZE; size <= Board::MAX_SIZE; ++size) {
        for (int round = 0; round < 50; ++round) {
            Board board(size);
            BattleshipAI::placeShipsRandomly(board);

            int occupied = 0;
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                    if (board.getCell(x, y) == CellState::ShipPresent) ++occupied;

            ASSERT_EQ(occupied, 17) << "size " << size << ", round " << round;
        }
    }
}

TEST(BoardSizeTest, AIShotsCoverWholeBoardExactlyOnce) {
    const int size = 13;
    BattleshipAI ai;
    ai.reset(size);

    std::vector<std::vector<bool>> seen(size, std::vector<bool>(size, false));
    for (int i = 0; i < size * size; ++i) {
        Coordinate c = ai.getNextShot();
        ASSERT_GE(c.x, 0); ASSERT_LT(c.x, size);
        ASSERT_GE(c.y, 0); ASSERT_LT(c.y, size);
        ASSERT_FALSE(seen[c.y][c.x]) << "duplicate shot at " << c.x << "," << c.y;
        seen[c.y][c.x] = true;
    }
}

TEST(BoardSizeTest, EngineAppliesSizeOnNewGame) {
    BattleshipEngine engine(OpponentType::LocalAI);
    EXPECT_EQ(engine.getHumanBoard().size(), Board::DEFAULT_SIZE);

    engine.setBoardSize(14);
    engine.startNewGame();
    EXPECT_EQ(engine.getBoardSize(), 14);
    EXPECT_EQ(engine.getHumanBoard().size(), 14);
    EXPECT_EQ(engine.getOpponentBoard().size(), 14);

    engine.setBoardSize(100);
    EXPECT_EQ(engine.getBoardSize(), Board::MAX_SIZE);
}
