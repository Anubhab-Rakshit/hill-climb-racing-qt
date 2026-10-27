#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "Terrain.h"
#include "DayNightCycle.h"
#include <string>
#include <vector>

namespace Graphics {

/**
 * @brief Rich, layered pixel-art environment rasterizer.
 * Provides multi-layer parallax mountain ranges, celestial bodies (Sun, Moon, Stars, Earth, Aurora),
 * procedural flora and terrain scenery (trees, cacti, bushes, fences, barns, boulders, landmarks),
 * animated wildlife (flocks of birds with flapping wings), and atmospheric weather particles.
 */
class EnvironmentRenderer {
public:
    EnvironmentRenderer();

    // 1. Sky, Celestial Bodies, Parallax Mountains & Distant Atmosphere
    void renderSkyAndParallax(Framebuffer& fb, const Camera& cam, const std::string& biomeId,
                             const DayNightCycle& cycle, const int* surfaceY, float gameTime);

    // 2. Foreground Procedural Scenery anchored to terrain (Trees, Cacti, Boulders, Fences, Landmarks)
    void renderForegroundScenery(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain,
                                const DayNightCycle& cycle, float gameTime);

    // 3. Foreground Atmospheric Particles & Wildlife (Fireflies, Butterflies, Snow, Embers, Dust)
    void renderAtmosphericParticles(Framebuffer& fb, const Camera& cam, const std::string& biomeId,
                                   const DayNightCycle& cycle, const int* surfaceY, float gameTime);

private:
    // Celestial drawing primitives
    void drawSun(Framebuffer& fb, int cx, int cy, float opacity, uint32_t sunColor, uint32_t glowColor);
    void drawMoon(Framebuffer& fb, int cx, int cy, float opacity, uint32_t moonColor);
    void drawEarth(Framebuffer& fb, int cx, int cy, float camX);
    void drawStars(Framebuffer& fb, int W, int H, float camX, float opacity, float gameTime);
    void drawAurora(Framebuffer& fb, int W, int H, float camX, float opacity, float gameTime);
    void drawClouds(Framebuffer& fb, int W, int H, float camX, const LightingState& lighting);

    // Wildlife
    void drawBirds(Framebuffer& fb, int W, int H, float camX, const std::string& biomeId, float gameTime, const LightingState& lighting);

    // Pixel-art flora & scenery primitives
    void drawOakTree(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting);
    void drawPineTree(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting, bool snowCovered = false);
    void drawBush(Framebuffer& fb, int sx, int sy, int r, const LightingState& lighting, uint32_t flowerColor = 0);
    void drawFence(Framebuffer& fb, int sx1, int sy1, int sx2, int sy2, const LightingState& lighting);
    void drawBarn(Framebuffer& fb, int sx, int sy, const LightingState& lighting, const Physics::Terrain& terrain, const Camera& cam);
    void drawWindmill(Framebuffer& fb, int sx, int sy, float gameTime, const LightingState& lighting);

    void drawSaguaroCactus(Framebuffer& fb, int sx, int sy, int h, int arms, const LightingState& lighting);
    void drawPricklyPear(Framebuffer& fb, int sx, int sy, const LightingState& lighting);
    void drawDesertBoulder(Framebuffer& fb, int sx, int sy, int w, int h, const LightingState& lighting);
    void drawDesertArch(Framebuffer& fb, int sx, int sy, const LightingState& lighting);

    void drawIceSpire(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting);
    void drawResearchDome(Framebuffer& fb, int sx, int sy, const LightingState& lighting);

    void drawLunarLander(Framebuffer& fb, int sx, int sy, const LightingState& lighting);
    void drawLunarBoulder(Framebuffer& fb, int sx, int sy, int r, const LightingState& lighting);

    void drawBasaltColumn(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting);
    void drawMagmaVent(Framebuffer& fb, int sx, int sy, float gameTime, const LightingState& lighting);
    void drawDeadTree(Framebuffer& fb, int sx, int sy, int h, const LightingState& lighting);
    void drawCairn(Framebuffer& fb, int sx, int sy, const LightingState& lighting);
    void drawWatchtower(Framebuffer& fb, int sx, int sy, float gameTime, const LightingState& lighting);

    // Precomputed star table
    struct StarData {
        float xNorm;
        float yNorm;
        uint32_t color;
        int size;
        float twinkleSpeed;
        float twinklePhase;
    };
    std::vector<StarData> m_stars;
    void initStars();
};

} // namespace Graphics
