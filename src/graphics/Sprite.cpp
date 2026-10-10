#include "Sprite.h"
#include "VehicleSprites.h"
#include "UITheme.h"
#include <cmath>
#include <algorithm>

namespace Graphics {

QImage SpriteRenderer::s_chassisImg;
QImage SpriteRenderer::s_wheelImg;
QImage SpriteRenderer::s_driverHeadImg;
bool SpriteRenderer::s_loaded = false;

void SpriteRenderer::ensureLoaded() {
    if (s_loaded) return;
    VehicleSprites::init();
    s_loaded = true;
}

void SpriteRenderer::blitRotated(Framebuffer& fb, const QImage& img, float targetX, float targetY,
                                float anchorX, float anchorY, float angleRad, float scale) {
    if (img.isNull() || scale <= 0.001f) return;

    float invScale = 1.0f / scale;
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);

    int imgW = img.width();
    int imgH = img.height();

    // Bounding radius from anchor to image corners
    float d1 = std::hypot(anchorX, anchorY);
    float d2 = std::hypot(imgW - anchorX, anchorY);
    float d3 = std::hypot(anchorX, imgH - anchorY);
    float d4 = std::hypot(imgW - anchorX, imgH - anchorY);
    float maxDist = std::max({d1, d2, d3, d4}) * scale + 2.0f;

    int minPx = std::max(0, static_cast<int>(targetX - maxDist));
    int maxPx = std::min(fb.width() - 1, static_cast<int>(targetX + maxDist));
    int minPy = std::max(0, static_cast<int>(targetY - maxDist));
    int maxPy = std::min(fb.height() - 1, static_cast<int>(targetY + maxDist));

    if (minPx > maxPx || minPy > maxPy) return;

    for (int py = minPy; py <= maxPy; ++py) {
        float dy = py - targetY;
        for (int px = minPx; px <= maxPx; ++px) {
            float dx = px - targetX;

            // Invert screen rotation to find source pixel in sprite space
            float rx = (dx * cosA + dy * sinA) * invScale;
            float ry = (-dx * sinA + dy * cosA) * invScale;

            int u = static_cast<int>(std::round(rx + anchorX));
            int v = static_cast<int>(std::round(ry + anchorY));

            if (u >= 0 && u < imgW && v >= 0 && v < imgH) {
                const uint32_t* line = reinterpret_cast<const uint32_t*>(img.scanLine(v));
                uint32_t pixel = line[u];
                uint32_t alpha = (pixel >> 24) & 0xFF;
                if (alpha > 0) {
                    fb.blendPixelFast(px, py, pixel);
                }
            }
        }
    }
}

