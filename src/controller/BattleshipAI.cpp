#include "controller/BattleshipAI.hpp"
#include "helpers/Log.hpp"
#include <algorithm>
#include <chrono>

BattleshipAI::BattleshipAI()
    : m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())) {
    reset();
}

void BattleshipAI::reset() {
    m_remainingShips = standardFleet;
}

Coordinate BattleshipAI::getNextShot() {
}

void BattleshipAI::recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk) {
}

Coordinate BattleshipAI::huntMode(){
}

Coordinate BattleshipAI::targetMode(){
}

const FleetPlacement BattleshipAI::generateLegalPlacement() {
    // TODO: for now, uses a random placement
    Board scratch;
    return scratch.generateRandomFleet(m_rng);
}
