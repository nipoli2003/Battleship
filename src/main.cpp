#include "controller/BattleshipEngine.hpp"
#include "view/GraphicalView.hpp"
#include "view/AudioManager.hpp"
#include "raylib.h"
#include <iostream>

int main() {
    std::cout << "Starting Battleship GUI..." << std::endl;

    BattleshipEngine engine(OpponentType::LocalAI);
    GraphicalView view(engine);

    view.init();
    AudioManager::instance().init();

    while (!view.shouldClose()) {
        float dt = GetFrameTime();
        engine.update(dt);
        view.render();
        AudioManager::instance().update();
    }

    view.close();
    AudioManager::instance().close();
    return 0;
}