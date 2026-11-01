# Developer Setup & Friends' Backend Physics Guide
**How to Open, Build, Extend, and Test the Hill Climb Racing Project**

---

## 🏎️ Welcome to the Team!
The frontend architecture, pure-raster software rendering pipeline, Box2D 2.2.1 physics engine, and interactive UI/UX are **100% complete, fully tested, and compiling with 0 warnings**.

This guide is written specifically for teammates working on **Physics Tuning, Vehicle Dynamics, Mathematics, and Gameplay Extensions** so you can immediately dive in without getting bogged down by Qt GUI boilerplate.

---

## 🛠️ 1. Technology Stack & Building

### Standard Stack
- **Language**: Modern C++ (C++20 standard)
- **Framework**: Qt 6.11.1 on macOS (Core, Gui, Widgets, Multimedia)
- **Build System**: CMake 3.20+ with Ninja generator
- **Physics Backend**: Vendored Box2D 2.2.1 (`third_party/Box2D/`) with fallback to analytical solver (`--legacy-physics`)
- **Rendering**: 100% pure software rasterizer writing directly to 32-bit ARGB `Framebuffer` (zero QPainter/SVG/vector graphics)

### Building via Terminal (macOS)
```bash
# Configure with Ninja and Qt 6.11.1
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="/Users/anubhabrakshit/Qt/6.11.1/macos"

# Build Hill Climb Racing executable
cmake --build build -j8
```

### Windows & Linux Setup Notes
- **Windows**: Install Qt 6.x (Desktop C++ MinGW or MSVC 2022) via the official Qt Online Installer. From the build terminal, run `cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="<path-to-qt6>"` followed by `cmake --build build`.
- **Linux**: Install Qt 6 developer packages (`qt6-base-dev`, `qt6-multimedia-dev`, `cmake`, `ninja-build`). Run `cmake -B build -G Ninja` and `cmake --build build`.
- **Zero External Physics Dependencies**: Box2D 2.2.1 is vendored under `third_party/Box2D/`, so you do not need to install Box2D externally on any platform.

### Opening in Qt Creator
1. Open **Qt Creator**.
2. Select **File > Open File or Project...**
3. Choose [`CMakeLists.txt`](../CMakeLists.txt).
4. Select your installed Qt 6 Desktop kit and click **Configure Project**.
5. Press `Ctrl + R` (or `Cmd + R` on macOS) to build and run!

---

## 🎮 2. Running the Game & Key Controls

### Launch Commands
Run with Box2D 2.2.1 physics backend (Default):
```bash
./build/HillClimbRacingQt
```

Run with legacy analytical physics engine (Fallback):
```bash
./build/HillClimbRacingQt --legacy-physics
```

### Controls Mapping Table
| Action | Player 1 (Single / Left) | Player 2 (Local Multiplayer) | System / Global |
| :--- | :--- | :--- | :--- |
| **Throttle / Accelerate** | `W` | `Up Arrow` | — |
| **Brake / Reverse** | `S` | `Down Arrow` | — |
| **Tilt Back (Air Pitch Up)** | `A` | `Left Arrow` | — |
| **Tilt Forward (Air Pitch Down)** | `D` | `Right Arrow` | — |
| **Pause / Resume** | — | — | `Esc` or `P` |
| **Quick Restart** | — | — | `R` |
| **Select / Confirm / Click** | — | — | `Enter` or `Space` |
| **Toggle Multiplayer Mode** | — | — | `M` (on Menu screen) |

---

## 🧩 3. Architecture Map: Where Does Everything Live?

