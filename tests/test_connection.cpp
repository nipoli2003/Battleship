#include "model/Protocol.hpp"
#include "network/Client.hpp"
#include "network/Server.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <variant>

// test to check different game states

static const int PORT = 6534;

std::optional<Network::NetEvent> waitForEvent(NetworkClient &client, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (auto ev = client.poll())
            return ev;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return std::nullopt;
}

// two clients connect to server, create & join lobby, place ships, start game
TEST(GameTest, TestGameStart) {
    GameServer server;
    std::thread server_thread([&server]() {
        server.run(PORT);
    });
    // give the server thread a brief moment to bind port 6534
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    NetworkClient bob;
    bob.connect("127.0.0.1", PORT);

    // create lobby
    alice.createLobby();
    auto event = waitForEvent(alice);

    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*event));
    
    auto lobbyCode = std::get<Protocol::LobbyCode>(*event);

    // join lobby
    bob.joinLobby(lobbyCode.code);
    event = waitForEvent(bob);

    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyJoined>(*event));

    // alice, bob place ships
    Board board;
    auto placement = board.generateRandomFleet();

    std::vector<Protocol::ShipPlacement> placement_vec;
    for (auto& [type, coordOrient] : placement) {
        auto& [coord, orient] = coordOrient;
        placement_vec.push_back({(int)type, coord.x, coord.y, orient == Orientation::Horizontal ? 0 : 1});
    }

    alice.sendPlaceShips(placement_vec);
    bob.sendPlaceShips(placement_vec);

    // event = waitForEvent(alice);
    // std::cout << std::holds_alternative<Protocol::LobbyJoined>(*event) << std::endl << std::flush;

    // shut down cleanly
    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }
}