#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "Vehicle.h"
#include "ProfileManager.h"
#include <QImage>

namespace Graphics {

/**
 * @brief Renders the authentic Hill Climber, wheels, and Bill Newton using pure software rasterization.
 */
class SpriteRenderer {
public:
    static void renderVehicle(Framebuffer& fb, const Camera& cam, const Physics::Vehicle& vehicle);
    static void renderGarageVehicle(Framebuffer& fb, int cx, int cy, float bounceY, const Core::ProfileManager& profile);
    static void renderGarageVehicle(Framebuffer& fb, int cx, int cy, float bounceY, const Core::ProfileManager& profile,
                                    Physics::VehicleType vType, Physics::DriverType dType, bool drawLift = true);

    static void blitRotated(Framebuffer& fb, const QImage& img, float targetX, float targetY,
                            float anchorX, float anchorY, float angleRad, float scale);

private:
    static void ensureLoaded();

    static QImage s_chassisImg;
    static QImage s_wheelImg;
    static QImage s_driverHeadImg;
    static bool s_loaded;
};

} // namespace Graphics
