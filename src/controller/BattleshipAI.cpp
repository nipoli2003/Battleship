#include "controller/BattleshipAI.hpp"
#include <chrono>

BattleshipAI::BattleshipAI()
    : m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())) {
    reset();
}

const FleetPlacement BattleshipAI::generateLegalPlacement() {
    // TODO: for now, uses a random placement
    Board scratch;
    return scratch.generateRandomFleet(m_rng);
}

Coordinate BattleshipAI::getNextShot() {
    Coordinate target = {};
    std::vector<Coordinate> hits = findCellsByKnowledge(CellKnowledge::Hit);
    if (hits.empty()) {
        target = huntMode();
    } else {
        target = targetMode(hits);
    }
    return target;
}

void BattleshipAI::recordShotResult(Coordinate coord, AttackResult result, std::optional<ShipType> sunk) {
    if (result == AttackResult::Hit) {
        m_knowledge[coord.y][coord.x] = CellKnowledge::Hit;
    } else if (result == AttackResult::Miss) {
        m_knowledge[coord.y][coord.x] = CellKnowledge::Miss;
    } else {
        updateSunkCells(coord, sunk);

    }
}

void BattleshipAI::reset() {
    m_remainingShips = standardFleet;
    m_knowledge = {};
    m_score = {};
}

Coordinate BattleshipAI::huntMode() {
    std::vector<Coordinate> targets = {};
    evaluateScores();
    targets = getBestCells();
    std::uniform_int_distribution<size_t> dist(0, targets.size() - 1);
    return targets[dist(m_rng)];
}

Coordinate BattleshipAI::targetMode(std::vector<Coordinate> hits) {
    if (hits.size() == 1) {
        std::vector<Coordinate> candidates;
        candidates = getUnknownNeighborsOfHits(hits);
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        return candidates[dist(m_rng)];
    } else {
        std::vector<Coordinate> candidates = pickLineShots(hits);
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        return candidates[dist(m_rng)];
    }
}

std::vector<Coordinate> BattleshipAI::pickLineShots(std::vector<Coordinate> hits) {
    std::vector<Coordinate> candidates;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto &h : hits) {
        for (const auto &d : dirs) {
            int x = h.x + d[0];
            int y = h.y + d[1];
            if (!inBounds(x, y) || m_knowledge[y][x] != CellKnowledge::Hit)
                continue;
            while (inBounds(x, y) && m_knowledge[y][x] == CellKnowledge::Hit) {
                x += d[0];
                y += d[1];
            }
            if (inBounds(x, y) && m_knowledge[y][x] == CellKnowledge::Unknown)
                candidates.push_back({x, y});
        }
    }
    if (candidates.empty()){
        return getUnknownNeighborsOfHits(hits);
    }
    return candidates;
}

void BattleshipAI::evaluateScores() {
    m_score = {};
    for (const auto &e : m_remainingShips) {
        int shipLength = Ship(e).length();
        scoreShipPlacements(shipLength);
    }
}

void BattleshipAI::scoreShipPlacements(int shipLength) {
    for (int orient = 0; orient < 2; ++orient) {
        const int dx = (orient == 0) ? 1 : 0;
        const int dy = (orient == 0) ? 0 : 1;
        const int maxX = Board::SIZE - (dx ? shipLength : 1);
        const int maxY = Board::SIZE - (dy ? shipLength : 1);

        for (int y = 0; y <= maxY; ++y) {
            for (int x = 0; x <= maxX; ++x) {
                bool fits = true;
                for (int i = 0; i < shipLength; ++i) {
                    if (m_knowledge[y + i * dy][x + i * dx] != CellKnowledge::Unknown) {
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

std::vector<Coordinate> BattleshipAI::getBestCells() {
    std::vector<Coordinate> bests;
    int maxScore = 0;
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            if (m_knowledge[y][x] != CellKnowledge::Unknown)
                continue;
            const int score = m_score[y][x];
            if (score > maxScore) {
                maxScore = score;
                bests.clear();
                bests.push_back({x, y});
            } else if (score == maxScore && score > 0) {
                bests.push_back({x, y});
            }
        }
    }
    return bests;
}

void BattleshipAI::updateSunkCells(Coordinate coord, std::optional<ShipType> sunk) {
    m_knowledge[coord.y][coord.x] = CellKnowledge::Hit;
    if (!sunk)
        return;
    // erase sunk ship from the list
    std::erase(m_remainingShips, *sunk);
    const int length = Ship(*sunk).length();
    for (int orient = 0; orient < 2; ++orient) {
        const int dx = (orient == 0) ? 1 : 0;
        const int dy = (orient == 0) ? 0 : 1;
        for (int k = 0; k < length; ++k) {
            const int startX = coord.x - k * dx;
            const int startY = coord.y - k * dy;
            const int endX = startX + (length - 1) * dx;
            const int endY = startY + (length - 1) * dy;
            if (!inBounds(startX, startY) || !inBounds(endX, endY))
                continue;
            bool allHit = true;
            for (int i = 0; i < length; ++i) {
                if (m_knowledge[startY + i * dy][startX + i * dx] != CellKnowledge::Hit) {
                    allHit = false;
                    break;
                }
            }
            if (!allHit)
                continue;
            for (int i = 0; i < length; ++i)
                m_knowledge[startY + i * dy][startX + i * dx] = CellKnowledge::Sunk;
            return;
        }
    }
}

std::vector<Coordinate> BattleshipAI::findCellsByKnowledge(CellKnowledge value) const {
    std::vector<Coordinate> result;
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            if (m_knowledge[y][x] == value)
                result.push_back({x, y});
        }
    }
    return result;
}

std::vector<Coordinate> BattleshipAI::getNeighbors(Coordinate center) {
    const Coordinate candidates[4] = {
        {center.x + 1, center.y},
        {center.x - 1, center.y},
        {center.x, center.y + 1},
        {center.x, center.y - 1}
    };

    std::vector<Coordinate> neighbors;
    neighbors.reserve(4);
    for (const auto &c : candidates) {
        if (inBounds(c))
            neighbors.push_back(c);
    }
    return neighbors;
}

std::vector<Coordinate> BattleshipAI::getUnknownNeighborsOfHits(std::vector<Coordinate> hits) {
    std::vector<Coordinate> candidates;
    for (const auto &h : hits) {
        for (const auto &n : getNeighbors(h)) {
            if (m_knowledge[n.y][n.x] == CellKnowledge::Unknown)
                candidates.push_back(n);
        }
    }
    return candidates;
}

bool BattleshipAI::inBounds(int x, int y) noexcept {
    return x >= 0 && x < Board::SIZE && y >= 0 && y < Board::SIZE;
}

bool BattleshipAI::inBounds(Coordinate c) noexcept {
    return inBounds(c.x, c.y);
}