```
src/
├── core/                  # Engine Loop, States, Input, Profile
│   ├── GameEngine.*       # 60Hz/120Hz fixed-timestep accumulator loop
│   ├── GameStateManager.*# Finite state machine (Menu -> Garage -> Stages -> Gameplay -> Over)
│   ├── InputController.*  # Non-blocking keyboard state tracking (W/A/S/D & Arrow keys)
│   └── ProfileManager.*   # Coins balance, high scores, upgrade tiers (Engine, Susp, Tires, 4WD)
│
├── graphics/              # Pure 32-bit ARGB Software Rasterizer (NO QPainter/SVG)
│   ├── Framebuffer.*      # Raw scanline memory buffer (0xAARRGGBB uint32_t*)
│   ├── PixelCanvas.*      # QWidget hosting framebuffer with aspect-ratio letterboxing
│   ├── RasterFont.*       # 5x7 / 8x8 pixel bitmap typography engine
│   ├── ScanlineRasterizer.*# Fast vertical column terrain & strata polygon filler
│   ├── Camera.*           # Dynamic velocity look-ahead, speed zoom, and trauma shake
│   ├── ParticleSystem.*   # Exhaust smoke puffs & tire dirt sprays
│   └── Sprite.*           # Rotated vehicle chassis, driver head, and wheel blitter
│
├── ui/                    # Rich Arcade Pixel UI Layer
│   ├── UITheme.h          # Retro color palette tokens
│   ├── RetroButton.*      # Tactile 3D beveled buttons
│   ├── AnalogGauge.*      # Tachometer (RPM) & Speedometer (km/h) needle dials
│   ├── FuelBar.*          # 16-segment LED fuel gauge with low-fuel blink alarm
│   ├── StuntBanner.*      # Floating animated popups (Backflip, Air Time)
│   ├── HUD.*              # Complete in-game cockpit with Minimap radar & telemetry card
│   ├── MainMenuScreen.*   # Title screen with multi-layer parallax hills & showcase plateau
│   ├── GarageScreen.*     # Tuning shop with 4 upgrade cards & dynamic hydraulic lift
│   ├── StageSelectScreen.*# Biome selection (Countryside, Desert, Arctic, Mountain, Moon, Volcano)
│   ├── PauseOverlay.*     # Modal pause menu (Resume, Restart, Garage, Menu)
│   └── GameOverScreen.*   # Detailed run summary table
│
├── audio/                 # Dynamic Audio Subsystem
│   └── SoundManager.*     # Synthesized RPM pitch modulation, SFX mixing, CC0 music
│
├── physics/               # 🎯 THE BACKEND & SIMULATION SUBSYSTEM
│   ├── PhysicsTypes.h     # Vec2, ContactPoint structs, unit conversion factors
│   ├── Box2DPhysics.*     # Vendored Box2D 2.2.1 backend (b2World, b2WheelJoint, EdgeChain)
│   ├── PhysicsWorld.*     # Unified facade & analytical fallback solver
│   ├── Vehicle.*          # Multi-body vehicle abstraction & physical properties
│   └── Terrain.*          # Procedural harmonic heightmaps & analytical derivatives
│
└── third_party/
    └── Box2D/             # Vendored Box2D 2.2.1 C++ physics library
```

---

## 🔬 4. Tuning Physics & Vehicle Dynamics

Physical behaviors are encapsulated in `src/physics/`:
- **Box2D Vehicle Backend (`src/physics/Box2DPhysics.cpp`)**: Contains the `b2WheelJointDef` setup, spring frequency ($f_0$ in Hz), damping ratio ($\zeta$), motor torque, and rotational inertia for all 12 vehicles.
- **Vehicle Facade (`src/physics/Vehicle.cpp`)**: Supplies wheelbase dimensions, tire radii, mass properties, air pitch torque, and upgrade multipliers.
- **Procedural Terrain (`src/physics/Terrain.cpp`)**: Generates elevation functions across the 6 biomes with reachability and climbability constraints.

---

## 🧪 5. Automated Verification & Test Suites

The project features **6 deterministic custom CLI test suites** built directly into the game executable.

> [!NOTE]
> Tests use custom built-in CLI verification routines rather than QTest. Prepend `QT_QPA_PLATFORM=offscreen` to run them headlessly in any terminal or automated CI pipeline.

```bash
# 1. Box2D 2.2.1 Backend Integrity Suite
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d

# 2. All 12 Vehicles Behavior Suite (Idle, Drive, Brake, Slope, Air Tilt)
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d-vehicles

# 3. Box2D Gameplay-World Integration Suite (Drive, Fuel, Checkpoints, MP, Crash)
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-box2d-world

# 4. Comprehensive 19-Test Vehicle & Suspension Physics Audit
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-physics

# 5. Dynamic Realistic Audio Test Suite
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --test-audio

# 6. Full Smoke Test & Procedural Reachability Scan
QT_QPA_PLATFORM=offscreen ./build/HillClimbRacingQt --smoke-test
```

---

## ⚠️ 6. Known Limitations & Testing Caveats

1. **Offscreen Platform**: Always specify `QT_QPA_PLATFORM=offscreen` when running CLI test suites in environments without an active display server.
2. **Optimized Builds**: Pure software rasterization performs CPU-side pixel blitting. Always test in Release mode (`-O2` / `-O3`) for accurate 60 FPS performance.
3. **Audio Output in Headless Mode**: In headless environments without hardware speakers, audio output falls back cleanly without crashing; FFmpeg multimedia warnings may be safely ignored during test runs.
