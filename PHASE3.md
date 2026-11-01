# PHASE 3: Box2D Parity, Tuning, and Gameplay QA

## Status: COMPLETE

## Goal
Make the Box2D backend the reliable default for every vehicle and biome, while preserving the legacy solver as a fallback.

## Completed Tasks

### 1. All-vehicle Box2D parity suite
- [x] Added `--test-box2d-vehicles`.
- [x] Exercises all 12 vehicles.
- [x] Per vehicle, verifies:
  - Idle stability.
  - Forward direction and minimum speed.
  - Reverse direction.
  - Braking stops the vehicle.
  - 15-degree slope climb.
  - Air-control pitch direction.
  - Driver remains alive and vehicle remains upright during normal driving.
- [x] Added combined `S` brake/reverse regression test.

### 2. Vehicle tuning
- [x] Added grounded anti-wheelie reaction-torque compensation.
- [x] Added grounded braking attitude PD stabilization.
- [x] Increased Box2D brake torque factor to `0.75`.
- [x] Preserved the reduced-mass suspension frequency fix.
- [x] Kept the legacy solver behavior unchanged.
- [x] All 12 vehicles pass forward, reverse, brake, slope, and air-control tests.

### 3. Gameplay integration
- [x] Added `--test-box2d-world`.
- [x] Verifies single-player drive, fuel drain, and terrain deformation.
- [x] Verifies item pickup and coin accounting.
- [x] Verifies checkpoint crossing.
- [x] Verifies crash detection.
- [x] Verifies both vehicles in multiplayer.
- [x] Smoke test passes across all biomes and vehicles.

### 4. Fallback
- [x] Added `--legacy-physics`.
- [x] Verified `--smoke-test --legacy-physics`.
- [x] Kept `--test-physics` on the legacy solver.

### 5. Final QA
- [x] `--test-box2d`
- [x] `--test-box2d-vehicles`
- [x] `--test-box2d-world`
- [x] `--test-physics`
- [x] `--test-audio`
- [x] `--smoke-test`
- [ ] Interactive playtest by the user.

## Test Commands
```bash
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d-vehicles
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d-world
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-physics
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-audio
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --smoke-test
```

## Acceptance
- [x] All automated suites pass.
- [x] Every vehicle can drive forward, reverse, brake, and climb the standard slope.
- [x] Controls are `W` gas, `S` brake/reverse, `A/D` air tilt.
- [x] No automated gameplay regression in menus, HUD, camera, or audio.
- [ ] User interactive playtest for feel and visual polish.
