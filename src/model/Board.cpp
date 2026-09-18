#include "model/Board.hpp"

Board::Board() {
    reset();
}

void Board::reset() {
    m_grid.assign(SIZE, std::vector<CellState>(SIZE, CellState::Empty));
    m_shipGrid.assign(SIZE, std::vector<std::shared_ptr<Ship>>(SIZE, nullptr));
    m_placedShips.clear();
}

bool Board::inBounds(int x, int y) const noexcept {
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

CellState Board::getCell(int x, int y) const {
    if (!inBounds(x, y)) return CellState::Empty;
    return m_grid[y][x];
}

bool Board::canPlaceShip(const Ship& ship, Coordinate start) const {
    int len = ship.length();
    bool horiz = (ship.orientation() == Orientation::Horizontal);

    int endX = horiz ? start.x + len - 1 : start.x;
    int endY = horiz ? start.y : start.y + len - 1;

    if (!inBounds(start.x, start.y) || !inBounds(endX, endY)) {
        return false;
    }

    for (int i = 0; i < len; ++i) {
        int cx = horiz ? start.x + i : start.x;
        int cy = horiz ? start.y : start.y + i;

        if (m_grid[cy][cx] != CellState::Empty) {
            return false;
        }
    }
    return true;
}

bool Board::placeShip(std::shared_ptr<Ship> ship, Coordinate start) {
    if (!ship || !canPlaceShip(*ship, start)) {
        return false;
    }

    int len = ship->length();
    bool horiz = (ship->orientation() == Orientation::Horizontal);

    for (int i = 0; i < len; ++i) {
        int cx = horiz ? start.x + i : start.x;
        int cy = horiz ? start.y : start.y + i;

        m_grid[cy][cx] = CellState::ShipPresent;
        m_shipGrid[cy][cx] = ship;
    }

    m_placedShips.push_back(std::move(ship));
    return true;
}

AttackResult Board::receiveAttack(Coordinate target) {
    if (!inBounds(target.x, target.y)) {
        return AttackResult::Invalid;
    }

    CellState& current = m_grid[target.y][target.x];

    if (current == CellState::Hit || current == CellState::Miss) {
        return AttackResult::Invalid; // Already attacked
    }

    if (current == CellState::Empty) {
        current = CellState::Miss;
        return AttackResult::Miss;
    }

    // Hit a ship
    current = CellState::Hit;
    auto ship = m_shipGrid[target.y][target.x];
    if (ship) {
        ship->registerHit();
        if (ship->isSunk()) {
            return AttackResult::Sunk;
        }
    }

    return AttackResult::Hit;
}

bool Board::allShipsSunk() const noexcept {
    if (m_placedShips.empty()) return false;
    for (const auto& ship : m_placedShips) {
        if (!ship->isSunk()) return false;
    }
    return true;
}