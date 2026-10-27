# Cross-Branch Feature & Code Architecture Audit

This document provides a comprehensive comparative audit of all git branches in the **Hill Climb Racing Qt** repository:
- `main` (Upstream base)
- `origin/shreyak` (Asset & Audio expansion)
- `origin/hrishikesh` (Local Split-Screen Multiplayer)
- `Antariksh` (Current Master Branch: Remote LAN Multiplayer, Minimap Radar, Account System, Developer Mode, Multi-Track Audio, Bengali Mode, Expanded Fleet & Biomes, RL Benchmarks)

---

## 1. Branch Topology & Git Lineage

```mermaid
gitGraph
    commit id: "0eb7f7d: Init Software Rasterizer UI"
    commit id: "7ae6e1e: Authentic Fingersoft Climber"
    branch main
    checkout main
    commit id: "340c8b1: Harmonic Biome Physics"
    commit id: "b1cd58b: Framebuffer Bounds Check"
    checkout main
    branch shreyak
    checkout shreyak
    commit id: "b7aea1b: Vehicle Physics & Sprites"
    commit id: "ac4f52d: Audio Tracks & Terrain Objects"
    branch hrishikesh
    checkout hrishikesh
    commit id: "e961d95: Local 2P Split-Screen"
    branch Antariksh
    checkout Antariksh
    merge hrishikesh
    merge main
    commit id: "Remote LAN Multiplayer (TCP 2+ P)"
    commit id: "Minimap Radar & Live Opponent Auras"
    commit id: "Account Profile & Password Auth"
    commit id: "Developer Mode & Live Tweaker (F1)"
    commit id: "Multi-Track Audio & Pokemon Chiptune (F2)"
    commit id: "Bengali Language Mode - Bangla (B)"
    commit id: "Poki Physics & Speedometer Fixes"
    commit id: "RL Benchmark Suite (PPO, TD3, TRPO)"
```

---

## 2. Comprehensive Comparative Matrix Across Branches

| Subsystem / Feature Area | `main` | `origin/shreyak` | `origin/hrishikesh` | `Antariksh` (Current) |
| :--- | :--- | :--- | :--- | :--- |
| **Multiplayer Architecture** | None (Solo Only) | None (Solo Only) | **Local Split-Screen** (Dual viewports on 1 screen) | **Remote LAN / IP Network** (2+ players, Fullscreen per laptop, Zero split-screen cramping) |
| **Network Protocol** | None | None | None | **TCP Socket Server/Client (Port 7777)**, JSON packet framing, 30-60 Hz telemetry sync |
| **Track Radar / Minimap** | None | None | None | **Contracted Track Radar (0-1500m)** with real-time colored racer pins & leader crown |
| **Vehicle Roster** | 1 (Classic Offroader) | 8 Vehicles (Sprite art assets added) | 8 Vehicles (Local P1/P2 selection) | **10 Vehicles**: Added **Motocross Bike** (high agility) & **Quad Bike** (4WD high grip) |
| **Track & Biome Selection** | 6 Stages (Countryside, Desert, Arctic, Highway, Moon, Mountain) | 6 Stages + Day/Night Cycle | 6 Stages | **9 Stages (3 Pages)**: Added **Neon Highway**, **Super Countryside**, **Changing Seasons** |
| **Developer Console** | None | None | None | **Interactive Live Tweaker (`F1`)**: Live sliders for $g$, $k_s$, $c_d$, $\tau$, $\mu$, and gyro |
| **Audio Subsystem** | Basic procedural beeps | WAV Asset loader + multi-tier RPM loops | Same as Shreyak | **Multi-Track Engine (`F2`)**: Added **Pokemon OG Game Boy 8-bit**, Soothing, Edgy Synthwave, Neck Crunch |
| **Localization** | English only | English only | English only | **Bengali Mode (`B`)**: Humorous localizations (*Govindapur*, *Jhikimiki*, *Phatphati*, *Chalao*, *Thamo*) |
| **User Profile & Accounts** | Hardcoded profile data | Local JSON file | Same as Shreyak | **AccountManager**: Username uniqueness check, 4 smart name suggestions, password auth, XP leveling |
| **Championship Mode** | None | None | None | **Grand Prix Challenge**: 9-round multi-track championship with points leaderboard |
| **Evaluator Sandbox** | None | None | None | **VIP Sandbox Mode**: Unlocked garage, infinite fuel, slow-mo (`Z`), hot-swap rig (`V`), stage hops (`1-9`) |
| **AI Benchmarking** | None | None | None | **Standalone RL Benchmark Suite**: VPG, PPO, DDPG, TD3, A3C, TRPO agents with leaderboard report |

