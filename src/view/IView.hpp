/**
 * @file IView.hpp
 * @brief Interface for all UI implementations.
 */

#pragma once

struct ClientState;

/**
 * @brief Abstract interface implemented by graphical and non-graphical views.
 */
class IView {
public:
    /**
     * @brief Virtual destructor for interface cleanup.
     */
    virtual ~IView() = default;

    /**
     * @brief Initializes the view before use.
     */
    virtual void init() = 0;

    /**
     * @brief Renders the current application state.
     * @param state State snapshot provided by the app controller.
     */
    virtual void render(const ClientState& state) = 0;

    /**
     * @brief Checks whether the view should request shutdown.
     * @return true when the UI wants the app to close.
     */
    [[nodiscard]] virtual bool shouldClose() const = 0;

    /**
     * @brief Releases resources owned by the view.
     */
    virtual void close() = 0;
};
