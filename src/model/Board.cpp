#include "model/Board.hpp"
#include "helpers/Log.hpp"

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

bool Board::hasShipAt(int x, int y) const noexcept {
    if (!inBounds(x, y)) return false;
    return m_shipGrid[y][x] != nullptr;
}

std::optional<std::shared_ptr<Ship>> Board::getShipAt(int x, int y) const noexcept {
    if (!inBounds(x, y)) return std::nullopt;
    return m_shipGrid[y][x];
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

        m_grid[cy][cx] = [&]() {
            switch (ship->type()) {
                case ShipType::Carrier: return CellState::Carrier;
                case ShipType::Battleship: return CellState::Battleship;
                case ShipType::Cruiser: return CellState::Cruiser;
                case ShipType::Submarine: return CellState::Submarine;
                case ShipType::Destroyer: return CellState::Destroyer;
            }
            return CellState::Empty; // Should never reach here
        }();
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

FleetPlacement Board::generateRandomFleet() {
    std::mt19937 rng{std::random_device{}()};
    return generateRandomFleet(rng);
}

FleetPlacement Board::generateRandomFleet(std::mt19937 &rng) {
    std::uniform_int_distribution<int> orientDist(0, 1);

    reset();

    FleetPlacement placement;
    placement.reserve(standardFleet.size());

    for (auto type : standardFleet) {
        bool placed = false;
        for (int attempt = 0; !placed && attempt < 1000; ++attempt) {
            auto orientation = (orientDist(rng) == 0) ? Orientation::Horizontal : Orientation::Vertical;

            // Generate coordinates constrained to board limits based on orientation
            Ship testShip(type, orientation);
            int maxX = (orientation == Orientation::Horizontal) ? (Board::SIZE - testShip.length()) : (Board::SIZE - 1);
            int maxY = (orientation == Orientation::Vertical) ? (Board::SIZE - testShip.length()) : (Board::SIZE - 1);

            std::uniform_int_distribution<int> xDist(0, maxX);
            std::uniform_int_distribution<int> yDist(0, maxY);
            Coordinate coord{xDist(rng), yDist(rng)};

            if (placeShip(std::make_shared<Ship>(type, orientation), coord)) {
                placement.push_back({type, {coord, orientation}});
                placed = true;
            }
        }
        if (!placed)
            LOG_SRV_WARN("failed to place ships");
    }
    return placement;
}