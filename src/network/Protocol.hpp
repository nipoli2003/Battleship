#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace protocol {

// all values sent over the wire in big-endian (network byte order)
enum class PacketType : uint8_t {
    LobbyHandshake = 0x01, // Client -> Server on connect
    PlacementReady = 0x02, // Client -> Server: fleet locked in
    FireCoordinate = 0x03, // Client -> Server: (x, y) attack
    ShotResult = 0x04,     // Server -> Client: forwarded result
    GameStart = 0x05,      // Server -> both: game can begin
    Disconnect = 0xFF,
};

// "dont insert padding bytes into the struct, please"
#pragma pack(push, 1)

struct PacketHeader {
    PacketType type;
    uint16_t bodyLen; // length of the payload that follows
};

struct FirePayload {
    uint8_t x;
    uint8_t y;
};

struct ShotResultPayload {
    uint8_t x;
    uint8_t y;
    uint8_t result; // maps to AttackResult enum (see model/Board.hpp)
};

#pragma pack(pop)

struct BinaryPacket {
    PacketHeader header;
    std::vector<uint8_t> payload;
};

} // namespace protocol
