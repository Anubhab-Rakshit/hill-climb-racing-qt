#include "Sprite.h"
#include "VehicleSprites.h"
#include "NetworkTypes.h"
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

    // 4 rotated corners relative to anchor in screen pixels
    float c0x = -anchorX * scale;
    float c0y = -anchorY * scale;
    float c1x = (imgW - anchorX) * scale;
    float c1y = -anchorY * scale;
    float c2x = -anchorX * scale;
    float c2y = (imgH - anchorY) * scale;
    float c3x = (imgW - anchorX) * scale;
    float c3y = (imgH - anchorY) * scale;

    auto rotX = [&](float lx, float ly) { return targetX + (lx * cosA - ly * sinA); };
    auto rotY = [&](float lx, float ly) { return targetY + (lx * sinA + ly * cosA); };

    float r0x = rotX(c0x, c0y), r0y = rotY(c0x, c0y);
    float r1x = rotX(c1x, c1y), r1y = rotY(c1x, c1y);
    float r2x = rotX(c2x, c2y), r2y = rotY(c2x, c2y);
    float r3x = rotX(c3x, c3y), r3y = rotY(c3x, c3y);

    int minPx = std::max(0, static_cast<int>(std::floor(std::min({r0x, r1x, r2x, r3x}))) - 1);
    int maxPx = std::min(fb.width() - 1, static_cast<int>(std::ceil(std::max({r0x, r1x, r2x, r3x}))) + 1);
    int minPy = std::max(0, static_cast<int>(std::floor(std::min({r0y, r1y, r2y, r3y}))) - 1);
    int maxPy = std::min(fb.height() - 1, static_cast<int>(std::ceil(std::max({r0y, r1y, r2y, r3y}))) + 1);

    if (minPx > maxPx || minPy > maxPy) return;

    float drx = cosA * invScale;
    float dry = -sinA * invScale;

    const uint32_t* srcBits = reinterpret_cast<const uint32_t*>(img.constBits());
    int srcStride = img.bytesPerLine() / static_cast<int>(sizeof(uint32_t));

    for (int py = minPy; py <= maxPy; ++py) {
        float dy = py - targetY;
        float dx0 = minPx - targetX;

        float rx = (dx0 * cosA + dy * sinA) * invScale + anchorX;
        float ry = (-dx0 * sinA + dy * cosA) * invScale + anchorY;

        uint32_t* dstLine = fb.scanLine(py);

        for (int px = minPx; px <= maxPx; ++px, rx += drx, ry += dry) {
            int u = static_cast<int>(rx);
            int v = static_cast<int>(ry);

            if (static_cast<unsigned>(u) < static_cast<unsigned>(imgW) &&
                static_cast<unsigned>(v) < static_cast<unsigned>(imgH)) {
                uint32_t pixel = srcBits[v * srcStride + u];
                uint32_t sa = (pixel >> 24) & 0xFF;
                if (sa == 255) {
                    dstLine[px] = pixel;
                } else if (sa > 0) {
                    uint32_t sr = (pixel >> 16) & 0xFF;
                    uint32_t sg = (pixel >> 8) & 0xFF;
                    uint32_t sb = pixel & 0xFF;
                    uint32_t invA = 255 - sa;
                    dstLine[px] = Framebuffer::blendOpaqueDst(dstLine[px], sr, sg, sb, sa, invA);
                }
            }
        }
    }
}

