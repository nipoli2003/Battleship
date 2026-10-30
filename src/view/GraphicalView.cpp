#include "view/GraphicalView.hpp"
#include "app/AppController.hpp"
#include "app/AudioManager.hpp"
#include "app/ClientState.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <string>

const std::vector<ShipType> GraphicalView::SHIP_ORDER = {
    ShipType::Carrier,
    ShipType::Battleship,
    ShipType::Cruiser,
    ShipType::Submarine,
    ShipType::Destroyer};

GraphicalView::GraphicalView() = default;

void GraphicalView::init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(m_windowedWidth, m_windowedHeight, "Battleship - Naval Combat");
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);
    m_target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(m_target.texture, TEXTURE_FILTER_BILINEAR);
}

bool GraphicalView::shouldClose() const {
    return WindowShouldClose();
}

void GraphicalView::close() {
    UnloadRenderTexture(m_target);
    CloseWindow();
}

void GraphicalView::playClick() {
    AudioManager::instance().playSFX(SoundEffect::ButtonClick);
}

// ------------------------------------------------------------------ helpers

void GraphicalView::handleFullscreenToggle() {
    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11))
        ToggleFullscreen();
}

Vector2 GraphicalView::getVirtualMousePosition() const {
    Vector2 raw = GetMousePosition();
    float scale = std::min((float)GetScreenWidth() / VIRTUAL_WIDTH,
                           (float)GetScreenHeight() / VIRTUAL_HEIGHT);
    float ox = (GetScreenWidth() - VIRTUAL_WIDTH * scale) * 0.5f;
    float oy = (GetScreenHeight() - VIRTUAL_HEIGHT * scale) * 0.5f;
    return {
        std::clamp((raw.x - ox) / scale, 0.f, (float)VIRTUAL_WIDTH),
        std::clamp((raw.y - oy) / scale, 0.f, (float)VIRTUAL_HEIGHT)};
}

void GraphicalView::drawButton(Rectangle b, const char* text, bool hovered) {
    DrawRectangleRec(b, hovered ? Color{50, 80, 110, 255} : Color{25, 45, 65, 255});
    DrawRectangleLinesEx(b, 2, hovered ? SKYBLUE : LIGHTGRAY);
    int fs = 20, tw = MeasureText(text, fs);
    DrawText(text, (int)(b.x + (b.width - tw) / 2),
             (int)(b.y + (b.height - fs) / 2), fs, RAYWHITE);
}

void GraphicalView::handleTextInput(std::string& field, int maxLen) {
    int c;
    while ((c = GetCharPressed()) != 0)
        if ((int)field.size() < maxLen && c >= 32 && c < 127)
            field += (char)c;
    if (IsKeyPressed(KEY_BACKSPACE) && !field.empty())
        field.pop_back();
}

std::optional<int> GraphicalView::drawSlider(Rectangle bounds, const char* label, int currentValue, bool& isDragging) {
    Vector2 mouse = getVirtualMousePosition();
    const float knobRadius = bounds.height * 0.75f;
    
    Rectangle hitArea{bounds.x - knobRadius, bounds.y - 5.0f, bounds.width + 2.0f * knobRadius, bounds.height + 10.0f};
    if (CheckCollisionPointRec(mouse, hitArea) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        isDragging = true;

    int displayValue = currentValue;
    if (isDragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float normalized = std::clamp((mouse.x - bounds.x) / bounds.width, 0.0f, 1.0f);
            displayValue = static_cast<int>(normalized * 100.0f);
        } else {
            isDragging = false;
        }
    }

    DrawText(label, bounds.x, bounds.y - 22, 18, RAYWHITE);
    std::string valStr = std::to_string(currentValue) + "%";
    DrawText(valStr.c_str(), bounds.x + bounds.width - MeasureText(valStr.c_str(), 18), bounds.y - 22, 18, SKYBLUE);

    Rectangle track = bounds;
    DrawRectangleRounded(track, 0.4f, 4, Color{30, 45, 60, 255});
    DrawRectangleRoundedLines(track, 0.4f, 4, DARKGRAY);

    float fillWidth = (static_cast<float>(currentValue) / 100.0f) * bounds.width;
    Rectangle filledTrack{bounds.x, bounds.y, fillWidth, bounds.height};
    DrawRectangleRounded(filledTrack, 0.4f, 4, SKYBLUE);

    Vector2 knobCenter{bounds.x + fillWidth, bounds.y + bounds.height / 2.0f};
    bool knobHovered = CheckCollisionPointCircle(mouse, knobCenter, knobRadius);

    DrawCircleV(knobCenter, knobRadius, (isDragging || knobHovered) ? WHITE : LIGHTGRAY);
    DrawCircleLines(knobCenter.x, knobCenter.y, knobRadius, DARKBLUE);

    if(displayValue != currentValue)
        return displayValue;
    return std::nullopt;
}

