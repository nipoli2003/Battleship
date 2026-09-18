#include "controller/BattleshipEngine.hpp"
#include "view/GraphicalView.hpp"
#include <iostream>

int main() {
    std::cout << "Starting Battleship GUI..." << std::endl;

    BattleshipEngine engine(OpponentType::LocalAI);
    GraphicalView view(engine);

    view.init();

    while (!view.shouldClose()) {
        // If it's the bot's turn, trigger its move
        if (engine.getSnapshot().state == MatchState::OpponentTurn) {
            engine.processAITurn();
        }

        view.render();
    }

    view.close();
    return 0;
}