void SpriteRenderer::renderVehicle(Framebuffer& fb, const Camera& cam, const Physics::Vehicle& vehicle, float alpha) {
    ensureLoaded();

    const auto& config = vehicle.config();
    const QImage& chassisImg = VehicleSprites::getChassisSprite(vehicle.vehicleType());
    const QImage& wheelImg = VehicleSprites::getWheelSprite(vehicle.vehicleType());
    const QImage& driverImg = VehicleSprites::getDriverSprite(vehicle.driverType());

    float zoom = cam.zoom();
    // 80.25 sprite pixels per physical meter
    float spriteScale = zoom / 80.25f;

    // 1. Chassis Screen Position & Rotation (Interpolated)
    Physics::Vec2 cPos = vehicle.interpolatedChassisPos(alpha);
    float angle = vehicle.interpolatedChassisAngle(alpha);
    Physics::Vec2 chassisScr = cam.worldToScreen(cPos);
    float screenChassisAngle = -angle; // Screen Y is inverted

    // Helper: local chassis world offset (lx, ly) to world coordinates
    auto chassisPointToWorld = [&](const Physics::Vec2& local) -> Physics::Vec2 {
        float cosW = std::cos(angle);
        float sinW = std::sin(angle);
        return {cPos.x + (local.x * cosW - local.y * sinW),
                cPos.y + (local.x * sinW + local.y * cosW)};
    };

    // 2. Suspension Mount Points & Wheel Positions (Interpolated)
    Physics::Vec2 rearMountScr = cam.worldToScreen(chassisPointToWorld(vehicle.rearMountOffset()));
    Physics::Vec2 frontMountScr = cam.worldToScreen(chassisPointToWorld(vehicle.frontMountOffset()));

    Physics::Vec2 rearWheelPos = vehicle.interpolatedRearWheelPos(alpha);
    Physics::Vec2 frontWheelPos = vehicle.interpolatedFrontWheelPos(alpha);
    Physics::Vec2 rearWheelScr = cam.worldToScreen(rearWheelPos);
    Physics::Vec2 frontWheelScr = cam.worldToScreen(frontWheelPos);

    // 3. Render High-Performance Mechanical Suspension Struts with Coiled Springs & Brake Calipers
    auto drawStrut = [&](const Physics::Vec2& m, const Physics::Vec2& w) {
        float dx = w.x - m.x;
        float dy = w.y - m.y;
        float len = std::hypot(dx, dy);
        if (len < 1.0f) return;

        float ux = dx / len;
        float uy = dy / len;
        float nx = -uy;
        float ny = ux;

        int x0 = static_cast<int>(m.x);
        int y0 = static_cast<int>(m.y);
        int x1 = static_cast<int>(w.x);
        int y1 = static_cast<int>(w.y);

        // A. Hydraulic Damper Shaft (Chrome center rod flanked by shadow & highlight)
        fb.drawLine(x0 - static_cast<int>(nx), y0 - static_cast<int>(ny),
                    x1 - static_cast<int>(nx), y1 - static_cast<int>(ny), 0xFF1C252B);
        fb.drawLine(x0, y0, x1, y1, 0xFFECEFF1);
        fb.drawLine(x0 + static_cast<int>(nx), y0 + static_cast<int>(ny),
                    x1 + static_cast<int>(nx), y1 + static_cast<int>(ny), 0xFF78909C);

        // B. Coiled Heavy-Duty Suspension Spring (6 coils zigzagging across middle 65% of strut)
        const int numCoils = 6;
        float coilWidth = 4.5f;
        for (int c = 0; c < numCoils; ++c) {
            float tA = 0.18f + (c / static_cast<float>(numCoils)) * 0.65f;
            float tB = 0.18f + ((c + 0.5f) / static_cast<float>(numCoils)) * 0.65f;
            float tC = 0.18f + ((c + 1.0f) / static_cast<float>(numCoils)) * 0.65f;

            Physics::Vec2 pA = m + Physics::Vec2(ux * len * tA + nx * coilWidth, uy * len * tA + ny * coilWidth);
            Physics::Vec2 pB = m + Physics::Vec2(ux * len * tB - nx * coilWidth, uy * len * tB - ny * coilWidth);
            Physics::Vec2 pC = m + Physics::Vec2(ux * len * tC + nx * coilWidth, uy * len * tC + ny * coilWidth);

            fb.drawLine(static_cast<int>(pA.x), static_cast<int>(pA.y),
                        static_cast<int>(pB.x), static_cast<int>(pB.y), 0xFFFF1744); // Racing Red coil
            fb.drawLine(static_cast<int>(pB.x), static_cast<int>(pB.y),
                        static_cast<int>(pC.x), static_cast<int>(pC.y), 0xFFFF5252); // Coil highlight
        }

        // C. Disc Brake Rotor & Racing Caliper at Hub
        fb.fillCircle(x1, y1, 7, 0xFF90A4AE); // Steel Brake Disc
        fb.drawCircle(x1, y1, 7, 0xFF455A64);
        fb.fillRect(x1 - 3, y1 - 8, 7, 5, 0xFFD50000); // Brembo Red Racing Caliper
        fb.drawRect(x1 - 3, y1 - 8, 7, 5, 0xFFFF5252);
    };
    drawStrut(rearMountScr, rearWheelScr);
    drawStrut(frontMountScr, frontWheelScr);

    // 4. Render Wheels (Rotating with physical wheel angle, strictly anchored to struts)
    float wheelScale = (vehicle.rearWheelRadius() * zoom) / config.wheelSpriteRadius;
    float wAnchorX = wheelImg.width() * 0.5f;
    float wAnchorY = wheelImg.height() * 0.5f;
    blitRotated(fb, wheelImg, rearWheelScr.x, rearWheelScr.y, wAnchorX, wAnchorY, vehicle.interpolatedRearWheelAngle(alpha), wheelScale);
    blitRotated(fb, wheelImg, frontWheelScr.x, frontWheelScr.y, wAnchorX, wAnchorY, vehicle.interpolatedFrontWheelAngle(alpha), wheelScale);

    // 5. Render Driver with Physics Inertia (if not already integrated into bespoke vehicle asset)
    if (!config.hasIntegratedDriver) {
        Physics::Vec2 driverHeadScr = cam.worldToScreen(vehicle.interpolatedDriverHeadPos(alpha));
        float driverHeadScreenAngle = screenChassisAngle + vehicle.interpolatedDriverHeadAngle(alpha);
        float driverScale = spriteScale * 0.52f;
        float dAnchorX = driverImg.width() * 0.5f;
        float dAnchorY = driverImg.height() * 0.88f;
        blitRotated(fb, driverImg, driverHeadScr.x, driverHeadScr.y, dAnchorX, dAnchorY, driverHeadScreenAngle, driverScale);
    }

    // 6. Render Pixel-Art Chassis Body (Drawn on top so wheels sit inside wheel arches)
    blitRotated(fb, chassisImg, chassisScr.x, chassisScr.y, config.spriteAnchor.x, config.spriteAnchor.y, screenChassisAngle, spriteScale);
}

