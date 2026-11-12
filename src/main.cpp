#include "app/AppController.hpp"
#include <iostream>
#include <sockpp/socket.h>

int main() {
    sockpp::initialize(); //for windows support
    std::cout << "Starting Battleship..." << std::endl;

    AppController app;
    app.run();
}