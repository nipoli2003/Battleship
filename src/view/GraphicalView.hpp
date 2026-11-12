/**
 * @file GraphicalView.hpp
 * @brief Raylib-based graphical view for the Battleship client.
 */

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

/**
 * @brief Concrete graphical UI implementation used for the main game application.
 */
class GraphicalView : public IView {
  public:
    /**
     * @brief Creates an uninitialized graphical view.
     */
    GraphicalView();

    /**
     * @brief Destroys the view.
     */
    ~GraphicalView() override = default;

    /**
     * @brief Stores the owning controller for callback access.
     * @param owner Controller that owns this view.
     */
    void setOwner(AppController* owner) { m_owner = owner; }

    /**
     * @brief Initializes the rendering layer and UI state.
     */
    void init() override;

    /**
     * @brief Renders the current screen based on the application state.
     * @param state Client state snapshot to draw.
     */
    void render(const ClientState& state) override;

    /**
     * @brief Checks whether the view requests program shutdown.
     * @return true if the window should close.
     */
    [[nodiscard]] bool shouldClose() const override;

    /**
     * @brief Closes any active rendering resources.
     */
    void close() override;

  private:
    /**
     * @brief Toggles fullscreen mode.
     */
    void handleFullscreenToggle();

    /**
     * @brief Converts the current mouse position into virtual-view coordinates.
     * @return Mouse position mapped to the virtual resolution.
     */
    [[nodiscard]] Vector2 getVirtualMousePosition() const;

    /**
     * @brief Draws a button with hover styling.
     * @param bounds Button bounds in screen coordinates.
     * @param text Button label.
     * @param hovered Whether the mouse is currently over the button.
     */
    void drawButton(Rectangle bounds, const char* text, bool hovered);

    /**
     * @brief Handles text input for a UI field.
     * @param field Destination string being edited.
     * @param maxLen Maximum allowed length.
     */
    void handleTextInput(std::string& field, int maxLen);

    /**
     * @brief Plays the UI click sound.
     */
    void playClick();

    // scene renderers
    /**
     * @brief Renders the title screen.
     * @param state Current client state.
     */
    void renderTitleScreen(const ClientState& state);

    /**
     * @brief Renders the main menu screen.
     * @param state Current client state.
     */
    void renderMainMenu(const ClientState& state);

    /**
     * @brief Renders the lobby setup screen.
     * @param state Current client state.
     */
    void renderLobbySetup(const ClientState& state);

    /**
     * @brief Renders the lobby waiting screen.
     * @param state Current client state.
     */
    void renderLobbyWait(const ClientState& state);

    /**
     * @brief Renders the fleet placement screen.
     * @param state Current client state.
     */
    void renderPlacement(const ClientState& state);

    /**
     * @brief Renders the active match screen.
     * @param state Current client state.
     */
    void renderGame(const ClientState& state);

    /**
     * @brief Renders the post-game result screen.
     * @param state Current client state.
     */
    void renderGameOver(const ClientState& state);

    // drawing helpers
    /**
     * @brief Renders the settings modal dialog.
     * @param state Current client state.
     */
    void renderSettingsModal(const ClientState& state);

    /**
     * @brief Renders the exit confirmation modal.
     */
    void renderExitConfirmModal();

    /**
     * @brief Draws the settings trigger button.
     * @param x X position.
     * @param y Y position.
     */
    void drawSettingsTriggerButton(float x, float y);

    /**
     * @brief Draws an adjustable slider and returns its selected value.
     * @param bounds Slider bounds.
     * @param label Slider label.
     * @param currentValue Current value.
     * @param isDragging Whether the user is dragging the slider.
     * @return Selected value if the slider changed, otherwise std::nullopt.
     */
    std::optional<int> drawSlider(Rectangle bounds, const char* label, int currentValue, bool& isDragging);

    /**
     * @brief Draws a board grid from serialized cell data.
     * @param startX Left edge of the grid.
     * @param startY Top edge of the grid.
     * @param cells Board values to visualize.
     * @param hideShips Whether enemy ship positions should remain hidden.
     * @param isEnemy Whether this board belongs to the opponent.
     */
    void drawGrid(int startX, int startY, const std::vector<int>& cells, bool hideShips, bool isEnemy);

    /**
     * @brief Draws a board instance using the model representation.
     * @param startX Left edge of the board.
     * @param startY Top edge of the board.
     * @param board Board to render.
     */
    void drawLocalBoard(int startX, int startY, const Board& board);

    /**
     * @brief Handles click interactions on the enemy board.
     * @param enemyStartX Left edge of the enemy board.
     * @param enemyStartY Top edge of the enemy board.
     */
    void handleBoardClicks(int enemyStartX, int enemyStartY);

    /**
     * @brief Resets the state used for placing ships.
     */
    void resetPlacementState();

    /**
     * @brief Randomizes the local player's fleet for placement.
     */
    void randomizeLocalFleet();

    AppController* m_owner{nullptr}; ///< Owning app controller.

    // --- Settings (main thread only) ---
    bool m_showSettingsModal{false}; ///< Whether the settings modal is visible.
    bool m_draggingMusic{false}; ///< Whether the music slider is actively dragged.
    bool m_draggingSFX{false}; ///< Whether the SFX slider is actively dragged.

    // --- Exit confirmation (main thread only) ---
    bool m_showExitConfirm{false}; ///< Whether the exit confirmation box is visible.

    // --- Lobby input (main thread only) ---
    bool m_isCreating{true}; ///< Whether the user is creating a lobby.
    std::string m_ipInput{"127.0.0.1"}; ///< Default IP address entry.
    std::string m_codeInput; ///< Lobby code entry field.
    int m_activeInputField{0}; ///< Currently focused input field index.

    // --- temporary state while placing ---
    Board m_localBoard; ///< Local placement preview board.
    std::vector<Protocol::ShipPlacement> m_pendingPlacements; ///< Pending placement list.
    Orientation m_placementOrientation{Orientation::Horizontal}; ///< Current ship orientation.
    bool m_placementSent{false}; ///< Whether the current placement has already been sent.

    // --- reset placement state when entering InGame ---
    AppScene m_prevScene{AppScene::TitleScreen}; ///< Previous app scene before entering game view.

    static constexpr int VIRTUAL_WIDTH = 1200; ///< Base virtual width for UI scaling.
    static constexpr int VIRTUAL_HEIGHT = 700; ///< Base virtual height for UI scaling.
    static constexpr int CELL_SIZE = 35; ///< Cell size used in board rendering.
    RenderTexture2D m_target{}; ///< Render target used for resolution scaling.

    int m_windowedWidth{VIRTUAL_WIDTH}; ///< Current window width in windowed mode.
    int m_windowedHeight{VIRTUAL_HEIGHT}; ///< Current window height in windowed mode.
};