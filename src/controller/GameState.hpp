/**
 * @file GameState.hpp
 * @brief Match and UI state definitions used by the controller layer.
 */

#pragma once

#include "model/Board.hpp"
#include <string>

/**
 * @brief Current phase of the match state machine.
 */
enum class MatchState {
    PlacementPhase, ///< Players are still placing ships.
    PlayerTurn,     ///< Human/local player is to act.
    OpponentTurn,  ///< Opponent is currently acting.
    Victory,        ///< The local player has won.
    Defeat          ///< The local player has lost.
};

/**
 * @brief Sub-state used by the lobby and connection UI.
 */
enum class LobbySubState {
    Setup,      ///< Lobby setup screen.
    Connecting, ///< Attempting to connect.
    ShowCode,   ///< Displaying the lobby code.
    Waiting,    ///< Waiting for the second player.
    Error       ///< Lobby-related error state.
};

/**
 * @brief Snapshot of the current match state used by the engine and UI.
 */
struct GameSnapshot {
    MatchState state{MatchState::PlacementPhase}; ///< Current match phase.
    std::string statusMessage{"Waiting for both players to place ships."}; ///< Status text for the UI.
    int turnNumber{0}; ///< Current turn number.
    // who fired last and what happened, so each client can get a message
    // from its own point of view (-1 = no shot yet)
    int lastShooter{-1}; ///< Index of the last player to fire, or -1 if none.
    AttackResult lastResult{AttackResult::Invalid}; ///< Outcome of the most recent shot.
};