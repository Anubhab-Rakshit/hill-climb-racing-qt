#include "EnvironmentRenderer.h"
#include "UITheme.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Graphics {

// Deterministic 32-bit integer hash for coherent procedural generation
static inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

EnvironmentRenderer::EnvironmentRenderer() {
    initStars();
}

void EnvironmentRenderer::initStars() {
    m_stars.clear();
    m_stars.reserve(95);

    // Deterministic distribution across sky dome
    uint32_t seed = 0x9E3779B9;
    auto rnd = [&]() -> float {
        seed = hash32(seed);
        return static_cast<float>(seed & 0xFFFF) / 65535.0f;
    };

    for (int i = 0; i < 95; ++i) {
        StarData s;
        s.xNorm = rnd();
        s.yNorm = rnd() * 0.62f; // Upper 62% of viewport
        s.size = (rnd() > 0.85f) ? 2 : 1;

        float c = rnd();
        if (c < 0.60f) {
            s.color = 0xFFFFFFFF; // Pure white star
        } else if (c < 0.80f) {
            s.color = 0xFF80D8FF; // Bright blue celestial star
        } else if (c < 0.93f) {
            s.color = 0xFFFFE082; // Warm golden star
        } else {
            s.color = 0xFFFF80AB; // Soft pink star
        }

        s.twinkleSpeed = 2.0f + rnd() * 4.5f;
        s.twinklePhase = rnd() * 6.28f;
        m_stars.push_back(s);
    }
}

void EnvironmentRenderer::drawSun(Framebuffer& fb, int cx, int cy, float opacity, uint32_t sunColor, uint32_t glowColor) {
    if (opacity <= 0.01f || cx < -120 || cx > fb.width() + 120 || cy < -120 || cy > fb.height() + 120) return;

    uint32_t aCorona = static_cast<uint32_t>(opacity * 42.0f);
    uint32_t aMid    = static_cast<uint32_t>(opacity * 85.0f);
    uint32_t aInner  = static_cast<uint32_t>(opacity * 255.0f);

    // 1. Soft Outer Solar Corona Halo
    fb.fillCircle(cx, cy, 46, (aCorona << 24) | (glowColor & 0x00FFFFFF));

    // 2. Mid Warm Radiation Ring
    fb.fillCircle(cx, cy, 28, (aMid << 24) | (glowColor & 0x00FFFFFF));

    // 3. Crisp Solar Disc Core
    fb.fillCircle(cx, cy, 16, (aInner << 24) | (sunColor & 0x00FFFFFF));

    // 4. White Brilliant Center
    fb.fillCircle(cx, cy, 9, (aInner << 24) | 0x00FFFFFF);

    // 5. Delicate Cardinal Pixel Flares
    int flareLen = static_cast<int>(12 * opacity);
    for (int d = 16; d < 16 + flareLen; ++d) {
        uint32_t fCol = (static_cast<uint32_t>(opacity * 160.0f) << 24) | 0x00FFFFFF;
        fb.blendPixelFast(cx + d, cy, fCol);
        fb.blendPixelFast(cx - d, cy, fCol);
        fb.blendPixelFast(cx, cy + d, fCol);
        fb.blendPixelFast(cx, cy - d, fCol);
    }
}

void EnvironmentRenderer::drawMoon(Framebuffer& fb, int cx, int cy, float opacity, uint32_t moonColor) {
    if (opacity <= 0.01f || cx < -100 || cx > fb.width() + 100 || cy < -100 || cy > fb.height() + 100) return;

    uint32_t aHalo  = static_cast<uint32_t>(opacity * 38.0f);
    uint32_t aBody  = static_cast<uint32_t>(opacity * 255.0f);

    // 1. Soft Blue Moonlight Corona
    fb.fillCircle(cx, cy, 36, (aHalo << 24) | 0x0090CAF9);
    fb.fillCircle(cx, cy, 22, (aHalo << 24) | 0x00B0BEC5);

    // 2. Main Moon Disc
    fb.fillCircle(cx, cy, 15, (aBody << 24) | (moonColor & 0x00FFFFFF));

    // 3. Shaded Crater Formations
    uint32_t craterCol = (aBody << 24) | 0x0090A4AE;
    fb.fillCircle(cx - 4, cy - 3, 3, craterCol);
    fb.fillCircle(cx + 4, cy + 2, 2, craterCol);
    fb.fillCircle(cx - 2, cy + 5, 2, craterCol);

    // 4. Crescent Shadow Rim (Gibbous phase depth)
    fb.drawCircle(cx, cy, 15, (aBody << 24) | 0x00CFD8DC);
}

void EnvironmentRenderer::drawEarth(Framebuffer& fb, int cx, int cy, float camX) {
    int W = fb.width();
    int H = fb.height();
    int ex = static_cast<int>(cx - camX * 0.008f);
    int ey = cy;

    if (ex < -60 || ex > W + 60 || ey < -60 || ey > H + 60) return;

    // 1. Glowing Cyan Atmospheric Corona
    fb.fillCircle(ex, ey, 32, 0x2800E5FF);
    fb.fillCircle(ex, ey, 26, 0x441976D2);

    // 2. Vibrant Deep Blue Ocean Disc
    fb.fillCircle(ex, ey, 22, 0xFF1E88E5);

    // 3. Detailed Green Continents
    fb.fillCircle(ex - 5, ey - 3, 9, 0xFF43A047);  // Eurasia / Africa
    fb.fillCircle(ex + 8, ey + 4, 7, 0xFF388E3C);  // Americas
    fb.fillCircle(ex - 8, ey + 8, 4, 0xFF66BB6A);  // Australia

    // 4. Polar Ice Cap
    fb.fillCircle(ex + 2, ey - 15, 6, 0xFFFFFFFF);

    // 5. Swirling White Atmospheric Cloud Bands
    fb.drawLine(ex - 14, ey - 6, ex - 2, ey - 8, 0x99FFFFFF);
    fb.drawLine(ex - 12, ey - 5, ex, ey - 7, 0x99FFFFFF);
    fb.drawLine(ex + 2, ey + 6, ex + 14, ey + 4, 0x99FFFFFF);
    fb.drawLine(ex - 4, ey + 10, ex + 8, ey + 9, 0x88FFFFFF);

    // 6. Planetary Spherical Shading (Crescent Day/Night Terminator)
    fb.drawCircle(ex, ey, 22, 0x44FFFFFF);
}

void EnvironmentRenderer::drawStars(Framebuffer& fb, int W, int H, float camX, float opacity, float gameTime) {
    if (opacity <= 0.01f) return;

    for (const auto& s : m_stars) {
        float sx = std::fmod(s.xNorm * W - camX * 0.006f + W * 3.0f, static_cast<float>(W));
        float sy = s.yNorm * (H * 0.65f);

        int ix = static_cast<int>(sx);
        int iy = static_cast<int>(sy);

        float tw = 0.55f + 0.45f * std::sin(gameTime * s.twinkleSpeed + s.twinklePhase);
        uint32_t alpha = static_cast<uint32_t>(opacity * tw * 255.0f);
        if (alpha == 0) continue;

        uint32_t starCol = (alpha << 24) | (s.color & 0x00FFFFFF);
        if (s.size == 1) {
            fb.blendPixelFast(ix, iy, starCol);
        } else {
            fb.blendPixelFast(ix, iy, starCol);
            fb.blendPixelFast(ix + 1, iy, starCol);
            fb.blendPixelFast(ix, iy + 1, starCol);
            fb.blendPixelFast(ix + 1, iy + 1, starCol);
        }
    }
}

void EnvironmentRenderer::drawAurora(Framebuffer& fb, int W, int H, float camX, float opacity, float gameTime) {
    if (opacity <= 0.01f) return;

    int numSteps = W / 4;
    for (int i = 0; i < numSteps; ++i) {
        int x = i * 4;
        float fx = static_cast<float>(x) + camX * 0.02f;

        // Waving northern lights ribbon equations
        float wave1 = std::sin(fx * 0.010f + gameTime * 0.8f) * 22.0f;
        float wave2 = std::sin(fx * 0.022f - gameTime * 0.5f) * 12.0f;
        int yBase = static_cast<int>(55.0f + wave1 + wave2);

        int ribbonH = 35 + static_cast<int>(std::sin(fx * 0.015f + gameTime) * 12.0f);
        uint32_t aVal = static_cast<uint32_t>(opacity * 48.0f);

        for (int y = yBase; y < yBase + ribbonH && y < H * 0.6f; ++y) {
            float frac = static_cast<float>(y - yBase) / static_cast<float>(ribbonH);
            // Emerald Green -> Electric Cyan -> Royal Violet
            uint32_t col = (frac < 0.5f) ? DayNightCycle::blendColor(0x002ECC71, 0x0000E5FF, frac * 2.0f) :
                                          DayNightCycle::blendColor(0x0000E5FF, 0x009C27B0, (frac - 0.5f) * 2.0f);
            uint32_t pixelCol = (aVal << 24) | (col & 0x00FFFFFF);
            for (int dx = 0; dx < 4 && x + dx < W; ++dx) {
                fb.blendPixelFast(x + dx, y, pixelCol);
            }
        }
    }
}