void GraphicalView::drawSettingsTriggerButton(float x, float y) {
    Vector2 mouse = getVirtualMousePosition();
    Rectangle cogBounds{x, y, 130.0f, 30.0f};
    bool hovered = CheckCollisionPointRec(mouse, cogBounds);

    DrawCircle(cogBounds.x + 12.0f, cogBounds.y + 15.0f, 9.0f, hovered ? SKYBLUE : LIGHTGRAY);
    DrawCircle(cogBounds.x + 12.0f, cogBounds.y + 15.0f, 4.0f, Color{15, 25, 35, 255});
    DrawText("Settings", cogBounds.x + 28.0f, cogBounds.y + 6.0f, 18, hovered ? SKYBLUE : LIGHTGRAY);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        m_showSettingsModal = true;
    }
}

void GraphicalView::renderSettingsModal(const ClientState& state) {
    Vector2 mouse = getVirtualMousePosition();

    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{0, 0, 0, 190});

    float modalW = 460.0f;
    float modalH = 340.0f;
    float modalX = (VIRTUAL_WIDTH - modalW) / 2.0f;
    float modalY = (VIRTUAL_HEIGHT - modalH) / 2.0f;
    Rectangle modalRec{modalX, modalY, modalW, modalH};

    DrawRectangleRec(modalRec, Color{20, 32, 45, 255});
    DrawRectangleLinesEx(modalRec, 2, SKYBLUE);

    const char* header = "AUDIO & SETTINGS";
    int headerSize = 24;
    DrawText(header, modalX + (modalW - MeasureText(header, headerSize)) / 2.0f, modalY + 25.0f, headerSize, RAYWHITE);



    Rectangle musicSliderRec{modalX + 40.0f, modalY + 105.0f, modalW - 80.0f, 12.0f};
    if (auto newVol = drawSlider(musicSliderRec, "Music Volume", state.musicVolume, m_draggingMusic)) {
        if(m_owner) m_owner->setMusicVolume(*newVol);
    }

    Rectangle sfxSliderRec{modalX + 40.0f, modalY + 175.0f, modalW - 80.0f, 12.0f};
    if (auto newVol = drawSlider(sfxSliderRec, "SFX Volume", state.sfxVolume, m_draggingSFX)) {
        if(m_owner) m_owner->setSFXVolume(*newVol);
    }

    Rectangle btnClose{modalX + (modalW - 160.0f) / 2.0f, modalY + modalH - 65.0f, 160.0f, 40.0f};
    bool hovClose = CheckCollisionPointRec(mouse, btnClose);
    drawButton(btnClose, "Close", hovClose);

    if ((hovClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
        playClick();
        m_showSettingsModal = false;
        m_draggingMusic = false;
        m_draggingSFX = false;
    }
}

void GraphicalView::resetPlacementState() {
    m_localBoard.reset();
    m_pendingPlacements.clear();
    m_placementOrientation = Orientation::Horizontal;
    m_placementSent = false;
}

