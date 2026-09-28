#include "controller/BattleshipAI.hpp"
#include <algorithm>
#include <chrono>

BattleshipAI::BattleshipAI() 
    : m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())) {
    reset();
}

void BattleshipAI::reset(int boardSize) {
    m_boardSize = boardSize;
    m_remainingTargets.clear();
    m_priorityTargets.clear();

    for (int y = 0; y < m_boardSize; ++y) {
        for (int x = 0; x < m_boardSize; ++x) {
            m_remainingTargets.push_back({x, y});
        }
    }
}

Coordinate BattleshipAI::getNextShot() {
    Coordinate target{};

    // 1. Target Mode: attack adjacent cells if there are recent unconfirmed hits
    while (!m_priorityTargets.empty()) {
        target = m_priorityTargets.back();
        m_priorityTargets.pop_back();

        auto it = std::find(m_remainingTargets.begin(), m_remainingTargets.end(), target);
        if (it != m_remainingTargets.end()) {
            m_remainingTargets.erase(it);
            return target;
        }
    }

    // 2. Hunt Mode: pick a random coordinate from available cells
    std::uniform_int_distribution<size_t> dist(0, m_remainingTargets.size() - 1);
    size_t index = dist(m_rng);
    target = m_remainingTargets[index];
    m_remainingTargets.erase(m_remainingTargets.begin() + index);

    return target;
}

void BattleshipAI::recordShotResult(Coordinate coord, AttackResult result) {
    if (result == AttackResult::Hit) {
        addNeighbors(coord);
    } else if (result == AttackResult::Sunk) {
        // Ship is destroyed: clear hunting neighbors and resume broad sweep
        m_priorityTargets.clear();
    }
}

void BattleshipAI::addNeighbors(Coordinate c) {
    const std::vector<Coordinate> neighbors = {
        {c.x + 1, c.y}, {c.x - 1, c.y},
        {c.x, c.y + 1}, {c.x, c.y - 1}
    };

    for (const auto& n : neighbors) {
        if (n.x >= 0 && n.x < m_boardSize && n.y >= 0 && n.y < m_boardSize) {
            m_priorityTargets.push_back(n);
        }
    }
}

void BattleshipAI::placeShipsRandomly(Board& board) {
    // True hardware entropy device to avoid seed repetition
    std::random_device rd;
    std::mt19937 rng(rd());

    const int n = board.size();
    std::uniform_int_distribution<int> orientDist(0, 1);

    const std::vector<ShipType> shipTypes = {
        ShipType::Carrier,
        ShipType::Battleship,
        ShipType::Cruiser,
        ShipType::Submarine,
        ShipType::Destroyer
    };

    for (auto type : shipTypes) {
        bool placed = false;
        int attempts = 0;

        while (!placed && attempts < 1000) {
            attempts++;
            auto orientation = (orientDist(rng) == 0) ? Orientation::Horizontal : Orientation::Vertical;
            
            // Generate coordinates constrained to board limits based on orientation
            Ship testShip(type, orientation);
            int maxX = (orientation == Orientation::Horizontal) ? (n - testShip.length()) : (n - 1);
            int maxY = (orientation == Orientation::Vertical) ? (n - testShip.length()) : (n - 1);

            std::uniform_int_distribution<int> xDist(0, maxX);
            std::uniform_int_distribution<int> yDist(0, maxY);

            Coordinate coord{xDist(rng), yDist(rng)};
            auto ship = std::make_shared<Ship>(type, orientation);
            placed = board.placeShip(ship, coord);
        }
    }
}