void EnvironmentRenderer::drawClouds(Framebuffer& fb, int W, int /*H*/, float camX, const LightingState& lighting) {
    auto drawSingleCloud = [&](int cx, int cy, int size) {
        if (cx < -size * 2 || cx > W + size * 2) return;

        // Shaded underside
        uint32_t shadowCol = DayNightCycle::applyAmbient(0xFFB0BEC5, lighting.horizonGlowColor, lighting.ambientBrightness * 0.85f);
        // Fluffy top highlight
        uint32_t bodyCol   = DayNightCycle::applyAmbient(0xFFFFFFFF, lighting.ambientColor, lighting.ambientBrightness);

        fb.fillCircle(cx, cy + 4, size + 2, shadowCol);
        fb.fillCircle(cx + size, cy + 6, (size * 4) / 5 + 2, shadowCol);
        fb.fillCircle(cx - size, cy + 6, (size * 3) / 5 + 2, shadowCol);
        fb.fillRect(cx - size, cy + 3, size * 2, size / 2 + 4, shadowCol);

        fb.fillCircle(cx, cy, size, bodyCol);
        fb.fillCircle(cx + size, cy + 2, (size * 4) / 5, bodyCol);
        fb.fillCircle(cx - size, cy + 3, (size * 3) / 5, bodyCol);
        fb.fillRect(cx - size, cy, size * 2, size / 2, bodyCol);
    };

    // Parallax clouds in multiple altitude layers
    int c1X = static_cast<int>(W + 200 - std::fmod(camX * 0.035f + 100, W + 450));
    int c2X = static_cast<int>(W + 400 - std::fmod(camX * 0.022f + 320, W + 550));
    int c3X = static_cast<int>(W + 300 - std::fmod(camX * 0.045f + 700, W + 480));

    drawSingleCloud(c1X, 55, 24);
    drawSingleCloud(c2X, 105, 30);
    drawSingleCloud(c3X, 80, 20);
}

void EnvironmentRenderer::drawBirds(Framebuffer& fb, int W, int, float camX, const std::string& biomeId, float gameTime, const LightingState& lighting) {
    if (biomeId == "moon") return; // No wildlife in lunar vacuum

    // 2 Flying bird flocks
    for (int flock = 0; flock < 2; ++flock) {
        float fSpeed = (flock == 0) ? 28.0f : 20.0f;
        float fOffX = (flock == 0) ? 150.0f : 550.0f;
        float fBaseY = (flock == 0) ? 80.0f : 135.0f;

        float fx = std::fmod(fOffX + gameTime * fSpeed - camX * 0.08f + W * 3.0f, static_cast<float>(W + 200)) - 100.0f;
        int birdCount = (flock == 0) ? 4 : 3;

        uint32_t birdColor = (biomeId == "arctic") ? 0xFFECEFF1 :
                             (biomeId == "desert") ? 0xFF4E342E :
                             (biomeId == "volcano") ? 0xFF212121 : 0xFF263238;
        birdColor = DayNightCycle::applyAmbient(birdColor, lighting.ambientColor, lighting.ambientBrightness);

        for (int b = 0; b < birdCount; ++b) {
            int bx = static_cast<int>(fx - b * 14);
            int by = static_cast<int>(fBaseY + std::abs(b - birdCount / 2) * 8 + std::sin(gameTime * 2.0f + b) * 4.0f);

            if (bx < -10 || bx >= W + 10) continue;

            // 3-Frame wing flap cycle
            int wingCycle = static_cast<int>(std::fmod(gameTime * 4.5f + b * 0.4f, 3.0f));
            if (wingCycle == 0) {
                // Wings Up: \ /
                fb.setPixelFast(bx - 2, by - 2, birdColor);
                fb.setPixelFast(bx - 1, by - 1, birdColor);
                fb.setPixelFast(bx, by, birdColor);
                fb.setPixelFast(bx + 1, by - 1, birdColor);
                fb.setPixelFast(bx + 2, by - 2, birdColor);
            } else if (wingCycle == 1) {
                // Wings Level: - -
                fb.setPixelFast(bx - 2, by, birdColor);
                fb.setPixelFast(bx - 1, by, birdColor);
                fb.setPixelFast(bx, by, birdColor);
                fb.setPixelFast(bx + 1, by, birdColor);
                fb.setPixelFast(bx + 2, by, birdColor);
            } else {
                // Wings Down
                fb.setPixelFast(bx - 2, by + 1, birdColor);
                fb.setPixelFast(bx - 1, by, birdColor);
                fb.setPixelFast(bx, by, birdColor);
                fb.setPixelFast(bx + 1, by, birdColor);
                fb.setPixelFast(bx + 2, by + 1, birdColor);
            }
        }
    }
}