void SpriteRenderer::renderVehicle(Framebuffer& fb, const Camera& cam, const Physics::Vehicle& vehicle) {
    ensureLoaded();

    const auto& config = vehicle.config();
    const QImage& chassisImg = VehicleSprites::getChassisSprite(vehicle.vehicleType());
    const QImage& wheelImg = VehicleSprites::getWheelSprite(vehicle.vehicleType());
    const QImage& driverImg = VehicleSprites::getDriverSprite(vehicle.driverType());

    float zoom = cam.zoom();
    // 80.25 sprite pixels per physical meter
    float spriteScale = zoom / 80.25f;

    // 1. Chassis Screen Position & Rotation
    Physics::Vec2 chassisScr = cam.worldToScreen(vehicle.chassisPos());
    float angle = vehicle.chassisAngle();
    float screenChassisAngle = -angle; // Screen Y is inverted

    // Helper: local chassis world offset (lx, ly) to world coordinates
    auto chassisPointToWorld = [&](const Physics::Vec2& local) -> Physics::Vec2 {
        float cosW = std::cos(angle);
        float sinW = std::sin(angle);
        return {vehicle.chassisPos().x + (local.x * cosW - local.y * sinW),
                vehicle.chassisPos().y + (local.x * sinW + local.y * cosW)};
    };

    // 2. Suspension Mount Points & Wheel Positions
    Physics::Vec2 rearMountScr = cam.worldToScreen(chassisPointToWorld(vehicle.rearMountOffset()));
    Physics::Vec2 frontMountScr = cam.worldToScreen(chassisPointToWorld(vehicle.frontMountOffset()));

    Physics::Vec2 rearWheelScr = cam.worldToScreen(vehicle.rearWheelPos());
    Physics::Vec2 frontWheelScr = cam.worldToScreen(vehicle.frontWheelPos());

    // 3. Render Suspension Struts (Clean shock absorbers behind the wheels)
    auto drawStrut = [&](const Physics::Vec2& m, const Physics::Vec2& w) {
        int x0 = static_cast<int>(m.x);
        int y0 = static_cast<int>(m.y);
        int x1 = static_cast<int>(w.x);
        int y1 = static_cast<int>(w.y);
        fb.drawLine(x0 - 1, y0, x1 - 1, y1, 0xFF1C252B);
        fb.drawLine(x0, y0, x1, y1, 0xFFCFD8DC);
        fb.drawLine(x0 + 1, y0, x1 + 1, y1, 0xFF78909C);
    };
    drawStrut(rearMountScr, rearWheelScr);
    drawStrut(frontMountScr, frontWheelScr);

    // 4. Render Wheels (Rotating with physical wheel angle, strictly anchored to struts)
    float wheelScale = (vehicle.rearWheelRadius() * zoom) / config.wheelSpriteRadius;
    float wAnchorX = wheelImg.width() * 0.5f;
    float wAnchorY = wheelImg.height() * 0.5f;
    blitRotated(fb, wheelImg, rearWheelScr.x, rearWheelScr.y, wAnchorX, wAnchorY, vehicle.rearWheelAngle(), wheelScale);
    blitRotated(fb, wheelImg, frontWheelScr.x, frontWheelScr.y, wAnchorX, wAnchorY, vehicle.frontWheelAngle(), wheelScale);

    // 5. Render Driver with Physics Inertia (if not already integrated into bespoke vehicle asset)
    if (!config.hasIntegratedDriver) {
        Physics::Vec2 driverHeadScr = cam.worldToScreen(vehicle.driverHeadPos());
        float driverHeadScreenAngle = screenChassisAngle + vehicle.driverHeadAngle();
        float driverScale = spriteScale * 0.52f;
        float dAnchorX = driverImg.width() * 0.5f;
        float dAnchorY = driverImg.height() * 0.88f;
        blitRotated(fb, driverImg, driverHeadScr.x, driverHeadScr.y, dAnchorX, dAnchorY, driverHeadScreenAngle, driverScale);
    }

    // 6. Render Pixel-Art Chassis Body (Drawn on top so wheels sit inside wheel arches)
    blitRotated(fb, chassisImg, chassisScr.x, chassisScr.y, config.spriteAnchor.x, config.spriteAnchor.y, screenChassisAngle, spriteScale);
}

void SpriteRenderer::renderGarageVehicle(Framebuffer& fb, int cx, int cy, float bounceY, const Core::ProfileManager& profile) {
    renderGarageVehicle(fb, cx, cy, bounceY, profile, profile.selectedVehicle(), profile.selectedDriver());
}