void GraphicalView::render(const ClientState& state) {
    handleFullscreenToggle();

    // Reset UI placement state on scene entry.
    if (state.scene == AppScene::InGame && m_prevScene != AppScene::InGame) {
        resetPlacementState();
    }
    m_prevScene = state.scene;

    BeginTextureMode(m_target);
    ClearBackground(Color{15, 25, 35, 255});

    switch (state.scene) {
    case AppScene::TitleScreen:
        renderTitleScreen(state);
        break;
    case AppScene::MainMenu:
        renderMainMenu(state);
        break;
    case AppScene::OnlineLobbyWait: {
        if (state.lobbySubState == LobbySubState::Setup)
            renderLobbySetup(state);
        else
            renderLobbyWait(state);
        break;
    }
    case AppScene::InGame: {
        const auto s = state.serverState;
        if (s == Protocol::UserState::Victory || s == Protocol::UserState::Defeat)
            renderGameOver(state);
        else if (s == Protocol::UserState::None || s == Protocol::UserState::PlacementPhase)
            renderPlacement(state);
        else
            renderGame(state);
        break;
    }
    }

    if (m_showSettingsModal) {
        renderSettingsModal(state);
    }

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    float scale = std::min(static_cast<float>(GetScreenWidth()) / VIRTUAL_WIDTH,
                           static_cast<float>(GetScreenHeight()) / VIRTUAL_HEIGHT);

    Rectangle srcRec{
        0.0f, 0.0f,
        static_cast<float>(m_target.texture.width),
        -static_cast<float>(m_target.texture.height)
    };

    Rectangle destRec{
        (GetScreenWidth() - (VIRTUAL_WIDTH * scale)) * 0.5f,
        (GetScreenHeight() - (VIRTUAL_HEIGHT * scale)) * 0.5f,
        VIRTUAL_WIDTH * scale,
        VIRTUAL_HEIGHT * scale};

    DrawTexturePro(m_target.texture, srcRec, destRec, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

void GraphicalView::renderTitleScreen(const ClientState& state) {
    Vector2 mouse = getVirtualMousePosition();
    float time = static_cast<float>(GetTime());

    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT,
                           Color{10, 20, 32, 255}, Color{5, 10, 18, 255});
    for (int y = 200; y < VIRTUAL_HEIGHT - 80; y += 40) {
        DrawLine(0, y, VIRTUAL_WIDTH, y, Color{25, 45, 65, 80});
    }

    const char* subtitle = "* * *   CLASH OF THE DREADNOUGHTS   * * *";
    int subSize = 20;
    DrawText(subtitle, (VIRTUAL_WIDTH - MeasureText(subtitle, subSize)) / 2, 190, subSize, SKYBLUE);

    const char* title = "B A T T L E S H I P";
    int titleSize = 58;
    int titleW = MeasureText(title, titleSize);
    int titleBoxW = titleW + 60;
    int titleBoxH = 90;
    int titleBoxX = (VIRTUAL_WIDTH - titleBoxW) / 2;
    int titleBoxY = 240;

    DrawRectangle(titleBoxX, titleBoxY, titleBoxW, titleBoxH, Color{18, 30, 44, 230});
    DrawRectangleLinesEx(Rectangle{static_cast<float>(titleBoxX), static_cast<float>(titleBoxY),
                                   static_cast<float>(titleBoxW), static_cast<float>(titleBoxH)},
                         2, SKYBLUE);
    DrawText(title, (VIRTUAL_WIDTH - titleW) / 2, titleBoxY + 16, titleSize, RAYWHITE);

    float pulseAlpha = 0.35f + 0.65f * (0.5f * (std::sin(time * 3.5f) + 1.0f));
    const char* prompt = "[ PRESS ANY BUTTON TO PLAY ]";
    int promptSize = 24;
    Color promptColor = Fade(RAYWHITE, pulseAlpha);
    DrawText(prompt, (VIRTUAL_WIDTH - MeasureText(prompt, promptSize)) / 2, 420, promptSize, promptColor);

    Rectangle cogBounds{40.0f, static_cast<float>(VIRTUAL_HEIGHT - 65), 140.0f, 32.0f};
    bool cogHover = CheckCollisionPointRec(mouse, cogBounds);
    drawSettingsTriggerButton(40.0f, VIRTUAL_HEIGHT - 65.0f);

    DrawText("[F / F11: Fullscreen]", 40, VIRTUAL_HEIGHT - 32, 16, DARKGRAY);
    const char* version = "Version 1.0";
    DrawText(version, VIRTUAL_WIDTH - MeasureText(version, 16) - 40, VIRTUAL_HEIGHT - 32, 16, DARKGRAY);

    bool keyPressed = (GetKeyPressed() > 0) && !IsKeyPressed(KEY_F) && !IsKeyPressed(KEY_F11);
    bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !cogHover;


    if (!m_showSettingsModal && (keyPressed || mouseClicked)) {
        if (m_owner) m_owner->goToMenu();
    }
}

void GraphicalView::renderMainMenu(const ClientState& state) {
    const char* title = "BATTLESHIP";
    DrawText(title, (VIRTUAL_WIDTH - MeasureText(title, 96)) / 2, VIRTUAL_HEIGHT / 4, 96, SKYBLUE);

    Vector2 mouse = getVirtualMousePosition();
    float bw = 280.f, bh = 50.f, bx = (VIRTUAL_WIDTH - bw) / 2.f;
    float by = VIRTUAL_HEIGHT / 2.f - 60.f;

    Rectangle btnCreate{bx, by, bw, bh};
    bool hovCreate = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnCreate);
    drawButton(btnCreate, "Create Lobby", hovCreate);
    if (hovCreate && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        m_isCreating = true;
        m_codeInput.clear();
        if (m_owner) m_owner->openLobbySetup();
    }

    Rectangle btnJoin{bx, by + 65.f, bw, bh};
    bool hovJoin = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnJoin);
    drawButton(btnJoin, "Join Lobby", hovJoin);
    if (hovJoin && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        m_isCreating = false;
        m_codeInput.clear();
        if (m_owner) m_owner->openLobbySetup();
    }

    Rectangle btnBot{bx, by + 130.f, bw, bh};
    bool hovBot = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnBot);
    drawButton(btnBot, "Play vs Bot", hovBot);
    if (hovBot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        if (m_owner)
            m_owner->startLocalBotMatch();
    }

    Rectangle btnExit{(VIRTUAL_WIDTH - bw) / 2.0f, by + 195.0f, bw, bh};
    bool hovExit = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnExit);
    drawButton(btnExit, "Exit Game", hovExit);
    if (hovExit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        if (m_owner) m_owner->requestExit();
    }

    if (!m_showSettingsModal) {
        drawSettingsTriggerButton(40.0f, VIRTUAL_HEIGHT - 65.0f);
    }
}

