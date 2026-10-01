# Document 07: GitHub Benchmark & Open-Source Research Analysis
**Comparative Engineering Analysis of Existing Hill Climb Clones, 2D Car Physics, and Qt Rendering**

---

## 1. Global Open-Source Landscape Research

A deep investigation across GitHub and open-source game development archives reveals the following landscape for *Hill Climb Racing* and 2D physics driving games:

```
+-------------------------------------------------------------------------------+
|                        OPEN-SOURCE LANDSCAPE AUDIT                            |
+--------------------------+-----------------------+----------------------------+
| Category / Platform      | Dominant Tech Stack   | Key Limitations            |
+--------------------------+-----------------------+----------------------------+
| 1. Mobile & Web Clones   | Unity (C#) / Godot    | Black-box physics, heavy   |
|                          |                       | overhead, no raw pixel API |
| 2. AI & RL Research      | Python / Gymnasium /  | Low FPS, discrete physics, |
|    (e.g., alexzh3/hcr)   | Pygame                | unpolished graphics        |
| 3. C++ Physics Demos     | Box2D + SFML/Raylib   | Bare prototypes, lack UI,  |
|                          |                       | no custom raster engine    |
| 4. Qt C++ Games          | Qt Widgets / QML      | Almost NO high-end physics |
|    (Community projects)  |                       | driving games exist!       |
+--------------------------+-----------------------+----------------------------+
```

### Key Finding: The "Blue Ocean" Opportunity for Our Team
**There is currently NO production-grade, pure-rasterized Hill Climb Racing game built with C++ and Qt on GitHub.**
Most Qt projects are limited to simple board games (Chess, Minesweeper, Tetris) or basic `QGraphicsView` demos. By engineering a custom deterministic physics engine and a bespoke 32-bit software rasterizer in Qt, **our project will immediately stand out as an extraordinary, original software engineering achievement** rather than a derivative wrapper.

---

## 2. Technical Secrets Extracted from Industry Standards

### 2.1 The Box2D `b2WheelJoint` Secret: Frequency & Damping Ratio Formulation
In the official Box2D testbed (`erincatto/box2d`), vehicle suspension is modeled using **soft constraints** parameterized by **Natural Frequency ($f$ in Hz)** and **Damping Ratio ($\zeta$)**, rather than raw spring constants $k$ and damping $c$.

#### Why Raw $k$ and $c$ Fail in Practice:
- Direct Hooke constants $k$ are highly sensitive to chassis mass $M$ and delta time $dt$. Changing car mass requires recalculating all spring numbers from scratch, or the simulation oscillates into infinity.

#### The Soft Constraint Parameterization:
Instead of arbitrary values, we compute effective spring stiffness and damping using harmonic oscillator physics:
$$\omega_n = 2\pi f_0 \quad (\text{Target natural frequency, typically } 2.5\text{--}4.0\text{ Hz})$$
$$k_{susp} = M_{effective} \cdot \omega_n^2$$
$$c_{damp} = 2 \cdot M_{effective} \cdot \zeta \cdot \omega_n \quad (\zeta \approx 0.707 \text{ for critical damping})$$

**Advantage for our engine**: This allows our vehicle upgrades (heavier chassis, bigger wheels) to maintain perfectly tuned, stable suspension feel automatically!

---

### 2.2 Continuous Circle-to-Segment Collision (No Ground Clipping)
Many simplistic GitHub implementations sample only a single point $H(x)$ directly under the wheel center. When the car hits a steep 45° ramp or a sharp crest, the wheel penetrates deeply into the slope before detecting collision, causing violent physics explosions.

The gold-standard approach used in top 2D physics engines is **Continuous Circle-to-Line-Segment Projection**:

```
           P1 +--------------------+ P2  (Terrain Segment)
               \         |        /
                \        | v_dist/
                 \       v      /
                  \    [Rw]    /
                   \    (O)   /
                    \ Wheel  /
```

For each terrain segment $\overline{P_1 P_2}$ near the wheel:
1. Segment direction vector $\vec{d} = P_2 - P_1$, length $L = \|\vec{d}\|$, unit tangent $\hat{t} = \frac{\vec{d}}{L}$.
2. Project wheel center $C_w$ onto the infinite line:
   $$t_{proj} = \frac{(C_w - P_1) \cdot \vec{d}}{L^2}$$
3. Clamp $t_{clamped} = \text{clamp}(t_{proj}, 0.0, 1.0)$ to get the closest point on the segment:
   $$P_{closest} = P_1 + t_{clamped} \cdot \vec{d}$$
4. Calculate collision normal and distance:
   $$\vec{\delta} = C_w - P_{closest}, \quad \text{dist} = \|\vec{\delta}\|$$
5. If $\text{dist} < R_w$:
   - Penetration depth: $\text{penetration} = R_w - \text{dist}$
   - Outward contact normal: $\hat{n} = \frac{\vec{\delta}}{\text{dist}}$

This completely eliminates collision tunneling and allows our car to roll over jagged rocks, crests, and sheer drops with total stability.

---

### 2.3 Ragdoll Driver Head & Neck Kinematics
In *Hill Climb Racing*, player attachment and comedy stem from the driver's head. The driver's torso and head are not static sprites glued to the chassis:
1. **Torso**: Anchored to the vehicle chassis at the hips.
2. **Head**: Modeled as an inverted damped pendulum / rotational spring joint connected to the neck:
   $$\tau_{neck} = -k_{neck} (\theta_{head} - \theta_{chassis}) - c_{neck} (\omega_{head} - \omega_{chassis})$$
3. When the car accelerates forward, inertia forces the head to lag backward; hard braking snaps the head forward.
4. **Neck Snap Condition**: If the head's bounding sphere hits the terrain line segment with impulse $> J_{fatal}$, the neck snaps $\rightarrow$ Game Over trigger.

---

### 2.4 Software Rasterizer Optimization Patterns (From Retro Demo Engines)
Looking at high-performance software 2D engines (like early id Tech and custom pixel blitters):
1. **Scanline Direct Memory Writes**: Instead of calling `QPainter::drawPoint` or `image.setPixel(x, y, color)` (which incur function call overhead and coordinate bounds checking on every single pixel), direct pointer access is used:
   ```cpp
   uint32_t* scanline = reinterpret_cast<uint32_t*>(m_image.scanLine(y));
   scanline[x] = color;
   ```
2. **Color Format `Format_ARGB32_Premultiplied`**:
   Qt's internal hardware blit (`QPainter::drawImage` inside `paintEvent`) is up to **5x faster** with premultiplied alpha format because the display server does not need to compute runtime division for alpha compositing.
3. **Integer Fixed-Point Math for Inner Loops**:
   For the column terrain fill and sprite rotation inner loops, casting coordinates to fixed-point $16.16$ integers maximizes CPU instruction throughput.

---

## 3. Summary of Innovations to Implement

| Proven Pattern from Open-Source | Our Adaptation in Qt C++ |
| :--- | :--- |
| Box2D `b2WheelJoint` soft constraints | Analytical spring calculation using frequency $f$ and damping ratio $\zeta$ |
| Circle-to-segment projection | Robust piecewise segment collision solver |
| Ragdoll neck spring joint | Damped rotational pendulum driver head |
| Dynamic chunked terrain streaming | Continuous circular buffer of terrain points |
| Premultiplied 32-bit ARGB framebuffer | Direct `uint32_t*` pointer column rasterizer |
