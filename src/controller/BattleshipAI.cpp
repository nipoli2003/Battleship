#include "controller/BattleshipAI.hpp"
#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>

BattleshipAI::BattleshipAI()
    : m_rng(static_cast<unsigned>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {
    reset();
}

void BattleshipAI::reset() {
    m_activeHits.clear();
    m_huntParity.clear();
    m_huntOther.clear();
    m_fired.assign(Board::SIZE, std::vector<bool>(Board::SIZE, false));

    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x)
            ((x + y) % 2 == 0 ? m_huntParity : m_huntOther).push_back({x, y});
}

Coordinate BattleshipAI::getNextShot() {
    auto isFree = [this](Coordinate c) {
        return c.x >= 0 && c.x < Board::SIZE &&
               c.y >= 0 && c.y < Board::SIZE &&
               !m_fired[c.x][c.y];
    };
    auto isHit = [this](Coordinate c) {
        return std::find(m_activeHits.begin(), m_activeHits.end(), c) != m_activeHits.end();
    };
    auto commit = [this](Coordinate c) { m_fired[c.x][c.y] = true; return c; };
    static const std::pair<int, int> dirs[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    // 1. Extend a line of 2+ collinear hits (highest priority)
    for (const auto &h : m_activeHits)
        for (auto [dx, dy] : dirs) {
            Coordinate n{h.x + dx, h.y + dy};
            if (isFree(n) &&
                (isHit({h.x - dx, h.y - dy}) || isHit({h.x + 2 * dx, h.y + 2 * dy})))
                return commit(n);
        }

    // 2. Any free neighbor of an active hit
    for (const auto &h : m_activeHits)
        for (auto [dx, dy] : dirs) {
            Coordinate n{h.x + dx, h.y + dy};
            if (isFree(n))
                return commit(n);
        }

    // 3. Hunt: parity-preferred, then any remaining cell
    auto pick = [this, &commit](std::vector<Coordinate> &list) -> std::optional<Coordinate> {
        while (!list.empty()) {
            std::uniform_int_distribution<size_t> dist(0, list.size() - 1);
            size_t i = dist(m_rng);
            Coordinate t = list[i];
            list.erase(list.begin() + i);
            if (!m_fired[t.x][t.y])
                return commit(t);
        }
        return std::nullopt;
    };
    if (auto t = pick(m_huntParity))
        return *t;
    if (auto t = pick(m_huntOther))
        return *t;
    return {-1, -1};
}

void BattleshipAI::recordShotResult(Coordinate coord, AttackResult result) {
    if (result == AttackResult::Hit) {
        m_activeHits.push_back(coord);
        return;
    }
    if (result != AttackResult::Sunk)
        return;

    // Ships are straight lines. Find the axis (horizontal/vertical) that
    // contains the most contiguous active hits through `coord`, then erase
    // all contiguous active hits along that axis (those are the sunk ship's
    // cells).
    auto active = [this](Coordinate c) {
        return std::find(m_activeHits.begin(), m_activeHits.end(), c) != m_activeHits.end();
    };
    auto countIn = [&](int dx, int dy) {
        int len = 1; // the sunk shot itself counts as one cell of the ship
        for (int s : {1, -1})
            for (int k = 1; active({coord.x + dx * s * k, coord.y + dy * s * k}); ++k)
                ++len;
        return len;
    };
    bool horizontal = countIn(1, 0) >= countIn(0, 1);
    int dx = horizontal ? 1 : 0, dy = horizontal ? 0 : 1;

    auto erase = [this](Coordinate c) {
        m_activeHits.erase(std::remove(m_activeHits.begin(), m_activeHits.end(), c),
                           m_activeHits.end());
    };
    erase(coord); // typically a no-op; Sunk never adds coord to m_activeHits
    for (int s : {1, -1}) {
        for (int k = 1;; ++k) {
            Coordinate c{coord.x + dx * s * k, coord.y + dy * s * k};
            if (!active(c))
                break;
            erase(c);
        }
    }
}

const FleetPlacement BattleshipAI::generateLegalPlacement() {
    Board scratch;
    return scratch.generateRandomFleet(m_rng);
}