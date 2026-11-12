#include "controller/BattleshipEngine.hpp"
#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include <string>

BattleshipEngine::BattleshipEngine() {
    startNewGame();
}

void BattleshipEngine::startNewGame() {
    m_boards[0].reset();
    m_boards[1].reset();
    m_placement_done[0] = false;
    m_placement_done[1] = false;

    for (int p = 0; p < 2; ++p) {
        for (int c = 0; c < Protocol::ABILITY_COUNT; ++c) {
            m_abilities[p][c] = Protocol::START_CHARGES[c];
        }
    }
    m_sonarLog[0].clear();   
    m_sonarLog[1].clear();   
    m_torpedoLog[0].clear();
    m_torpedoLog[1].clear();
    m_bombingRunLog[0].clear();
    m_bombingRunLog[1].clear();

    m_snapshot = GameSnapshot{};
    m_snapshot.state = MatchState::PlacementPhase;
    m_snapshot.statusMessage = "Waiting for both players to place ships.";
}

bool BattleshipEngine::placeShips(int playerIdx, const std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>> &ships) {
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

int BattleshipEngine::useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target, Orientation orientation) {
    const int ability_number = static_cast<int>(ability);

    // 1. Once per game
    if (m_abilities[playerIdx][ability_number] == 0) return -1;

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
        return useSonar(playerIdx, target);
    }

    if (ability == Protocol::AbilityType::Torpedo) {
        // Requires active submarine
        if (!hasActiveShip(playerIdx, ShipType::Submarine)) return -1;
        return fireTorpedo(playerIdx, target, orientation);
    }

    if (ability == Protocol::AbilityType::BombingRun) {
        // Requires active Carrier
        if (!hasActiveShip(playerIdx, ShipType::Carrier)) return -1; 
        return fireBombingRun(playerIdx, target, orientation);
    }

    return -1;
}

int BattleshipEngine::useSonar(int playerIdx, Coordinate target) {
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

    m_abilities[playerIdx][static_cast<int>(Protocol::AbilityType::Sonar)] = 0;
    int count = static_cast<int>(found.size());

    m_snapshot.lastShooter = playerIdx; 
    m_snapshot.lastResult = (count > 0) ? AttackResult::Hit : AttackResult::Miss; // Just so the SFX plays
    m_snapshot.lastAbilityType = static_cast<int>(Protocol::AbilityType::Sonar);
    m_snapshot.lastAbilityX = target.x;
    m_snapshot.lastAbilityY = target.y;
    m_snapshot.lastAbilityResult = count;

    m_snapshot.state = (playerIdx == 0) ? MatchState::OpponentTurn : MatchState::PlayerTurn;
    m_snapshot.turnNumber++;
    return count;
}
 
int BattleshipEngine::fireTorpedo(int playerIdx, Coordinate target, Orientation orientation) {
    Board& enemy = m_boards[1 - playerIdx];
    const int dx = (orientation == Orientation::Horizontal) ? 1 : 0;
    const int dy = (orientation == Orientation::Vertical) ? 1 : 0;

    // Start from the edge of the board
    int startX = (orientation == Orientation::Horizontal) ? 0 : target.x;
    int startY = (orientation == Orientation::Vertical) ? 0 : target.y;

    std::vector<Coordinate> path;   /// the path the torpedo takes until a ship is hit, or it reaches the end of the board
    Coordinate impact;              /// the coordinate of the impact
    AttackResult result = AttackResult::Miss;

    for (int i = 0; i < Board::SIZE; ++i) {
        Coordinate c{startX + i * dx, startY + i * dy};
        
        if(c.x < 0 || c.x >= Board::SIZE || c.y < 0 || c.y >= Board::SIZE) break;
        
        CellState enemy_cell = enemy.getCell(c.x, c.y);
        path.push_back(c);
        
        // Skip cells that were already shot
        if (enemy_cell == CellState::Hit || enemy_cell == CellState::Miss) continue;
        
        result = enemy.receiveAttack(c);
        
        // Stop the torpedo if it hits a ship
        if ((result == AttackResult::Hit) || (result == AttackResult::Sunk)) {
            impact = c;
            break;
        }
    }

    const bool hit = (result == AttackResult::Hit) || (result == AttackResult::Sunk);

    m_abilities[playerIdx][static_cast<int>(Protocol::AbilityType::Torpedo)]--;

    m_snapshot.lastShooter = playerIdx;
    m_snapshot.lastResult = result;
    
    m_snapshot.lastAbilityType = static_cast<int>(Protocol::AbilityType::Torpedo);
    m_snapshot.lastAbilityX = target.x;
    m_snapshot.lastAbilityY = target.y;
    m_snapshot.lastAbilityResult = hit ? 1 : 0; // Torpedo returns 1 for hit, 0 for miss

    m_torpedoLog[playerIdx] = "Torpedo @ (" + std::to_string(target.x) + "," + std::to_string(target.y) +
                                "): " + (hit ? "hit at (" + std::to_string(impact.x) + "," + std::to_string(impact.y) +
                                "). " :  "no hits. ");
                            
    if (enemy.allShipsSunk()) {
        m_snapshot.state = (playerIdx == 0) ? MatchState::Victory : MatchState::Defeat;
        m_snapshot.statusMessage = "Game over!";
        return 1;
    }
    else {
        m_snapshot.state = (playerIdx == 0) ? MatchState::OpponentTurn : MatchState::PlayerTurn;
        m_snapshot.turnNumber++;
        return hit ? 1 : 0;
    }
}

int BattleshipEngine::fireBombingRun(int playerIdx, Coordinate target, Orientation orientation) {
    Board& enemy = m_boards[1 - playerIdx];
    const int dx = (orientation == Orientation::Horizontal) ? 1 : 0;
    const int dy = (orientation == Orientation::Vertical) ? 1 : 0;
    int startX = target.x;
    int startY = target.y;

    Coordinate impact;
    AttackResult result = AttackResult::Miss;
    int hits = 0; // <-- Track hits

    for (int i = 0; i < 3; ++i) {
        int cx = startX + (i * dx);
        int cy = startY + (i * dy);

        if (cx >= 0 && cx < Board::SIZE && cy >= 0 && cy < Board::SIZE) {
            AttackResult res = enemy.receiveAttack({cx, cy});
            if (res == AttackResult::Sunk) {
                result = AttackResult::Sunk;
                impact = {cx, cy};
                hits++;
            } else if (res == AttackResult::Hit && result != AttackResult::Sunk) {
                result = AttackResult::Hit;
                impact = {cx, cy};
                hits++;
            }
        }
    }

    m_abilities[playerIdx][static_cast<int>(Protocol::AbilityType::BombingRun)]--;

    m_snapshot.lastShooter = playerIdx;
    m_snapshot.lastResult = result;
    
    m_snapshot.lastAbilityType = static_cast<int>(Protocol::AbilityType::BombingRun);
    m_snapshot.lastAbilityX = target.x;
    m_snapshot.lastAbilityY = target.y;
    m_snapshot.lastAbilityResult = hits;

    if (enemy.allShipsSunk()) {
        m_snapshot.state = (playerIdx == 0) ? MatchState::Victory : MatchState::Defeat;
        m_snapshot.statusMessage = "Game over!";
        return hits;
    }
    else {
        m_snapshot.state = (playerIdx == 0) ? MatchState::OpponentTurn : MatchState::PlayerTurn;
        m_snapshot.turnNumber++;
        return hits;
    }
}