void SpriteRenderer::renderGarageVehicle(Framebuffer& fb, int cx, int cy, float bounceY, const Core::ProfileManager& profile,
                                        Physics::VehicleType vType, Physics::DriverType dType, bool drawLift) {
    ensureLoaded();

    const auto& config = Physics::VehicleRegistry::getConfig(vType);
    const QImage& chassisImg = VehicleSprites::getChassisSprite(vType);
    const QImage& wheelImg = VehicleSprites::getWheelSprite(vType);
    const QImage& driverImg = VehicleSprites::getDriverSprite(dType);

    float garageScale = 0.72f;
    float liftY = cy + 46.0f;

    // 1. Wheelbase and Wheel Positions
    float rearWheelX = cx + config.rearMountOffset.x * 80.25f * garageScale;
    float frontWheelX = cx + config.frontMountOffset.x * 80.25f * garageScale;

    // Hydraulic Lift Platform (Optional, only drawn in Garage)
    if (drawLift) {
        int liftW = static_cast<int>((frontWheelX - rearWheelX) + 110.0f);
        int liftH = 14;
        int liftX = cx - liftW / 2;
        fb.fillRect(liftX, static_cast<int>(liftY), liftW, liftH, 0xFF212B36);
        fb.drawRect(liftX, static_cast<int>(liftY), liftW, liftH, 0xFF455A64);

        // Yellow Hazard Stripes
        for (int s = liftX + 6; s < liftX + liftW - 12; s += 16) {
            fb.fillRect(s, static_cast<int>(liftY + 2), 8, liftH - 4, UI::Theme::GOLD);
        }
        // Lift hydraulic posts
        fb.fillRect(cx - 75, static_cast<int>(liftY + liftH), 16, 35, 0xFF37474F);
        fb.fillRect(cx + 59, static_cast<int>(liftY + liftH), 16, 35, 0xFF37474F);
    }

    // 2. Wheels Resting on Lift
    int tireLvl = profile.getUpgradeLevel(vType, Core::ProfileManager::UPGRADE_TIRES);
    float tireUpgrMult = 1.0f + std::min(0.18f, (tireLvl - 1) * 0.012f);
    float wheelScale = garageScale * tireUpgrMult;
    float wheelRadiusPx = config.wheelSpriteRadius * wheelScale;
    float wheelCenterY = liftY - wheelRadiusPx;

    // 3. Chassis on Suspension (bounces playfully when clicked or upgraded!)
    float suspHeightPx = config.suspRestLength * 80.25f * garageScale;
    float chassisCenterY = wheelCenterY - suspHeightPx + bounceY;

    // Shock struts
    float rearMountY = chassisCenterY - config.rearMountOffset.y * 80.25f * garageScale;
    float frontMountY = chassisCenterY - config.frontMountOffset.y * 80.25f * garageScale;
    fb.drawLine(static_cast<int>(rearWheelX), static_cast<int>(rearMountY),
                static_cast<int>(rearWheelX), static_cast<int>(wheelCenterY), 0xFFCFD8DC);
    fb.drawLine(static_cast<int>(frontWheelX), static_cast<int>(frontMountY),
                static_cast<int>(frontWheelX), static_cast<int>(wheelCenterY), 0xFFCFD8DC);

    // Wheels
    float wAnchorX = wheelImg.width() * 0.5f;
    float wAnchorY = wheelImg.height() * 0.5f;
    blitRotated(fb, wheelImg, rearWheelX, wheelCenterY, wAnchorX, wAnchorY, 0.0f, wheelScale);
    blitRotated(fb, wheelImg, frontWheelX, wheelCenterY, wAnchorX, wAnchorY, 0.0f, wheelScale);

    // Driver in Cockpit (if not already integrated into bespoke vehicle asset)
    if (!config.hasIntegratedDriver) {
        float driverX = cx + config.driverSeatOffset.x * 80.25f * garageScale;
        float driverY = chassisCenterY - config.driverSeatOffset.y * 80.25f * garageScale;
        float driverScale = garageScale * 0.52f;
        float dAnchorX = driverImg.width() * 0.5f;
        float dAnchorY = driverImg.height() * 0.88f;
        blitRotated(fb, driverImg, driverX, driverY, dAnchorX, dAnchorY, bounceY * 0.035f, driverScale);
    }

    // Chassis
    blitRotated(fb, chassisImg, cx, chassisCenterY, config.spriteAnchor.x, config.spriteAnchor.y, 0.0f, garageScale);
}

} // namespace Graphics
