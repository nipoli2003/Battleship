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
        target = huntMode();
    }else{
        target = targetMode();
    }
    return target;
}

void BattleshipAI::recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk) {
}

Coordinate BattleshipAI::huntMode(){
    std::vector<Coordinate> targets = {};
    evaluateCells();
    targets = getMaximalScorePositions();
    m_score = {};
    std::uniform_int_distribution<size_t> dist(0, targets.size() - 1);
    return targets[dist(m_rng)];
}

Coordinate BattleshipAI::targetMode(){
}

void BattleshipAI::evaluateCells(){
    for (const auto &e : m_remainingShips){
        int shipLength = Ship(e).length();
        evaluateCellsSingleShip(shipLength);
    }
}

void BattleshipAI::evaluateCellsSingleShip(int shipLength){
    for (int orient = 0; orient < 2; ++orient) {
        const int dx = (orient == 0) ? 1 : 0;
        const int dy = (orient == 0) ? 0 : 1;
        const int maxX = Board::SIZE - (dx ? shipLength : 1);
        const int maxY = Board::SIZE - (dy ? shipLength : 1);

        for (int y = 0; y <= maxY; ++y) {
            for (int x = 0; x <= maxX; ++x) {
                bool fits = true;
                for (int i = 0; i < shipLength; ++i) {
                    if (m_known[y + i * dy][x + i * dx] != Knowledge::Unknown) {
                        fits = false;
                        break;
                    }
                }
                if (!fits)
                    continue;
                for (int i = 0; i < shipLength; ++i)
                    ++m_score[y + i * dy][x + i * dx];
            }
        }
    }
}

std::vector<Coordinate> BattleshipAI::getMaximalScorePositions(){
    std::vector<Coordinate> best;
    int maxScore = 0;

    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            if (m_known[y][x] != Knowledge::Unknown)
                continue;

            const int score = m_score[y][x];
            if (score > maxScore) {
                maxScore = score;
                best.clear();
                best.push_back({x, y});
            } else if (score == maxScore && score > 0) {
                best.push_back({x, y});
            }
        }
    }
    return best;
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
