/**
 * @file AppController.hpp
 * @brief Main application controller responsible for app flow and game state.
 */

#ifndef APP_CONTROLLER_HPP
#define APP_CONTROLLER_HPP

#include "app/ClientState.hpp"
#include "model/Protocol.hpp"
#include "network/NetEvent.hpp"
#include "view/GraphicalView.hpp"
#include "controller/LocalMatch.hpp"
#include <memory>
#include <string>
#include <vector>

class NetworkClient;

/**
 * @brief Central controller that owns the UI, network client, and local game logic.
 *
 * The AppController coordinates application flow such as menu navigation,
 * online lobby setup, local bot matches, and network event handling.
 */
class AppController {
  public:
    /**
     * @brief Constructs the application controller with default state.
     */
    AppController();

    /**
     * @brief Releases controller-owned resources.
     */
    ~AppController();

    /**
     * @brief Runs the application's main loop.
     */
    void run();

    // --- Called by GraphicalView (main thread only) ---
    /**
     * @brief Returns the app to the title/menu screen.
     */
    void goToMenu();

    /**
     * @brief Opens the lobby creation/join UI screen.
     */
    void openLobbySetup();

    /**
     * @brief Connects to a server and creates a new lobby.
     * @param ip Server address.
     * @param port Server port.
     */
    void connectAndCreate(const std::string& ip, int port);

    /**
     * @brief Connects to a server and joins an existing lobby.
     * @param ip Server address.
     * @param port Server port.
     * @param code Lobby code to join.
     */
    void connectAndJoin(const std::string& ip, int port, const std::string& code);

    /**
     * @brief Sends a fleet placement payload to the server.
     * @param ships List of ship placement entries.
     * @return true if the placement was accepted by the server.
     */
    bool sendPlacement(const std::vector<Protocol::ShipPlacement>& ships);

    /**
     * @brief Sends a shot at the chosen board coordinate.
     * @param x Column index.
     * @param y Row index.
     */
    void sendShot(int x, int y);

    /**
     * @brief Resets the app back to the main menu state.
     */
    void resetToMenu();

    /**
     * @brief Starts a local match against a bot.
     */
    void startLocalBotMatch();

    /**
     * @brief Requests the app to shut down.
     */
    void requestExit();

    /**
     * @brief Sets the music volume for the app.
     * @param volume Volume in the range 0-100.
     */
    void setMusicVolume(int volume);

    /**
     * @brief Sets the SFX volume for the app.
     * @param volume Volume in the range 0-100.
     */
    void setSFXVolume(int volume);

  private:
    /**
     * @brief Application runtime mode.
     */
    enum class Mode { Online, Local };

    /**
     * @brief Queued user action waiting for network connectivity.
     */
    enum class PendingAction { None, CreateLobby, JoinLobby };

    /**
     * @brief Drains all queued network events and dispatches them to handlers.
     */
    void drainNetEvents();

    /**
     * @brief Handles a specific network event received from the server.
     * @param ev Event to process.
     */
    void handleNetEvent(const Network::NetEvent& ev);

    /**
     * @brief Initializes the in-game state when entering a match.
     */
    void onEnterGame();

    /**
     * @brief Applies a locally computed or server-provided snapshot to the current state.
     * @param snap Snapshot to apply.
     */
    void applyLocalSnapshot(const Protocol::StateUpdate& snap);

    Mode m_mode{Mode::Online}; ///< Current execution mode.
    std::unique_ptr<LocalMatch> m_localMatch; ///< Local match instance, if any.

    ClientState m_state; ///< Current UI and gameplay state.
    std::unique_ptr<NetworkClient> m_net; ///< Active network client, if connected.
    GraphicalView m_view; ///< Main graphical UI object.
    PendingAction m_pendingAction{PendingAction::None}; ///< Deferred action after connect.
    std::string m_pendingCode; ///< Lobby code used by a queued join action.
};

#endif