---

## 3. Deep-Dive Subsystem Architectural Evolution

### A. Multiplayer: From Local Split-Screen to Remote LAN Synchronization
- **`origin/hrishikesh`**:
  - Implemented local split-screen:
    `m_camera` rendered top half ($960 \times 266$ px);
    `m_camera2` rendered bottom half ($960 \times 266$ px).
  - Both players shared the same physical keyboard (P1: WASD / P2: Arrow Keys).
  - *Limitation*: Vertically cramped viewports made high-flying jumps disorienting, and it could only support 2 players on a single machine.
- **`Antariksh`**:
  - Replaced split-screen with an asynchronous **Client-Server TCP Networking Engine** (`GameServer.h/cpp`, `GameClient.h/cpp`):
    - Each player enjoys a **fullscreen $960 \times 540$ view** centered on their own rig.
    - Remote competitors are rendered directly in the camera view using `SpriteRenderer::renderRemoteVehicle` with custom underglow auras and floating distance differential tags (`+14m`, `-8m`).
    - The server automatically broadcasts room status, syncs stage selection, and maintains a room lobby supporting 2+ players across different laptops over LAN or Wi-Fi.

### B. User Interface & HUD Layout Polish
- **`main`**:
  - Minimal HUD with raw numbers and simple text displays.
- **`origin/shreyak`**:
  - Added analog gauges, but needles remained static or lacked real-time state hooks.
  - Added title marquee, but sized at $460 \times 96$ px with font scale 4, causing screen overcrowding.
- **`Antariksh`**:
  - **Speedometer & Tachometer Fix**: Wired 60 Hz vehicle angular velocity, transmission ratio, and engine RPM to `AnalogGauge::setValue` and `AnalogGauge::update(dt)`. Needle sweeps smoothly with exponential damping and digital LCD numbers reflect exact speed in km/h.
  - **Poki-Style UI Proportions**: Redesigned title marquee to a sleek $340 \times 44$ px arcade badge with specular gleam, ensuring zero overlap with top pills and buttons.
  - **Coin Collection Fix**: Replaced point-distance check with a comprehensive 4-point collision solver (Chassis + Rear Wheel + Front Wheel + Vehicle Bounding Box) to guarantee 100% reliable pickup of gold/silver coins and fuel jerrycans.

### C. Physics & Dynamics Improvements
- **`main`**:
  - Fundamental spring-damper Euler integration on chassis and two wheels.
- **`origin/shreyak`**:
  - Added reverse braking logic and tire slip speed telemetry for particle systems.
- **`Antariksh`**:
  - **Live Runtime Tuning**: Exposed gravity, spring rate ($k_s$), damping ($c_d$), peak torque, grip ($\mu$), and air gyro multipliers to the interactive Developer Console (`F1`).
  - **Dynamic Weight Transfer**: Implemented authentic Poki/Fingersoft chassis pitch torques: acceleration squat ($+360\,\text{N}\cdot\text{m}$) and braking dive ($-420\,\text{N}\cdot\text{m}$).
  - **Neck Snap Detection**: Real-time driver helmet and roof terrain collision triggers fatal neck crunch sound effect and ragdoll wreck tumble.
  - **Multiplayer Trophy Rules**: Adjustable finish line distance ($500\text{m}$ to $\infty$) with optional slipstream catch-up rubberbanding.
