#include "network/GameSession.hpp"
#include "model/Protocol.hpp"

void GameSession::setNames(std::string name0, std::string name1) {
    m_names[0] = std::move(name0); m_names[1] = std::move(name1);
}

Protocol::StateUpdate GameSession::buildSnapshot(int playerIdx) const {
    auto out = m_engine.buildPlayerSnapshot(playerIdx);
    out.yourName = m_names[playerIdx]; out.opponentName = m_names[1 - playerIdx];
    return out;
}

bool GameSession::placeShips(int playerIdx, const std::vector<Protocol::ShipPlacement> &ships) {
    // translate Protocol::ShipPlacement into what the engine expects
    std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>> parsed;
    parsed.reserve(ships.size());

    for (const auto &sp : ships) {
        ShipType type = static_cast<ShipType>(sp.typeID);
        Coordinate coord = {sp.x, sp.y};
        Orientation orient = (sp.orientation == 0) ? Orientation::Horizontal : Orientation::Vertical;
        parsed.push_back({type, {coord, orient}});
    }

    bool ok = m_engine.placeShips(playerIdx, parsed);

    if (ok && m_engine.bothReady()) {
        m_engine.finishPlacement();
    }

    return ok;
}

bool GameSession::fire(int playerIdx, Coordinate coord) {
    return m_engine.fire(playerIdx, coord);
}

int GameSession::useAbility(int playerIdx, Protocol::AbilityType ability, Coordinate target, Orientation orientation) {
    return m_engine.useAbility(playerIdx, ability, target, orientation);
}

bool GameSession::hasAbility(int playerIdx, Protocol::AbilityType ability) const noexcept {
    return m_engine.getAbilityCharges(playerIdx, ability);
}
