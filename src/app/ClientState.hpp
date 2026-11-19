/**
 * @file ClientState.hpp
 * @brief Runtime state for the client UI and current match.
 */

#ifndef CLIENT_STATE_HPP
#define CLIENT_STATE_HPP

#include "controller/GameState.hpp"
#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include "view/AppScene.hpp"
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Snapshot of the client’s current application state.
 *
 * This state is updated by the app controller and read by the UI to drive
 * screens, lobby flow, match progression, and audio settings.
 */
struct ClientState {
    // --- Scene / app ---
    AppScene scene = AppScene::TitleScreen; ///< Current application scene.
    bool shouldExit = false;                ///< Request to exit the app loop.

    // --- Lobby ---
    LobbySubState lobbySubState = LobbySubState::Setup; ///< Current lobby UI panel.
    std::string lobbyCode; ///< Lobby code currently in use or generated.
    std::string lobbyError; ///< Last lobby-related error reported by the server.

    // --- Game (server-authoritative) ---
    // yourBoard / enemyBoard are raw CellState values from StateUpdate.
    Protocol::UserState serverState = Protocol::UserState::None; ///< Current match phase.
    std::string statusMessage; ///< Human-readable status text displayed to the user.
    std::vector<int> yourBoard; ///< Board state visible to the local player.
    std::vector<int> enemyBoard; ///< Opponent board state visible to the local player.
    std::optional<Protocol::GameOver> gameOver; ///< Final match result, if the game is over.

    // --- Settings ---
    int musicVolume = 70; ///< Music volume on a 0-100 scale.
    int sfxVolume = 80; ///< Sound effect volume on a 0-100 scale.
};

#endif