# Document 03: Rasterization & Render Pipeline Specification
**Software Pixel Buffers, Scanline Terrain Rasterization, and Sprite Blitting**

---

## 1. Core Architecture & Philosophy
Per the project mandate, all visual rendering is **purely raster-based (pixel manipulation and blitting)**. No vector curves, SVG paths, or anti-aliased geometry drawing functions (`QPainterPath`, `drawPolygon`, etc.) are used for in-game entities.

Instead, the game renders into a dedicated offscreen pixel framebuffer (`QImage` formatted as `Format_ARGB32_Premultiplied`), which is then blitted to the active `QWidget` viewport in a single pass. This ensures:
- Direct control over every single pixel.
- Maximum cache locality and SIMD suitability.
- Authentic retro/arcade visual aesthetic.
- Zero dependency on platform-specific GPU vector drivers.

---

## 2. Virtual Framebuffer & Viewport Management

```
+-------------------------------------------------------+
| Window Viewport (e.g. 1920x1080, HiDPI)                |
|  +-------------------------------------------------+  |
|  | Virtual Framebuffer (Native: 960 x 540 ARGB32)  |  |
|  |  [Layer 0: Raster Sky & Parallax]               |  |
|  |  [Layer 1: Column-Scanline Terrain Rasterizer]  |  |
|  |  [Layer 2: Tire Tracks & Skid Marks Buffer]     |  |
|  |  [Layer 3: Rotated Vehicle & Wheel Sprites]     |  |
|  |  [Layer 4: Raster Particle System]             |  |
|  |  [Layer 5: Pixel HUD & Gauges]                  |  |
|  +-------------------------------------------------+  |
|                                                       |
+-------------------------------------------------------+
```

### 2.1 Resolution Standards
- **Internal Virtual Resolution**: $960 \times 540$ (Quarter-HD 16:9) or $1280 \times 720$ (HD). 
- **Internal Buffer Representation**:
  ```cpp
  class Framebuffer {
  public:
      Framebuffer(int width, int height);
      uint32_t* data(); // Direct 32-bit pixel array access (0xAARRGGBB)
      int stride() const; // Scanline byte offset
      void clear(uint32_t color);
      void setPixel(int x, int y, uint32_t color);
      uint32_t getPixel(int x, int y) const;
  private:
      QImage m_image;
  };
  ```

---

## 3. High-Speed Column-Scanline Terrain Rasterization
The iconic undulating hills are rendered using a **Vertical Scanline Column Rasterizer**. Because terrain in Hill Climb Racing is a single-valued height field $y = H(x)$, every vertical pixel column $x \in [0, W-1]$ of the screen maps to exactly one surface height $Y_{surface}$.

```
For each screen column x (0 to Width-1):
  WorldX = Camera.X + (x - ScreenCenterX) / Zoom;
  WorldY = Heightmap.sample(WorldX);
  ScreenY = ScreenCenterY - (WorldY - Camera.Y) * Zoom;

  // Pixels above ScreenY -> Sky (already drawn or transparent)
  // ScreenY to ScreenY + GrassDepth -> Grass Pixel Texture
  // ScreenY + GrassDepth to ScreenY + DirtDepth -> Dirt Pixel Texture
  // ScreenY + DirtDepth to ScreenBottom -> Deep Bedrock / Rock Texture
```

### 3.1 Pixel-Level Scanline Algorithm (C++ Implementation)
```cpp
void RasterRenderer::rasterizeTerrain(const Camera& cam, const TerrainHeightmap& terrain) {
    const int W = m_buffer.width();
    const int H = m_buffer.height();
    const float zoom = cam.zoom();
    const float invZoom = 1.0f / zoom;

    for (int x = 0; x < W; ++x) {
        float worldX = cam.x() + (x - W * 0.5f) * invZoom;
        float worldY = terrain.getHeight(worldX);
        int surfaceScreenY = static_cast<int>(H * 0.5f - (worldY - cam.y()) * zoom);

        // Clamp surface to viewport
        int startY = std::max(0, surfaceScreenY);

        for (int y = startY; y < H; ++y) {
            int depthFromSurface = y - surfaceScreenY;
            uint32_t pixelColor;

            if (depthFromSurface < 6) {
                // Vibrant grass top layer with noise variation
                pixelColor = s_grassPalette[depthFromSurface + (x & 3)];
            } else if (depthFromSurface < 64) {
                // Textured soil layer with procedural dithering
                int texU = (static_cast<int>(worldX * 4) & 31);
                int texV = (depthFromSurface & 31);
                pixelColor = s_dirtTexture[texV * 32 + texU];
            } else {
                // Deep subterranean rock / strata
                int texU = (x & 63);
                int texV = (y & 63);
                pixelColor = s_bedrockTexture[texV * 64 + texU];
            }

            m_buffer.setPixelFast(x, y, pixelColor);
        }
    }
}
```
**Why this is a competitive powerhouse**:
- Operates at **sub-millisecond execution times** per frame ($O(W \cdot H)$ without polygon sorting or tessellation).
- Pure rasterization: every pixel of dirt, grass blade, and bedrock stratum is computed mathematically and written directly to memory.

