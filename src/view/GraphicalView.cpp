#include "view/GraphicalView.hpp"
#include <algorithm>
#include <string>

GraphicalView::GraphicalView(BattleshipEngine& engine)
    : m_engine(engine) {}

void GraphicalView::init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(m_windowedWidth, m_windowedHeight, "Battleship - Naval Combat");
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);

    // Create the virtual canvas buffer
    m_target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(m_target.texture, TEXTURE_FILTER_BILINEAR);
}

bool GraphicalView::shouldClose() const {
    return WindowShouldClose() || m_shouldExit;
}

void GraphicalView::close() {
    UnloadRenderTexture(m_target);
    CloseWindow();
}

void GraphicalView::handleFullscreenToggle() {
    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }
}

// Maps mouse coordinates from the actual physical window onto the virtual 1200x700 canvas
Vector2 GraphicalView::getVirtualMousePosition() const {
    Vector2 rawMouse = GetMousePosition();
    float scale = std::min(static_cast<float>(GetScreenWidth()) / VIRTUAL_WIDTH,
                           static_cast<float>(GetScreenHeight()) / VIRTUAL_HEIGHT);

    float offsetX = (GetScreenWidth() - (VIRTUAL_WIDTH * scale)) * 0.5f;
    float offsetY = (GetScreenHeight() - (VIRTUAL_HEIGHT * scale)) * 0.5f;

    Vector2 virtualMouse{
        (rawMouse.x - offsetX) / scale,
        (rawMouse.y - offsetY) / scale
    };

    virtualMouse.x = std::clamp(virtualMouse.x, 0.0f, static_cast<float>(VIRTUAL_WIDTH));
    virtualMouse.y = std::clamp(virtualMouse.y, 0.0f, static_cast<float>(VIRTUAL_HEIGHT));

    return virtualMouse;
}

void GraphicalView::drawButton(Rectangle bounds, const char* text, bool hovered) {
    Color bg = hovered ? Color{50, 80, 110, 255} : Color{25, 45, 65, 255};
    DrawRectangleRec(bounds, bg);
    DrawRectangleLinesEx(bounds, 2, hovered ? SKYBLUE : LIGHTGRAY);

    int fontSize = 20;
    int textW = MeasureText(text, fontSize);
    DrawText(text, bounds.x + (bounds.width - textW) / 2, bounds.y + (bounds.height - fontSize) / 2, fontSize, RAYWHITE);
}

