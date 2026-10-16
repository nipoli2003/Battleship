#include "view/GraphicalView.hpp"
#include "view/AudioManager.hpp"
#include "raylib.h"
#include <helpers/logger.hpp>

int main() {
    Log::setLevel(Log::Level::Debug);
    BS_INFO << "Starting Battleship";
    GraphicalView view;
    view.init();
    AudioManager::instance().init();

    while (!view.shouldClose()) {
        float dt = GetFrameTime();
        // engine.update(dt);
        view.render();
        AudioManager::instance().update();
    }

    view.close();
    AudioManager::instance().close();
    return 0;
}
