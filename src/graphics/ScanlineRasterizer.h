#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "Terrain.h"
#include "DayNightCycle.h"
#include "EnvironmentRenderer.h"

namespace Graphics {

/**
 * @brief Ultra-Fast Vertical Column-Scanline Terrain Rasterizer.
 * Renders undulating hills, multi-layer parallax sky, procedural scenery, day-night cycle, and collectible items using pure pixel algorithms.
 */
class ScanlineRasterizer {
public:
    ScanlineRasterizer();

    void render(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime);

    DayNightCycle& dayNightCycle() { return m_dayNightCycle; }
    const DayNightCycle& dayNightCycle() const { return m_dayNightCycle; }
    EnvironmentRenderer& envRenderer() { return m_envRenderer; }
    const EnvironmentRenderer& envRenderer() const { return m_envRenderer; }

private:
    void renderItems(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime);
    void renderMilestones(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, const LightingState& lighting);

    DayNightCycle m_dayNightCycle;
    EnvironmentRenderer m_envRenderer;
};

} // namespace Graphics
