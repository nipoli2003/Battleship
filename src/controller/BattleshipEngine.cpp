#include "controller/BattleshipEngine.hpp"

BattleshipEngine::BattleshipEngine(OpponentType opponentType)
    : m_opponentType(opponentType) {
    startNewGame();
}

void BattleshipEngine::startNewGame() {
    m_humanBoard.reset();
    m_opponentBoard.reset();
    m_ai.reset();

    // Standard Battleship fleet
    m_fleetToPlace = {
        ShipType::Carrier,     // 5
        ShipType::Battleship,  // 4
        ShipType::Cruiser,     // 3
        ShipType::Submarine,   // 3
        ShipType::Destroyer    // 2
    };
    m_currentPlacementIndex = 0;

    // Bot places its fleet immediately
    BattleshipAI::placeShipsRandomly(m_opponentBoard);

    m_snapshot = GameSnapshot{};
    m_snapshot.state = MatchState::PlacementPhase;
    m_snapshot.statusMessage = "Place your Carrier (5). Press [R] to rotate.";
    m_aiTimer = 0.0f;
}

std::optional<ShipType> BattleshipEngine::getCurrentPlacementType() const noexcept {
    if (m_currentPlacementIndex < m_fleetToPlace.size()) {
        return m_fleetToPlace[m_currentPlacementIndex];
    }
    return std::nullopt;
}

bool BattleshipEngine::isPlacementComplete() const noexcept {
    return m_currentPlacementIndex >= m_fleetToPlace.size();
}

bool BattleshipEngine::placeCurrentShip(Coordinate target, Orientation orientation) {
    if (m_snapshot.state != MatchState::PlacementPhase || isPlacementComplete()) {
        return false;
    }

    auto type = m_fleetToPlace[m_currentPlacementIndex];
    auto ship = std::make_shared<Ship>(type, orientation);

    if (!m_humanBoard.placeShip(ship, target)) {
        return false;
    }

    m_currentPlacementIndex++;

    if (isPlacementComplete()) {
        finishPlacement();
    } else {
        auto nextType = m_fleetToPlace[m_currentPlacementIndex];
        Ship temp(nextType);
        m_snapshot.statusMessage = "Place your " + std::string(temp.name()) + 
                                   " (" + std::to_string(temp.length()) + "). Press [R] to rotate.";
    }

    return true;
}

void BattleshipEngine::randomizeHumanFleet() {
    m_humanBoard.reset();
    BattleshipAI::placeShipsRandomly(m_humanBoard);
    m_currentPlacementIndex = m_fleetToPlace.size();
    finishPlacement();
}

void BattleshipEngine::finishPlacement() {
    m_snapshot.state = MatchState::PlayerTurn;
    m_snapshot.statusMessage = "Battle stations! Your turn to strike.";
}

bool BattleshipEngine::humanFire(Coordinate target) {
    if (m_snapshot.state != MatchState::PlayerTurn) return false;

    AttackResult result = m_opponentBoard.receiveAttack(target);
    if (result == AttackResult::Invalid) return false;

    if (m_opponentBoard.allShipsSunk()) {
        m_snapshot.state = MatchState::Victory;
        m_snapshot.statusMessage = "VICTORY! All enemy ships destroyed!";
        return true;
    }

    // Play again on Hit or Sunk; turn passes only on Miss
    if (result == AttackResult::Hit || result == AttackResult::Sunk) {
        m_snapshot.statusMessage = (result == AttackResult::Sunk) ? 
            "Enemy ship SUNK! Fire again!" : "Direct HIT! Take another shot!";
    } else {
        m_snapshot.statusMessage = "Splash... Miss! Enemy is targeting...";
        m_snapshot.state = MatchState::OpponentTurn;
        m_aiTimer = AI_DELAY_SECONDS; // Arm the 2.0-second delay
    }

    return true;
}

void BattleshipEngine::update(float dt) {
    if (m_snapshot.state == MatchState::OpponentTurn && m_opponentType == OpponentType::LocalAI) {
        m_aiTimer -= dt;
        if (m_aiTimer <= 0.0f) {
            processAITurn();
        }
    }
}

void BattleshipEngine::processAITurn() {
    Coordinate shot = m_ai.getNextShot();
    AttackResult result = m_humanBoard.receiveAttack(shot);
    m_ai.recordShotResult(shot, result);

    if (m_humanBoard.allShipsSunk()) {
        m_snapshot.state = MatchState::Defeat;
        m_snapshot.statusMessage = "DEFEAT! Your fleet has been sunk.";
        return;
    }

    // AI also fires again upon Hit / Sunk
    if (result == AttackResult::Hit || result == AttackResult::Sunk) {
        m_snapshot.statusMessage = (result == AttackResult::Sunk) ? 
            "Alert! Bot SUNK one of your ships! Bot fires again..." : "Warning: bot scored a HIT! Bot fires again...";
        m_aiTimer = AI_DELAY_SECONDS; // Delay before subsequent bot shot
    } else {
        m_snapshot.statusMessage = "Bot missed! Your turn!";
        m_snapshot.state = MatchState::PlayerTurn;
        m_snapshot.turnNumber++;
    }
}