void EnvironmentRenderer::renderSkyAndParallax(Framebuffer& fb, const Camera& cam, const std::string& biomeId,
                                              const DayNightCycle& cycle, const int* surfaceY, float gameTime) {
    int W = fb.width();
    int H = fb.height();
    if (W <= 0 || H <= 0) return;

    float camX = cam.x();
    int stride = fb.stride();
    LightingState lighting = cycle.getLighting(biomeId, W, H, camX);

    // 1. Precompute Vertical Sky Gradient Scanline LUT
    static std::vector<uint32_t> s_skyLut;
    static uint32_t s_lastTop = 0;
    static uint32_t s_lastBot = 0;
    static int s_lastH = 0;

    if (static_cast<int>(s_skyLut.size()) != H || s_lastTop != lighting.skyTopColor || s_lastBot != lighting.skyBottomColor || s_lastH != H) {
        s_skyLut.resize(H);
        s_lastTop = lighting.skyTopColor;
        s_lastBot = lighting.skyBottomColor;
        s_lastH = H;

        float invH = (H > 1) ? (1.0f / static_cast<float>(H - 1)) : 1.0f;
        for (int y = 0; y < H; ++y) {
            float t = y * invH;
            s_skyLut[y] = DayNightCycle::blendColor(lighting.skyTopColor, lighting.skyBottomColor, t);
        }
    }

    // 2. Parallax Mountain Color Palettes modulated by Day-Night Ambient Lighting
    const auto& spec = Physics::BiomeRegistry::getBiome(biomeId);
    uint32_t baseMtnCol = spec.mountainColor;

    // Atmospheric Perspective: Far mountains blend heavily toward horizon sky
    uint32_t farMtnCol = DayNightCycle::applyAmbient(
        UI::Theme::blend(baseMtnCol, lighting.skyBottomColor, 130),
        lighting.ambientColor, lighting.ambientBrightness
    );
    uint32_t midMtnCol = DayNightCycle::applyAmbient(baseMtnCol, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t nearMtnCol = DayNightCycle::applyAmbient(
        UI::Theme::darken(baseMtnCol, 22), lighting.ambientColor, lighting.ambientBrightness
    );

    uint32_t snowCapCol = DayNightCycle::applyAmbient(0xFFECEFF1, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t ridgeHighlight = UI::Theme::lighten(midMtnCol, 20);

    // 3. Unified Single-Scanline Zero-Overdraw Column Pass
    // Computes heights for: Far Mountains, Mid-Ground Ridges, and Near Foothills
    for (int x = 0; x < W; ++x) {
        // A. Far Mountain Profile
        float pxFar = (camX * 0.025f) + (static_cast<float>(x) * 0.0035f);
        float hFar = std::sin(pxFar * 2.8f) * 75.0f + std::sin(pxFar * 6.2f) * 25.0f + (H * 0.44f);
        if (biomeId == "desert") {
            // Flat-topped mesas
            hFar = std::max(H * 0.38f, hFar);
        }
        int syFar = std::clamp(static_cast<int>(hFar), 0, H);

        // B. Mid-Ground Mountain Profile
        float pxMid = (camX * 0.065f) + (static_cast<float>(x) * 0.0055f);
        float hMid = std::sin(pxMid * 3.2f) * 55.0f + std::sin(pxMid * 7.8f) * 18.0f + (H * 0.52f);
        if (biomeId == "arctic") {
            // Jagged sharp glacial peaks
            float tri = std::abs(std::fmod(pxMid * 2.5f, 2.0f) - 1.0f) * 65.0f;
            hMid = H * 0.42f + tri;
        }
        int syMid = std::clamp(static_cast<int>(hMid), 0, H);

        // C. Near Foothills Profile
        float pxNear = (camX * 0.160f) + (static_cast<float>(x) * 0.0085f);
        float hNear = std::sin(pxNear * 2.5f) * 35.0f + std::sin(pxNear * 5.5f) * 12.0f + (H * 0.58f);
        int syNear = std::clamp(static_cast<int>(hNear), 0, H);

        int terrainLimit = std::clamp(surfaceY ? surfaceY[x] : H, 0, H);
        int skyLimit = std::min({syFar, syMid, syNear, terrainLimit});

        // 1. Fill Visible Sky Column
        uint32_t* p = fb.scanLine(0) + x;
        for (int y = 0; y < skyLimit; ++y) {
            *p = s_skyLut[y];
            p += stride;
        }

        // 2. Far Mountain Fill
        int farLimit = std::min({syMid, syNear, terrainLimit});
        if (syFar < farLimit) {
            p = fb.scanLine(syFar) + x;
            for (int y = syFar; y < farLimit; ++y) {
                *p = farMtnCol;
                p += stride;
            }
            // Snowcap
            if ((biomeId == "mountain" || biomeId == "arctic") && syFar < H * 0.38f) {
                int snowH = std::min(4, farLimit - syFar);
                p = fb.scanLine(syFar) + x;
                for (int y = 0; y < snowH; ++y) {
                    *p = snowCapCol;
                    p += stride;
                }
            }
        }

        // 3. Mid-Ground Ridge Fill
        int midLimit = std::min(syNear, terrainLimit);
        if (syMid < midLimit) {
            fb.setPixelFast(x, syMid, ridgeHighlight);
            p = fb.scanLine(syMid + 1) + x;
            for (int y = syMid + 1; y < midLimit; ++y) {
                *p = midMtnCol;
                p += stride;
            }
            if ((biomeId == "mountain" || biomeId == "arctic") && syMid < H * 0.45f) {
                int snowH = std::min(3, midLimit - syMid);
                p = fb.scanLine(syMid) + x;
                for (int y = 0; y < snowH; ++y) {
                    *p = snowCapCol;
                    p += stride;
                }
            }
        }

        // 4. Near Foothill Fill
        if (syNear < terrainLimit) {
            p = fb.scanLine(syNear) + x;
            for (int y = syNear; y < terrainLimit; ++y) {
                *p = nearMtnCol;
                p += stride;
            }
        }
    }

    // 4. Celestial Elements & Sky Objects
    // A. Twinkling Stars
    drawStars(fb, W, H, camX, lighting.starsOpacity, gameTime);

    // B. Aurora Borealis (Arctic Night)
    if (lighting.auroraOpacity > 0.01f) {
        drawAurora(fb, W, H, camX, lighting.auroraOpacity, gameTime);
    }

    // C. Lunar Biome Earth
    if (biomeId == "moon") {
        drawEarth(fb, 140, 85, camX);
    }

    // D. The Sun
    if (lighting.sunOpacity > 0.01f) {
        drawSun(fb, static_cast<int>(lighting.sunScreenX), static_cast<int>(lighting.sunScreenY),
                lighting.sunOpacity, lighting.sunColor, lighting.horizonGlowColor);
    }

    // E. The Moon
    if (lighting.moonOpacity > 0.01f) {
        drawMoon(fb, static_cast<int>(lighting.moonScreenX), static_cast<int>(lighting.moonScreenY),
                 lighting.moonOpacity, lighting.moonColor);
    }

    // F. Clouds
    if (biomeId != "moon") {
        drawClouds(fb, W, H, camX, lighting);
    }

    // G. Wildlife Birds
    drawBirds(fb, W, H, camX, biomeId, gameTime, lighting);
}

// =========================================================================
// PIXEL-ART SCENERY & FLORA IMPLEMENTATIONS
// =========================================================================

void EnvironmentRenderer::drawOakTree(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting) {
    if (sx < -80 || sx > fb.width() + 80 || sy < -100 || sy > fb.height() + 100) return;

    uint32_t trunkCol   = DayNightCycle::applyAmbient(0xFF4E342E, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t trunkHi    = DayNightCycle::applyAmbient(0xFF6D4C41, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t trunkShd   = DayNightCycle::applyAmbient(0xFF3E2723, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t leafDark   = DayNightCycle::applyAmbient(0xFF1B5E20, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t leafMid    = DayNightCycle::applyAmbient(0xFF2E7D32, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t leafLight  = DayNightCycle::applyAmbient(0xFF66BB6A, lighting.ambientColor, lighting.ambientBrightness);

    int trunkW = std::clamp(h / 11, 14, 24);
    int trunkH = static_cast<int>(h * 0.45f);

    // Root flare anchored into ground
    fb.fillTriangle(sx - trunkW / 2 - 8, sy, sx + trunkW / 2 + 8, sy, sx, sy - 14, trunkShd);
    fb.fillRect(sx - trunkW / 2, sy - trunkH, trunkW, trunkH, trunkCol);
    // Vertical bark grooves
    fb.fillRect(sx - trunkW / 2 + 3, sy - trunkH, 2, trunkH, trunkHi);
    fb.fillRect(sx + 2, sy - trunkH + 4, 2, trunkH - 4, trunkShd);

    // Multi-tier lush foliage clusters (5 overlapping puffs for rich volume)
    int crownR = h / 4;
    int cy = sy - trunkH - crownR / 2;

    // Deep under-shadow layer
    fb.fillCircle(sx, cy + 6, crownR + 4, leafDark);
    fb.fillCircle(sx - crownR * 3 / 4, cy + 8, crownR * 4 / 5, leafDark);
    fb.fillCircle(sx + crownR * 3 / 4, cy + 8, crownR * 4 / 5, leafDark);

    // Midtone foliage volume
    fb.fillCircle(sx, cy, crownR + 2, leafMid);
    fb.fillCircle(sx - crownR * 3 / 4, cy + 2, crownR * 3 / 4, leafMid);
    fb.fillCircle(sx + crownR * 3 / 4, cy + 2, crownR * 3 / 4, leafMid);
    fb.fillCircle(sx - crownR / 3, cy - crownR * 2 / 3, crownR * 3 / 5, leafMid);
    fb.fillCircle(sx + crownR / 3, cy - crownR * 2 / 3, crownR * 3 / 5, leafMid);

    // Sunlit top canopy rim highlights
    fb.fillCircle(sx - 4, cy - crownR / 2, crownR / 2, leafLight);
    fb.fillCircle(sx - crownR * 2 / 3, cy - 2, crownR / 3, leafLight);
}

void EnvironmentRenderer::drawPineTree(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting, bool snowCovered) {
    if (sx < -80 || sx > fb.width() + 80 || sy < -100 || sy > fb.height() + 100) return;

    uint32_t trunkCol = DayNightCycle::applyAmbient(0xFF3E2723, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t pineDark = DayNightCycle::applyAmbient(0xFF0D3813, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t pineMid  = DayNightCycle::applyAmbient(0xFF1B5E20, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t pineHi   = DayNightCycle::applyAmbient(0xFF2E7D32, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t snowCol  = DayNightCycle::applyAmbient(0xFFECEFF1, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t snowShd  = DayNightCycle::applyAmbient(0xFFB0BEC5, lighting.ambientColor, lighting.ambientBrightness);

    int trunkW = std::clamp(h / 16, 8, 14);
    int trunkH = static_cast<int>(h * 0.22f);
    fb.fillRect(sx - trunkW / 2, sy - trunkH, trunkW, trunkH, trunkCol);

    // 5 Conical Tiers cascading downward
    const int tiers = 5;
    int foliageH = h - trunkH;
    int tierH = foliageH / (tiers - 1);

    for (int t = 0; t < tiers; ++t) {
        int ty = sy - trunkH - t * (tierH - 4);
        int halfW = (tiers - t) * (h / 20) + 12;

        // Dark conifer base
        fb.fillTriangle(sx - halfW, ty, sx + halfW, ty, sx, ty - tierH - 6, pineDark);
        // Sunlit midtone needle layer
        fb.fillTriangle(sx - halfW + 3, ty - 2, sx + halfW - 3, ty - 2, sx, ty - tierH - 6, pineMid);
        // Needle tip highlight
        fb.drawLine(sx, ty - tierH - 4, sx, ty - 4, pineHi);

        if (snowCovered) {
            // Thick pixel snow blanket atop tier
            fb.fillRect(sx - halfW + 1, ty - 4, (halfW - 1) * 2, 4, snowCol);
            fb.fillRect(sx - halfW + 2, ty, (halfW - 2) * 2, 1, snowShd);
            // Icicles hanging down
            for (int ix = sx - halfW + 6; ix < sx + halfW - 6; ix += 8) {
                fb.setPixelFast(ix, ty + 1, snowCol);
                fb.setPixelFast(ix, ty + 2, snowShd);
            }
        }
    }
}

void EnvironmentRenderer::drawBush(Framebuffer& fb, int sx, int sy, int r, const LightingState& lighting, uint32_t flowerColor) {
    if (sx < -40 || sx > fb.width() + 40 || sy < -40 || sy > fb.height() + 40) return;

    uint32_t bushDark = DayNightCycle::applyAmbient(0xFF1B5E20, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t bushMid  = DayNightCycle::applyAmbient(0xFF2E7D32, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t bushHi   = DayNightCycle::applyAmbient(0xFF43A047, lighting.ambientColor, lighting.ambientBrightness);

    // Clustered dense shrub volumes
    fb.fillCircle(sx, sy - r * 2 / 3, r, bushDark);
    fb.fillCircle(sx - r / 2, sy - r / 2, r * 3 / 4, bushDark);
    fb.fillCircle(sx + r / 2, sy - r / 2, r * 3 / 4, bushDark);

    fb.fillCircle(sx, sy - r * 2 / 3 - 2, r * 4 / 5, bushMid);
    fb.fillCircle(sx - r / 2, sy - r / 2 - 2, r * 3 / 5, bushMid);
    fb.fillCircle(sx + r / 2, sy - r / 2 - 2, r * 3 / 5, bushMid);

    fb.fillCircle(sx - 3, sy - r - 1, r / 2, bushHi);

    if (flowerColor != 0) {
        uint32_t fCol = DayNightCycle::applyAmbient(flowerColor, lighting.ambientColor, lighting.ambientBrightness);
        fb.fillRect(sx - r / 2, sy - r + 3, 2, 2, fCol);
        fb.fillRect(sx + r / 3, sy - r + 5, 2, 2, fCol);
        fb.fillRect(sx, sy - r / 2, 2, 2, fCol);
        fb.fillRect(sx - r / 4, sy - r / 3, 2, 2, fCol);
        fb.fillRect(sx + r / 2, sy - r / 3, 2, 2, fCol);
    }
}

void EnvironmentRenderer::drawFence(Framebuffer& fb, int sx1, int sy1, int sx2, int sy2, const LightingState& lighting) {
    uint32_t woodCol = DayNightCycle::applyAmbient(0xFF5D4037, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t woodHi  = DayNightCycle::applyAmbient(0xFF795548, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t woodShd = DayNightCycle::applyAmbient(0xFF3E2723, lighting.ambientColor, lighting.ambientBrightness);

    int postH = 34;
    int postW = 5;

    // Post 1 with chamfered cap
    fb.fillRect(sx1 - postW / 2, sy1 - postH, postW, postH, woodCol);
    fb.fillRect(sx1 - postW / 2, sy1 - postH, 1, postH, woodHi);
    fb.fillRect(sx1 + postW / 2, sy1 - postH, 1, postH, woodShd);

    // Post 2 with chamfered cap
    fb.fillRect(sx2 - postW / 2, sy2 - postH, postW, postH, woodCol);
    fb.fillRect(sx2 - postW / 2, sy2 - postH, 1, postH, woodHi);
    fb.fillRect(sx2 + postW / 2, sy2 - postH, 1, postH, woodShd);

    // Two sturdy horizontal timber rails spanning between posts
    for (int t = 0; t < 2; ++t) {
        int rOff = (t == 0) ? 24 : 10;
        int y1 = sy1 - rOff;
        int y2 = sy2 - rOff;
        fb.drawLine(sx1, y1 - 1, sx2, y2 - 1, woodHi);
        fb.drawLine(sx1, y1,     sx2, y2,     woodCol);
        fb.drawLine(sx1, y1 + 1, sx2, y2 + 1, woodCol);
        fb.drawLine(sx1, y1 + 2, sx2, y2 + 2, woodShd);
    }
}

void EnvironmentRenderer::drawBarn(Framebuffer& fb, int sx, int sy, const LightingState& lighting) {
    // Grand agricultural landmark: ~220px wide, ~135px tall
    if (sx < -240 || sx > fb.width() + 240 || sy < -180 || sy > fb.height() + 180) return;

    int bW = 210;
    int bH = 130;
    int bx = sx - bW / 2;
    int by = sy - bH;

    uint32_t wallCol   = DayNightCycle::applyAmbient(0xFFB71C1C, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t wallDark  = DayNightCycle::applyAmbient(0xFF880E4F, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t roofCol   = DayNightCycle::applyAmbient(0xFF37474F, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t roofShd   = DayNightCycle::applyAmbient(0xFF212121, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t trimCol   = DayNightCycle::applyAmbient(0xFFECEFF1, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t doorCol   = DayNightCycle::applyAmbient(0xFF4E342E, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t stoneCol  = DayNightCycle::applyAmbient(0xFF78909C, lighting.ambientColor, lighting.ambientBrightness);

    // 1. Stone masonry foundation along bottom
    fb.fillRect(bx - 2, sy - 8, bW + 4, 8, stoneCol);
    for (int st = bx; st < bx + bW; st += 14) {
        fb.setPixelFast(st, sy - 4, 0xFF37474F);
    }

    // 2. Red Timber Barn Walls with vertical board-and-batten siding
    fb.fillRect(bx, by + 35, bW, bH - 35 - 8, wallCol);
    for (int vx = bx + 6; vx < bx + bW; vx += 10) {
        fb.fillRect(vx, by + 35, 1, bH - 35 - 8, wallDark);
    }

    // 3. Crisp White Corner & Horizontal Trim
    fb.fillRect(bx, by + 35, 4, bH - 35 - 8, trimCol);
    fb.fillRect(bx + bW - 4, by + 35, 4, bH - 35 - 8, trimCol);
    fb.fillRect(bx, by + 35, bW, 3, trimCol);

    // 4. Classic Gambrel Roof (Two sloped pitches)
    // Lower steep slope
    fb.fillTriangle(bx - 8, by + 36, bx + bW + 8, by + 36, sx, by - 12, roofShd);
    fb.fillTriangle(bx - 6, by + 35, bx + bW + 6, by + 35, sx, by - 14, roofCol);

    // Shingle lines on gambrel roof
    for (int ly = by + 28; ly >= by - 8; ly -= 9) {
        fb.drawLine(bx + (by + 35 - ly), ly, bx + bW - (by + 35 - ly), ly, roofShd);
    }

    // 5. Cupola & Weather Vane atop roof peak
    int cupW = 20;
    int cupH = 16;
    fb.fillRect(sx - cupW / 2, by - 26, cupW, cupH, trimCol);
    fb.fillRect(sx - cupW / 2 + 3, by - 24, cupW - 6, cupH - 4, 0xFF212121);
    // Weather vane pole & rooster
    fb.drawLine(sx, by - 26, sx, by - 40, trimCol);
    fb.drawLine(sx - 7, by - 36, sx + 7, by - 36, trimCol); // Direction arms
    fb.fillRect(sx - 3, by - 42, 6, 4, UI::Theme::GOLD); // Golden rooster ornament

    // 6. Large Double Sliding Barn Doors with White "X" Cross-Braces
    int dW = 54;
    int dH = 55;
    int dx = sx - dW / 2;
    int dy = sy - 8 - dH;

    fb.fillRect(dx, dy, dW, dH, doorCol);
    fb.drawRect(dx, dy, dW, dH, trimCol);
    fb.drawRect(dx + 1, dy + 1, dW - 2, dH - 2, trimCol);
    // Split line down middle
    fb.drawLine(sx, dy, sx, sy - 8, trimCol);

    // Left door X
    fb.drawLine(dx + 2, dy + 2, sx - 2, dy + dH - 2, trimCol);
    fb.drawLine(dx + 2, dy + dH - 2, sx - 2, dy + 2, trimCol);

    // Right door X
    fb.drawLine(sx + 2, dy + 2, dx + dW - 2, dy + dH - 2, trimCol);
    fb.drawLine(sx + 2, dy + dH - 2, dx + dW - 2, dy + 2, trimCol);

    // 7. Upper Hayloft Door & Pulley Boom
    int hW = 26;
    int hH = 30;
    int hx = sx - hW / 2;
    int hy = by + 2;

    fb.fillRect(hx, hy, hW, hH, doorCol);
    fb.drawRect(hx, hy, hW, hH, trimCol);
    // Hayloft X
    fb.drawLine(hx + 1, hy + 1, hx + hW - 1, hy + hH - 1, trimCol);
    fb.drawLine(hx + 1, hy + hH - 1, hx + hW - 1, hy + 1, trimCol);

    // Pulley boom beam projecting from roof
    fb.fillRect(sx - 2, hy - 8, 4, 8, trimCol);
    fb.drawLine(sx + 2, hy - 4, sx + 2, hy + 14, 0xFFFFF9C4); // Rope

    // 8. Flanking Multi-Pane Farm Windows (Warm glowing at dusk/night)
    uint32_t winCol = (lighting.windowLightOpacity > 0.05f) ?
        DayNightCycle::blendColor(0xFF263238, 0xFFFFD54F, lighting.windowLightOpacity) : 0xFF263238;

    auto drawWindow = [&](int wx, int wy) {
        fb.fillRect(wx, wy, 16, 20, winCol);
        fb.drawRect(wx, wy, 16, 20, trimCol);
        fb.drawLine(wx + 8, wy, wx + 8, wy + 20, trimCol);
        fb.drawLine(wx, wy + 10, wx + 16, wy + 10, trimCol);
    };

    drawWindow(bx + 20, by + 50);
    drawWindow(bx + bW - 36, by + 50);
}

void EnvironmentRenderer::drawWindmill(Framebuffer& fb, int sx, int sy, float gameTime, const LightingState& lighting) {
    // Grand rustic windmill: Tower H=150px, sails diameter=144px
    if (sx < -200 || sx > fb.width() + 200 || sy < -240 || sy > fb.height() + 240) return;

    int tH = 150;
    int tW_base = 66;
    int tW_top = 38;
    int topY = sy - tH;

    uint32_t stoneCol  = DayNightCycle::applyAmbient(0xFF78909C, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t stoneDark = DayNightCycle::applyAmbient(0xFF546E7A, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t roofCol   = DayNightCycle::applyAmbient(0xFF455A64, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t sailCol   = DayNightCycle::applyAmbient(0xFFFFF9C4, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t sailShd   = DayNightCycle::applyAmbient(0xFFB0BEC5, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t woodCol   = DayNightCycle::applyAmbient(0xFF3E2723, lighting.ambientColor, lighting.ambientBrightness);

    // 1. Tapered Stone Tower with Masonry Texture
    int ySteps = tH;
    for (int y = 0; y < ySteps; ++y) {
        float frac = static_cast<float>(y) / static_cast<float>(ySteps);
        int curW = static_cast<int>(tW_top + (tW_base - tW_top) * frac);
        int curY = topY + y;
        fb.fillRect(sx - curW / 2, curY, curW, 1, stoneCol);
        // Stone shadow edge on left
        fb.fillRect(sx - curW / 2, curY, 3, 1, stoneDark);
    }

    // Masonry stone blocks
    for (int y = topY + 12; y < sy - 8; y += 14) {
        int curW = static_cast<int>(tW_top + (tW_base - tW_top) * (static_cast<float>(y - topY) / tH));
        fb.drawLine(sx - curW / 2 + 4, y, sx + curW / 2 - 4, y, stoneDark);
    }

    // 2. Conical Cap Roof
    fb.fillTriangle(sx - tW_top / 2 - 6, topY + 4, sx + tW_top / 2 + 6, topY + 4, sx, topY - 26, roofCol);
    fb.fillTriangle(sx - tW_top / 2 - 4, topY + 4, sx, topY + 4, sx, topY - 26, stoneDark);

    // 3. Arched Wooden Entrance Door
    int dW = 16;
    int dH = 26;
    fb.fillRect(sx - dW / 2, sy - dH, dW, dH, woodCol);
    fb.drawCircle(sx, sy - dH + 2, dW / 2, woodCol);

    // 4. Observation Windows (Illuminated at night)
    uint32_t winCol = (lighting.windowLightOpacity > 0.05f) ?
        DayNightCycle::blendColor(0xFF263238, 0xFFFFD54F, lighting.windowLightOpacity) : 0xFF263238;
    fb.fillRect(sx - 6, topY + 36, 12, 14, winCol);
    fb.drawRect(sx - 6, topY + 36, 12, 14, woodCol);
    fb.drawLine(sx, topY + 36, sx, topY + 50, woodCol);

    fb.fillRect(sx - 5, topY + 76, 10, 12, winCol);
    fb.drawRect(sx - 5, topY + 76, 10, 12, woodCol);

    // 5. Huge Rotating Sails (Hub at top center)
    int hubX = sx;
    int hubY = topY + 12;
    int sailLen = 72;
    float sailAngle = gameTime * 1.5f;

    for (int s = 0; s < 4; ++s) {
        float a = sailAngle + s * (static_cast<float>(M_PI) / 2.0f);
        int ex = hubX + static_cast<int>(std::cos(a) * sailLen);
        int ey = hubY + static_cast<int>(std::sin(a) * sailLen);

        // Heavy wooden spar
        fb.drawLine(hubX, hubY, ex, ey, woodCol);

        // Billowing Canvas Lattice Blade
        int bx1 = hubX + static_cast<int>(std::cos(a) * 16.0f);
        int by1 = hubY + static_cast<int>(std::sin(a) * 16.0f);
        int bx2 = ex;
        int by2 = ey;

        int normX = static_cast<int>(-std::sin(a) * 14.0f);
        int normY = static_cast<int>(std::cos(a) * 14.0f);

        fb.fillTriangle(bx1, by1, bx2, by2, bx2 + normX, by2 + normY, sailCol);
        fb.fillTriangle(bx1, by1, bx2 + normX, by2 + normY, bx1 + normX * 0.7f, by1 + normY * 0.7f, sailShd);
        // Lattice framing
        fb.drawLine(bx1 + normX * 0.7f, by1 + normY * 0.7f, bx2 + normX, by2 + normY, woodCol);
    }

    // Heavy iron/wood center hub
    fb.fillCircle(hubX, hubY, 7, woodCol);
    fb.fillCircle(hubX, hubY, 4, 0xFF78909C);
    fb.setPixelFast(hubX, hubY, 0xFF212121);
}

void EnvironmentRenderer::drawSaguaroCactus(Framebuffer& fb, int sx, int sy, int h, int arms, const LightingState& lighting) {
    if (sx < -60 || sx > fb.width() + 60 || sy < -100 || sy > fb.height() + 100) return;

    uint32_t cDark  = DayNightCycle::applyAmbient(0xFF1B5E20, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t cMid   = DayNightCycle::applyAmbient(0xFF2E7D32, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t cHi    = DayNightCycle::applyAmbient(0xFF43A047, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t cSpine = DayNightCycle::applyAmbient(0xFFFFEE58, lighting.ambientColor, lighting.ambientBrightness);

    int trunkW = 16;
    int trunkH = h;
    int topY = sy - trunkH;

    // Main Column
    fb.fillRect(sx - trunkW / 2, topY, trunkW, trunkH, cDark);
    fb.fillRect(sx - trunkW / 2 + 2, topY, trunkW - 4, trunkH, cMid);
    fb.fillRect(sx - trunkW / 2 + 3, topY, 3, trunkH, cHi); // Sunlight ridge
    fb.fillCircle(sx, topY, trunkW / 2, cMid);

    // Left Arm (Curved upward)
    if (arms >= 1) {
        int armY = sy - static_cast<int>(trunkH * 0.62f);
        int armLen = 30;
        int armH = 45;
        int armW = 12;

        fb.fillRect(sx - armLen, armY, armLen, armW, cDark);
        fb.fillRect(sx - armLen, armY - armH, armW, armH + armW, cDark);
        fb.fillRect(sx - armLen + 2, armY - armH, armW - 4, armH + armW - 2, cMid);
        fb.fillRect(sx - armLen + 2, armY - armH, 2, armH, cHi);
        fb.fillCircle(sx - armLen + armW / 2, armY - armH, armW / 2, cMid);
    }

    // Right Arm (Curved upward, offset height)
    if (arms >= 2) {
        int armY = sy - static_cast<int>(trunkH * 0.45f);
        int armLen = 28;
        int armH = 40;
        int armW = 12;

        fb.fillRect(sx, armY, armLen, armW, cDark);
        fb.fillRect(sx + armLen - armW, armY - armH, armW, armH + armW, cDark);
        fb.fillRect(sx + armLen - armW + 2, armY - armH, armW - 4, armH + armW - 2, cMid);
        fb.fillRect(sx + armLen - armW + 2, armY - armH, 2, armH, cHi);
        fb.fillCircle(sx + armLen - armW / 2, armY - armH, armW / 2, cMid);
    }

    // Golden Desert Spines
    for (int y = topY + 6; y < sy - 6; y += 10) {
        fb.setPixelFast(sx - trunkW / 2 - 1, y, cSpine);
        fb.setPixelFast(sx + trunkW / 2 + 1, y + 3, cSpine);
    }
}

void EnvironmentRenderer::drawPricklyPear(Framebuffer& fb, int sx, int sy, const LightingState& lighting) {
    if (sx < -60 || sx > fb.width() + 60 || sy < -60 || sy > fb.height() + 60) return;

    uint32_t padCol   = DayNightCycle::applyAmbient(0xFF2E7D32, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t padDark  = DayNightCycle::applyAmbient(0xFF1B5E20, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t padHi    = DayNightCycle::applyAmbient(0xFF43A047, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t spineCol = DayNightCycle::applyAmbient(0xFFFFEE58, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t fruitCol = DayNightCycle::applyAmbient(0xFFC2185B, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t fruitHi  = DayNightCycle::applyAmbient(0xFFE91E63, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t stemCol  = DayNightCycle::applyAmbient(0xFF5D4037, lighting.ambientColor, lighting.ambientBrightness);

    // Rooted woody base stem anchored in ground
    fb.fillRect(sx - 3, sy - 5, 6, 6, stemCol);
    fb.fillRect(sx - 2, sy - 5, 2, 6, DayNightCycle::applyAmbient(0xFF795548, lighting.ambientColor, lighting.ambientBrightness));

    // Helper: draw an authentic cladode (paddle) with outline, shading and spine clusters
    auto drawCladode = [&](int cx, int cy, int rx, int ry, float tilt) {
        float cosT = std::cos(tilt);
        float sinT = std::sin(tilt);
        for (int dy = -ry; dy <= ry; ++dy) {
            float yF = static_cast<float>(dy) / ry;
            float maxW = std::sqrt(std::max(0.0f, 1.0f - yF * yF)) * rx;
            int minX = static_cast<int>(-maxW);
            int maxX = static_cast<int>(maxW);
            for (int dx = minX; dx <= maxX; ++dx) {
                int px = static_cast<int>(cx + dx * cosT - dy * sinT);
                int py = static_cast<int>(cy + dx * sinT + dy * cosT);
                if (px < 0 || px >= fb.width() || py < 0 || py >= fb.height()) continue;

                bool isEdge = (dx == minX || dx == maxX || dy == -ry || dy == ry);
                if (isEdge) {
                    fb.setPixelFast(px, py, padDark);
                } else if (dx < 0 && dy < 0) {
                    fb.setPixelFast(px, py, padHi);
                } else {
                    fb.setPixelFast(px, py, padCol);
                }
            }
        }
        // Spine prickle clusters
        fb.setPixelFast(cx, cy, spineCol);
        int sx1 = static_cast<int>(cx - rx * 0.4f * cosT);
        int sy1 = static_cast<int>(cy - ry * 0.4f);
        if (sx1 >= 0 && sx1 < fb.width() && sy1 >= 0 && sy1 < fb.height()) fb.setPixelFast(sx1, sy1, spineCol);
        int sx2 = static_cast<int>(cx + rx * 0.4f * cosT);
        int sy2 = static_cast<int>(cy + ry * 0.3f);
        if (sx2 >= 0 && sx2 < fb.width() && sy2 >= 0 && sy2 < fb.height()) fb.setPixelFast(sx2, sy2, spineCol);
    };

    // 1. Central base cladode
    drawCladode(sx, sy - 14, 10, 13, 0.0f);

    // 2. Left side branching cladode
    drawCladode(sx - 13, sy - 23, 8, 11, -0.32f);

    // 3. Right side branching cladode
    drawCladode(sx + 13, sy - 21, 8, 11, 0.30f);

    // 4. Upper terminal cladodes
    drawCladode(sx - 6, sy - 34, 7, 10, -0.12f);
    drawCladode(sx + 8, sy - 32, 7, 10, 0.15f);

    // 5. Bright magenta prickly pear fruits perched atop upper pad crowns
    auto drawFruit = [&](int fx, int fy) {
        fb.fillRect(fx - 1, fy - 3, 3, 4, fruitCol);
        fb.fillRect(fx - 1, fy - 3, 1, 3, fruitHi);
        if (fy - 4 >= 0) fb.setPixelFast(fx, fy - 4, 0xFF880E4F); // dark calyx tip
    };
    drawFruit(sx - 18, sy - 31);
    drawFruit(sx - 7, sy - 44);
    drawFruit(sx + 8, sy - 42);
    drawFruit(sx + 19, sy - 28);
}

void EnvironmentRenderer::drawDesertBoulder(Framebuffer& fb, int sx, int sy, int w, int h, const LightingState& lighting) {
    uint32_t bDark = DayNightCycle::applyAmbient(0xFF8D4E2A, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t bMid  = DayNightCycle::applyAmbient(0xFFBF6E34, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t bHili = DayNightCycle::applyAmbient(0xFFFFB74D, lighting.ambientColor, lighting.ambientBrightness);

    // Chiseled stratified boulder
    fb.fillTriangle(sx - w / 2, sy, sx + w / 2, sy, sx - w / 6, sy - h, bDark);
    fb.fillTriangle(sx - w / 2 + 4, sy - 1, sx + w / 2 - 4, sy - 1, sx - w / 6, sy - h + 2, bMid);
    // Sunlit top rim
    fb.drawLine(sx - w / 3, sy - h + 1, sx + w / 4, sy - h + 5, bHili);
    fb.drawLine(sx - w / 3, sy - h + 2, sx + w / 4, sy - h + 6, bHili);
}

void EnvironmentRenderer::drawDesertArch(Framebuffer& fb, int sx, int sy, const LightingState& lighting) {
    // Grand natural Utah sandstone arch: ~240px wide, ~150px tall with hollow portal opening
    if (sx < -260 || sx > fb.width() + 260 || sy < -200 || sy > fb.height() + 200) return;

    int archW = 240;
    int archH = 150;
    int topY = sy - archH;

    uint32_t rockDark = DayNightCycle::applyAmbient(0xFF8D4E2A, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t rockMid  = DayNightCycle::applyAmbient(0xFFBF6E34, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t rockHi   = DayNightCycle::applyAmbient(0xFFFFB74D, lighting.ambientColor, lighting.ambientBrightness);

    // Scanline-based natural sandstone archway with hollow portal opening
    for (int x = sx - archW / 2; x <= sx + archW / 2; ++x) {
        if (x < 0 || x >= fb.width()) continue;
        float relX = static_cast<float>(x - sx) / (archW * 0.5f); // -1.0 to 1.0
        float relX2 = relX * relX;
        if (relX2 > 1.0f) continue;

        // Outer rock arch contour (gently curving crown)
        int yTop = static_cast<int>(topY + (1.0f - std::sqrt(1.0f - relX2)) * 24.0f);
        int yBot = sy;

        // Hollow interior portal curve:
        // Inside relX within [-0.74, 0.74], the underside curves up to form the arch opening
        if (std::abs(relX) < 0.74f) {
            float innerNorm = relX / 0.74f;
            float innerArch = std::sqrt(std::max(0.0f, 1.0f - innerNorm * innerNorm));
            int portalTop = static_cast<int>((sy - 10) - innerArch * (archH - 40.0f));
            // Fill upper rock arch span
            for (int y = yTop; y < portalTop && y < sy; ++y) {
                if (y < 0 || y >= fb.height()) continue;
                uint32_t col = (y < yTop + 3) ? rockHi : ((x < sx - 20) ? rockDark : rockMid);
                fb.setPixelFast(x, y, col);
            }
        } else {
            // Solid natural stone pillar on left and right sides
            for (int y = yTop; y < yBot; ++y) {
                if (y < 0 || y >= fb.height()) continue;
                uint32_t col = (x < sx - archW / 2 + 5 || (x > sx + archW / 2 - 10 && x < sx + archW / 2 - 7)) ? rockHi :
                               ((x < sx) ? rockDark : rockMid);
                fb.setPixelFast(x, y, col);
            }
        }
    }
}

void EnvironmentRenderer::drawIceSpire(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting) {
    uint32_t iceDark  = DayNightCycle::applyAmbient(0xFF00838F, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t iceMid   = DayNightCycle::applyAmbient(0xFF00ACC1, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t iceLight = DayNightCycle::applyAmbient(0xFF80DEEA, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t iceGlint = DayNightCycle::applyAmbient(0xFFFFFFFF, lighting.ambientColor, lighting.ambientBrightness);

    int halfW = std::clamp(h / 4, 12, 32);
    fb.fillTriangle(sx - halfW, sy, sx + halfW, sy, sx, sy - h, iceDark);
    fb.fillTriangle(sx - halfW + 3, sy - 2, sx + 2, sy - 2, sx, sy - h, iceMid);
    fb.fillTriangle(sx - halfW + 5, sy - 4, sx - 2, sy - 4, sx, sy - h, iceLight);
    fb.drawLine(sx, sy - h, sx + 2, sy - h / 3, iceGlint);
}

void EnvironmentRenderer::drawResearchDome(Framebuffer& fb, int sx, int sy, const LightingState& lighting) {
    // Grand Arctic research outpost dome: diameter 96px resting on ground
    if (sx < -100 || sx > fb.width() + 100 || sy < -100 || sy > fb.height() + 100) return;

    int r = 48;
    int cy = sy - 2;

    uint32_t domeCol   = DayNightCycle::applyAmbient(0xFFCFD8DC, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t domeShd   = DayNightCycle::applyAmbient(0xFF78909C, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t rimCol    = DayNightCycle::applyAmbient(0xFF455A64, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t stripeCol = DayNightCycle::applyAmbient(0xFFFF6D00, lighting.ambientColor, lighting.ambientBrightness);

    // Draw Upper Hemisphere resting on ground
    for (int dy = -r; dy <= 0; ++dy) {
        int y = cy + dy;
        int halfW = static_cast<int>(std::sqrt(r * r - dy * dy));
        fb.fillRect(sx - halfW, y, halfW * 2, 1, domeCol);
        // Shadow on left edge
        fb.fillRect(sx - halfW, y, std::max(2, halfW / 4), 1, domeShd);
    }

    // Geodesic panel seams
    fb.drawLine(sx - r + 8, cy - 16, sx + r - 8, cy - 16, domeShd);
    fb.drawLine(sx - r + 18, cy - 30, sx + r - 18, cy - 30, domeShd);

    // Hazard orange safety stripe around base
    fb.fillRect(sx - r + 4, cy - 8, (r - 4) * 2, 4, stripeCol);

    // Reinforced Airlock Entryway
    int aW = 24;
    int aH = 18;
    fb.fillRect(sx - aW / 2, sy - aH, aW, aH, rimCol);
    uint32_t winCol = (lighting.windowLightOpacity > 0.05f) ?
        DayNightCycle::blendColor(0xFF263238, 0xFFFFD54F, lighting.windowLightOpacity) : 0xFF263238;
    fb.fillRect(sx - aW / 2 + 4, sy - aH + 3, aW - 8, aH - 6, winCol);

    // Foundation Base
    fb.fillRect(sx - r - 4, sy - 3, (r + 4) * 2, 4, rimCol);

    // Satellite Dish & Aviation Beacon Mast
    fb.drawLine(sx + 16, cy - r + 6, sx + 26, cy - r - 20, rimCol);
    fb.fillCircle(sx + 26, cy - r - 20, 6, domeCol);
    fb.drawCircle(sx + 26, cy - r - 20, 6, rimCol);

    fb.drawLine(sx - 14, cy - r + 4, sx - 14, cy - r - 24, rimCol);
    fb.setPixelFast(sx - 14, cy - r - 24, 0xFFFF1744); // Blinking red beacon
}

void EnvironmentRenderer::drawLunarLander(Framebuffer& fb, int sx, int sy, const LightingState& lighting) {
    // Grand Apollo Lunar Module: ~170px wide, ~140px tall
    if (sx < -190 || sx > fb.width() + 190 || sy < -160 || sy > fb.height() + 160) return;

    uint32_t goldFoil  = DayNightCycle::applyAmbient(0xFFFFD54F, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t goldShd   = DayNightCycle::applyAmbient(0xFFC79100, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t ascentCol = DayNightCycle::applyAmbient(0xFFECEFF1, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t ascentShd = DayNightCycle::applyAmbient(0xFF90A4AE, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t strutCol  = DayNightCycle::applyAmbient(0xFF78909C, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t engineCol = DayNightCycle::applyAmbient(0xFF37474F, lighting.ambientColor, lighting.ambientBrightness);

    int stageBaseY = sy - 28;

    // 1. Four Heavy Landing Gear Legs & Footpads
    // Far left leg & pad
    fb.drawLine(sx - 70, sy, sx - 28, stageBaseY, strutCol);
    fb.drawLine(sx - 70, sy, sx - 45, stageBaseY - 10, strutCol);
    fb.fillRect(sx - 78, sy - 3, 16, 4, goldFoil); // Dish footpad
    fb.drawRect(sx - 78, sy - 3, 16, 4, goldShd);

    // Far right leg & pad
    fb.drawLine(sx + 70, sy, sx + 28, stageBaseY, strutCol);
    fb.drawLine(sx + 70, sy, sx + 45, stageBaseY - 10, strutCol);
    fb.fillRect(sx + 62, sy - 3, 16, 4, goldFoil);
    fb.drawRect(sx + 62, sy - 3, 16, 4, goldShd);

    // Center ladder strut on left leg
    fb.drawLine(sx - 28, stageBaseY, sx - 48, sy, strutCol);
    for (int ly = stageBaseY + 6; ly < sy - 2; ly += 6) {
        fb.drawLine(sx - 34 - (ly - stageBaseY) / 2, ly, sx - 28 - (ly - stageBaseY) / 2, ly, strutCol);
    }

    // 2. Descent Engine Bell Nozzle
    fb.fillTriangle(sx - 14, stageBaseY + 12, sx + 14, stageBaseY + 12, sx, stageBaseY, engineCol);

    // 3. Octagonal Gold Kapton Thermal Foil Descent Stage
    int dW = 72;
    int dH = 34;
    int dy = stageBaseY - dH;
    fb.fillRect(sx - dW / 2, dy, dW, dH, goldFoil);
    fb.drawRect(sx - dW / 2, dy, dW, dH, goldShd);
    // Crumpled foil texture
    for (int y = dy + 4; y < dy + dH - 2; y += 6) {
        fb.drawLine(sx - dW / 2 + 4, y, sx + dW / 2 - 4, y, goldShd);
    }

    // 4. White Ascent Stage (Crew Cabin)
    int aW = 56;
    int aH = 46;
    int ay = dy - aH;
    fb.fillRect(sx - aW / 2, ay, aW, aH, ascentCol);
    fb.fillRect(sx - aW / 2, ay, 6, aH, ascentShd); // Left shadow
    fb.drawRect(sx - aW / 2, ay, aW, aH, 0xFF37474F);

    // Cockpit Triangular Windows
    fb.fillTriangle(sx - 18, ay + 14, sx - 8, ay + 14, sx - 13, ay + 24, 0xFF212121);
    fb.fillTriangle(sx + 8, ay + 14, sx + 18, ay + 14, sx + 13, ay + 24, 0xFF212121);
    // Ingress Hatch
    fb.drawRect(sx - 7, ay + 12, 14, 22, 0xFF78909C);

    // 5. Reaction Control Thruster Quads (RCS)
    fb.fillRect(sx - aW / 2 - 6, ay + 18, 6, 8, engineCol);
    fb.fillRect(sx + aW / 2, ay + 18, 6, 8, engineCol);

    // 6. Communications Steerable Antenna Mast
    fb.drawLine(sx + 10, ay, sx + 18, ay - 24, strutCol);
    fb.fillCircle(sx + 18, ay - 24, 8, ascentCol);
    fb.drawCircle(sx + 18, ay - 24, 8, strutCol);

    // 7. Apollo American Flag planted beside lander
    int flagX = sx + 88;
    int flagPoleTop = sy - 52;
    fb.drawLine(flagX, sy, flagX, flagPoleTop, strutCol);
    // Gold finial sphere
    fb.fillCircle(flagX, flagPoleTop - 1, 2, UI::Theme::GOLD);
    // 24x16 Flag Cloth with Waving ripple
    int fW = 24;
    int fH = 16;
    fb.fillRect(flagX + 1, flagPoleTop, fW, fH, 0xFFC62828); // Red stripes
    for (int s = 2; s < fH; s += 4) {
        fb.fillRect(flagX + 1, flagPoleTop + s, fW, 2, 0xFFFFFFFF); // White stripes
    }
    // Blue Canton & Star field
    fb.fillRect(flagX + 1, flagPoleTop, 11, 9, 0xFF1565C0);
    fb.setPixelFast(flagX + 3, flagPoleTop + 2, 0xFFFFFFFF);
    fb.setPixelFast(flagX + 8, flagPoleTop + 2, 0xFFFFFFFF);
    fb.setPixelFast(flagX + 5, flagPoleTop + 4, 0xFFFFFFFF);
    fb.setPixelFast(flagX + 3, flagPoleTop + 6, 0xFFFFFFFF);
    fb.setPixelFast(flagX + 8, flagPoleTop + 6, 0xFFFFFFFF);
}

void EnvironmentRenderer::drawLunarBoulder(Framebuffer& fb, int sx, int sy, int r, const LightingState& lighting) {
    uint32_t bDark = DayNightCycle::applyAmbient(0xFF37474F, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t bMid  = DayNightCycle::applyAmbient(0xFF607D8B, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t bHi   = DayNightCycle::applyAmbient(0xFFCFD8DC, lighting.ambientColor, lighting.ambientBrightness);

    // Harsh cratered boulder with stark lunar shadow
    fb.fillCircle(sx, sy - r * 2 / 3, r, bDark);
    fb.fillCircle(sx - r / 3, sy - r * 3 / 4, r * 3 / 4, bMid);
    fb.fillCircle(sx - r / 2, sy - r, r / 3, bHi);

    // Micro-crater depressions
    fb.fillCircle(sx + r / 4, sy - r / 2, r / 4, bDark);
    fb.drawCircle(sx + r / 4, sy - r / 2, r / 4, bHi);
}

void EnvironmentRenderer::drawBasaltColumn(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting) {
    uint32_t colDark = DayNightCycle::applyAmbient(0xFF212121, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colMid  = DayNightCycle::applyAmbient(0xFF37474F, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colHi   = DayNightCycle::applyAmbient(0xFF546E7A, lighting.ambientColor, lighting.ambientBrightness);

    // Cluster of stepped hexagonal columns
    int w = 18;
    fb.fillRect(sx - w / 2, sy - h, w, h, colDark);
    fb.fillRect(sx - w / 2 + 2, sy - h, w / 2, h, colMid);
    fb.fillRect(sx - w / 2 + 3, sy - h, 2, h, colHi);

    // Hexagonal facet top
    fb.fillTriangle(sx - w / 2, sy - h, sx + w / 2, sy - h, sx, sy - h - 6, colHi);
    fb.drawLine(sx, sy - h - 6, sx + w / 2, sy - h, colMid);
}

void EnvironmentRenderer::drawMagmaVent(Framebuffer& fb, int sx, int sy, float gameTime, const LightingState&) {
    float pulse = 0.6f + 0.4f * std::sin(gameTime * 3.5f + sx);
    uint32_t lavaCol = DayNightCycle::blendColor(0xFFFF3D00, 0xFFFFAB00, pulse);
    uint32_t rockCol = 0xFF212121;

    // Volcanic rim crater
    fb.fillRect(sx - 16, sy - 3, 32, 6, rockCol);
    fb.fillRect(sx - 12, sy - 2, 24, 4, lavaCol);
    fb.fillRect(sx - 6, sy - 3, 12, 2, 0xFFFFF59D); // High temperature core

    // Spattering sparks
    int sparkY = sy - 6 - static_cast<int>(std::fmod(gameTime * 20.0f + sx, 16.0f));
    fb.setPixelFast(sx - 2, sparkY, 0xFFFFD54F);
}

void EnvironmentRenderer::drawDeadTree(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting) {
    uint32_t burntDark = DayNightCycle::applyAmbient(0xFF140D0B, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t burntMid  = DayNightCycle::applyAmbient(0xFF261C19, lighting.ambientColor, lighting.ambientBrightness);

    int trunkW = std::clamp(h / 12, 10, 16);
    int trunkH = h;
    fb.fillRect(sx - trunkW / 2, sy - trunkH, trunkW, trunkH, burntDark);
    fb.fillRect(sx - trunkW / 2 + 2, sy - trunkH, 2, trunkH, burntMid);

    // Gnarled, jagged branches
    fb.drawLine(sx, sy - trunkH * 2 / 3, sx - 24, sy - trunkH + 8, burntDark);
    fb.drawLine(sx - 24, sy - trunkH + 8, sx - 32, sy - trunkH - 6, burntDark);

    fb.drawLine(sx, sy - trunkH * 4 / 5, sx + 26, sy - trunkH - 4, burntDark);
    fb.drawLine(sx + 26, sy - trunkH - 4, sx + 36, sy - trunkH - 20, burntDark);
}

void EnvironmentRenderer::drawCairn(Framebuffer& fb, int sx, int sy, const LightingState& lighting) {
    uint32_t rockCol = DayNightCycle::applyAmbient(0xFF546E7A, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t rockHi  = DayNightCycle::applyAmbient(0xFF78909C, lighting.ambientColor, lighting.ambientBrightness);

    // 5 Stacked weathered river stones: Height ~50px
    fb.fillCircle(sx, sy - 8, 16, rockCol);
    fb.fillCircle(sx - 2, sy - 10, 10, rockHi);

    fb.fillCircle(sx, sy - 22, 13, rockCol);
    fb.fillCircle(sx - 2, sy - 24, 8, rockHi);

    fb.fillCircle(sx, sy - 34, 10, rockCol);
    fb.fillCircle(sx - 1, sy - 36, 6, rockHi);

    fb.fillCircle(sx, sy - 44, 7, rockCol);
    fb.fillCircle(sx, sy - 51, 4, rockCol);
}

void EnvironmentRenderer::drawWatchtower(Framebuffer& fb, int sx, int sy, float gameTime, const LightingState& lighting) {
    // Grand Mountain Forest Fire Lookout Watchtower: ~180px tall
    if (sx < -100 || sx > fb.width() + 100 || sy < -220 || sy > fb.height() + 220) return;

    int wH = 180;
    int topY = sy - wH;

    uint32_t woodCol = DayNightCycle::applyAmbient(0xFF4E342E, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t woodHi  = DayNightCycle::applyAmbient(0xFF6D4C41, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t woodShd = DayNightCycle::applyAmbient(0xFF3E2723, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t cabCol  = DayNightCycle::applyAmbient(0xFF5D4037, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t roofCol = DayNightCycle::applyAmbient(0xFF37474F, lighting.ambientColor, lighting.ambientBrightness);

    // 1. Heavy Timber Trestle Legs (Wider stance at ground)
    int baseW = 60;
    int cabW = 46;
    int cabH = 34;
    int cabY = topY + 12;

    fb.drawLine(sx - baseW / 2, sy, sx - cabW / 2, cabY + cabH, woodCol);
    fb.drawLine(sx - baseW / 2 + 1, sy, sx - cabW / 2 + 1, cabY + cabH, woodHi);

    fb.drawLine(sx + baseW / 2, sy, sx + cabW / 2, cabY + cabH, woodCol);
    fb.drawLine(sx + baseW / 2 - 1, sy, sx + cabW / 2 - 1, cabY + cabH, woodShd);

    // Diagonal Cross-Bracing
    for (int y = sy - 24; y > cabY + cabH + 12; y -= 32) {
        float frac1 = static_cast<float>(sy - y) / (sy - (cabY + cabH));
        float frac2 = static_cast<float>(sy - (y - 32)) / (sy - (cabY + cabH));
        int w1 = static_cast<int>(baseW - (baseW - cabW) * frac1);
        int w2 = static_cast<int>(baseW - (baseW - cabW) * frac2);

        fb.drawLine(sx - w1 / 2, y, sx + w2 / 2, y - 32, woodShd);
        fb.drawLine(sx + w1 / 2, y, sx - w2 / 2, y - 32, woodShd);
        // Horizontal beam
        fb.drawLine(sx - w1 / 2, y, sx + w1 / 2, y, woodCol);
    }

    // 2. Lookout Observation Platform & Railing
    fb.fillRect(sx - cabW / 2 - 6, cabY + cabH, cabW + 12, 4, woodCol);
    fb.drawRect(sx - cabW / 2 - 6, cabY + cabH - 8, cabW + 12, 8, woodHi);

    // 3. Weather Enclosed Cabin
    fb.fillRect(sx - cabW / 2, cabY, cabW, cabH, cabCol);
    // Wrap-around Observation Windows (Warmly lit at night)
    uint32_t winCol = (lighting.windowLightOpacity > 0.05f) ?
        DayNightCycle::blendColor(0xFF263238, 0xFFFFD54F, lighting.windowLightOpacity) : 0xFFECEFF1;

    for (int wx = sx - cabW / 2 + 4; wx < sx + cabW / 2 - 8; wx += 10) {
        fb.fillRect(wx, cabY + 6, 8, 14, winCol);
        fb.drawRect(wx, cabY + 6, 8, 14, woodShd);
    }

    // 4. Overhanging Pitched Roof
    fb.fillTriangle(sx - cabW / 2 - 10, cabY, sx + cabW / 2 + 10, cabY, sx, topY - 10, roofCol);

    // 5. Blinking Red Aviation Hazard Strobe atop roof peak
    bool beaconOn = (static_cast<int>(gameTime * 2.0f) % 2 == 0);
    fb.drawLine(sx, topY - 10, sx, topY - 22, 0xFFCFD8DC);
    fb.setPixelFast(sx, topY - 22, beaconOn ? 0xFFFF1744 : 0xFF7F0000);
    if (beaconOn) {
        fb.setPixelFast(sx - 1, topY - 22, 0x88FF1744);
        fb.setPixelFast(sx + 1, topY - 22, 0x88FF1744);
    }
}

void EnvironmentRenderer::renderForegroundScenery(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain,
                                                 const DayNightCycle& cycle, float gameTime) {
    float zoom = cam.zoom();
    if (zoom <= 0.001f) return;

    int W = fb.width();
    int H = fb.height();
    float camX = cam.x();
    float halfView = (W * 0.5f) / zoom;
    float xMin = camX - halfView - 16.0f;
    float xMax = camX + halfView + 16.0f;

    LightingState lighting = cycle.getLighting(terrain.getBiome(), W, H, camX);
    const std::string& biomeId = terrain.getBiome();

    // Chunk size 22.0 meters: accommodates large, realistic environmental landmarks
    // while guaranteeing comfortable visual breathing room without overcrowding
    const float CHUNK_WIDTH = 22.0f;
    int startChunk = static_cast<int>(std::floor(xMin / CHUNK_WIDTH));
    int endChunk   = static_cast<int>(std::ceil(xMax / CHUNK_WIDTH));

    uint32_t biomeSeed = 42;
    if (biomeId == "desert") biomeSeed = 101;
    else if (biomeId == "arctic") biomeSeed = 202;
    else if (biomeId == "mountain") biomeSeed = 303;
    else if (biomeId == "moon") biomeSeed = 404;
    else if (biomeId == "volcano") biomeSeed = 505;

    for (int k = startChunk; k <= endChunk; ++k) {
        uint32_t seed = hash32(static_cast<uint32_t>(k * 1337) ^ biomeSeed);

        // Landmark chunks center their structures to ensure optimal visual staging
        bool isLandmark = false;
        if (biomeId == "countryside") isLandmark = (std::abs(k % 18) == 0 || std::abs(k % 26) == 0);
        else if (biomeId == "desert") isLandmark = (std::abs(k % 20) == 0);
        else if (biomeId == "arctic") isLandmark = (std::abs(k % 20) == 0);
        else if (biomeId == "mountain") isLandmark = (std::abs(k % 22) == 0);
        else if (biomeId == "moon") isLandmark = (std::abs(k % 20) == 0);

        float xNorm = static_cast<float>(seed & 0xFFF) / 4095.0f;
        float objWorldX = isLandmark ? (k * CHUNK_WIDTH + CHUNK_WIDTH * 0.5f) :
                                       (k * CHUNK_WIDTH + xNorm * (CHUNK_WIDTH - 4.0f) + 2.0f);

        if (objWorldX < xMin || objWorldX > xMax) continue;

        float groundY = terrain.getHeight(objWorldX);
        float slope = terrain.getSlope(objWorldX);
        Physics::Vec2 scr = cam.worldToScreen(Physics::Vec2(objWorldX, groundY));
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -200 || sx > W + 200 || sy < -240 || sy > H + 240) continue;

        int typeSelector = (seed >> 12) & 15;

        if (biomeId == "countryside") {
            // Milestone landmarks every ~400m and ~600m
            if (std::abs(k % 18) == 0) {
                drawBarn(fb, sx, sy, lighting);
            } else if (std::abs(k % 26) == 0) {
                drawWindmill(fb, sx, sy, gameTime, lighting);
            } else if (typeSelector < 5) {
                int treeH = 175 + (seed % 60); // 175 - 235 px tall
                drawOakTree(fb, sx, sy, treeH, lighting);
            } else if (typeSelector < 8) {
                int pineH = 160 + (seed % 55); // 160 - 215 px tall
                drawPineTree(fb, sx, sy, pineH, lighting, false);
            } else if (typeSelector < 11) {
                int bushR = 18 + (seed % 10);  // 18 - 28 px radius
                uint32_t flCol = (seed % 2 == 0) ? 0xFFFFEE58 : 0xFFFFFFFF;
                drawBush(fb, sx, sy, bushR, lighting, flCol);
            } else if (typeSelector < 14) {
                // Post-and-rail wooden fence
                float nextX = objWorldX + 6.0f;
                float nextY = terrain.getHeight(nextX);
                Physics::Vec2 nextScr = cam.worldToScreen(Physics::Vec2(nextX, nextY));
                drawFence(fb, sx, sy, static_cast<int>(nextScr.x), static_cast<int>(nextScr.y), lighting);
            }
        } else if (biomeId == "desert") {
            if (std::abs(k % 20) == 0) {
                drawDesertArch(fb, sx, sy, lighting);
            } else if (typeSelector < 6) {
                int cactusH = 135 + (seed % 55); // 135 - 190 px tall
                int arms = 1 + (seed % 2);
                drawSaguaroCactus(fb, sx, sy, cactusH, arms, lighting);
            } else if (typeSelector < 9) {
                drawPricklyPear(fb, sx, sy, lighting);
            } else if (typeSelector < 14) {
                int bW = 55 + (seed % 35);
                int bH = 32 + (seed % 22);
                drawDesertBoulder(fb, sx, sy, bW, bH, lighting);
            }
        } else if (biomeId == "arctic") {
            if (std::abs(k % 20) == 0) {
                drawResearchDome(fb, sx, sy, lighting);
            } else if (typeSelector < 7) {
                int pineH = 160 + (seed % 55);
                drawPineTree(fb, sx, sy, pineH, lighting, true); // Snow-covered
            } else if (typeSelector < 11) {
                int spireH = 85 + (seed % 45);
                drawIceSpire(fb, sx, sy, spireH, lighting);
            } else if (typeSelector < 14) {
                int bW = 50 + (seed % 30);
                int bH = 30 + (seed % 18);
                drawDesertBoulder(fb, sx, sy, bW, bH, lighting);
            }
        } else if (biomeId == "mountain") {
            if (std::abs(k % 22) == 0) {
                drawWatchtower(fb, sx, sy, gameTime, lighting);
            } else if (typeSelector < 6) {
                int pineH = 150 + (seed % 55);
                drawPineTree(fb, sx, sy, pineH, lighting, std::abs(slope) > 0.3f);
            } else if (typeSelector < 10) {
                int bW = 50 + (seed % 35);
                int bH = 32 + (seed % 22);
                drawDesertBoulder(fb, sx, sy, bW, bH, lighting);
            } else if (typeSelector < 14) {
                drawCairn(fb, sx, sy, lighting);
            }
        } else if (biomeId == "moon") {
            if (std::abs(k % 20) == 0) {
                drawLunarLander(fb, sx, sy, lighting);
            } else if (typeSelector < 9) {
                int r = 18 + (seed % 16);
                drawLunarBoulder(fb, sx, sy, r, lighting);
            }
        } else if (biomeId == "volcano") {
            if (typeSelector < 5) {
                int colH = 70 + (seed % 45);
                drawBasaltColumn(fb, sx, sy, colH, lighting);
            } else if (typeSelector < 8) {
                int treeH = 115 + (seed % 40);
                drawDeadTree(fb, sx, sy, treeH, lighting);
            } else if (typeSelector < 12) {
                drawMagmaVent(fb, sx, sy, gameTime, lighting);
            }
        }
    }
}

void EnvironmentRenderer::renderAtmosphericParticles(Framebuffer& fb, const Camera& cam, const std::string& biomeId,
                                                    const DayNightCycle& cycle, const int* surfaceY, float gameTime) {
    int W = fb.width();
    int H = fb.height();
    if (W <= 0 || H <= 0) return;

    float camX = cam.x();
    LightingState lighting = cycle.getLighting(biomeId, W, H, camX);

    // 1. Fireflies in Countryside at Night
    if (lighting.fireflyOpacity > 0.05f) {
        int count = 22;
        for (int i = 0; i < count; ++i) {
            float px = std::fmod(static_cast<float>(i * 47) - camX * 0.12f + gameTime * 8.0f + W * 4.0f, static_cast<float>(W));
            int ix = static_cast<int>(px);
            int surfY = (surfaceY && ix >= 0 && ix < W) ? surfaceY[ix] : H / 2;

            float yWave = std::sin(gameTime * 3.2f + i * 1.5f) * 12.0f;
            int iy = surfY - 12 - static_cast<int>(yWave) - (i % 5) * 6;

            if (iy >= 0 && iy < H) {
                float pulse = 0.5f + 0.5f * std::sin(gameTime * 5.0f + i * 2.0f);
                uint32_t aVal = static_cast<uint32_t>(lighting.fireflyOpacity * pulse * 255.0f);
                uint32_t fireflyCore = (aVal << 24) | 0x00D4E157; // Glowing yellow-green
                fb.blendPixelFast(ix, iy, fireflyCore);
                fb.blendPixelFast(ix + 1, iy, (aVal / 2 << 24) | 0x00CDDC39);
            }
        }
    }

    // 2. Arctic Blizzard Falling Snowflakes
    if (biomeId == "arctic") {
        int count = 45;
        for (int i = 0; i < count; ++i) {
            float sx = std::fmod(static_cast<float>(i * 59 + 23) - camX * 0.05f + gameTime * 15.0f + W * 4.0f, static_cast<float>(W));
            float sy = std::fmod(static_cast<float>(i * 37 + 11) + gameTime * 35.0f, static_cast<float>(H));
            int ix = static_cast<int>(sx);
            int iy = static_cast<int>(sy);

            if (ix >= 0 && ix < W && iy >= 0 && iy < H) {
                fb.setPixelFast(ix, iy, 0xFFECEFF1);
                if (i % 3 == 0 && ix + 1 < W) fb.setPixelFast(ix + 1, iy, 0xFFFFFFFF);
            }
        }
    }

    // 3. Desert Drifting Dust Particles
    if (biomeId == "desert") {
        int count = 30;
        for (int i = 0; i < count; ++i) {
            float sx = std::fmod(static_cast<float>(i * 67 + 17) - camX * 0.08f + gameTime * 45.0f + W * 4.0f, static_cast<float>(W));
            float sy = std::fmod(static_cast<float>(i * 41 + 29) + std::sin(gameTime * 2.0f + i) * 6.0f, static_cast<float>(H));
            int ix = static_cast<int>(sx);
            int iy = static_cast<int>(sy);

            if (ix >= 0 && ix < W && iy >= 0 && iy < H) {
                fb.blendPixelFast(ix, iy, 0x66FFE082);
            }
        }
    }

    // 4. Volcanic Floating Embers
    if (biomeId == "volcano") {
        int count = 35;
        for (int i = 0; i < count; ++i) {
            float sx = std::fmod(static_cast<float>(i * 61 + 19) - camX * 0.04f + std::sin(gameTime * 3.0f + i) * 8.0f + W * 4.0f, static_cast<float>(W));
            float sy = std::fmod(H - (static_cast<float>(i * 31) + gameTime * 28.0f), static_cast<float>(H));
            if (sy < 0) sy += H;
            int ix = static_cast<int>(sx);
            int iy = static_cast<int>(sy);

            if (ix >= 0 && ix < W && iy >= 0 && iy < H) {
                uint32_t col = (i % 2 == 0) ? 0xFFFF5722 : 0xFFFFD54F;
                fb.setPixelFast(ix, iy, col);
            }
        }
    }
}

} // namespace Graphics
