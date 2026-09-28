#pragma once

#include "Ship.hpp"
#include <vector>
#include <memory>
#include <optional>

struct Coordinate {
    int x{0}; // Column (0 to board.size() - 1)
    int y{0}; // Row    (0 to board.size() - 1)

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
    static constexpr int DEFAULT_SIZE = 10;
    static constexpr int MIN_SIZE = 8;   // smallest board that reliably fits the 17-cell fleet
    static constexpr int MAX_SIZE = 15;  // largest board that still fits the UI layout

    explicit Board(int size = DEFAULT_SIZE);

    [[nodiscard]] int size() const noexcept { return m_size; }

    [[nodiscard]] CellState getCell(int x, int y) const;
    [[nodiscard]] bool canPlaceShip(const Ship& ship, Coordinate start) const;
    bool placeShip(std::shared_ptr<Ship> ship, Coordinate start);

    AttackResult receiveAttack(Coordinate target);
    [[nodiscard]] bool allShipsSunk() const noexcept;

    void reset();             // clear the board, keep the current size
    void reset(int newSize);  // clear the board and resize it (clamped to MIN_SIZE..MAX_SIZE)

private:
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;

    int m_size{DEFAULT_SIZE};

    std::vector<std::vector<CellState>> m_grid;
    std::vector<std::vector<std::shared_ptr<Ship>>> m_shipGrid;
    std::vector<std::shared_ptr<Ship>> m_placedShips;
};