#pragma once

#include "view/IView.hpp"
#include "view/AppScene.hpp"
#include "controller/BattleshipEngine.hpp"
#include "raylib.h"
#include <vector>
#include <string>

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

    // scene renderers and handlers
    void renderMainMenu();
    void renderLobbyWait();
    void renderPlacement();
    void renderGame();

    // drawing helpers
    void drawButton(Rectangle bounds, const char* text, bool hovered);
    void drawGrid(int startX, int startY, const Board& board, bool hideShips, bool isEnemy);
    void handleBoardClicks(int enemyStartX, int enemyStartY);

    BattleshipEngine& m_engine;
    AppScene m_currentScene{AppScene::MainMenu};
    bool m_shouldExit{false};

    // Placement state
    Orientation m_placementOrientation{Orientation::Horizontal};

    int m_windowedWidth{1200};
    int m_windowedHeight{700};
    static constexpr int CELL_SIZE = 35;
};