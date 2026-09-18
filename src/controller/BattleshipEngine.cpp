#include "controller/BattleshipEngine.hpp"

BattleshipEngine::BattleshipEngine(OpponentType opponentType)
    : m_opponentType(opponentType) {
    startNewGame();
}

void BattleshipEngine::startNewGame() {
    m_humanBoard.reset();
    m_opponentBoard.reset();
    m_ai.reset();

    // Auto-populate ships for testing / default setup
    BattleshipAI::placeShipsRandomly(m_humanBoard);
    BattleshipAI::placeShipsRandomly(m_opponentBoard);

    m_snapshot = GameSnapshot{};
    m_snapshot.state = MatchState::PlayerTurn;
    m_snapshot.statusMessage = "Battle stations! Select an enemy tile to fire.";
}

bool BattleshipEngine::humanFire(Coordinate target) {
    if (m_snapshot.state != MatchState::PlayerTurn) return false;

    AttackResult result = m_opponentBoard.receiveAttack(target);
    if (result == AttackResult::Invalid) return false;

    if (result == AttackResult::Hit || result == AttackResult::Sunk) {
        m_snapshot.statusMessage = (result == AttackResult::Sunk) ? "Enemy ship SUNK!" : "Direct hit!";
    } else {
        m_snapshot.statusMessage = "Splash... Miss!";
    }

    if (m_opponentBoard.allShipsSunk()) {
        m_snapshot.state = MatchState::Victory;
        m_snapshot.statusMessage = "VICTORY! All enemy ships destroyed!";
        return true;
    }

    // Transfer turn
    m_snapshot.state = MatchState::OpponentTurn;
    return true;
}

void BattleshipEngine::processAITurn() {
    if (m_snapshot.state != MatchState::OpponentTurn || m_opponentType != OpponentType::LocalAI) {
        return;
    }

    Coordinate shot = m_ai.getNextShot();
    AttackResult result = m_humanBoard.receiveAttack(shot);

    m_ai.recordShotResult(shot, result);

    if (m_humanBoard.allShipsSunk()) {
        m_snapshot.state = MatchState::Defeat;
        m_snapshot.statusMessage = "DEFEAT! Your fleet has been annihilated.";
        return;
    }

    m_snapshot.state = MatchState::PlayerTurn;
    m_snapshot.turnNumber++;
}
