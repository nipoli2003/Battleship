#include "controller/LocalMatch.hpp"
#include "helpers/Log.hpp"

LocalMatch::LocalMatch(int humanPlayerIdx)
    : m_humanIdx(humanPlayerIdx), m_botIdx(1 - humanPlayerIdx) {
    m_engine.startNewGame();
}

void LocalMatch::start() {
    m_engine.placeShips(m_botIdx, m_bot.generateLegalPlacement());
}

bool LocalMatch::submitPlacement(const std::vector<Protocol::ShipPlacement> &ships) {
    std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>> parsed;
    parsed.reserve(ships.size());
    // TODO: probably duplicate code
    for (const auto &sp : ships) {
        parsed.push_back(
            {static_cast<ShipType>(sp.typeID),
             {{sp.x, sp.y},
              sp.orientation == 0 ? Orientation::Horizontal : Orientation::Vertical}});
    }
    if (!m_engine.placeShips(m_humanIdx, parsed))
        return false;
    if (m_engine.bothReady())
        m_engine.finishPlacement();
    markDirty();
    return true;
}

bool LocalMatch::submitShot(int x, int y) {
    if (!isHumanTurn())
        return false;
    if (!m_engine.fire(m_humanIdx, {x, y}))
        return false;
    markDirty();
    return true;
}

bool LocalMatch::isHumanTurn() const {
    auto s = m_engine.getSnapshot().state;
    bool p0 = (s == MatchState::PlayerTurn);
    bool p1 = (s == MatchState::OpponentTurn);
    return (m_humanIdx == 0 && p0) || (m_humanIdx == 1 && p1);
}

bool LocalMatch::isBotTurn() const {
    auto s = m_engine.getSnapshot().state;
    bool p0 = (s == MatchState::PlayerTurn);
    bool p1 = (s == MatchState::OpponentTurn);
    return (m_botIdx == 0 && p0) || (m_botIdx == 1 && p1);
}

void LocalMatch::tick() {
    if (isOver() || !isBotTurn())
        return;
    if (botFireOnce())
        markDirty();
}

bool LocalMatch::botFireOnce() {
    Coordinate target = m_bot.getNextShot();
    if (!m_engine.fire(m_botIdx, target)) {
        LOG_SRV_WARN("bot tried to place illegal shot");
        return false; // target already tried; AI shouldn't do this
    }
    m_bot.recordShotResult(target, m_engine.getSnapshot().lastResult);
    return true;
}

Protocol::StateUpdate LocalMatch::snapshotForHuman() const {
    return m_engine.buildPlayerSnapshot(m_humanIdx);
}

bool LocalMatch::isOver() const {
    auto s = m_engine.getSnapshot().state;
    return s == MatchState::Victory || s == MatchState::Defeat;
}

std::optional<Protocol::StateUpdate> LocalMatch::consumeUpdate() {
    if (!m_dirty)
        return std::nullopt;
    m_dirty = false;
    return m_engine.buildPlayerSnapshot(m_humanIdx);
}