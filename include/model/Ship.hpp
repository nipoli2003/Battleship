#pragma once

#include <string>
#include <string_view>

enum class ShipType {
    Carrier,
    Battleship,
    Cruiser,
    Submarine,
    Destroyer
};

enum class Orientation {
    Horizontal,
    Vertical
};

class Ship {
public:
    explicit Ship(ShipType type, Orientation orientation = Orientation::Horizontal)
        : m_type(type),
          m_orientation(orientation),
          m_hits(0) {}

    [[nodiscard]] ShipType type() const noexcept { return m_type; }
    [[nodiscard]] Orientation orientation() const noexcept { return m_orientation; }
    [[nodiscard]] int hits() const noexcept { return m_hits; }
    [[nodiscard]] bool isSunk() const noexcept { return m_hits >= length(); }

    void setOrientation(Orientation orientation) noexcept { m_orientation = orientation; }
    void registerHit() noexcept { if (m_hits < length()) ++m_hits; }

    [[nodiscard]] int length() const noexcept {
        switch (m_type) {
            case ShipType::Carrier:    return 5;
            case ShipType::Battleship: return 4;
            case ShipType::Cruiser:    return 3;
            case ShipType::Submarine:  return 3;
            case ShipType::Destroyer:  return 2;
        }
        return 0;
    }

    [[nodiscard]] std::string_view name() const noexcept {
        switch (m_type) {
            case ShipType::Carrier:    return "Carrier";
            case ShipType::Battleship: return "Battleship";
            case ShipType::Cruiser:    return "Cruiser";
            case ShipType::Submarine:  return "Submarine";
            case ShipType::Destroyer:  return "Destroyer";
        }
        return "Unknown";
    }

private:
    ShipType m_type;
    Orientation m_orientation;
    int m_hits;
};