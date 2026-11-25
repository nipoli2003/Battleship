#include "model/Protocol.hpp"
#include "network/Client.hpp"

#include "helpers.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
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

TEST(ServerTest, SecondCreateLobbyOnSameConnectionRejected) {
    ServerThread st(PORT);

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    alice.createLobby();
    auto ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*ev));

    alice.createLobby();
    ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value()); // not silence/timeout
    EXPECT_TRUE(std::holds_alternative<Protocol::ServerError>(*ev));
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

TEST(Server, JoinOwnLobbyRejected) {
    ServerThread st(PORT);

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);

    alice.createLobby();
    auto ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*ev));
    auto code = std::get<Protocol::LobbyCode>(*ev).code;

    alice.joinLobby(code);
    ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::ServerError>(*ev));
    auto error = std::get<Protocol::ServerError>(*ev);
    EXPECT_EQ(error.error, "Cannot join a lobby while already in one");

    // And the lobby should still be joinable by someone else.
    NetworkClient bob;
    bob.connect("127.0.0.1", PORT);
    bob.joinLobby(code);
    ev = waitForEvent(bob);
    ASSERT_TRUE(ev.has_value());
    EXPECT_TRUE(std::holds_alternative<Protocol::LobbyJoined>(*ev));
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

TEST(Server, CreatorDisconnectSendsGameOverToSurvivor) {
    ServerThread st(PORT);

    NetworkClient alice, bob;
    alice.connect("127.0.0.1", PORT);
    bob.connect("127.0.0.1", PORT);

    alice.createLobby();
    auto ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*ev));
    auto code = std::get<Protocol::LobbyCode>(*ev).code;

    bob.joinLobby(code);
    ev = waitForEvent(bob);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyJoined>(*ev));
    ev = waitForEvent(bob);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::StateUpdate>(*ev));

    alice.disconnect();

    ev = waitForEvent(bob);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::GameOver>(*ev));
    EXPECT_EQ(std::get<Protocol::GameOver>(*ev).result, Protocol::UserState::Victory);
}

TEST(Server, TwoJoinersRaceOnlyOneWins) {
    ServerThread st(PORT);

    NetworkClient alice, bob, charlie;
    alice.connect("127.0.0.1", PORT);
    bob.connect("127.0.0.1", PORT);
    charlie.connect("127.0.0.1", PORT);

    alice.createLobby();
    auto ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*ev));
    auto code = std::get<Protocol::LobbyCode>(*ev).code;

    bob.joinLobby(code);
    charlie.joinLobby(code);

    auto bobEv = waitForEvent(bob);
    auto charlieEv = waitForEvent(charlie);
    ASSERT_TRUE(bobEv.has_value());
    ASSERT_TRUE(charlieEv.has_value());

    int joined = std::holds_alternative<Protocol::LobbyJoined>(*bobEv) + std::holds_alternative<Protocol::LobbyJoined>(*charlieEv);
    int errored = std::holds_alternative<Protocol::LobbyError>(*bobEv) + std::holds_alternative<Protocol::LobbyError>(*charlieEv);
    EXPECT_EQ(joined, 1);
    EXPECT_EQ(errored, 1);
}

TEST(Server, TwoConcurrentSessionsIsolated) {
    ServerThread st(PORT);

    NetworkClient a1, b1, a2, b2;
    a1.connect("127.0.0.1", PORT);
    b1.connect("127.0.0.1", PORT);
    a2.connect("127.0.0.1", PORT);
    b2.connect("127.0.0.1", PORT);

    a1.createLobby();
    a2.createLobby();
    auto c1 = waitForEvent(a1);
    auto c2 = waitForEvent(a2);
    ASSERT_TRUE(c1.has_value() && std::holds_alternative<Protocol::LobbyCode>(*c1));
    ASSERT_TRUE(c2.has_value() && std::holds_alternative<Protocol::LobbyCode>(*c2));

    b1.joinLobby(std::get<Protocol::LobbyCode>(*c1).code);
    b2.joinLobby(std::get<Protocol::LobbyCode>(*c2).code);

    auto b1Ev = waitForEvent(b1);
    auto b2Ev = waitForEvent(b2);
    ASSERT_TRUE(b1Ev.has_value() && std::holds_alternative<Protocol::LobbyJoined>(*b1Ev));
    ASSERT_TRUE(b2Ev.has_value() && std::holds_alternative<Protocol::LobbyJoined>(*b2Ev));

    // Sessions should not have leaked any messages to each other:
    auto a1Ev = waitForEvent(a1);
    auto a2Ev = waitForEvent(a2);
    ASSERT_TRUE(a1Ev.has_value() && std::holds_alternative<Protocol::LobbyJoined>(*a1Ev));
    ASSERT_TRUE(a2Ev.has_value() && std::holds_alternative<Protocol::LobbyJoined>(*a2Ev));
}

TEST(Server, LobbyExpiresAfterTtl) {
    ServerThread st(PORT,1); // 1s timeout

    NetworkClient alice;
    alice.connect("127.0.0.1", PORT);
    alice.createLobby();
    auto ev = waitForEvent(alice);
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyCode>(*ev));

    ev = waitForEvent(alice, std::chrono::seconds(2));
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<Protocol::LobbyError>(*ev));
}
