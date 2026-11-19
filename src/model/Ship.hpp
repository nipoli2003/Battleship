/**
 * @file Ship.hpp
 * @brief Ship types, orientations, the standard fleet and the Ship class.
 */

#pragma once

#include <string_view>
#include <vector>

/**
 * @brief The five ship classes of a standard Battleship fleet.
 *
 * The type determines the ship's length (see Ship::length()).
 */
enum class ShipType {
    Carrier,    ///< Length 5.
    Battleship, ///< Length 4.
    Cruiser,    ///< Length 3.
    Submarine,  ///< Length 3.
    Destroyer   ///< Length 2.
};

/**
 * @brief The fleet every player must place: one ship of each ShipType.
 */
inline static const std::vector<ShipType> standardFleet{
    ShipType::Carrier,
    ShipType::Battleship,
    ShipType::Cruiser,
    ShipType::Submarine,
    ShipType::Destroyer
};

/**
 * @brief Direction in which a ship extends from its starting cell.
 */
enum class Orientation {
    Horizontal, ///< Extends to the right (increasing x).
    Vertical    ///< Extends downwards (increasing y).
};

/**
 * @brief A single ship: its type, orientation and the damage it has taken.
 *
 * A Ship does not know where it is on the grid; its position is stored by
 * Board, which calls registerHit() when one of the ship's cells is attacked.
 */
class Ship {
public:
    /**
     * @brief Creates an undamaged ship.
     * @param type        Ship class; determines the length.
     * @param orientation Horizontal (default) or vertical.
     */
    explicit Ship(ShipType type, Orientation orientation = Orientation::Horizontal)
        : m_type(type),
          m_orientation(orientation),
          m_hits(0) {}

    /// @return The ship class.
    [[nodiscard]] ShipType type() const noexcept { return m_type; }

    /// @return The current orientation.
    [[nodiscard]] Orientation orientation() const noexcept { return m_orientation; }

    /// @return How many of the ship's cells have been hit so far.
    [[nodiscard]] int hits() const noexcept { return m_hits; }

    /// @return true once every cell of the ship has been hit.
    [[nodiscard]] bool isSunk() const noexcept { return m_hits >= length(); }

    /**
     * @brief Changes the orientation.
     *
     * Only meaningful before the ship is placed on a Board.
     * @param orientation The new orientation.
     */
    void setOrientation(Orientation orientation) noexcept { m_orientation = orientation; }

    /**
     * @brief Records one hit on the ship.
     *
     * The hit count never exceeds length(), so extra calls on a sunk
     * ship have no effect.
     */
    void registerHit() noexcept { if (m_hits < length()) ++m_hits; }

    /**
     * @brief Number of cells the ship occupies.
     * @return 5 for Carrier, 4 for Battleship, 3 for Cruiser and Submarine,
     *         2 for Destroyer.
     */
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

    /// @return Human-readable name of the ship class (e.g. "Carrier").
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
    ShipType m_type;           ///< Ship class.
    Orientation m_orientation; ///< Current orientation.
    int m_hits;                ///< Cells hit so far (0 to length()).
};