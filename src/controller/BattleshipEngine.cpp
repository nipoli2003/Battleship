#include "controller/BattleshipEngine.hpp"

BattleshipEngine::BattleshipEngine() {
    startNewGame();
}

void BattleshipEngine::startNewGame() {
    m_boards[0].reset();
    m_boards[1].reset();
    m_placement_done[0] = false;
    m_placement_done[1] = false;
    m_snapshot = GameSnapshot{};
    m_snapshot.state = MatchState::PlacementPhase;
    m_snapshot.statusMessage = "Waiting for both players to place ships.";
}

bool BattleshipEngine::placeShips(int playerIdx,
                                  const std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>> &ships) {
    if (m_placement_done[playerIdx])
        return false;

    Board &board = m_boards[playerIdx];
    board.reset();

    for (const auto &[type, coordOrient] : ships) {
        const auto &[coord, orient] = coordOrient;
        auto ship = std::make_shared<Ship>(type, orient);
        if (!board.placeShip(ship, coord)) {
            board.reset(); // reject the whole placement if any ship is invalid
            return false;
        }
    }

    m_placement_done[playerIdx] = true;
    return true;
}

void BattleshipEngine::finishPlacement() {
    m_snapshot.state = MatchState::PlayerTurn; // player 0 always goes first
    m_snapshot.statusMessage = "Battle stations! Player 0 fires first.";
}

bool BattleshipEngine::fire(int playerIdx, Coordinate target) {
    // playerIdx fires at the opponent's board
    bool isPlayer0Turn = (m_snapshot.state == MatchState::PlayerTurn);
    if (playerIdx == 0 && !isPlayer0Turn)
        return false;
    if (playerIdx == 1 && isPlayer0Turn)
        return false;

    int victimIdx = 1 - playerIdx;
    AttackResult result = m_boards[victimIdx].receiveAttack(target);
    if (result == AttackResult::Invalid)
        return false;

    if (m_boards[victimIdx].allShipsSunk()) {
        m_snapshot.state = (playerIdx == 0) ? MatchState::Victory : MatchState::Defeat;
        m_snapshot.statusMessage = "Game over!";
        return true;
    }

    if (result == AttackResult::Hit || result == AttackResult::Sunk) {
        m_snapshot.statusMessage = (result == AttackResult::Sunk)
                                       ? "Ship sunk! Fire again."
                                       : "Hit! Fire again.";
        // same player fires again — state unchanged
    } else {
        m_snapshot.statusMessage = "Miss! Other player's turn.";
        m_snapshot.state = (playerIdx == 0) ? MatchState::OpponentTurn : MatchState::PlayerTurn;
        m_snapshot.turnNumber++;
    }

    return true;
}
