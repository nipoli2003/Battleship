#ifndef GAME_SESSION_HPP
#define GAME_SESSION_HPP

#include "controller/BattleshipEngine.hpp"
#include "model/Protocol.hpp"

// game-state wrapper for GameServer
class GameSession {
  public:
    GameSession() = default;

    // Returns false if placement was already submitted or ships are invalid.
    bool placeShips(int playerIdx, const std::vector<Protocol::ShipPlacement> &ships);
    bool bothReady() const noexcept { return m_engine.bothReady(); }

    // returns false if it's not this player's turn or the shot is invalid.
    bool fire(int playerIdx, Coordinate coord);

    bool isOver() const noexcept {
        auto s = m_engine.getSnapshot().state;
        return s == MatchState::Victory || s == MatchState::Defeat;
    }

    // builds a tailored StateUpdate for playerIdx:
    // - yourBoard shows ships; enemyBoard masks ShipPresent as Empty.
    // - Victory/Defeat are flipped correctly per perspective.
    Protocol::StateUpdate buildSnapshot(int playerIdx) const;

  private:
    BattleshipEngine m_engine;
};

#endif