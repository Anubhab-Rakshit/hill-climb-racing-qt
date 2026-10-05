# Developer Setup & Friends' Backend Physics Guide
**How to Open, Build, and Extend the Hill Climb Racing Project in Qt Creator**

---

## 🏎️ Welcome to the Team!
The frontend architecture, pure-raster software rendering pipeline, and interactive UI/UX are **100% complete, fully tested, and compiling with 0 warnings**.

This guide is written specifically for teammates working on **Physics, Mathematics, Vehicle Dynamics, and Sound Synthesis** so you can immediately dive in without getting bogged down by Qt GUI boilerplate.

---

## 🛠️ 1. Opening the Project in Qt Creator (One-Click)

The repository provides both **CMake** and **qmake** project configurations:

### Option A: Using CMake (Recommended for Modern Qt 6)
1. Open **Qt Creator**.
2. Select **File > Open File or Project...**
3. Choose [`CMakeLists.txt`](../CMakeLists.txt).
4. Select your installed Qt 6 (or Qt 5) Desktop kit and click **Configure Project**.
5. Press `Ctrl + R` (or `Cmd + R` on macOS) to build and run!

### Option B: Using qmake (.pro)
1. Open **Qt Creator**.
2. Select **File > Open File or Project...**
3. Choose [`hill-climbing-qt.pro`](../hill-climbing-qt.pro).
4. Hit **Run**!

---

## 🧩 2. Architecture Map: Where Does Everything Live?

```
src/
├── core/                  # Engine Loop, States, Input, Profile
│   ├── GameEngine.*       # 120Hz fixed-timestep accumulator loop
│   ├── GameStateManager.*# Transitions (Menu -> Garage -> Stages -> Gameplay -> Over)
│   ├── InputController.*  # Asynchronous non-blocking keyboard & mouse pedal tracking
│   └── ProfileManager.*   # Coins balance, high scores, upgrade multipliers (Engine, Susp, Tires, 4WD)
│
├── graphics/              # Pure 32-bit ARGB Software Rasterizer
│   ├── Framebuffer.*      # Direct scanline memory pointer (0xAARRGGBB)
│   ├── PixelCanvas.*      # QWidget with aspect-ratio letterboxing
│   ├── RasterFont.*       # 5x7 / 8x8 pixel bitmap typography renderer
│   ├── ScanlineRasterizer.*# Fast vertical column terrain & parallax filler
│   ├── Camera.*           # Dynamic look-ahead, speed zoom, and trauma shake
│   ├── ParticleSystem.*   # Exhaust smoke puffs & tire dirt sprays
│   └── Sprite.*           # Rotated vehicle chassis, driver head, and wheel spokes
│
├── ui/                    # Rich Arcade UI/UX Layer
│   ├── UITheme.h          # HSL curated retro color palette tokens
│   ├── RetroButton.*      # 3D beveled arcade buttons with tactile press offset
│   ├── AnalogGauge.*      # Tachometer (RPM) & Speedometer (km/h) dials
│   ├── FuelBar.*          # 16-segment LED fuel gauge with low-fuel blink alarm
│   ├── StuntBanner.*      # Floating animated popups (Backflip, Air Time)
│   ├── HUD.*              # Complete in-game cockpit with interactive pedals
│   ├── MainMenuScreen.*   # Title screen with animated hills
│   ├── GarageScreen.*     # Tuning shop with 4 upgrade cards & spring-bouncing car
│   ├── StageSelectScreen.*# Biome selection (Countryside, Desert, Moon, Mountain)
│   ├── PauseOverlay.*     # Modal pause menu (Resume, Restart, Garage, Menu)
│   └── GameOverScreen.*   # Run summary (Distance, Coins, Flips, Air Time, New Record)
│
└── physics/               # 🎯 THE BACKEND & MATH PLAYGROUND FOR TEAMMATES!
    ├── PhysicsTypes.h     # Vec2, ContactPoint structs
    ├── Terrain.*          # Procedural harmonic heightmap & normal derivations
    ├── Vehicle.*          # Multi-body chassis, wheels, suspension struts, driver head
    └── PhysicsWorld.*     # 120Hz solver, stunt evaluator, coin/fuel pickups
```

