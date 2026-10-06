#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "Terrain.h"

namespace Graphics {

/**
 * @brief Ultra-Fast Vertical Column-Scanline Terrain Rasterizer.
 * Renders undulating hills, multi-layer parallax sky, and collectible items using pure pixel algorithms.
 */
class ScanlineRasterizer {
public:
    ScanlineRasterizer();

    void render(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime);

private:
    void renderParallaxSky(Framebuffer& fb, const Camera& cam, const std::string& biomeId);
    void renderItems(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime);
    void renderMilestones(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain);
};

} // namespace Graphics
