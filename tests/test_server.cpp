#include "model/Protocol.hpp"
#include "network/Client.hpp"

#include "helpers.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <variant>

static const int PORT = 6534;

TEST(ServerTest, TestStartServer) {
    ServerThread st(PORT);
    // automatic cleanup
}

TEST(ServerTest, CreateMultipleLobbies) {
    ServerThread st(PORT);

    std::vector<std::unique_ptr<NetworkClient>> cl;
    for (size_t i = 0; i < 10; ++i) {
        auto c = std::make_unique<NetworkClient>();
        c->connect("127.0.0.1", PORT);
        c->createLobby();
        auto event = waitForEvent(*c);
        ASSERT_TRUE(event.has_value());
        EXPECT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*event));

        // move to vector so client does not deconstruct (disconnect)
        cl.push_back(std::move(c));
    }
}

TEST(ServerTest, NoMultipleLobbiesForSameClient) {
    ServerThread st(PORT);

    // give the server thread a brief moment to bind port 6534
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    for (size_t i = 0; i < 2; ++i) {
        alice.createLobby();
        auto event = waitForEvent(alice);
        ASSERT_TRUE(event.has_value());
        EXPECT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*event));
    }
}

TEST(Server, TestJoinInvalidLobbyFails) {
    ServerThread st(PORT);

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    alice.joinLobby("000000");

    auto event = waitForEvent(alice);
    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyError>(*event));
    auto error = std::get<Protocol::LobbyError>(*event);
    std::cout << error.error << std::endl;
}

TEST(Server, TestJoinEmptyLobby) {
    ServerThread st(PORT);

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    NetworkClient bob;
    bob.connect("127.0.0.1", PORT);

    // create lobby, disconnect
    alice.createLobby();
    auto event = waitForEvent(alice);

    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*event));
    alice.disconnect();

    // join empty lobby
    auto lobbyCode = std::get<Protocol::LobbyCode>(*event);
    bob.joinLobby(lobbyCode.code);
    event = waitForEvent(bob);

    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyError>(*event));
    auto error = std::get<Protocol::LobbyError>(*event);
    std::cout << error.error << std::endl;
}

TEST(Server, TestCreateAndJoinLobby) {
    ServerThread st(PORT);

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    NetworkClient bob;
    bob.connect("127.0.0.1", PORT);

    // create lobby, disconnect
    alice.createLobby();
    auto event = waitForEvent(alice);

    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*event));

    // join created lobby
    auto lobbyCode = std::get<Protocol::LobbyCode>(*event);
    bob.joinLobby(lobbyCode.code);

    event = waitForEvent(bob);
    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyJoined>(*event));

    event = waitForEvent(bob);
    ASSERT_TRUE(event.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::StateUpdate>(*event));
}
