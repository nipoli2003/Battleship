#include "controller/BattleshipEngine.hpp"

BattleshipEngine::BattleshipEngine() {
    startNewGame();
}

void BattleshipEngine::startNewGame() {
    m_boards[0].reset();
    m_boards[1].reset();
    m_placement_done[0] = false;
    m_placement_done[1] = false;

    m_abilityUsed[0] = false;
    m_abilityUsed[1] = false;
    m_sonarLog[0].clear();   
    m_sonarLog[1].clear();   

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

bool BattleshipEngine::hasActiveShip(int playerIdx, ShipType type) const {
    const Board& myBoard = m_boards[playerIdx];
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            auto shipOpt = myBoard.getShipAt(x, y);
            if (shipOpt && *shipOpt) {
                const Ship& ship = **shipOpt;
                if (ship.type() == type && !ship.isSunk()) return true;
            }
        }
    }
    return false;
}

int BattleshipEngine::useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target) {
    // 1. Once per game
    if (m_abilityUsed[playerIdx]) return -1;

    // 2. Must be this player's turn
    bool isPlayerTurn = (m_snapshot.state == MatchState::PlayerTurn);
    if(playerIdx == 0 && !isPlayerTurn) return -1;
    if(playerIdx == 1 && isPlayerTurn) return -1;

    // 3. Target must be in bounds (needed for Sonar and Torpedo/Bombing)
    if (target.x < 0 || target.x >= Board::SIZE || target.y < 0 || target.y >= Board::SIZE) return -1;
    
    // 4. Check ship status and execute ability
    if(ability == Protocol::AbilityType::Sonar){
        // Requires active destroyer
        if(!hasActiveShip(playerIdx, ShipType::Destroyer)) return -1;

        const Board& enemy = m_boards[1 - playerIdx];
        std::vector<const Ship*> found;
        found.reserve(3);
        for(int dy = -1; dy <= 1; ++dy){
            for(int dx = -1; dx <= 1; ++dx){
                int cx = target.x + dx;
                int cy = target.y + dy;
                if(cx < 0 || cx >= Board::SIZE || cy < 0 || cy >= Board::SIZE) continue;
                auto shipOpt = enemy.getShipAt(cx, cy);
                if(shipOpt && *shipOpt){
                    const Ship* s = shipOpt->get();
                    if(std::find(found.begin(), found.end(), s) == found.end()) found.push_back(s);
                }
            }
        }

        m_abilityUsed[playerIdx] = true;
        int count = static_cast<int>(found.size());
        m_sonarLog[playerIdx] = "Sonar @ (" + std::to_string(target.x) + "," + std::to_string(target.y) + "): " +
            (count == 0 ? "no enemy ships detected. " : std::to_string(count) + " enemy ship" + (count == 1 ? "" : "s") + " detected. ");

        return count;
    }

    // if (ability == Protocol::AbilityType::Torpedo) {
    //     if (!hasActiveShip(playerIdx, ShipType::Submarine)) return -1;
    //     // TODO: Implement torpedo (e.g., hit a 3x1 line)
    //     return -1; // Not implemented yet
    // }

    // if (ability == Protocol::AbilityType::BombingRun) {
    //     if (!hasActiveShip(playerIdx, ShipType::Carrier)) return -1;
    //     // TODO: Implement bombing run (e.g., hit a cross shape)
    //     return -1; // Not implemented yet
    // }

    return -1;
}