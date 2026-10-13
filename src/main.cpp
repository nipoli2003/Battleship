#include "view/GraphicalView.hpp"
#include <iostream>

int main() {
    std::cout << "Starting Battleship..." << std::endl;
    GraphicalView view;
    view.init();
    while (!view.shouldClose())
        view.render();
    view.close();
    return 0;
}