void GraphicalView::renderLobbySetup(const ClientState& state) {
    const char* heading = m_isCreating ? "CREATE LOBBY" : "JOIN LOBBY";
    DrawText(heading, (VIRTUAL_WIDTH - MeasureText(heading, 32)) / 2, 80, 32, SKYBLUE);

    Vector2 mouse = getVirtualMousePosition();
    float fw = 340.f, fh = 40.f, fx = (VIRTUAL_WIDTH - fw) / 2.f;

    DrawText("Server IP:", (int)fx, 195, 20, RAYWHITE);
    Rectangle ipBox{fx, 220.f, fw, fh};
    bool ipSel = (m_activeInputField == 0);
    DrawRectangleRec(ipBox, Color{10, 20, 35, 255});
    DrawRectangleLinesEx(ipBox, 2, ipSel ? SKYBLUE : LIGHTGRAY);
    DrawText(m_ipInput.c_str(), (int)(ipBox.x + 8), (int)(ipBox.y + 10), 20, RAYWHITE);
    if (!m_showSettingsModal && CheckCollisionPointRec(mouse, ipBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        m_activeInputField = 0;
    if (ipSel && !m_showSettingsModal)
        handleTextInput(m_ipInput, 64);

    float nextY = 290.f;

    if (!m_isCreating) {
        DrawText("Lobby Code:", (int)fx, (int)nextY - 5, 20, RAYWHITE);
        Rectangle codeBox{fx, nextY + 20.f, fw, fh};
        bool codeSel = (m_activeInputField == 1);
        DrawRectangleRec(codeBox, Color{10, 20, 35, 255});
        DrawRectangleLinesEx(codeBox, 2, codeSel ? SKYBLUE : LIGHTGRAY);
        DrawText(m_codeInput.c_str(), (int)(codeBox.x + 8), (int)(codeBox.y + 10), 20, RAYWHITE);
        if (!m_showSettingsModal && CheckCollisionPointRec(mouse, codeBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            m_activeInputField = 1;
        if (codeSel && !m_showSettingsModal)
            handleTextInput(m_codeInput, 6);
        nextY += 80.f;
    }

    Rectangle btnConn{(VIRTUAL_WIDTH - 220.f) / 2.f, nextY + 20.f, 220.f, 45.f};
    bool hovConn = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnConn);
    drawButton(btnConn, m_isCreating ? "Connect & Create" : "Connect & Join", hovConn);
    if (hovConn && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !m_ipInput.empty()) {
        playClick();
        if (m_owner) {
            if (m_isCreating)
                m_owner->connectAndCreate(m_ipInput, 9000);
            else
                m_owner->connectAndJoin(m_ipInput, 9000, m_codeInput);
        }
    }

    Rectangle btnBack{(VIRTUAL_WIDTH - 200.f) / 2.f, nextY + 80.f, 200.f, 45.f};
    bool hovBack = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnBack);
    drawButton(btnBack, "Back to Menu", hovBack);
    if (hovBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        m_owner->resetToMenu();
    }
}

void GraphicalView::renderLobbyWait(const ClientState& state) {
    const auto sub = state.lobbySubState;
    const auto& code = state.lobbyCode;
    const auto& err  = state.lobbyError;

    if (sub == LobbySubState::Connecting) {
        DrawText("Connecting...", (VIRTUAL_WIDTH - MeasureText("Connecting...", 28)) / 2, 300, 28, RAYWHITE);
    } else if (sub == LobbySubState::ShowCode) {
        const char* h = "LOBBY CREATED";
        DrawText(h, (VIRTUAL_WIDTH - MeasureText(h, 32)) / 2, 120, 32, SKYBLUE);
        const char* sub1 = "Share this code with your opponent:";
        DrawText(sub1, (VIRTUAL_WIDTH - MeasureText(sub1, 20)) / 2, 200, 20, RAYWHITE);
        DrawText(code.c_str(), (VIRTUAL_WIDTH - MeasureText(code.c_str(), 64)) / 2, 240, 64, YELLOW);
        const char* sub2 = "Waiting for opponent to join...";
        DrawText(sub2, (VIRTUAL_WIDTH - MeasureText(sub2, 20)) / 2, 340, 20, LIGHTGRAY);
    } else if (sub == LobbySubState::Waiting) {
        const char* msg = "Joined! Waiting for game to start...";
        DrawText(msg, (VIRTUAL_WIDTH - MeasureText(msg, 22)) / 2, 300, 22, RAYWHITE);
    } else if (sub == LobbySubState::Error) {
        DrawText("Error", (VIRTUAL_WIDTH - MeasureText("Error", 28)) / 2, 250, 28, RED);
        DrawText(err.c_str(), (VIRTUAL_WIDTH - MeasureText(err.c_str(), 20)) / 2, 295, 20, LIGHTGRAY);
    }

    Vector2 mouse = getVirtualMousePosition();
    Rectangle btnBack{(VIRTUAL_WIDTH - 200.f) / 2.f, 430.f, 200.f, 45.f};
    bool hovBack = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnBack);
    drawButton(btnBack, "Back to Menu", hovBack);
    if (hovBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        if (m_owner) m_owner->resetToMenu();
    }
}

void GraphicalView::randomizeLocalFleet() {
    m_localBoard.reset();
    m_pendingPlacements.clear();

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> orientDist(0, 1);

    for (auto type : SHIP_ORDER) {
        bool placed = false;
        for (int attempts = 0; !placed && attempts < 1000; ++attempts) {
            auto orient = orientDist(rng) == 0 ? Orientation::Horizontal : Orientation::Vertical;
            auto ship = std::make_shared<Ship>(type, orient);
            const int len = ship->length();
            int maxX = (orient == Orientation::Horizontal) ? Board::SIZE - len : Board::SIZE - 1;
            int maxY = (orient == Orientation::Vertical)   ? Board::SIZE - len : Board::SIZE - 1;
            int x = std::uniform_int_distribution<int>(0, maxX)(rng);
            int y = std::uniform_int_distribution<int>(0, maxY)(rng);
            if (m_localBoard.placeShip(ship, {x, y})) {
                m_pendingPlacements.push_back({(int)type, x, y, orient == Orientation::Horizontal ? 0 : 1});
                placed = true;
            }
        }
    }
}

void GraphicalView::drawLocalBoard(int startX, int startY, const Board& board) {
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Rectangle cell{(float)(startX + x * CELL_SIZE), (float)(startY + y * CELL_SIZE),
                           (float)CELL_SIZE, (float)CELL_SIZE};
            Color fill = Color{20, 40, 60, 255};
            CellState cs = board.getCell(x, y);
            if (cs != CellState::Empty && cs != CellState::Hit && cs != CellState::Miss)
                fill = DARKGRAY;
            DrawRectangleRec(cell, fill);
            DrawRectangleLinesEx(cell, 1, Color{50, 75, 100, 255});
        }
    }
}

