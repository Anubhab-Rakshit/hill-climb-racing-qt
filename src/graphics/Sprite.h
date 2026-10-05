#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "Vehicle.h"

namespace Graphics {

/**
 * @brief Renders the physical multi-body vehicle, wheels, and ragdoll driver in pure software pixels.
 */
class SpriteRenderer {
public:
    static void renderVehicle(Framebuffer& fb, const Camera& cam, const Physics::Vehicle& vehicle);

private:
    static void drawRotatedWheel(Framebuffer& fb, int cx, int cy, int radius, float angleRad);
};

} // namespace Graphics
