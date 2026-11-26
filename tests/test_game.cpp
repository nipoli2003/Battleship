#include "model/Protocol.hpp"
#include "network/Client.hpp"

#include "helpers.hpp"

#include <gtest/gtest.h>
#include <variant>

// test to check different game states

static const int PORT = 6534;

// two clients connect to server, create & join lobby, place ships, start game
TEST(GameTest, TestGameStart) {
    ServerThread st(PORT);

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
    for (auto &[type, coordOrient] : placement) {
        auto &[coord, orient] = coordOrient;
        placement_vec.push_back({(int)type, coord.x, coord.y, orient == Orientation::Horizontal ? 0 : 1});
    }

    alice.sendPlaceShips(placement_vec);
    bob.sendPlaceShips(placement_vec);

    // event = waitForEvent(alice);
    // std::cout << std::holds_alternative<Protocol::LobbyJoined>(*event) << std::endl << std::flush;
}