void GraphicalView::renderPlacement(const ClientState& state) {
    if (m_placementSent) {
        const char* msg = "Fleet deployed — waiting for opponent...";
        DrawText(msg, (VIRTUAL_WIDTH - MeasureText(msg, 22)) / 2, VIRTUAL_HEIGHT / 2 - 11, 22, YELLOW);
        return;
    }

    if (IsKeyPressed(KEY_R))
        m_placementOrientation = (m_placementOrientation == Orientation::Horizontal)
                                     ? Orientation::Vertical
                                     : Orientation::Horizontal;

    DrawText("FLEET DEPLOYMENT", (VIRTUAL_WIDTH - MeasureText("FLEET DEPLOYMENT", 30)) / 2, 35, 30, SKYBLUE);

    std::optional<ShipType> currentType;
    if (m_pendingPlacements.size() < SHIP_ORDER.size())
        currentType = SHIP_ORDER[m_pendingPlacements.size()];

    if (currentType) {
        Ship tmp(*currentType, m_placementOrientation);
        std::string msg = "Place your ";
        msg += tmp.name();
        msg += " (Length: " + std::to_string(tmp.length()) + "). [R] to rotate.";
        DrawText(msg.c_str(), (VIRTUAL_WIDTH - MeasureText(msg.c_str(), 20)) / 2, 80, 20, YELLOW);
    } else {
        const char* msg = "All ships placed! Click Confirm to send.";
        DrawText(msg, (VIRTUAL_WIDTH - MeasureText(msg, 20)) / 2, 80, 20, GREEN);
    }

    int gw = Board::SIZE * CELL_SIZE;
    int gx = (VIRTUAL_WIDTH - gw) / 2;
    int gy = 150;

    drawLocalBoard(gx, gy, m_localBoard);

    Vector2 mouse = getVirtualMousePosition();

    if (currentType) {
        Ship preview(*currentType, m_placementOrientation);
        int hx = (int)(mouse.x - gx) / CELL_SIZE;
        int hy = (int)(mouse.y - gy) / CELL_SIZE;
        if (hx >= 0 && hx < Board::SIZE && hy >= 0 && hy < Board::SIZE) {
            bool valid = m_localBoard.canPlaceShip(preview, {hx, hy});
            Color ghost = valid ? Color{0, 220, 100, 120} : Color{220, 40, 40, 120};
            bool horiz = (m_placementOrientation == Orientation::Horizontal);
            for (int i = 0; i < preview.length(); ++i) {
                int cx = horiz ? hx + i : hx;
                int cy = horiz ? hy : hy + i;
                if (cx < Board::SIZE && cy < Board::SIZE)
                    DrawRectangle(gx + cx * CELL_SIZE, gy + cy * CELL_SIZE, CELL_SIZE, CELL_SIZE, ghost);
            }
            if (!m_showSettingsModal && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && valid) {
                auto ship = std::make_shared<Ship>(*currentType, m_placementOrientation);
                m_localBoard.placeShip(ship, {hx, hy});
                m_pendingPlacements.push_back({(int)*currentType, hx, hy,
                                                   m_placementOrientation == Orientation::Horizontal ? 0 : 1});
            }
        }
    }

    Rectangle btnAuto{(float)(gx + gw + 30), (float)(gy + 50), 160.f, 40.f};
    bool hovAuto = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnAuto);
    drawButton(btnAuto, "Auto-Deploy", hovAuto);
    if (hovAuto && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        randomizeLocalFleet();
    }

    if (m_pendingPlacements.size() == SHIP_ORDER.size()) {
        Rectangle btnConf{(float)(gx + gw + 30), (float)(gy + 110), 160.f, 40.f};
        bool hovConf = !m_showSettingsModal && CheckCollisionPointRec(mouse, btnConf);
        drawButton(btnConf, "Confirm", hovConf);
        if (hovConf && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            playClick();
            if (m_owner && m_owner->sendPlacement(m_pendingPlacements))
                m_placementSent = true;
        }
    }

    DrawText("Orientation: [R] to Rotate", gx, gy + gw + 25, 18, RAYWHITE);
    DrawText(m_placementOrientation == Orientation::Horizontal ? "(Horizontal)" : "(Vertical)",
             gx + 230, gy + gw + 25, 18, SKYBLUE);
}

