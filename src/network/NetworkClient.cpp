#include "network/NetworkClient.hpp"
#include <iostream>

static std::string NEW = "NEW";
static std::string JOIN = "JOIN";
static std::string BYE = "BYE";
static std::string ERROR = "ERR";

NetworkClient::~NetworkClient() {
  sendBye();
  // todo: how to end thread?
}

bool NetworkClient::connect(const std::string &host, int port) {
  if (connected())
    sendBye();

  try {
    sockpp::tcp_connector conn({host, static_cast<in_port_t>(port)});
    if (!conn) {
      std::cerr << "Connection failed\n";
      return false;
    }
    m_conn = std::move(conn);
    m_connected = true;
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Connection failed: " << e.what() << "\n";
    return false;
  }
}

std::string NetworkClient::sendNew() {
  if (!connected())
    return "";

  auto rsp = sendPlain(NEW);
  if (rsp.size() == 2 && rsp[0] == "CODE")
    return rsp[1];
  return "";
}

void NetworkClient::sendJoin(const std::string &code) {
  if (!connected())
    return;

  auto rsp = sendPlain(JOIN + " " + code);
}

void NetworkClient::sendBye() {
  if (!connected())
    return;

  const std::string bye = "BYE\n";
  m_conn.write_n(bye.data(), bye.size());
  m_conn.close();
  m_connected = false;
}

void NetworkClient::sendFire(uint8_t x, uint8_t y) {
  if (!connected())
    return;

  protocol::BinaryPacket packet;
  packet.header.type = protocol::PacketType::FireCoordinate;
}

std::vector<std::string> NetworkClient::sendPlain(const std::string &msg) {
  if (!connected())
    return {ERROR};

  // msg needs newline at end
  std::string formatted_msg = msg;
  if (formatted_msg.empty() || formatted_msg.back() != '\n')
    formatted_msg += '\n';

  if (m_conn.write_n(formatted_msg.data(), formatted_msg.size()) !=
      formatted_msg.size())
    return {ERROR};

  // read response byte-by-byte
  std::vector<std::string> rsp;
  std::string buf;
  char ch;
  while (m_conn.read(&ch, 1) == static_cast<unsigned long>(1)) {
    if (ch == '\n')
      break;
    if (ch == ' ') {
      rsp.push_back(buf);
      buf.clear();
      continue;
    }
    buf += ch;
  }
  if (buf.size() > 0)
    rsp.push_back(buf);

  // return response
  if (rsp.empty())
    return {ERROR};

  return rsp;
}