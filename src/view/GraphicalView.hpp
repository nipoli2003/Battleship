#pragma once

#include "model/Board.hpp"
#include "model/Protocol.hpp"
#include "model/Ship.hpp"
#include "raylib.h"
#include "view/AppScene.hpp"
#include "view/IView.hpp"
#include <string>
#include <vector>

class AppController;
struct ClientState;

class GraphicalView : public IView {
  public:
    GraphicalView();
    ~GraphicalView() override = default;

    void setOwner(AppController* owner) { m_owner = owner; }

    void init() override;
    void render(const ClientState& state) override;
    [[nodiscard]] bool shouldClose() const override;
    void close() override;

  private:
    void handleFullscreenToggle();
    [[nodiscard]] Vector2 getVirtualMousePosition() const;
    void drawButton(Rectangle bounds, const char* text, bool hovered);
    void handleTextInput(std::string& field, int maxLen);
    void playClick();

    // scene renderers
    void renderTitleScreen(const ClientState& state);
    void renderMainMenu(const ClientState& state);
    void renderLobbySetup(const ClientState& state);
    void renderLobbyWait(const ClientState& state);
    void renderPlacement(const ClientState& state);
    void renderGame(const ClientState& state);
    void renderGameOver(const ClientState& state);

    // drawing helpers
    void renderSettingsModal(const ClientState& state);
    void drawSettingsTriggerButton(float x, float y);
    std::optional<int> drawSlider(Rectangle bounds, const char* label, int currentValue, bool& isDragging);
    void drawGrid(int startX, int startY, const std::vector<int>& cells, bool hideShips, bool isEnemy);
    void drawLocalBoard(int startX, int startY, const Board& board);
    void handleBoardClicks(int enemyStartX, int enemyStartY);

    void resetPlacementState();
    void randomizeLocalFleet();

    AppController* m_owner{nullptr};

    // --- Settings (main thread only) ---
    bool m_showSettingsModal{false};
    bool m_draggingMusic{false};
    bool m_draggingSFX{false};

    // --- Lobby input (main thread only) ---
    bool m_isCreating{true};
    std::string m_ipInput{"127.0.0.1"};
    std::string m_codeInput;
    int m_activeInputField{0};

    // --- temporary state while placing ---
    Board m_localBoard;
    std::vector<Protocol::ShipPlacement> m_pendingPlacements;
    Orientation m_placementOrientation{Orientation::Horizontal};
    bool m_placementSent{false};

    // --- reset placement state when entering InGame ---
    AppScene m_prevScene{AppScene::TitleScreen};

    static const std::vector<ShipType> SHIP_ORDER;

    static constexpr int VIRTUAL_WIDTH = 1200;
    static constexpr int VIRTUAL_HEIGHT = 700;
    static constexpr int CELL_SIZE = 35;
    RenderTexture2D m_target{};

    int m_windowedWidth{VIRTUAL_WIDTH};
    int m_windowedHeight{VIRTUAL_HEIGHT};
};