---

## 4. Sprite Blitting & Raster Rotation Kernel
Entities (Vehicle Chassis, Driver Body, Driver Head, Wheels, Coins, Fuel Cans) are drawn as raster sprites.

### 4.1 Inverse-Mapping Rotation with Alpha Blending
Rotating a sprite without vector engines requires an inverse rotation matrix to avoid pixel gaps (holes):

$$\begin{bmatrix} u - u_0 \\ v - v_0 \end{bmatrix} = \begin{bmatrix} \cos\theta & \sin\theta \\ -\sin\theta & \cos\theta \end{bmatrix} \begin{bmatrix} x - x_c \\ y - y_c \end{bmatrix}$$

```cpp
void RasterRenderer::blitSpriteRotated(const Sprite& sprite, float screenX, float screenY, 
                                      float angleRad, float scale) {
    const float cosA = std::cos(-angleRad) / scale;
    const float sinA = std::sin(-angleRad) / scale;
    
    const int u0 = sprite.originX();
    const int v0 = sprite.originY();
    
    // Compute bounding box on screen
    int minX = std::max(0, static_cast<int>(screenX - sprite.radius() * scale));
    int maxX = std::min(m_buffer.width() - 1, static_cast<int>(screenX + sprite.radius() * scale));
    int minY = std::max(0, static_cast<int>(screenY - sprite.radius() * scale));
    int maxY = std::min(m_buffer.height() - 1, static_cast<int>(screenY + sprite.radius() * scale));

    for (int y = minY; y <= maxY; ++y) {
        float dy = y - screenY;
        for (int x = minX; x <= maxX; ++x) {
            float dx = x - screenX;

            // Inverse transform to source sprite coordinates
            int u = static_cast<int>(dx * cosA - dy * sinA + u0);
            int v = static_cast<int>(dx * sinA + dy * cosA + v0);

            if (u >= 0 && u < sprite.width() && v >= 0 && v < sprite.height()) {
                uint32_t srcColor = sprite.pixel(u, v);
                uint8_t alpha = (srcColor >> 24) & 0xFF;
                
                if (alpha == 255) {
                    m_buffer.setPixelFast(x, y, srcColor);
                } else if (alpha > 0) {
                    m_buffer.blendPixelFast(x, y, srcColor);
                }
            }
        }
    }
}
```

---

## 5. Multi-Layer Parallax Background
To convey immense scale and distance without vector art:
1. **Layer 0: Procedural Sky Gradient** (Linear interpolation between zenith blue `#28558A` and horizon tint `#96C8E6`).
2. **Layer 1: Distant Mountain Silhouette** (Scroll factor $s_x = 0.08$, low contrast hazy blue).
3. **Layer 2: Rolling Mid-Hills** (Scroll factor $s_x = 0.25$, pine silhouettes and ridgelines).
4. **Layer 3: Foreground Playfield** (Scroll factor $s_x = 1.00$, the active physical terrain).

---

## 6. Raster Particle Effects (Juice & Feedback)

```
        / (Smoke puff: expanding dithered pixel circle)
       O  .
      o .  .
    +-------+
    | CAR   |  ===> Forward Motion
    +---+---+
       (O) ======> (Tire rooster tail: ballistic pixel sparks)
      / / | \
```

1. **Exhaust Smoke**:
   - Emitted from exhaust pipe position every $K$ ticks.
   - Expands in radius $r(t) = r_0 + \alpha t$, drifts backward and upward with subtle wind velocity, and fades through a 4-step dither mask.
2. **Tire Dirt / Mud Rooster Tails**:
   - Spawned at wheel contact patch proportional to tire slip velocity $v_{slip}$.
   - Individual pixel clusters subject to ballistic gravity $\vec{g}$ and initial launch angle tangent to the wheel rotation.
3. **Coin Pickup Sparkles**:
   - Radial burst of 12 bright yellow/white pixel sparks upon collision with a coin.
4. **Traumatic Screen Shake**:
   - When chassis hits terrain with impact force $> F_{shake\_thresh}$, camera coordinates receive a decaying pseudo-random oscillation:
     $$\Delta X_{cam} = \text{ShakeMagnitude} \cdot e^{-\lambda t} \cdot \cos(45 t)$$
