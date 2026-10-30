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

bool BattleshipEngine::placeShips(int playerIdx, const ShiPlacement &ships) {
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

    m_snapshot.lastShooter = playerIdx;
    m_snapshot.lastResult = result;

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

// The engine keeps one neutral snapshot for both players, so its statusMessage
// can't say "you" vs "opponent". Build the text per player here instead.
std::string buildStatusMessage(const GameSnapshot &snap, int playerIdx) {
    const bool myTurn = (snap.state == MatchState::PlayerTurn && playerIdx == 0) ||
                        (snap.state == MatchState::OpponentTurn && playerIdx == 1);

    switch (snap.state) {
    case MatchState::PlacementPhase:
        return snap.statusMessage;
    case MatchState::Victory:
    case MatchState::Defeat:
        return "Game over!";
    default:
        break;
    }

    std::string last;
    if (snap.lastShooter != -1) {
        const bool iShot = (snap.lastShooter == playerIdx);
        switch (snap.lastResult) {
        case AttackResult::Miss:
            last = iShot ? "You missed. " : "Opponent missed. ";
            break;
        case AttackResult::Hit:
            last = iShot ? "Hit! " : "Opponent hit your ship! ";
            break;
        case AttackResult::Sunk:
            last = iShot ? "You sunk a ship! " : "Opponent sunk one of your ships! ";
            break;
        default:
            break;
        }
    }

    return last + (myTurn ? "Your turn - fire!" : "Opponent's turn...");
}

Protocol::StateUpdate BattleshipEngine::buildPlayerSnapshot(int playerIdx) const {
    const Board &myBoard = getBoard(playerIdx);
    const Board &foeBoard = getBoard(1 - playerIdx);

    Protocol::StateUpdate out;
    out.turnNumber = m_snapshot.turnNumber;
    out.statusMessage = buildStatusMessage(m_snapshot, playerIdx);

    // flip Victory/Defeat so each player sees their own outcome
    switch (m_snapshot.state) {
    case MatchState::PlacementPhase:
        out.state = Protocol::UserState::PlacementPhase;
        break;
    case MatchState::PlayerTurn:
        out.state = (playerIdx == 0) ? Protocol::UserState::YourTurn : Protocol::UserState::OpponentTurn;
        break;
    case MatchState::OpponentTurn:
        out.state = (playerIdx == 0) ? Protocol::UserState::OpponentTurn : Protocol::UserState::YourTurn;
        break;
    case MatchState::Victory:
        out.state = (playerIdx == 0) ? Protocol::UserState::Victory : Protocol::UserState::Defeat;
        break;
    case MatchState::Defeat:
        out.state = (playerIdx == 0) ? Protocol::UserState::Defeat : Protocol::UserState::Victory;
        break;
    }

    out.yourBoard.reserve(Board::SIZE * Board::SIZE);
    out.enemyBoard.reserve(Board::SIZE * Board::SIZE);

    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            out.yourBoard.push_back(static_cast<int>(myBoard.getCell(x, y)));

            CellState ec = foeBoard.getCell(x, y);
            if (ec != CellState::Empty && ec != CellState::Hit && ec != CellState::Miss)
                ec = CellState::Empty; // hide unshot ships
            out.enemyBoard.push_back(static_cast<int>(ec));
        }
    }

    return out;
}
