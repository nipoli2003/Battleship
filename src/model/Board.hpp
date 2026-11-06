/**
 * @file Board.hpp
 * @brief Game board representation, cells, and ship placement logic.
 */

#pragma once

#include "Ship.hpp"
#include <memory>
#include <optional>
#include <vector>
#include <random>

/**
 * @brief A coordinate on the board.
 *
 * x is the column, y is the row, both in the range 0..9 for a standard board.
 */
struct Coordinate {
    int x{0}; ///< Column index.
    int y{0}; ///< Row index.

    /**
     * @brief Compares two coordinates for equality.
     * @param other The coordinate to compare against.
     * @return true if both coordinates are identical.
     */
    bool operator==(const Coordinate &other) const noexcept {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief State of a single cell on the board.
 */
enum class CellState {
    Empty,      ///< No ship and no shot fired here.
    Hit,        ///< Occupied cell that has been hit.
    Miss,       ///< Empty cell that was attacked.
    Carrier,    ///< Occupied by a carrier.
    Battleship, ///< Occupied by a battleship.
    Cruiser,    ///< Occupied by a cruiser.
    Submarine,  ///< Occupied by a submarine.
    Destroyer   ///< Occupied by a destroyer.
};

/**
 * @brief Result of an attack on a target cell.
 */
enum class AttackResult {
    Miss,    ///< The shot missed all ships.
    Hit,     ///< A ship was hit but not sunk.
    Sunk,    ///< The shot sank a ship.
    Invalid  ///< Target was outside the board or invalid.
};

/**
 * @brief A ship placement record: type, starting coordinate and orientation.
 */
using FleetPlacement = std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>>;

/**
 * @brief A 10x10 Battleship board.
 *
 * The board stores ship ownership and shot history, and is responsible for
 * validating and applying placements and attacks.
 */
class Board {
  public:
    static constexpr int SIZE = 10; ///< Standard board size in both dimensions.

    /**
     * @brief Creates an empty board.
     */
    Board();

    /**
     * @brief Reads the state of a board cell.
     * @param x Column index.
     * @param y Row index.
     * @return The cell state at (x, y).
     */
    [[nodiscard]] CellState getCell(int x, int y) const;

    /**
     * @brief Checks whether a ship can be placed at the given starting cell.
     * @param ship The ship to place.
     * @param start The starting coordinate for the ship.
     * @return true if the placement is legal.
     */
    [[nodiscard]] bool canPlaceShip(const Ship &ship, Coordinate start) const;

    /**
     * @brief Places a ship on the board.
     * @param ship Ship to place.
     * @param start Starting coordinate for the ship.
     * @return true if the ship was placed successfully.
     */
    bool placeShip(std::shared_ptr<Ship> ship, Coordinate start);

    /**
     * @brief Resolves an attack on the target coordinate.
     * @param target The cell to attack.
     * @return The outcome of the attack.
     */
    AttackResult receiveAttack(Coordinate target);

    /**
     * @brief Checks whether all ships on the board have been sunk.
     * @return true once every placed ship is destroyed.
     */
    [[nodiscard]] bool allShipsSunk() const noexcept;

    /**
     * @brief Checks whether a ship occupies the given cell.
     * @param x Column index.
     * @param y Row index.
     * @return true if any ship occupies that cell.
     */
    bool hasShipAt(int x, int y) const noexcept;

    /**
     * @brief Fetches the ship occupying a given cell.
     * @param x Column index.
     * @param y Row index.
     * @return The ship at the cell, if any.
     */
    [[nodiscard]] std::optional<std::shared_ptr<Ship>> getShipAt(int x, int y) const noexcept;

    /**
     * @brief Removes all ships and resets the board to empty.
     */
    void reset();

    /**
     * @brief Places a full random legal fleet on the board.
     * @return The placements used for each ship.
     */
    FleetPlacement generateRandomFleet();

    /**
     * @brief Places a full random legal fleet on the board using a supplied RNG.
     * @param rng Random number generator to use.
     * @return The placements used for each ship.
     */
    FleetPlacement generateRandomFleet(std::mt19937 &rng);

  private:
    /**
     * @brief Checks whether the given coordinate lies inside the board.
     * @param x Column index.
     * @param y Row index.
     * @return true if the coordinate is valid.
     */
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;

    std::vector<std::vector<CellState>> m_grid; ///< Cell-by-cell board state.
    std::vector<std::vector<std::shared_ptr<Ship>>> m_shipGrid; ///< Ship reference at each cell.
    std::vector<std::shared_ptr<Ship>> m_placedShips; ///< Ships currently placed on the board.
};