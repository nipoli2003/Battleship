#include "view/GraphicalView.hpp"
#include <algorithm>
#include <string>
#include <cmath>

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

// Helper that renders a 0-100 slider and handles drag input mapped to virtual space
bool GraphicalView::drawSlider(Rectangle bounds, const char* label, int& value, bool& isDragging) {
    Vector2 mouse = getVirtualMousePosition();
    
    // Label and numeric readout
    DrawText(label, bounds.x, bounds.y - 22, 18, RAYWHITE);
    std::string valStr = std::to_string(value) + "%";
    DrawText(valStr.c_str(), bounds.x + bounds.width - MeasureText(valStr.c_str(), 18), bounds.y - 22, 18, SKYBLUE);

    // Track Background
    Rectangle track = bounds;
    DrawRectangleRounded(track, 0.4f, 4, Color{30, 45, 60, 255});
    DrawRectangleRoundedLines(track, 0.4f, 4, DARKGRAY);

    // Filled Active Bar
    float fillWidth = (static_cast<float>(value) / 100.0f) * bounds.width;
    Rectangle filledTrack{bounds.x, bounds.y, fillWidth, bounds.height};
    DrawRectangleRounded(filledTrack, 0.4f, 4, SKYBLUE);

    // Slider Knob Handle
    Vector2 knobCenter{bounds.x + fillWidth, bounds.y + bounds.height / 2.0f};
    float knobRadius = bounds.height * 0.75f;
    bool knobHovered = CheckCollisionPointCircle(mouse, knobCenter, knobRadius);

    // Interaction handling: start dragging on press within track/knob
    Rectangle hitArea{bounds.x - knobRadius, bounds.y - 5.0f, bounds.width + 2.0f * knobRadius, bounds.height + 10.0f};
    if (CheckCollisionPointRec(mouse, hitArea) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        isDragging = true;
    }

    if (isDragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float normalized = (mouse.x - bounds.x) / bounds.width;
            normalized = std::clamp(normalized, 0.0f, 1.0f);
            value = static_cast<int>(normalized * 100.0f);
        } else {
            isDragging = false;
        }
    }

    DrawCircleV(knobCenter, knobRadius, (isDragging || knobHovered) ? WHITE : LIGHTGRAY);
    DrawCircleLines(knobCenter.x, knobCenter.y, knobRadius, DARKBLUE);

    return isDragging;
}

// Universal Settings Cog button matching Chapter 2 mockups
void GraphicalView::drawSettingsTriggerButton(float x, float y) {
    Vector2 mouse = getVirtualMousePosition();
    Rectangle cogBounds{x, y, 130.0f, 30.0f};
    bool hovered = CheckCollisionPointRec(mouse, cogBounds);

    // Render icon [*] and label
    DrawCircle(cogBounds.x + 12.0f, cogBounds.y + 15.0f, 9.0f, hovered ? SKYBLUE : LIGHTGRAY);
    DrawCircle(cogBounds.x + 12.0f, cogBounds.y + 15.0f, 4.0f, Color{15, 25, 35, 255});
    DrawText("Settings", cogBounds.x + 28.0f, cogBounds.y + 6.0f, 18, hovered ? SKYBLUE : LIGHTGRAY);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_showSettingsModal = true;
    }
}

void GraphicalView::renderSettingsModal() {
    Vector2 mouse = getVirtualMousePosition();

    // 1. Semi-transparent backdrop overlay to dim the active scene
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{0, 0, 0, 190});

    // 2. Modal Box dimensions
    float modalW = 460.0f;
    float modalH = 340.0f;
    float modalX = (VIRTUAL_WIDTH - modalW) / 2.0f;
    float modalY = (VIRTUAL_HEIGHT - modalH) / 2.0f;
    Rectangle modalRec{modalX, modalY, modalW, modalH};

    // Body & Border
    DrawRectangleRec(modalRec, Color{20, 32, 45, 255});
    DrawRectangleLinesEx(modalRec, 2, SKYBLUE);

    // Modal Header
    const char* header = "AUDIO & SETTINGS";
    int headerSize = 24;
    DrawText(header, modalX + (modalW - MeasureText(header, headerSize)) / 2.0f, modalY + 25.0f, headerSize, RAYWHITE);

    // 3. Sliders (0-100)
    Rectangle musicSliderRec{modalX + 40.0f, modalY + 105.0f, modalW - 80.0f, 12.0f};
    drawSlider(musicSliderRec, "Music Volume", m_musicVolume, m_draggingMusic);

    Rectangle sfxSliderRec{modalX + 40.0f, modalY + 175.0f, modalW - 80.0f, 12.0f};
    drawSlider(sfxSliderRec, "SFX Volume", m_sfxVolume, m_draggingSFX);

    // Optional: Feed volume updates into your Audio Engine / Raylib masters
    // SetMasterVolume(m_musicVolume / 100.0f);

    // 4. Close / Apply Button
    Rectangle btnClose{modalX + (modalW - 160.0f) / 2.0f, modalY + modalH - 65.0f, 160.0f, 40.0f};
    bool hovClose = CheckCollisionPointRec(mouse, btnClose);
    drawButton(btnClose, "Close", hovClose);

    if ((hovClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
        m_showSettingsModal = false;
        m_draggingMusic = false;
        m_draggingSFX = false;
    }
}