void GraphicalView::render() {
    handleFullscreenToggle();

    // 1. Render game scenes onto the virtual 1200x700 canvas
    BeginTextureMode(m_target);
    ClearBackground(Color{15, 25, 35, 255});

    switch (m_currentScene) {
        case AppScene::MainMenu:
            renderMainMenu();
            break;
        case AppScene::OnlineLobbyWait:
            renderLobbyWait();
            break;
        case AppScene::InGame:
            if (m_engine.getSnapshot().state == MatchState::PlacementPhase) {
                renderPlacement();
            } else {
                renderGame();
            }
            break;
    }
    EndTextureMode();

    // 2. Scale and letterbox the virtual canvas to the physical window
    BeginDrawing();
    ClearBackground(BLACK); // Letterbox border bars

    float scale = std::min(static_cast<float>(GetScreenWidth()) / VIRTUAL_WIDTH,
                           static_cast<float>(GetScreenHeight()) / VIRTUAL_HEIGHT);

    Rectangle srcRec{
        0.0f, 0.0f,
        static_cast<float>(m_target.texture.width),
        -static_cast<float>(m_target.texture.height) // Invert Y because OpenGL coordinates are flipped
    };

    Rectangle destRec{
        (GetScreenWidth() - (VIRTUAL_WIDTH * scale)) * 0.5f,
        (GetScreenHeight() - (VIRTUAL_HEIGHT * scale)) * 0.5f,
        VIRTUAL_WIDTH * scale,
        VIRTUAL_HEIGHT * scale
    };

    DrawTexturePro(m_target.texture, srcRec, destRec, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

void GraphicalView::renderMainMenu() {
    const char* title = "BATTLESHIP";
    DrawText(title, (VIRTUAL_WIDTH - MeasureText(title, 48)) / 2, VIRTUAL_HEIGHT / 4, 48, SKYBLUE);

    Vector2 mouse = getVirtualMousePosition();
    float btnW = 280.0f;
    float btnH = 50.0f;
    float startY = VIRTUAL_HEIGHT / 2.0f - 40.0f;

    Rectangle btnAI{(VIRTUAL_WIDTH - btnW) / 2.0f, startY, btnW, btnH};
    bool hovAI = CheckCollisionPointRec(mouse, btnAI);
    drawButton(btnAI, "Play vs Bot (Offline)", hovAI);
    if (hovAI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_engine.startNewGame();
        m_currentScene = AppScene::InGame;
    }

    Rectangle btnNet{(VIRTUAL_WIDTH - btnW) / 2.0f, startY + 65.0f, btnW, btnH};
    bool hovNet = CheckCollisionPointRec(mouse, btnNet);
    drawButton(btnNet, "Online PVP Lobby", hovNet);
    if (hovNet && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::OnlineLobbyWait;
    }

    Rectangle btnExit{(VIRTUAL_WIDTH - btnW) / 2.0f, startY + 130.0f, btnW, btnH};
    bool hovExit = CheckCollisionPointRec(mouse, btnExit);
    drawButton(btnExit, "Exit Game", hovExit);
    if (hovExit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_shouldExit = true;
    }
}

void GraphicalView::renderLobbyWait() {
    DrawText("ONLINE LOBBY", (VIRTUAL_WIDTH - MeasureText("ONLINE LOBBY", 32)) / 2, 100, 32, SKYBLUE);
    DrawText("Waiting for network connection setup...", (VIRTUAL_WIDTH - MeasureText("Waiting for network connection setup...", 20)) / 2, 220, 20, RAYWHITE);

    Rectangle btnBack{(VIRTUAL_WIDTH - 200.0f) / 2.0f, 360.0f, 200.0f, 45.0f};
    bool hovBack = CheckCollisionPointRec(getVirtualMousePosition(), btnBack);
    drawButton(btnBack, "Back to Menu", hovBack);
    if (hovBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::MainMenu;
    }
}

void GraphicalView::renderPlacement() {
    const auto& snapshot = m_engine.getSnapshot();

    if (IsKeyPressed(KEY_R)) {
        m_placementOrientation = (m_placementOrientation == Orientation::Horizontal) ? 
                                 Orientation::Vertical : Orientation::Horizontal;
    }

    DrawText("FLEET DEPLOYMENT", (VIRTUAL_WIDTH - MeasureText("FLEET DEPLOYMENT", 30)) / 2, 35, 30, SKYBLUE);
    DrawText(snapshot.statusMessage.c_str(), (VIRTUAL_WIDTH - MeasureText(snapshot.statusMessage.c_str(), 20)) / 2, 80, 20, YELLOW);

    int gridWidth = Board::SIZE * CELL_SIZE;
    int gridX = (VIRTUAL_WIDTH - gridWidth) / 2;
    int gridY = 150;

    drawGrid(gridX, gridY, m_engine.getHumanBoard(), false, false);

    auto currentType = m_engine.getCurrentPlacementType();
    Vector2 mouse = getVirtualMousePosition();

    if (currentType) {
        Ship previewShip(*currentType, m_placementOrientation);
        int hoverX = (mouse.x - gridX) / CELL_SIZE;
        int hoverY = (mouse.y - gridY) / CELL_SIZE;

        if (hoverX >= 0 && hoverX < Board::SIZE && hoverY >= 0 && hoverY < Board::SIZE) {
            bool valid = m_engine.getHumanBoard().canPlaceShip(previewShip, {hoverX, hoverY});
            Color ghostColor = valid ? Color{0, 220, 100, 120} : Color{220, 40, 40, 120};

            int len = previewShip.length();
            bool horiz = (m_placementOrientation == Orientation::Horizontal);

            for (int i = 0; i < len; ++i) {
                int cx = horiz ? hoverX + i : hoverX;
                int cy = horiz ? hoverY : hoverY + i;
                if (cx < Board::SIZE && cy < Board::SIZE) {
                    DrawRectangle(gridX + cx * CELL_SIZE, gridY + cy * CELL_SIZE, CELL_SIZE, CELL_SIZE, ghostColor);
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && valid) {
                m_engine.placeCurrentShip({hoverX, hoverY}, m_placementOrientation);
            }
        }
    }

    Rectangle btnRandom{static_cast<float>(gridX + gridWidth + 30), static_cast<float>(gridY + 50), 160.0f, 40.0f};
    bool hovRandom = CheckCollisionPointRec(mouse, btnRandom);
    drawButton(btnRandom, "Auto-Deploy", hovRandom);
    if (hovRandom && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_engine.randomizeHumanFleet();
    }

    DrawText("Orientation: [R] to Rotate", gridX, gridY + gridWidth + 25, 18, RAYWHITE);
    DrawText(m_placementOrientation == Orientation::Horizontal ? "(Horizontal)" : "(Vertical)", 
             gridX + 230, gridY + gridWidth + 25, 18, SKYBLUE);
}

void GraphicalView::drawGrid(int startX, int startY, const Board& board, bool hideShips, bool isEnemy) {
    Vector2 mouse = getVirtualMousePosition();

    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Rectangle cellRec{static_cast<float>(startX + x * CELL_SIZE), static_cast<float>(startY + y * CELL_SIZE), 
                             static_cast<float>(CELL_SIZE), static_cast<float>(CELL_SIZE)};

            CellState cell = board.getCell(x, y);
            Color fill = Color{20, 40, 60, 255};

            if (cell == CellState::ShipPresent && !hideShips) fill = DARKGRAY;
            else if (cell == CellState::Hit) fill = RED;
            else if (cell == CellState::Miss) fill = WHITE;

            if (isEnemy && cell != CellState::Hit && cell != CellState::Miss && CheckCollisionPointRec(mouse, cellRec)) {
                fill = Color{40, 80, 120, 255};
            }

            DrawRectangleRec(cellRec, fill);
            DrawRectangleLinesEx(cellRec, 1, Color{50, 75, 100, 255});
        }
    }
}

void GraphicalView::handleBoardClicks(int enemyStartX, int enemyStartY) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    Vector2 mouse = getVirtualMousePosition();
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Rectangle cellRec{static_cast<float>(enemyStartX + x * CELL_SIZE), static_cast<float>(enemyStartY + y * CELL_SIZE), 
                             static_cast<float>(CELL_SIZE), static_cast<float>(CELL_SIZE)};

            if (CheckCollisionPointRec(mouse, cellRec)) {
                m_engine.humanFire({x, y});
                return;
            }
        }
    }
}

