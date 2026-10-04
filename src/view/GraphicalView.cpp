#include "view/GraphicalView.hpp"
#include <algorithm>
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
    InitWindow(m_windowedWidth, m_windowedHeight, "Battleship - Naval Combat");
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);
    m_target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(m_target.texture, TEXTURE_FILTER_BILINEAR);
}

bool GraphicalView::shouldClose() const {
    return WindowShouldClose() || m_shouldExit;
}

void GraphicalView::close() {
    if (m_client && m_client->isConnected())
        m_client->disconnect();
    UnloadRenderTexture(m_target);
    CloseWindow();
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

void GraphicalView::drawButton(Rectangle b, const char *text, bool hovered) {
    DrawRectangleRec(b, hovered ? Color{50, 80, 110, 255} : Color{25, 45, 65, 255});
    DrawRectangleLinesEx(b, 2, hovered ? SKYBLUE : LIGHTGRAY);
    int fs = 20, tw = MeasureText(text, fs);
    DrawText(text, (int)(b.x + (b.width - tw) / 2),
             (int)(b.y + (b.height - fs) / 2), fs, RAYWHITE);
}

void GraphicalView::handleTextInput(std::string &field, int maxLen) {
    int c;
    while ((c = GetCharPressed()) != 0)
        if ((int)field.size() < maxLen && c >= 32 && c < 127)
            field += (char)c;
    if (IsKeyPressed(KEY_BACKSPACE) && !field.empty())
        field.pop_back();
}

void GraphicalView::resetToMenu() {
    // disconnect() joins the listen thread, so no callbacks fire after this
    if (m_client && m_client->isConnected())
        m_client->disconnect();
    m_client.reset();
    {
        std::lock_guard lk(m_netMutex);
        m_serverState.clear();
        m_statusMessage.clear();
        m_yourBoard.clear();
        m_enemyBoard.clear();
        m_lobbyCode.clear();
        m_lobbyError.clear();
        m_lobbySubState = LobbySubState::Setup;
    }
    m_localBoard.reset();
    m_pendingPlacements.clear();
    m_placementSent = false;
    m_pendingGameStart.store(false);
    m_currentScene = AppScene::MainMenu;
}

// ------------------------------------------------------------------ render

void GraphicalView::render() {
    handleFullscreenToggle();

    // Game-start transition: signaled atomically by onStateUpdate, applied here on the main thread
    if (m_pendingGameStart.exchange(false)) {
        m_localBoard.reset();
        m_pendingPlacements.clear();
        m_placementSent = false;
        m_currentScene = AppScene::InGame;
    }

    BeginTextureMode(m_target);
    ClearBackground(Color{15, 25, 35, 255});

    switch (m_currentScene) {
    case AppScene::MainMenu:
        renderMainMenu();
        break;
    case AppScene::OnlineLobbyWait: {
        LobbySubState sub;
        {
            std::lock_guard lk(m_netMutex);
            sub = m_lobbySubState;
        }
        if (sub == LobbySubState::Setup)
            renderLobbySetup();
        else
            renderLobbyWait();
        break;
    }
    case AppScene::InGame: {
        std::string state;
        {
            std::lock_guard lk(m_netMutex);
            state = m_serverState;
        }
        if (state == "Victory" || state == "Defeat")
            renderGameOver();
        else if (state.empty() || state == "PlacementPhase")
            renderPlacement();
        else
            renderGame();
        break;
    }
    }

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    float scale = std::min((float)GetScreenWidth() / VIRTUAL_WIDTH,
                           (float)GetScreenHeight() / VIRTUAL_HEIGHT);
    Rectangle src = {0, 0, (float)m_target.texture.width, -(float)m_target.texture.height};
    Rectangle dest = {
        (GetScreenWidth() - VIRTUAL_WIDTH * scale) * 0.5f,
        (GetScreenHeight() - VIRTUAL_HEIGHT * scale) * 0.5f,
        VIRTUAL_WIDTH * scale, VIRTUAL_HEIGHT * scale};
    DrawTexturePro(m_target.texture, src, dest, {0, 0}, 0, WHITE);
    EndDrawing();
}

// ------------------------------------------------------------------ Main Menu

void GraphicalView::renderMainMenu() {
    const char *title = "BATTLESHIP";
    DrawText(title, (VIRTUAL_WIDTH - MeasureText(title, 96)) / 2, VIRTUAL_HEIGHT / 4, 96, SKYBLUE);

    Vector2 mouse = getVirtualMousePosition();
    float bw = 280.f, bh = 50.f, bx = (VIRTUAL_WIDTH - bw) / 2.f;
    float by = VIRTUAL_HEIGHT / 2.f - 60.f;

    Rectangle btnCreate{bx, by, bw, bh};
    bool hovCreate = CheckCollisionPointRec(mouse, btnCreate);
    drawButton(btnCreate, "Create Lobby", hovCreate);
    if (hovCreate && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_isCreating = true;
        m_codeInput.clear();
        {
            std::lock_guard lk(m_netMutex);
            m_lobbyError.clear();
            m_lobbySubState = LobbySubState::Setup;
        }
        m_currentScene = AppScene::OnlineLobbyWait;
    }

    Rectangle btnJoin{bx, by + 65.f, bw, bh};
    bool hovJoin = CheckCollisionPointRec(mouse, btnJoin);
    drawButton(btnJoin, "Join Lobby", hovJoin);
    if (hovJoin && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        m_isCreating = false;
        m_codeInput.clear();
        {
            std::lock_guard lk(m_netMutex);
            m_lobbyError.clear();
            m_lobbySubState = LobbySubState::Setup;
        }
        m_currentScene = AppScene::OnlineLobbyWait;
    }

    Rectangle btnExit{bx, by + 130.f, bw, bh};
    bool hovExit = CheckCollisionPointRec(mouse, btnExit);
    drawButton(btnExit, "Exit Game", hovExit);
    if (hovExit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        m_shouldExit = true;
}

// ------------------------------------------------------------------ Lobby Setup (text input)

void GraphicalView::renderLobbySetup() {
    const char *heading = m_isCreating ? "CREATE LOBBY" : "JOIN LOBBY";
    DrawText(heading, (VIRTUAL_WIDTH - MeasureText(heading, 32)) / 2, 80, 32, SKYBLUE);

    Vector2 mouse = getVirtualMousePosition();
    float fw = 340.f, fh = 40.f, fx = (VIRTUAL_WIDTH - fw) / 2.f;

    // IP field
    DrawText("Server IP:", (int)fx, 195, 20, RAYWHITE);
    Rectangle ipBox{fx, 220.f, fw, fh};
    bool ipSel = (m_activeInputField == 0);
    DrawRectangleRec(ipBox, Color{10, 20, 35, 255});
    DrawRectangleLinesEx(ipBox, 2, ipSel ? SKYBLUE : LIGHTGRAY);
    DrawText(m_ipInput.c_str(), (int)(ipBox.x + 8), (int)(ipBox.y + 10), 20, RAYWHITE);
    if (CheckCollisionPointRec(mouse, ipBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        m_activeInputField = 0;
    if (ipSel)
        handleTextInput(m_ipInput, 64);

    float nextY = 290.f;

    // Code field (join only)
    if (!m_isCreating) {
        DrawText("Lobby Code:", (int)fx, (int)nextY - 5, 20, RAYWHITE);
        Rectangle codeBox{fx, nextY + 20.f, fw, fh};
        bool codeSel = (m_activeInputField == 1);
        DrawRectangleRec(codeBox, Color{10, 20, 35, 255});
        DrawRectangleLinesEx(codeBox, 2, codeSel ? SKYBLUE : LIGHTGRAY);
        DrawText(m_codeInput.c_str(), (int)(codeBox.x + 8), (int)(codeBox.y + 10), 20, RAYWHITE);
        if (CheckCollisionPointRec(mouse, codeBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            m_activeInputField = 1;
        if (codeSel)
            handleTextInput(m_codeInput, 6);
        nextY += 80.f;
    }

    // Connect button
    Rectangle btnConn{(VIRTUAL_WIDTH - 220.f) / 2.f, nextY + 20.f, 220.f, 45.f};
    bool hovConn = CheckCollisionPointRec(mouse, btnConn);
    drawButton(btnConn, m_isCreating ? "Connect & Create" : "Connect & Join", hovConn);
    if (hovConn && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !m_ipInput.empty()) {
        if (m_isCreating)
            connectAndCreate(m_ipInput, 9000);
        else
            connectAndJoin(m_ipInput, 9000, m_codeInput);
    }

    // Back button
    Rectangle btnBack{(VIRTUAL_WIDTH - 200.f) / 2.f, nextY + 80.f, 200.f, 45.f};
    bool hovBack = CheckCollisionPointRec(mouse, btnBack);
    drawButton(btnBack, "Back to Menu", hovBack);
    if (hovBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        m_currentScene = AppScene::MainMenu;
}

// ------------------------------------------------------------------ Lobby Wait (status display)

void GraphicalView::renderLobbyWait() {
    LobbySubState sub;
    std::string code, err;
    {
        std::lock_guard lk(m_netMutex);
        sub = m_lobbySubState;
        code = m_lobbyCode;
        err = m_lobbyError;
    }

    if (sub == LobbySubState::Connecting) {
        DrawText("Connecting...", (VIRTUAL_WIDTH - MeasureText("Connecting...", 28)) / 2, 300, 28, RAYWHITE);
    } else if (sub == LobbySubState::ShowCode) {
        const char *h = "LOBBY CREATED";
        DrawText(h, (VIRTUAL_WIDTH - MeasureText(h, 32)) / 2, 120, 32, SKYBLUE);
        const char *sub1 = "Share this code with your opponent:";
        DrawText(sub1, (VIRTUAL_WIDTH - MeasureText(sub1, 20)) / 2, 200, 20, RAYWHITE);
        DrawText(code.c_str(), (VIRTUAL_WIDTH - MeasureText(code.c_str(), 64)) / 2, 240, 64, YELLOW);
        const char *sub2 = "Waiting for opponent to join...";
        DrawText(sub2, (VIRTUAL_WIDTH - MeasureText(sub2, 20)) / 2, 340, 20, LIGHTGRAY);
    } else if (sub == LobbySubState::Waiting) {
        const char *msg = "Joined! Waiting for game to start...";
        DrawText(msg, (VIRTUAL_WIDTH - MeasureText(msg, 22)) / 2, 300, 22, RAYWHITE);
    } else if (sub == LobbySubState::Error) {
        DrawText("Error", (VIRTUAL_WIDTH - MeasureText("Error", 28)) / 2, 250, 28, RED);
        DrawText(err.c_str(), (VIRTUAL_WIDTH - MeasureText(err.c_str(), 20)) / 2, 295, 20, LIGHTGRAY);
    }

    Vector2 mouse = getVirtualMousePosition();
    Rectangle btnBack{(VIRTUAL_WIDTH - 200.f) / 2.f, 430.f, 200.f, 45.f};
    bool hovBack = CheckCollisionPointRec(mouse, btnBack);
    drawButton(btnBack, "Back to Menu", hovBack);
    if (hovBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        resetToMenu();
}

// ------------------------------------------------------------------ Placement

void GraphicalView::randomizeLocalFleet() {
    m_localBoard.reset();
    m_pendingPlacements.clear();

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> orientDist(0, 1);

    for (auto type : SHIP_ORDER) {
        bool placed = false;
        for (int attempts = 0; !placed && attempts < 1000; ++attempts) {
            auto orient = orientDist(rng) == 0 ? Orientation::Horizontal : Orientation::Vertical;
            Ship tmp(type, orient);
            int maxX = (orient == Orientation::Horizontal) ? Board::SIZE - tmp.length() : Board::SIZE - 1;
            int maxY = (orient == Orientation::Vertical) ? Board::SIZE - tmp.length() : Board::SIZE - 1;
            int x = std::uniform_int_distribution<int>(0, maxX)(rng);
            int y = std::uniform_int_distribution<int>(0, maxY)(rng);
            auto ship = std::make_shared<Ship>(type, orient);
            if (m_localBoard.placeShip(ship, {x, y})) {
                m_pendingPlacements.push_back({(int)type, x, y, orient == Orientation::Horizontal ? 0 : 1});
                placed = true;
            }
        }
    }
}

void GraphicalView::drawLocalBoard(int startX, int startY) {
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            Rectangle cell{(float)(startX + x * CELL_SIZE), (float)(startY + y * CELL_SIZE),
                           (float)CELL_SIZE, (float)CELL_SIZE};
            Color fill = Color{20, 40, 60, 255};
            if (m_localBoard.getCell(x, y) == CellState::ShipPresent)
                fill = DARKGRAY;
            DrawRectangleRec(cell, fill);
            DrawRectangleLinesEx(cell, 1, Color{50, 75, 100, 255});
        }
    }
}

void GraphicalView::renderPlacement() {
    if (m_placementSent) {
        const char *msg = "Fleet deployed — waiting for opponent...";
        DrawText(msg, (VIRTUAL_WIDTH - MeasureText(msg, 22)) / 2, VIRTUAL_HEIGHT / 2 - 11, 22, YELLOW);
        return;
    }

    if (IsKeyPressed(KEY_R))
        m_placementOrientation = (m_placementOrientation == Orientation::Horizontal)
                                     ? Orientation::Vertical
                                     : Orientation::Horizontal;

    DrawText("FLEET DEPLOYMENT", (VIRTUAL_WIDTH - MeasureText("FLEET DEPLOYMENT", 30)) / 2, 35, 30, SKYBLUE);

    // Determine which ship is next
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
        const char *msg = "All ships placed! Click Confirm to send.";
        DrawText(msg, (VIRTUAL_WIDTH - MeasureText(msg, 20)) / 2, 80, 20, GREEN);
    }

    int gw = Board::SIZE * CELL_SIZE;
    int gx = (VIRTUAL_WIDTH - gw) / 2;
    int gy = 150;

    drawLocalBoard(gx, gy);

    Vector2 mouse = getVirtualMousePosition();

    // Ghost preview
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
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && valid) {
                auto ship = std::make_shared<Ship>(*currentType, m_placementOrientation);
                m_localBoard.placeShip(ship, {hx, hy});
                m_pendingPlacements.push_back({(int)*currentType, hx, hy,
                                               m_placementOrientation == Orientation::Horizontal ? 0 : 1});
            }
        }
    }

    // Auto-Deploy
    Rectangle btnAuto{(float)(gx + gw + 30), (float)(gy + 50), 160.f, 40.f};
    bool hovAuto = CheckCollisionPointRec(mouse, btnAuto);
    drawButton(btnAuto, "Auto-Deploy", hovAuto);
    if (hovAuto && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        randomizeLocalFleet();

    // Confirm (all ships placed)
    if (m_pendingPlacements.size() == SHIP_ORDER.size()) {
        Rectangle btnConf{(float)(gx + gw + 30), (float)(gy + 110), 160.f, 40.f};
        bool hovConf = CheckCollisionPointRec(mouse, btnConf);
        drawButton(btnConf, "Confirm", hovConf);
        if (hovConf && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            m_client->sendPlaceShips(m_pendingPlacements);
            m_placementSent = true;
        }
    }

    DrawText("Orientation: [R] to Rotate", gx, gy + gw + 25, 18, RAYWHITE);
    DrawText(m_placementOrientation == Orientation::Horizontal ? "(Horizontal)" : "(Vertical)",
             gx + 230, gy + gw + 25, 18, SKYBLUE);
}

// ------------------------------------------------------------------ Game

void GraphicalView::drawGrid(int startX, int startY, const std::vector<int> &cells, bool hideShips, bool isEnemy) {
    Vector2 mouse = getVirtualMousePosition();
    for (int y = 0; y < Board::SIZE; ++y) {
        for (int x = 0; x < Board::SIZE; ++x) {
            int idx = y * Board::SIZE + x;
            auto cs = (idx < (int)cells.size()) ? static_cast<CellState>(cells[idx]) : CellState::Empty;
            Rectangle cell{(float)(startX + x * CELL_SIZE), (float)(startY + y * CELL_SIZE),
                           (float)CELL_SIZE, (float)CELL_SIZE};
            Color fill = Color{20, 40, 60, 255};
            if (cs == CellState::ShipPresent && !hideShips)
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
                m_client->sendFireShot(x, y);
                return;
            }
}

void GraphicalView::renderGame() {
    std::string state, status;
    std::vector<int> your, enemy;
    {
        std::lock_guard lk(m_netMutex);
        state = m_serverState;
        status = m_statusMessage;
        your = m_yourBoard;
        enemy = m_enemyBoard;
    }

    DrawText(status.c_str(), (VIRTUAL_WIDTH - MeasureText(status.c_str(), 22)) / 2, 35, 22, YELLOW);

    int gw = Board::SIZE * CELL_SIZE;
    int hgx = VIRTUAL_WIDTH / 2 - gw - 50;
    int egx = VIRTUAL_WIDTH / 2 + 50;
    int gy = 160;

    DrawText("YOUR FLEET", hgx + 110, gy - 30, 20, RAYWHITE);
    DrawText("RADAR / ENEMY FLEET", egx + 70, gy - 30, 20, RAYWHITE);

    drawGrid(hgx, gy, your, false, false);
    drawGrid(egx, gy, enemy, true, true);

    if (state == "YourTurn")
        handleBoardClicks(egx, gy);

    Rectangle btnLeave{20.f, 20.f, 100.f, 35.f};
    bool hovLeave = CheckCollisionPointRec(getVirtualMousePosition(), btnLeave);
    drawButton(btnLeave, "< Menu", hovLeave);
    if (hovLeave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        resetToMenu();
}

void GraphicalView::renderGameOver() {
    std::string state, status;
    {
        std::lock_guard lk(m_netMutex);
        state = m_serverState;
        status = m_statusMessage;
    }

    bool won = (state == "Victory");
    const char *headline = won ? "VICTORY!" : "DEFEAT";
    DrawText(headline, (VIRTUAL_WIDTH - MeasureText(headline, 72)) / 2,
             VIRTUAL_HEIGHT / 2 - 80, 72, won ? GREEN : RED);
    DrawText(status.c_str(), (VIRTUAL_WIDTH - MeasureText(status.c_str(), 22)) / 2,
             VIRTUAL_HEIGHT / 2 + 20, 22, RAYWHITE);

    Rectangle btnMenu{(VIRTUAL_WIDTH - 200.f) / 2.f, VIRTUAL_HEIGHT / 2.f + 80.f, 200.f, 45.f};
    bool hovMenu = CheckCollisionPointRec(getVirtualMousePosition(), btnMenu);
    drawButton(btnMenu, "Back to Menu", hovMenu);
    if (hovMenu && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        resetToMenu();
}

// ------------------------------------------------------------------ Network

void GraphicalView::connectAndCreate(const std::string &ip, int port) {
    m_client = std::make_unique<NetworkClient>(
        [this](std::string code) { onLobbyCode(std::move(code)); },
        [this]() { onLobbyJoined(); },
        [this](std::string reason) { onLobbyError(std::move(reason)); },
        [this]() { onStartGame(); },
        [this](Protocol::StateUpdate upd) { onStateUpdate(std::move(upd)); },
        [this](Protocol::GameOver msg) { onGameOver(std::move(msg)); });
    {
        std::lock_guard lk(m_netMutex);
        m_lobbySubState = LobbySubState::Connecting;
    }
    if (!m_client->connect(ip, port)) {
        m_client.reset();
        std::lock_guard lk(m_netMutex);
        m_lobbyError = "Failed to connect to " + ip;
        m_lobbySubState = LobbySubState::Error;
        return;
    }
    m_client->createLobby();
}

void GraphicalView::connectAndJoin(const std::string &ip, int port, const std::string &code) {
    m_client = std::make_unique<NetworkClient>(
        [this](std::string c) { onLobbyCode(std::move(c)); },
        [this]() { onLobbyJoined(); },
        [this](std::string reason) { onLobbyError(std::move(reason)); },
        [this]() { onStartGame(); },
        [this](Protocol::StateUpdate upd) { onStateUpdate(std::move(upd)); },
        [this](Protocol::GameOver msg) { onGameOver(std::move(msg)); });
    {
        std::lock_guard lk(m_netMutex);
        m_lobbySubState = LobbySubState::Connecting;
    }
    if (!m_client->connect(ip, port)) {
        m_client.reset();
        std::lock_guard lk(m_netMutex);
        m_lobbyError = "Failed to connect to " + ip;
        m_lobbySubState = LobbySubState::Error;
        return;
    }
    m_client->joinLobby(code);
}

// Callbacks — invoked from the listen thread

void GraphicalView::onLobbyCode(std::string code) {
    std::lock_guard lk(m_netMutex);
    m_lobbyCode = std::move(code);
    m_lobbySubState = LobbySubState::ShowCode;
}

void GraphicalView::onLobbyJoined() {
    std::lock_guard lk(m_netMutex);
    m_lobbySubState = LobbySubState::Waiting;
}

void GraphicalView::onLobbyError(std::string reason) {
    std::lock_guard lk(m_netMutex);
    m_lobbyError = std::move(reason);
    m_lobbySubState = LobbySubState::Error;
}

void GraphicalView::onStartGame() {
    // server doesn't send START_GAME in the current protocol;
    // the first StateUpdate drives the transition instead.
}

void GraphicalView::onStateUpdate(Protocol::StateUpdate update) {
    bool first = false;
    {
        std::lock_guard lk(m_netMutex);
        first = m_serverState.empty(); // first update triggers scene switch
        m_serverState = update.state;
        m_statusMessage = update.statusMessage;
        m_yourBoard = std::move(update.yourBoard);
        m_enemyBoard = std::move(update.enemyBoard);
    }
    if (first)
        m_pendingGameStart.store(true); // render() picks this up on the next frame
}

void GraphicalView::onGameOver(Protocol::GameOver msg) {
    std::lock_guard lk(m_netMutex);
    m_serverState = msg.result; // "Victory" or "Defeat"
    m_statusMessage = msg.message;
}