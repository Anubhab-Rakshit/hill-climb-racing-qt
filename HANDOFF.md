# HANDOFF

## Project
Qt 6 / C++20 Hill Climb Racing clone with a software scanline rasterizer.
All gameplay and UI pixels are written into a 32-bit ARGB `Framebuffer` and blitted with `QImage::scanLine`.

## Current Architecture
- `src/core/GameStateManager.*` owns menus, race flow, input, camera, HUD, audio, and `PhysicsWorld`.
- `src/physics/PhysicsWorld.*` owns terrain, one or two `Vehicle` instances, pickups, checkpoints, stunts, and multiplayer rules.
- `src/physics/Vehicle.*` is the vehicle facade. It contains the legacy custom solver and the Box2D backend.
- `src/physics/Box2DPhysics.*` is the fixed-step Box2D 2.2.1 world wrapper.
- `third_party/Box2D` is the vendored Box2D 2.2.1 source.
- `src/graphics/` is the software raster pipeline.
- `src/ui/` contains all menu/HUD screens.
- `src/audio/` contains the procedural/asset audio system.

## Physics Status
- Box2D is the default backend (`PhysicsWorld::m_useBox2D = true`).
- Direct `Vehicle` construction defaults to the legacy solver unless `setUseBox2D(true)` is called.
- `--legacy-physics` launches the full game on the legacy custom solver.
- The Box2D vehicle uses:
  - A chassis box derived from sprite dimensions.
  - Two wheel circle bodies.
  - `b2WheelJoint` suspension and motor.
  - Solid terrain strips for stable wheel collision.
  - Local terrain rebuilt around the vehicle.
- Controls:
  - `W` gas
  - `S` brake/reverse
  - `A` tilt left (air)
  - `D` tilt right (air)
  - Arrow keys for Player 2
  - `Esc`/`P` pause, `R` restart, `Enter`/`Space` select, `M` multiplayer
- Box2D motor direction was fixed so `W` drives +X and `S` drives -X.
- `A/D` no longer trigger gas/brake.
- Combined `S` brake/reverse is covered by a dedicated Box2D regression test.

## Box2D Tuning Notes
- Suspension frequency uses reduced mass:
  `1 / (1/chassisMass + 1/wheelMass)`.
- Drive reaction torque is counter-applied to the chassis at 85% when grounded, preventing full-throttle backflips while preserving some wheelie feel.
- Braking uses a grounded attitude PD stabilizer:
  `-angleError * 9000 - angularVelocity * 2200`, targeting the local terrain slope.
- Box2D brake torque factor is `0.75 * baseBrakingTorque * brakeMultiplier * brakePressure`.
- These changes are Box2D-only; the legacy solver is unchanged.

## Build
```bash
CMAKE_BIN="/Users/anubhabrakshit/Qt/Tools/CMake/CMake.app/Contents/bin/cmake"
$CMAKE_BIN -B build -S . -G Ninja \
  -DCMAKE_PREFIX_PATH=/Users/anubhabrakshit/Qt/6.11.1/macos \
  -DCMAKE_BUILD_TYPE=Release
$CMAKE_BIN --build build -j8
```

## Run
```bash
./build/HillClimbRacingQt
./build/HillClimbRacingQt --legacy-physics
```

## Tests
```bash
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d-vehicles
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d-world
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-physics
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-audio
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --smoke-test
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-ui-layout
```

## Important Constraints
- Do not replace the software rasterizer with `QPainter`, SVG, or vector drawing.
- Do not change Box2D 2.2.1 math conventions without checking `b2Vec2` scalar order.
- `Terrain::getGravity()` returns a positive magnitude. Box2D gravity is negative.
- `--test-physics` uses the legacy solver. Box2D behavior is covered by `--test-box2d`.
- Run `profile.resetUpgradesForTesting()` in deterministic tests.
- Do not edit `src/physics/Vehicle.cpp`, `src/physics/PhysicsWorld.cpp`, or `src/main.cpp` while another agent is tuning physics.

## Current Status
- `--test-box2d`: passing.
- `--test-box2d-vehicles`: all 12 vehicles passing idle, forward, reverse, brake, 15-degree slope, and air control.
- `--test-box2d-world`: passing single-player drive, fuel drain, terrain deformation, pickup, checkpoint, crash detection, and multiplayer.
- `--test-physics`: passing.
- `--test-audio`: passing.
- `--smoke-test`: passing on Box2D.
- `--smoke-test --legacy-physics`: passing on the legacy solver.
- `--test-ui-layout`: passing automated layout regression and screenshot capture at 960x540 and 1280x720.

## Phase 3
Completed. See `PHASE3.md`.
