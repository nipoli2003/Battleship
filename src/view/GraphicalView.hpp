#pragma once

#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include "model/Ship.hpp"
#include "network/Client.hpp"
#include "raylib.h"
#include "view/AppScene.hpp"
#include "view/IView.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class GraphicalView : public IView {
  public:
    GraphicalView();
    ~GraphicalView() override = default;

    void init() override;
    void render() override;
    [[nodiscard]] bool shouldClose() const override;
    void close() override;

  private:
    void handleFullscreenToggle();
    [[nodiscard]] Vector2 getVirtualMousePosition() const;
    void drawButton(Rectangle bounds, const char *text, bool hovered);
    void handleTextInput(std::string &field, int maxLen);
    void resetToMenu();

    // scene renderers and handlers
    void renderMainMenu();
    void renderLobbySetup();
    void renderLobbyWait();
    void renderPlacement();
    void renderGame();
    void renderGameOver();

    // drawing helpers
    void drawGrid(int startX, int startY, const std::vector<int> &cells, bool hideShips, bool isEnemy);
    void drawLocalBoard(int startX, int startY);
    void handleBoardClicks(int enemyStartX, int enemyStartY);
    void randomizeLocalFleet();

    // network setup (called from main/render thread, blocks briefly on connect)
    // TODO: run async
    void connectAndCreate(const std::string &ip, int port);
    void connectAndJoin(const std::string &ip, int port, const std::string &code);

    // networkClient callbacks, invoked from the network listen thread
    void onLobbyCode(std::string code);
    void onLobbyJoined();
    void onLobbyError(std::string reason);
    void onStartGame();
    void onStateUpdate(Protocol::StateUpdate update);
    void onGameOver(Protocol::GameOver msg);

    // app state, main thread only
    AppScene m_currentScene{AppScene::MainMenu};
    bool m_shouldExit{false};
    std::atomic<bool> m_pendingGameStart{false}; // set by network thread, cleared in render()

    // lobby input, main thread only
    bool m_isCreating{true};
    std::string m_ipInput{"127.0.0.1"};
    std::string m_codeInput;
    int m_activeInputField{0};

    // network client (owned by main thread; listen thread uses it internally)
    std::unique_ptr<NetworkClient> m_client;

    // mutex-protected state (written by network thread, read by main/render thread)
    mutable std::mutex m_netMutex;
    LobbySubState m_lobbySubState{LobbySubState::Setup};
    std::string m_serverState; // "PlacementPhase" | "YourTurn" | "OpponentTurn" | "Victory" | "Defeat"
    std::string m_statusMessage;
    std::string m_lobbyCode;
    std::string m_lobbyError;
    std::vector<int> m_yourBoard; // 100 ints, CellState values
    std::vector<int> m_enemyBoard;

    // local placement state, main thread only
    Board m_localBoard;
    std::vector<Protocol::ShipPlacement> m_pendingPlacements;
    bool m_placementSent{false};
    Orientation m_placementOrientation{Orientation::Horizontal};

    static const std::vector<ShipType> SHIP_ORDER;

    // Virtual design canvas dimensions
    static constexpr int VIRTUAL_WIDTH = 1200;
    static constexpr int VIRTUAL_HEIGHT = 700;
    RenderTexture2D m_target{};

    int m_windowedWidth{VIRTUAL_WIDTH};
    int m_windowedHeight{VIRTUAL_HEIGHT};
    static constexpr int CELL_SIZE = 35;
};