#include "controller/BattleshipEngine.hpp"
#include <random>
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

bool BattleshipEngine::placeShips(int playerIdx, const FleetPlacement &ships) {
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
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> choosePlayer(0, 1);

    m_snapshot.state = choosePlayer(rng) == 0
                           ? MatchState::PlayerTurn
                           : MatchState::OpponentTurn;
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
    m_snapshot.lastAbilityType = -1;

    m_snapshot.lastAbilityType = -1;

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
        if (target.x < 1 || target.x > Board::SIZE - 2 || target.y < 1 || target.y > Board::SIZE - 2) {
            return -1; 
        }
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
