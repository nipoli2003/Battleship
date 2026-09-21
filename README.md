# Battleship

A C++/CMake implementation of Battleship, built with a Raylib GUI, targeting both offline and online (networked) play against other players or AIs.

## Building & Running

The whole build-test-run pipeline is wrapped in `run.sh`. From the repo root:

```bash
./run.sh
```

This script (with `set -e`, so it stops at the first failure) does four things in order:

1. **Configure** — `cmake -B build`
   Generates the build system into a `build/` directory. You only strictly need to re-run this if you add/remove source files, change `CMakeLists.txt`, or the `build/` folder doesn't exist yet.
2. **Build** — `cmake --build build`
   Compiles the project using whatever generator/toolchain CMake picked (Make, Ninja, etc.).
3. **Test** — `ctest --test-dir build --output-on-failure`
   Runs the unit test suite. `--output-on-failure` prints test output only for failing tests, so passing tests stay quiet.
4. **Run** — `./build/battleship`
   Launches the game, but only if configuring, building, and testing all succeeded.

### Running steps individually

If you don't want the full pipeline, you can run any stage manually:

```bash
cmake -B build                                   # configure only
cmake --build build                              # build only
ctest --test-dir build --output-on-failure        # test only
./build/battleship                               # run only
```

## Committing & Pushing

Instead of typing out `git add`, `git commit`, and `git push` separately every time, use `gitpush.sh`:

```bash
./gitpush.sh "your commit message"
```

This stages all changes (`git add .`), commits them with the message you pass in as an argument, and pushes to `origin main`. It exits with an error (without committing anything) if you forget to provide a commit message, and stops immediately if any step fails — so a failed push, for example, won't be silently ignored.

## In Case of F*** Up

Things go wrong. Here's how to recover, roughly in the order you should try them:

**1. Read the actual error first.**
`set -e` means `run.sh` stops at the *first* failing step, so the last thing printed to your terminal (`==> Configuring...`, `==> Building...`, or `==> Running Tests...`) tells you which stage broke. Don't start deleting things before you know what failed.

**2. Configure step fails (`cmake -B build` errors out).**
Usually a `CMakeLists.txt` problem or a missing dependency (e.g. Raylib not found).
- Check the CMake error message for the missing package name.
- Make sure you're on a clean clone/checkout if you've been hand-editing generated files.

**3. Build step fails (compiler errors).**
- Read the compiler output top-to-bottom — the *first* error is usually the real one; everything after it can be noise cascading from that one mistake.
- Re-run just `cmake --build build` after fixing the code so you don't waste time re-configuring and re-testing.

**4. Tests fail (`ctest` reports failures).**
- `--output-on-failure` will show you exactly what the failing test expected vs. got — read it before assuming the test itself is wrong.
- Never comment out or delete a failing test just to get a green run. If a test is genuinely obsolete, say so in the PR/commit message and remove it deliberately.

**5. The build directory itself is corrupted / acting weird / stale cache.**
Nuke it and start fresh — this fixes the majority of "it worked yesterday" problems:

```bash
rm -rf build
./run.sh
```

**6. Still stuck?**
- Check you're not out of sync with `main`/`master` — `git status` and `git pull`.
- Check your CMake and compiler versions against what the project expects.
- Ask in the team chat before spending more than ~20 minutes stuck — someone has probably hit it already.

## What's Next

We're following a MoSCoW-prioritized roadmap. The guiding principle: **get the networking backbone in place before piling on features**, since online multiplayer is a large chunk of the grade and every extra rule we add has to be serialized and agreed on by both clients.

### Sprint 0 - Progress Roadmap creation
- Decide future feature introduction.
- Debate current situation of the build's skeleton and overall setup.
- Set up roles in the project, split jobs and tasks, come up with a general program.

### Sprint 1 — Model Generalization (Variable Board & Obstacles)
- Replace the hardcoded `static constexpr int SIZE = 10` with a dynamic `Board(int width, int height)` constructor, so 8×8 / 10×10 / 12×12 (and arbitrary sizes) are supported.
- Add `CellState::Obstacle` (islands/reefs/shipwrecks) and a seedable island-generation step.
- `canPlaceShip()` returns `false` when overlapping an obstacle.
- `receiveAttack()` returns a distinct `AttackResult::Blocked` (or auto-miss) for obstacle cells.
- Unit tests covering: ship placement against obstacles, board resizing, and attack resolution on obstacle cells.

### Sprint 2 — Networking Backbone (crucial deliverable)
- Lightweight TCP client/server (via `asio` or POSIX sockets).
- Define core packet types: `LOBBY_HANDSHAKE`, `PLACEMENT_READY`, `FIRE_COORDINATE`, `SHOT_RESULT`.
- Get a standard 1v1 online game working end-to-end between two separate client instances before adding any more gameplay features.

### Sprint 3 — Tactical Abilities (Carrier Strike, Sonar, Torpedo)
- Wrap abilities behind an `IAbility` / Command-pattern interface, once per game per ship:
  - **Carrier — Airstrike:** reveals/strikes a 3×3 area or a full row.
  - **Cruiser/Submarine — Sonar Ping:** reveals occupancy in a 3×3 zone without dealing damage.
  - **Destroyer — Torpedo:** fires down a column until it hits the first obstacle or ship.
- Add a `FIRE_ABILITY(type, x, y)` packet and sync it over the network layer from Sprint 2.

### Sprint 4 — Visual Polish & Sound
- Ship sprites/textures via `DrawTexturePro`, water animation, and sound effects using Raylib's `raudio`.
- Deliberately last: don't spend time on art before the graded core (dynamic board, networking, abilities) is solid.