void GraphicalView::drawGrid(int startX, int startY, const std::vector<int>& cells, bool hideShips, bool isEnemy) {
    Vector2 mouse = getVirtualMousePosition();
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            int idx = y * Board::SIZE + x;
            auto cs = (idx < (int)cells.size()) ? static_cast<CellState>(cells[idx]) : CellState::Empty;
            Rectangle cell{(float)(startX + x * CELL_SIZE), (float)(startY + y * CELL_SIZE),
                           (float)CELL_SIZE, (float)CELL_SIZE};
            Color fill = Color{20, 40, 60, 255};
            if (!hideShips && cs != CellState::Empty && cs != CellState::Hit && cs != CellState::Miss)
                fill = DARKGRAY;
            else if (cs == CellState::Hit)
                fill = RED;
            else if (cs == CellState::Miss)
                fill = WHITE;
            if (isEnemy && cs != CellState::Hit && cs != CellState::Miss && CheckCollisionPointRec(mouse, cell))
                fill = Color{40, 80, 120, 255};
            DrawRectangleRec(cell, fill);
            DrawRectangleLinesEx(cell, 1, Color{50, 75, 100, 255});
        }
    }
}

void GraphicalView::handleBoardClicks(int ex, int ey) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return;
    Vector2 mouse = getVirtualMousePosition();
    for (int y = 0; y < Board::SIZE; ++y)
        for (int x = 0; x < Board::SIZE; ++x)
            if (CheckCollisionPointRec(mouse, {(float)(ex + x * CELL_SIZE), (float)(ey + y * CELL_SIZE),
                                               (float)CELL_SIZE, (float)CELL_SIZE})) {
                if (m_owner) m_owner->sendShot(x, y);
                return;
            }
}

