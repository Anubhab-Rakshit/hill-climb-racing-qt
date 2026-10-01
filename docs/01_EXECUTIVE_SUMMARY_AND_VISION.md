# Document 01: Executive Summary & Competitive Vision
**Hill Climb Racing: Qt C++ Pure-Raster Engineering Specification**

---

## 1. Executive Summary
This document establishes the strategic, aesthetic, and technical vision for creating an industry-grade, competitive 2D Hill Climb Racing game within Qt Creator using pure C++ and rasterization techniques. 

### Why This Project Will Win Top Honors
Most student or lab submissions fall into typical traps:
1. **Floaty or Broken Physics**: Using naive Euler integration ($v = v + a \cdot dt, x = x + v \cdot dt$) with fixed 60Hz or frame-rate-dependent delta times, causing the car to violently explode, clip through hills, or bounce erratically.
2. **Generic Vector Graphics / Default Qt Primitives**: Drawing simple colored circles and rectangles with default anti-aliased vector brushes, resulting in an unpolished, prototype appearance.
3. **Spaghetti Code in a Single QMainWindow Class**: Mixing input events, physics math, drawing loops, and UI states into one 2,000-line monolith.
4. **Poor Game Feel (Lack of "Juice")**: Stiff camera, absence of suspension compression visuals, no particle feedback, static sound, and unresponsive controls.

**Our Competitive Strategy**:
By addressing each of these four failure modes with production-grade engineering principles, our project will establish a clear technical gulf between our team and competitors:
- A custom, mathematically grounded **2D Multi-Body Spring-Damper Physics Engine** using Symplectic Euler integration at 120 Hz with sub-stepping and Continuous Collision Detection (CCD).
- A strictly **rasterized graphics pipeline** utilizing 32-bit ARGB pixel framebuffers, scanline-based polygonal terrain filling, pre-rendered rotated sprite blitting, and raster particle systems.
- A clean **Decoupled Architecture** (State Pattern, Entity-Component data structures, Service Locator, Fixed-Timestep Accumulator) adhering to strict SOLID principles and RAII.
- **Maximized "Juice" & Polish**: Dynamic camera lead and zoom, physical head bobbing / neck-snap kinematics, tire skid marks, exhaust smoke puffs, screen shake on impact, dynamic engine RPM pitch modulation, and a polished retro HUD.

---

## 2. Competitive Feature Matrix

| Feature Dimension | Typical Student Project | Our Planned System | Competitive Impact |
| :--- | :--- | :--- | :--- |
| **Physics Model** | Single point mass or rigid box sliding on slopes | Dual-wheel multi-body chassis with independent Hooke-damper suspension, Coulomb friction, torque transfer, and in-air pitch assist | Unmatched realism, addictive handling, and authentic "Hill Climb" feel |
| **Collision Handling** | Discrete bounding box checks (frequent tunneling through steep hills) | Continuous heightmap ray/circle sweeping with surface normal derivation and penetration impulse resolution | 100% stable, zero tunneling, handles vertical cliffs and deep valleys smoothly |
| **Rendering** | `QPainter::drawRect` and `drawEllipse` vector calls | Direct pixel-buffer rasterization (`QImage` 32-bit ARGB), scanline polygon rasterizer for ground layers, alpha-blended sprite blitter | Adheres strictly to the "pure rasterization" constraint while achieving an ultra-stylish retro aesthetic |
| **Frame Timing** | Uncapped or naive `QTimer(16)` with variable $\Delta t$ physics | Decoupled fixed-timestep accumulator ($dt = \frac{1}{120}\text{ s}$) with render interpolation $\alpha$ | Deterministic, identical physics on any display (60Hz, 144Hz, 240Hz, or under load) |
| **Audio & SFX** | Muted or single repetitive `.wav` click | Real-time audio engine with continuous engine RPM pitch shifting, suspension squeaks, coin chimes, and crash impacts | Immersion and sensory feedback that captivates evaluators instantly |
| **Progression System** | Single static hill track | Multi-tier upgrade system (Engine, Suspension, Tires, 4WD) affecting live physics parameters, plus procedural endless terrain | High replayability, demonstrates deep system integration |

---

## 3. Project Constraints & Boundary Conditions

### 3.1 What We Are Explicitly Building
1. **High-Performance Physics Core**: Custom-built in pure C++ (no external physics engines like Box2D or Chipmunk unless specifically requested; writing our own demonstrates 10x higher academic mastery).
2. **Pure Raster Graphics Engine**: A dedicated framebuffer class (`RasterCanvas`) that manages a pixel grid, handles scanline rasterization of heightmaps, and draws sprites via pixel blitting.
3. **Responsive Control Layer**: Instantaneous, zero-lag throttle and brake inputs with airborne pitch stabilization.
4. **Gameplay Loop & Economy**: Fuel management countdown, coin collection pickups, real-time stunt detection (Backflip, Frontflip, Air Time, Neck Flip), distance meter, and high-score saving.
5. **Interactive Garage / Upgrade System**: In-game currency shop allowing live upgrades to vehicle mechanical attributes.

### 3.2 What We Are Explicitly Deferring (Per Project Brief)
- **User Authentication / Login / Network Databases**: Deferred. The game operates entirely on local profile serialization (JSON / binary config).
- **Vector / SVG Graphics**: Strictly avoided. All visual elements are raster textures, bitmap fonts, or pixel algorithms.
- **Complex 3D Meshes**: The game is strictly 2D planar physics.

---

## 4. Key Performance & Quality Indicators (KPIs)
- **Frame Rate**: Rock-solid 60 FPS minimum on standard lab hardware, with physics tick stable at 120 Hz.
- **Physics Determinism**: Running identical inputs through the simulation results in identical coordinates across runs (enabling replayability).
- **Memory Footprint**: Under 150 MB RAM; 0 memory leaks confirmed via Valgrind / AddressSanitizer / Instruments.
- **Code Cleanliness**: Zero compiler warnings (`-Wall -Wextra -Wpedantic`), fully documented headers with Doxygen comments, and 100% modular compilation units.