---

## 🔬 3. The Teammate Physics & Math Playground

Your teammates do not need to touch the UI or graphics. All physical behaviors are modularly encapsulated in `src/physics/`:

### A. Tuning the Suspension (`src/physics/Vehicle.cpp`)
Look at lines `85-115` in `Vehicle.cpp`:
```cpp
// Suspension Parameters
float k_susp = 18000.0f * suspStiffnessMult; // Spring stiffness
float c_susp = 1200.0f * suspStiffnessMult;  // Viscous damping
```
Teammates can implement Box2D's natural frequency $f_0$ formula:
$$k = M (2\pi f_0)^2, \quad c = 2 M \zeta (2\pi f_0)$$

### B. Tuning Tire Friction & Ground Traction (`src/physics/Vehicle.cpp`)
Look at lines `125-165` in `Vehicle.cpp`:
```cpp
float surfSpeed = Vec2::dot(vel, tangent);
float wheelLinSpeed = angVel * m_wheelRadius;
float slipSpeed = surfSpeed - wheelLinSpeed;
float tractiveForce = -std::clamp(slipSpeed * 800.0f, -maxFriction, maxFriction);
```
Teammates can experiment with Pacejka slip curves or Coulomb friction models.

### C. In-Air Attitude Pitch Control (`src/physics/Vehicle.cpp`)
Look at lines `185-195` in `Vehicle.cpp`:
```cpp
if (!m_rearContact && !m_frontContact) {
    float airPitchTorque = 400.0f;
    if (effectiveGas > 0.0f)   m_chassisAngularVel += ...; // Gas leans back
    if (m_brakeInput > 0.0f)   m_chassisAngularVel -= ...; // Brake leans forward
}
```

### D. Procedural Terrain Math (`src/physics/Terrain.cpp`)
Look at `Terrain::getHeight(float x)` in `Terrain.cpp`:
```cpp
float h1 = std::sin(lx * 0.020f) * 12.0f;
float h2 = std::sin(lx * 0.065f) * 4.0f;
float h3 = std::sin(lx * 0.18f) * 1.0f;
return h1 + h2 + h3;
```
Teammates can plug in Perlin noise octaves, bezier curves, or dynamic terrain deformations here.

---

## 🎮 4. How the Frontend Automatically Reflects Backend Math

Whatever your friends calculate in the physics backend **instantly renders in real time**:
- **Speedometer**: Automatically displays `vehicle.getSpeedKmh()`.
- **Tachometer**: Automatically updates with wheel angular velocity + engine throttle load.
- **Fuel Bar**: Automatically displays `vehicle.fuel()` and triggers the low-fuel alarm when below 25%.
- **Suspension Compression**: Automatically compresses the visible struts between chassis and wheels.
- **Air Flips**: Completing a 360-degree rotation in `PhysicsWorld` automatically awards coins, sounds cheers, and triggers the `BACKFLIP! +1000` floating banner!
- **Driver Down**: If the driver's head touches terrain, `vehicle.isDriverDown()` triggers the `DRIVER DOWN!` Game Over screen.
- **Garage Upgrades**: When players buy Engine, Suspension, Tire, or 4WD upgrades in the Garage, the multipliers in `ProfileManager` directly scale the physical variables in `Vehicle.cpp`!

---

## 🏆 Summary Checklist for Defense Day
- [x] Compiles with **0 warnings** on `-Wall -Wextra -Wpedantic`.
- [x] Fixed 120Hz physics accumulator loop decoupled from 60Hz display refresh.
- [x] 100% pure software rasterization (`QImage` ARGB32) with zero vector drawings.
- [x] Complete screen suite: Main Menu, Garage, Stage Select, Cockpit HUD, Pause, Game Over.
- [x] Clean architecture ready for team division.