void SpriteRenderer::renderRemoteVehicle(Framebuffer& fb, const Camera& cam, const Network::RemotePlayerState& state) {
    ensureLoaded();

    Physics::VehicleType vType = static_cast<Physics::VehicleType>(state.vehicleType);
    const auto& config = Physics::VehicleRegistry::getConfig(vType);
    const QImage& chassisImg = VehicleSprites::getChassisSprite(vType);
    const QImage& wheelImg = VehicleSprites::getWheelSprite(vType);

    float zoom = cam.zoom();
    float spriteScale = zoom / 80.25f;

    Physics::Vec2 cPos(state.posX, state.posY);
    float angle = state.chassisAngle;
    Physics::Vec2 chassisScr = cam.worldToScreen(cPos);
    float screenChassisAngle = -angle;

    auto chassisPointToWorld = [&](const Physics::Vec2& local) -> Physics::Vec2 {
        float cosW = std::cos(angle);
        float sinW = std::sin(angle);
        return {cPos.x + (local.x * cosW - local.y * sinW),
                cPos.y + (local.x * sinW + local.y * cosW)};
    };

    Physics::Vec2 rearMountScr = cam.worldToScreen(chassisPointToWorld(config.rearMountOffset));
    Physics::Vec2 frontMountScr = cam.worldToScreen(chassisPointToWorld(config.frontMountOffset));

    Physics::Vec2 rearWheelScr(rearMountScr.x, rearMountScr.y + state.rearSuspLen * zoom);
    Physics::Vec2 frontWheelScr(frontMountScr.x, frontMountScr.y + state.frontSuspLen * zoom);

    // Colored Under-glow Aura in player's custom color!
    fb.fillCircle(static_cast<int>(chassisScr.x), static_cast<int>(chassisScr.y + 12), 22, (state.color & 0x00FFFFFF) | 0x44000000);

    // Draw suspension struts
    fb.drawLine(static_cast<int>(rearMountScr.x), static_cast<int>(rearMountScr.y),
                static_cast<int>(rearWheelScr.x), static_cast<int>(rearWheelScr.y), 0xFFB0BEC5);
    fb.drawLine(static_cast<int>(frontMountScr.x), static_cast<int>(frontMountScr.y),
                static_cast<int>(frontWheelScr.x), static_cast<int>(frontWheelScr.y), 0xFFB0BEC5);

    // Render Wheels
    float wheelScale = (config.wheelRadius * zoom) / config.wheelSpriteRadius;
    float wheelAnchor = wheelImg.width() * 0.5f;
    blitRotated(fb, wheelImg, rearWheelScr.x, rearWheelScr.y, wheelAnchor, wheelAnchor, -state.rearWheelAngle, wheelScale);
    blitRotated(fb, wheelImg, frontWheelScr.x, frontWheelScr.y, wheelAnchor, wheelAnchor, -state.frontWheelAngle, wheelScale);

    // Render Chassis
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

    // Shock struts with coil-over springs and brake discs
    auto drawGarageStrut = [&](float mx, float my, float wx, float wy) {
        float dx = wx - mx;
        float dy = wy - my;
        float len = std::hypot(dx, dy);
        if (len < 1.0f) return;
        float ux = dx / len, uy = dy / len;
        float nx = -uy, ny = ux;

        int x0 = static_cast<int>(mx), y0 = static_cast<int>(my);
        int x1 = static_cast<int>(wx), y1 = static_cast<int>(wy);

        fb.drawLine(x0, y0, x1, y1, 0xFFECEFF1);
        fb.drawLine(x0 - 1, y0, x1 - 1, y1, 0xFF263238);

        // Coiled spring
        for (int c = 0; c < 5; ++c) {
            float tA = 0.20f + (c / 5.0f) * 0.60f;
            float tB = 0.20f + ((c + 0.5f) / 5.0f) * 0.60f;
            float tC = 0.20f + ((c + 1.0f) / 5.0f) * 0.60f;
            float cw = 3.5f;

            float pAx = mx + ux * len * tA + nx * cw;
            float pAy = my + uy * len * tA + ny * cw;
            float pBx = mx + ux * len * tB - nx * cw;
            float pBy = my + uy * len * tB - ny * cw;
            float pCx = mx + ux * len * tC + nx * cw;
            float pCy = my + uy * len * tC + ny * cw;

            fb.drawLine(static_cast<int>(pAx), static_cast<int>(pAy), static_cast<int>(pBx), static_cast<int>(pBy), 0xFFFF1744);
            fb.drawLine(static_cast<int>(pBx), static_cast<int>(pBy), static_cast<int>(pCx), static_cast<int>(pCy), 0xFFFF5252);
        }

        // Disc brake rotor & caliper
        fb.fillCircle(x1, y1, 6, 0xFF90A4AE);
        fb.fillRect(x1 - 2, y1 - 7, 5, 4, 0xFFD50000);
    };

    float rearMountY = chassisCenterY - config.rearMountOffset.y * 80.25f * garageScale;
    float frontMountY = chassisCenterY - config.frontMountOffset.y * 80.25f * garageScale;
    drawGarageStrut(rearWheelX, rearMountY, rearWheelX, wheelCenterY);
    drawGarageStrut(frontWheelX, frontMountY, frontWheelX, wheelCenterY);

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
