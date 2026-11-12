#include "network/Server.hpp"
#include <sockpp/socket.h>

int main() {
  sockpp::initialize(); // for windows support  
  GameServer server;
  server.run(9000);
}
