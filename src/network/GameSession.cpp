#include "network/GameSession.hpp"
#include "model/Protocol.hpp"

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
