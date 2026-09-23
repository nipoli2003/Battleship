#include "network/RelayServer.hpp"

int main() {
    RelayServer server(9000);
    server.run();
}