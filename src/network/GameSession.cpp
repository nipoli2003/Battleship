#include "network/GameSession.hpp"

bool GameSession::placeShips(int playerIdx, const std::vector<Protocol::ShipPlacement> &ships) {
    // translate Protocol::ShipPlacement into what the engine expects
    std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>> parsed;
    parsed.reserve(ships.size());

    for (const auto &sp : ships) {
        ShipType type = static_cast<ShipType>(sp.typeID);
        Coordinate coord = {sp.x, sp.y};
        Orientation orient = (sp.orientation == 0) ? Orientation::Horizontal : Orientation::Vertical;
        parsed.push_back({type, {coord, orient}});
    }

    bool ok = m_engine.placeShips(playerIdx, parsed);

    if (ok && m_engine.bothReady()) {
        m_engine.finishPlacement();
    }

    return ok;
}

bool GameSession::fire(int playerIdx, Coordinate coord) {
    return m_engine.fire(playerIdx, coord);
}

Protocol::StateUpdate GameSession::buildSnapshot(int playerIdx) const {
    const auto &snap = m_engine.getSnapshot();
    const Board &myBoard = m_engine.getBoard(playerIdx);
    const Board &foeBoard = m_engine.getBoard(1 - playerIdx);

    Protocol::StateUpdate out;
    out.turnNumber = snap.turnNumber;
    out.statusMessage = snap.statusMessage;

    // flip Victory/Defeat so each player sees their own outcome
    switch (snap.state) {
    case MatchState::PlacementPhase:
        out.state = "PlacementPhase";
        break;
    case MatchState::PlayerTurn:
        out.state = (playerIdx == 0) ? "YourTurn" : "OpponentTurn";
        break;
    case MatchState::OpponentTurn:
        out.state = (playerIdx == 0) ? "OpponentTurn" : "YourTurn";
        break;
    case MatchState::Victory:
        out.state = (playerIdx == 0) ? "Victory" : "Defeat";
        break;
    case MatchState::Defeat:
        out.state = (playerIdx == 0) ? "Defeat" : "Victory";
        break;
    }

    out.yourBoard.reserve(Board::SIZE * Board::SIZE);
    out.enemyBoard.reserve(Board::SIZE * Board::SIZE);

    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            out.yourBoard.push_back(static_cast<int>(myBoard.getCell(x, y)));

            CellState ec = foeBoard.getCell(x, y);
            if (ec == CellState::ShipPresent)
                ec = CellState::Empty; // hide unshot ships
            out.enemyBoard.push_back(static_cast<int>(ec));
        }
    }

    return out;
}
