#pragma once

#include "view/IView.hpp"
#include "view/AppScene.hpp"
#include "controller/BattleshipEngine.hpp"
#include "raylib.h"
#include <vector>
#include <string>
#include <map>

class GraphicalView : public IView {
public:
    explicit GraphicalView(BattleshipEngine& engine);
    ~GraphicalView() override = default;

    void init() override;
    void render() override;
    [[nodiscard]] bool shouldClose() const override;
    void close() override;

private:
    void handleFullscreenToggle();
    [[nodiscard]] Vector2 getVirtualMousePosition() const;

    // scene renderers and handlers
    void renderTitleScreen();
    void renderMainMenu();
    void renderLobbyWait();
    void renderPlacement();
    void renderGame();

    // drawing helpers
    void renderSettingsModal();
    void drawSettingsTriggerButton(float x, float y);
    bool drawSlider(Rectangle bounds, const char* label, int& value, bool& isDragging);
    void drawButton(Rectangle bounds, const char* text, bool hovered);
    void drawGrid(int startX, int startY, const Board& board, bool hideShips, bool isEnemy);
    void handleBoardClicks(int enemyStartX, int enemyStartY);

    BattleshipEngine& m_engine;
    AppScene m_currentScene{AppScene::TitleScreen};
    bool m_shouldExit{false};
    
    // --- Options / Settings State ---
    bool m_showSettingsModal{false};
    int m_musicVolume{70};      // 0 - 100
    int m_sfxVolume{80};        // 0 - 100
    bool m_draggingMusic{false};
    bool m_draggingSFX{false};

    // Placement state
    Orientation m_placementOrientation{Orientation::Horizontal};

    // Virtual design canvas dimensions
    static constexpr int VIRTUAL_WIDTH = 1200;
    static constexpr int VIRTUAL_HEIGHT = 700;
    static constexpr int CELL_SIZE = 35;
    RenderTexture2D m_target{};

    int m_windowedWidth{VIRTUAL_WIDTH};
    int m_windowedHeight{VIRTUAL_HEIGHT};
};