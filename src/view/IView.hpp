#pragma once

struct ClientState;

class IView {
public:
    virtual ~IView() = default;

    virtual void init() = 0;
    virtual void render(const ClientState& state) = 0;
    [[nodiscard]] virtual bool shouldClose() const = 0;
    virtual void close() = 0;
};