void GraphicalView::renderGame() {
    const auto& snapshot = m_engine.getSnapshot();

    DrawText(snapshot.statusMessage.c_str(), (VIRTUAL_WIDTH - MeasureText(snapshot.statusMessage.c_str(), 22)) / 2, 35, 22, YELLOW);

    int gridWidth = Board::SIZE * CELL_SIZE;
    int humanGridX = (VIRTUAL_WIDTH / 2) - gridWidth - 50;
    int enemyGridX = (VIRTUAL_WIDTH / 2) + 50;
    int gridY = 160;

    DrawText("YOUR FLEET", humanGridX + 110, gridY - 30, 20, RAYWHITE);
    DrawText("RADAR / ENEMY FLEET", enemyGridX + 70, gridY - 30, 20, RAYWHITE);

    drawGrid(humanGridX, gridY, m_engine.getHumanBoard(), false, false);
    drawGrid(enemyGridX, gridY, m_engine.getOpponentBoard(), true, true);

    if (snapshot.state == MatchState::PlayerTurn) {
        handleBoardClicks(enemyGridX, gridY);
    }

    Rectangle btnLeave{20.0f, 20.0f, 100.0f, 35.0f};
    bool hovLeave = CheckCollisionPointRec(getVirtualMousePosition(), btnLeave);
    drawButton(btnLeave, "< Menu", hovLeave);
    if (hovLeave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::MainMenu;
    }
}