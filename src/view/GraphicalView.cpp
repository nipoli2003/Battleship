#include "view/GraphicalView.hpp"
#include <string>

GraphicalView::GraphicalView(BattleshipEngine& engine)
    : m_engine(engine) {}

void GraphicalView::init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT);
    InitWindow(m_windowedWidth, m_windowedHeight, "Battleship - Naval Combat");
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);
}

bool GraphicalView::shouldClose() const {
    return WindowShouldClose() || m_shouldExit;
}

void GraphicalView::close() {
    CloseWindow();
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
    BeginDrawing();
    ClearBackground(Color{15, 25, 35, 255}); // Dark naval ocean

    switch (m_currentScene) {
        case AppScene::MainMenu:
            renderMainMenu();
            break;
        case AppScene::OnlineLobbyWait:
            renderLobbyWait();
            break;
        case AppScene::InGame:
            renderGame();
            break;
    }

    EndDrawing();
}

void GraphicalView::renderMainMenu() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    const char* title = "BATTLESHIP";
    DrawText(title, (sw - MeasureText(title, 48)) / 2, sh / 4, 48, SKYBLUE);

    Vector2 mouse = GetMousePosition();
    float btnW = 280.0f;
    float btnH = 50.0f;
    float startY = sh / 2.0f - 40.0f;

    // 1. Play vs AI
    Rectangle btnAI{(sw - btnW) / 2.0f, startY, btnW, btnH};
    bool hovAI = CheckCollisionPointRec(mouse, btnAI);
    drawButton(btnAI, "Play vs Bot (Offline)", hovAI);
    if (hovAI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_engine.startNewGame();
        m_currentScene = AppScene::InGame;
    }

    // 2. Online Multiplayer
    Rectangle btnNet{(sw - btnW) / 2.0f, startY + 65.0f, btnW, btnH};
    bool hovNet = CheckCollisionPointRec(mouse, btnNet);
    drawButton(btnNet, "Online PVP Lobby", hovNet);
    if (hovNet && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::OnlineLobbyWait;
    }

    // 3. Exit
    Rectangle btnExit{(sw - btnW) / 2.0f, startY + 130.0f, btnW, btnH};
    bool hovExit = CheckCollisionPointRec(mouse, btnExit);
    drawButton(btnExit, "Exit Game", hovExit);
    if (hovExit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_shouldExit = true;
    }
}

void GraphicalView::renderLobbyWait() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawText("ONLINE LOBBY", (sw - MeasureText("ONLINE LOBBY", 32)) / 2, 100, 32, SKYBLUE);
    DrawText("Waiting for network connection setup...", (sw - MeasureText("Waiting for network connection setup...", 20)) / 2, 220, 20, RAYWHITE);

    Rectangle btnBack{(sw - 200.0f) / 2.0f, 360.0f, 200.0f, 45.0f};
    bool hovBack = CheckCollisionPointRec(GetMousePosition(), btnBack);
    drawButton(btnBack, "Back to Menu", hovBack);
    if (hovBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::MainMenu;
    }
}

void GraphicalView::drawGrid(int startX, int startY, const Board& board, bool hideShips, bool isEnemy) {
    Vector2 mouse = GetMousePosition();

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
                fill = Color{40, 80, 120, 255}; // Highlight target
            }

            DrawRectangleRec(cellRec, fill);
            DrawRectangleLinesEx(cellRec, 1, Color{50, 75, 100, 255});
        }
    }
}

void GraphicalView::handleBoardClicks(int enemyStartX, int enemyStartY) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    Vector2 mouse = GetMousePosition();
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
    int sw = GetScreenWidth();
    const auto& snapshot = m_engine.getSnapshot();

    // Top status message
    DrawText(snapshot.statusMessage.c_str(), (sw - MeasureText(snapshot.statusMessage.c_str(), 22)) / 2, 35, 22, YELLOW);

    int gridWidth = Board::SIZE * CELL_SIZE;
    int humanGridX = (sw / 2) - gridWidth - 50;
    int enemyGridX = (sw / 2) + 50;
    int gridY = 160;

    // Draw Labels
    DrawText("YOUR FLEET", humanGridX + 110, gridY - 30, 20, RAYWHITE);
    DrawText("RADAR / ENEMY FLEET", enemyGridX + 70, gridY - 30, 20, RAYWHITE);

    // Draw Grids
    drawGrid(humanGridX, gridY, m_engine.getHumanBoard(), false, false);
    drawGrid(enemyGridX, gridY, m_engine.getOpponentBoard(), true, true);

    // Human shot processing
    if (snapshot.state == MatchState::PlayerTurn) {
        handleBoardClicks(enemyGridX, gridY);
    }

    // Leave button
    Rectangle btnLeave{20.0f, 20.0f, 100.0f, 35.0f};
    bool hovLeave = CheckCollisionPointRec(GetMousePosition(), btnLeave);
    drawButton(btnLeave, "< Menu", hovLeave);
    if (hovLeave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::MainMenu;
    }
}