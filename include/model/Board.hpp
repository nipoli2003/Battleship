#pragma once

#include "Ship.hpp"
#include <vector>
#include <memory>
#include <optional>

struct Coordinate {
    int x{0}; // Column (0 to 9)
    int y{0}; // Row (0 to 9)

    bool operator==(const Coordinate& other) const = default;
};

enum class CellState {
    Empty,
    ShipPresent,
    Hit,
    Miss
};

enum class AttackResult {
    Miss,
    Hit,
    Sunk,
    Invalid
};

class Board {
public:
    static constexpr int SIZE = 10;

    Board();

    [[nodiscard]] CellState getCell(int x, int y) const;
    [[nodiscard]] bool canPlaceShip(const Ship& ship, Coordinate start) const;
    bool placeShip(std::shared_ptr<Ship> ship, Coordinate start);

    AttackResult receiveAttack(Coordinate target);
    [[nodiscard]] bool allShipsSunk() const noexcept;

    void reset();

private:
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;

    std::vector<std::vector<CellState>> m_grid;
    std::vector<std::vector<std::shared_ptr<Ship>>> m_shipGrid;
    std::vector<std::shared_ptr<Ship>> m_placedShips;
};