void GraphicalView::renderGame(const ClientState& state) {
    const auto& status = state.statusMessage;

    Color statusColor = (state.serverState == Protocol::UserState::YourTurn) ? GREEN : ORANGE;
    DrawText(status.c_str(), (VIRTUAL_WIDTH - MeasureText(status.c_str(), 22)) / 2, 35, 22, statusColor);

    int gw = Board::SIZE * CELL_SIZE;
    int hgx = VIRTUAL_WIDTH / 2 - gw - 50;
    int egx = VIRTUAL_WIDTH / 2 + 50;
    int gy = 160;

    DrawText("YOUR FLEET", hgx + 110, gy - 30, 20, RAYWHITE);
    DrawText("RADAR / ENEMY FLEET", egx + 70, gy - 30, 20, RAYWHITE);

    drawGrid(hgx, gy, state.yourBoard, false, false);
    drawGrid(egx, gy, state.enemyBoard, true, true);

    if (!m_showSettingsModal && state.serverState == Protocol::UserState::YourTurn)
        handleBoardClicks(egx, gy);

    Rectangle btnLeave{20.f, 20.f, 100.f, 35.f};
    bool hovLeave = !m_showSettingsModal && CheckCollisionPointRec(getVirtualMousePosition(), btnLeave);
    drawButton(btnLeave, "< Menu", hovLeave);
    if (hovLeave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        if (m_owner) m_owner->resetToMenu();
    }
}

void GraphicalView::renderGameOver(const ClientState& state) {
    const bool won = (state.serverState == Protocol::UserState::Victory);
    const char* headline = won ? "VICTORY!" : "DEFEAT";
    DrawText(headline, (VIRTUAL_WIDTH - MeasureText(headline, 72)) / 2,
             VIRTUAL_HEIGHT / 2 - 80, 72, won ? GREEN : RED);
    DrawText(state.statusMessage.c_str(),
             (VIRTUAL_WIDTH - MeasureText(state.statusMessage.c_str(), 22)) / 2,
             VIRTUAL_HEIGHT / 2 + 20, 22, RAYWHITE);

    Rectangle btnMenu{(VIRTUAL_WIDTH - 200.f) / 2.f, VIRTUAL_HEIGHT / 2.f + 80.f, 200.f, 45.f};
    bool hovMenu = CheckCollisionPointRec(getVirtualMousePosition(), btnMenu);
    drawButton(btnMenu, "Back to Menu", hovMenu);
    if (hovMenu && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        playClick();
        if (m_owner) m_owner->resetToMenu();
    }
}