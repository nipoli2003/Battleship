#include "network/GameSession.hpp"
#include "model/Protocol.hpp"

namespace {

    // The engine keeps one neutral snapshot for both players, so its statusMessage
    // can't say "you" vs "opponent". Build the text per player here instead.
    std::string buildStatusMessage(const GameSnapshot &snap, int playerIdx) {
        const bool iShot = (snap.lastShooter == playerIdx);

        // 1. Check if an ability was just used
        if (snap.lastAbilityType != -1) {
            std::string msg = iShot ? "Your " : "Opponent ";
            
            if (snap.lastAbilityType == static_cast<int>(Protocol::AbilityType::Sonar)) {
                msg += "Sonar: " + std::to_string(snap.lastAbilityResult) + " ships detected. ";
            } else if (snap.lastAbilityType == static_cast<int>(Protocol::AbilityType::Torpedo)) {
                msg += "Torpedo: " + std::string(snap.lastAbilityResult ? "Hit! " : "Missed. ");
            } else if (snap.lastAbilityType == static_cast<int>(Protocol::AbilityType::BombingRun)) {
                msg += "Bombing Run: " + std::to_string(snap.lastAbilityResult) + " hits. ";
            }
            
            msg += iShot ? "Opponent's turn..." : "Your turn - fire!";
            return msg;
        }

        // 2. Otherwise, standard shot logic
        const bool myTurn = (snap.state == MatchState::PlayerTurn && playerIdx == 0) ||
                            (snap.state == MatchState::OpponentTurn && playerIdx == 1);

        switch (snap.state) {
        case MatchState::PlacementPhase: return snap.statusMessage;
        case MatchState::Victory:
        case MatchState::Defeat: return "Game over!";
        default: break;
        }

        std::string last;
        if (snap.lastShooter != -1) {
            switch (snap.lastResult) {
            case AttackResult::Miss: last = iShot ? "You missed. " : "Opponent missed. "; break;
            case AttackResult::Hit:  last = iShot ? "Hit! " : "Opponent hit your ship! "; break;
            case AttackResult::Sunk: last = iShot ? "You sunk a ship! " : "Opponent sunk one of your ships! "; break;
            default: break;
            }
        }
        return last + (myTurn ? "Your turn - fire!" : "Opponent's turn...");
    }
} // namespace

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

    out.lastShooter = snap.lastShooter;
    out.lastResult  = static_cast<int>(snap.lastResult);
    
    // Prepend the persistent ability logs (empty string if unused)
    const std::string sonar = m_engine.getSonarLog(playerIdx);
    const std::string torpedo = m_engine.getTorpedoLog(playerIdx);

    // new implementation for status message and ability logs
    out.statusMessage = buildStatusMessage(snap, playerIdx);
    if(!sonar.empty()) out.abilityLogs.push_back(sonar);
    if(!torpedo.empty()) out.abilityLogs.push_back(torpedo);

    for (int a = 0; a < Protocol::ABILITY_COUNT; ++a) {
        out.ability_charges.push_back(m_engine.getAbilityCharges(playerIdx, static_cast<Protocol::AbilityType>(a)));
    }

    // flip Victory/Defeat so each player sees their own outcome
    switch (snap.state) {
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
    out.lastAbilityType = snap.lastAbilityType;
    out.lastAbilityX = snap.lastAbilityX;
    out.lastAbilityY = snap.lastAbilityY;
    out.lastAbilityResult = snap.lastAbilityResult;

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

int GameSession::useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target, Orientation orientation){
    return m_engine.useAbility(playerIdx, ability, target, orientation);
}

bool GameSession::hasAbility(int playerIdx, Protocol::AbilityType ability) const noexcept{
    return m_engine.getAbilityCharges(playerIdx, ability);
}