void GraphicalView::render() {
    handleFullscreenToggle();

    // 1. Render scenes onto the virtual 1200x700 canvas
    BeginTextureMode(m_target);
    ClearBackground(Color{15, 25, 35, 255});

    switch (m_currentScene) {
        case AppScene::TitleScreen:
            renderTitleScreen();
            break;
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

    // --- MOVE INSIDE TEXTURE MODE: Render modal on top of virtual buffer ---
    if (m_showSettingsModal) {
        renderSettingsModal();
    }

    EndTextureMode(); // Finish drawing to virtual canvas

    // 2. Scale and letterbox the virtual canvas to the physical window
    BeginDrawing();
    ClearBackground(BLACK); // Letterbox border bars

    float scale = std::min(static_cast<float>(GetScreenWidth()) / VIRTUAL_WIDTH,
                           static_cast<float>(GetScreenHeight()) / VIRTUAL_HEIGHT);

    Rectangle srcRec{
        0.0f, 0.0f,
        static_cast<float>(m_target.texture.width),
        -static_cast<float>(m_target.texture.height) // Invert Y for OpenGL
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

void GraphicalView::renderTitleScreen() {
    Vector2 mouse = getVirtualMousePosition();
    float time = static_cast<float>(GetTime());

    // 1. Temporary Geometric Background
    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, 
                           Color{10, 20, 32, 255}, Color{5, 10, 18, 255});
    for (int y = 200; y < VIRTUAL_HEIGHT - 80; y += 40) {
        DrawLine(0, y, VIRTUAL_WIDTH, y, Color{25, 45, 65, 80});
    }

    // 2. Subtitle / Theme Banner
    const char* subtitle = "* * *   CLASH OF THE DREADNOUGHTS   * * *";
    int subSize = 20;
    DrawText(subtitle, (VIRTUAL_WIDTH - MeasureText(subtitle, subSize)) / 2, 190, subSize, SKYBLUE);

    // 3. Main Title Box & Banner
    const char* title = "B A T T L E S H I P";
    int titleSize = 58;
    int titleW = MeasureText(title, titleSize);
    int titleBoxW = titleW + 60;
    int titleBoxH = 90;
    int titleBoxX = (VIRTUAL_WIDTH - titleBoxW) / 2;
    int titleBoxY = 240;

    DrawRectangle(titleBoxX, titleBoxY, titleBoxW, titleBoxH, Color{18, 30, 44, 230});
    DrawRectangleLinesEx(Rectangle{static_cast<float>(titleBoxX), static_cast<float>(titleBoxY), 
                                   static_cast<float>(titleBoxW), static_cast<float>(titleBoxH)}, 2, SKYBLUE);
    DrawText(title, (VIRTUAL_WIDTH - titleW) / 2, titleBoxY + 16, titleSize, RAYWHITE);

    // 4. Pulsing prompt
    float pulseAlpha = 0.35f + 0.65f * (0.5f * (std::sin(time * 3.5f) + 1.0f));
    const char* prompt = "[ PRESS ANY BUTTON TO PLAY ]";
    int promptSize = 24;
    Color promptColor = Fade(RAYWHITE, pulseAlpha);
    DrawText(prompt, (VIRTUAL_WIDTH - MeasureText(prompt, promptSize)) / 2, 420, promptSize, promptColor);

    // 5. Settings Cog Trigger (Clean single call)
    Rectangle cogBounds{40.0f, static_cast<float>(VIRTUAL_HEIGHT - 65), 140.0f, 32.0f};
    bool cogHover = CheckCollisionPointRec(mouse, cogBounds);
    drawSettingsTriggerButton(40.0f, VIRTUAL_HEIGHT - 65.0f);

    // 6. Footer Telemetry
    DrawText("[F / F11: Fullscreen]", 40, VIRTUAL_HEIGHT - 32, 16, DARKGRAY);
    const char* version = "Version 1.0";
    DrawText(version, VIRTUAL_WIDTH - MeasureText(version, 16) - 40, VIRTUAL_HEIGHT - 32, 16, DARKGRAY);

    // 7. Transition Input Handling
    bool keyPressed = (GetKeyPressed() > 0) && !IsKeyPressed(KEY_F) && !IsKeyPressed(KEY_F11);
    bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !cogHover;

    if (!m_showSettingsModal && (keyPressed || mouseClicked)) {
        m_currentScene = AppScene::MainMenu;
    }
}

void GraphicalView::renderMainMenu() {
    const char* title = "BATTLESHIP";
    DrawText(title, (VIRTUAL_WIDTH - MeasureText(title, 96)) / 2, VIRTUAL_HEIGHT / 4 - 30, 96, SKYBLUE);

    Vector2 mouse = getVirtualMousePosition();
    float btnW = 280.0f;
    float btnH = 48.0f;
    float startY = VIRTUAL_HEIGHT / 2.0f - 40.0f;

    // 1. Play vs Bot (AI)
    Rectangle btnAI{(VIRTUAL_WIDTH - btnW) / 2.0f, startY, btnW, btnH};
    bool hovAI = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnAI);
    drawButton(btnAI, "Play vs Bot (Offline)", hovAI);
    if (hovAI && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_engine.startNewGame();
        m_currentScene = AppScene::InGame;
    }

    // 2. Online PVP Lobby
    Rectangle btnNet{(VIRTUAL_WIDTH - btnW) / 2.0f, startY + 60.0f, btnW, btnH};
    bool hovNet = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnNet);
    drawButton(btnNet, "Online PVP Lobby", hovNet);
    if (hovNet && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::OnlineLobbyWait;
    }

    // 3. Settings (Offset: startY + 120)
    // Rectangle btnSettings{(VIRTUAL_WIDTH - btnW) / 2.0f, startY + 120.0f, btnW, btnH};
    // bool hovSet = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnSettings);
    // drawButton(btnSettings, "Settings", hovSet);
    // if (hovSet && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    //     m_showSettingsModal = true;
    // }

    // 4. Exit Game (Offset: startY + 180)
    Rectangle btnExit{(VIRTUAL_WIDTH - btnW) / 2.0f, startY + 180.0f, btnW, btnH};
    bool hovExit = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnExit);
    drawButton(btnExit, "Exit Game", hovExit);
    if (hovExit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_shouldExit = true;
    }

    // Bottom-left Cog
    if (!m_showSettingsModal) {
        drawSettingsTriggerButton(40.0f, VIRTUAL_HEIGHT - 65.0f);
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

    drawSettingsTriggerButton(40.0f, VIRTUAL_HEIGHT - 45.0f);
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

            if (board.hasShipAt(x, y) && !hideShips && cell != CellState::Hit) {
                fill = DARKGRAY;
            } else if (cell == CellState::Hit) {
                fill = RED; // Default fallback hit color

                auto shipOpt = board.getShipAt(x, y);
                if (shipOpt && *shipOpt) {
                    switch ((*shipOpt)->type()) {
                        case ShipType::Carrier:    fill = RED;     break;
                        case ShipType::Battleship: fill = ORANGE;  break;
                        case ShipType::Cruiser:    fill = YELLOW;  break;
                        case ShipType::Submarine:  fill = GREEN;   break;
                        case ShipType::Destroyer:  fill = BLUE;    break;
                    }
                }
            } else if (cell == CellState::Miss) {
                fill = WHITE;
            }

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

    // BLOCK INPUT IF SETTINGS MODAL IS OPEN
    if (!m_showSettingsModal) {
        if (snapshot.state == MatchState::PlayerTurn) {
            handleBoardClicks(enemyGridX, gridY);
        }
    }

    Rectangle btnLeave{20.0f, 20.0f, 100.0f, 35.0f};
    bool hovLeave = CheckCollisionPointRec(getVirtualMousePosition(), btnLeave);
    drawButton(btnLeave, "< Menu", hovLeave);
    if (hovLeave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_currentScene = AppScene::MainMenu;
    }

    drawSettingsTriggerButton(40.0f, VIRTUAL_HEIGHT - 45.0f);
}