#include <gtest/gtest.h>
#include "network/NetworkClient.hpp"
#include "network/RelayServer.hpp"
#include <sockpp/tcp_connector.h>
#include <chrono>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace {

// gtest_discover_tests runs each test in its own process, possibly in parallel.
// SO_REUSEPORT would let those relays share a port and steal each other's
// connections, so each process gets its own port.
int testPort() {
  static const int port = 20000 + (::getpid() % 20000);
  return port;
}

void startRelayOnce() {
  static std::once_flag flag;
  std::call_once(flag, [] {
    static RelayServer server;
    std::thread([] { server.run(testPort()); }).detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  });
}

template <class S>
void writeLine(S &s, const std::string &line) {
  std::string msg = line + "\n";
  s.write_n(msg.data(), msg.size());
}

template <class S>
std::string readLine(S &s) {
  std::string out;
  char ch;
  while (s.read(&ch, 1) == static_cast<unsigned long>(1)) {
    if (ch == '\n')
      break;
    out += ch;
  }
  return out;
}

sockpp::tcp_connector rawConnect() {
  return sockpp::tcp_connector({"127.0.0.1", static_cast<in_port_t>(testPort())});
}

} // namespace

// ---------- Relay server (raw sockets) ----------

TEST(RelayTest, NewReturnsCode) {
  startRelayOnce();
  auto host = rawConnect();
  ASSERT_TRUE(host);

  writeLine(host, "NEW");
  std::string rsp = readLine(host);

  ASSERT_EQ(rsp.rfind("CODE ", 0), 0u);
  EXPECT_EQ(rsp.size(), 5u + 6u); // "CODE " + 6 chars
}

TEST(RelayTest, JoinUnknownCodeGivesErr) {
  startRelayOnce();
  auto c = rawConnect();
  ASSERT_TRUE(c);

  writeLine(c, "JOIN NOPE99");
  EXPECT_EQ(readLine(c), "ERR");
}

TEST(RelayTest, UnknownCommandGivesErr) {
  startRelayOnce();
  auto c = rawConnect();
  ASSERT_TRUE(c);

  writeLine(c, "HELLO");
  EXPECT_EQ(readLine(c), "ERR");
}

TEST(RelayTest, PairedClientsRelayBothWays) {
  startRelayOnce();
  auto host = rawConnect();
  ASSERT_TRUE(host);
  writeLine(host, "NEW");
  std::string code = readLine(host).substr(5);

  auto guest = rawConnect();
  ASSERT_TRUE(guest);
  writeLine(guest, "JOIN " + code);
  EXPECT_EQ(readLine(guest), "OK");

  writeLine(guest, "ping");
  EXPECT_EQ(readLine(host), "ping");

  writeLine(host, "pong");
  EXPECT_EQ(readLine(guest), "pong");
}

TEST(RelayTest, CodeIsSingleUse) {
  startRelayOnce();
  auto host = rawConnect();
  writeLine(host, "NEW");
  std::string code = readLine(host).substr(5);

  auto g1 = rawConnect();
  writeLine(g1, "JOIN " + code);
  EXPECT_EQ(readLine(g1), "OK");

  auto g2 = rawConnect();
  writeLine(g2, "JOIN " + code);
  EXPECT_EQ(readLine(g2), "ERR");
}

// ---------- NetworkClient ----------

TEST(NetworkClientTest, ConnectFailsWhenNoServer) {
  NetworkClient client;
  EXPECT_FALSE(client.connect("127.0.0.1", 1)); // nothing listens on port 1
  EXPECT_FALSE(client.connected());
}

TEST(NetworkClientTest, SendNewReturnsSixCharCode) {
  startRelayOnce();
  NetworkClient client;
  ASSERT_TRUE(client.connect("127.0.0.1", testPort()));

  std::string code = client.sendNew();
  EXPECT_EQ(code.size(), 6u);
}

TEST(NetworkClientTest, SendNewWithoutConnectionReturnsEmpty) {
  NetworkClient client;
  EXPECT_EQ(client.sendNew(), "");
}

TEST(NetworkClientTest, ClientJoinsClientHostedLobby) {
  startRelayOnce();
  NetworkClient host;
  ASSERT_TRUE(host.connect("127.0.0.1", testPort()));
  std::string code = host.sendNew();
  ASSERT_EQ(code.size(), 6u);

  NetworkClient guest;
  ASSERT_TRUE(guest.connect("127.0.0.1", testPort()));
  guest.sendJoin(code);
  // Once sendJoin returns without hanging, the relay accepted the pairing.
  // Add byte-level assertions here after sendFire/poll are implemented.
}