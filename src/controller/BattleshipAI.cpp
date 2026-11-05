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
    m_known = {};
    m_score = {};
}

Coordinate BattleshipAI::getNextShot() {
    Coordinate target = {};
    std::vector<Coordinate> hits = this->findKnowledgeCells(Knowledge::Hit);
    if (hits.empty()){
        target = targetMode();
    }else{
        target = huntMode();
    }
    return target;
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

std::vector<Coordinate> BattleshipAI::findKnowledgeCells(Knowledge value) const {
    std::vector<Coordinate> result;
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            if (m_known[y][x] == value)
                result.push_back({x, y});
        }
    }
    return result;
}
