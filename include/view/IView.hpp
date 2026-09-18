#pragma once

class IView {
public:
    virtual ~IView() = default;
    virtual void init() = 0;
    virtual void render() = 0;
    [[nodiscard]] virtual bool shouldClose() const = 0;
    virtual void close() = 0;
};