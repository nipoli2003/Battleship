#pragma once

#include "Ship.hpp"
#include <memory>
#include <optional>
#include <vector>
#include <random>

struct Coordinate {
    int x{0}; // Column (0 to 9)
    int y{0}; // Row (0 to 9)

    bool operator==(const Coordinate &other) const noexcept {
        return x == other.x && y == other.y;
    }
};

enum class CellState {
    Empty,
    Hit,
    Miss,
    Carrier,
    Battleship,
    Cruiser,
    Submarine,
    Destroyer,
};

enum class AttackResult {
    Miss,
    Hit,
    Sunk,
    Invalid
};

using FleetPlacement = std::vector<std::pair<ShipType, std::pair<Coordinate, Orientation>>>;

class Board {
  public:
    static constexpr int SIZE = 10;

    Board();

    [[nodiscard]] CellState getCell(int x, int y) const;
    [[nodiscard]] bool canPlaceShip(const Ship &ship, Coordinate start) const;
    bool placeShip(std::shared_ptr<Ship> ship, Coordinate start);

    AttackResult receiveAttack(Coordinate target);
    [[nodiscard]] bool allShipsSunk() const noexcept;

    bool hasShipAt(int x, int y) const noexcept;
    [[nodiscard]] std::optional<std::shared_ptr<Ship>> getShipAt(int x, int y) const noexcept;
    void reset();

    // places a full random legal fleet into `board` and returns the placement
    FleetPlacement generateRandomFleet();
    FleetPlacement generateRandomFleet(std::mt19937 &rng);

  private:
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;

    std::vector<std::vector<CellState>> m_grid;
    std::vector<std::vector<std::shared_ptr<Ship>>> m_shipGrid;
    std::vector<std::shared_ptr<Ship>> m_placedShips;
};