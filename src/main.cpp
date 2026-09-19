#include "controller/BattleshipEngine.hpp"
#include "view/GraphicalView.hpp"
#include "raylib.h"
#include <iostream>

int main() {
    std::cout << "Starting Battleship GUI..." << std::endl;

    BattleshipEngine engine(OpponentType::LocalAI);
    GraphicalView view(engine);

    view.init();

    while (!view.shouldClose()) {
        float dt = GetFrameTime();
        engine.update(dt);
        view.render();
    }

    view.close();
    return 0;
}