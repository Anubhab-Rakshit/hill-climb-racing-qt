#include "Sprite.h"
#include "CarSpritesData.h"
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

    s_chassisImg.loadFromData(Assets::s_chassisPng, Assets::s_chassisPng_len);
    s_wheelImg.loadFromData(Assets::s_wheelPng, Assets::s_wheelPng_len);
    s_driverHeadImg.loadFromData(Assets::s_driverHeadPng, Assets::s_driverHeadPng_len);

    s_chassisImg = s_chassisImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    s_wheelImg = s_wheelImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    s_driverHeadImg = s_driverHeadImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);

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

    float zoom = cam.zoom();
    // 80.25 sprite pixels per physical meter
    float spriteScale = zoom / 80.25f;

    // 1. Chassis Screen Position & Rotation
    Physics::Vec2 chassisScr = cam.worldToScreen(vehicle.chassisPos());
    float angle = vehicle.chassisAngle();
    float screenChassisAngle = -angle; // Screen Y is inverted
    float cosA = std::cos(screenChassisAngle);
    float sinA = std::sin(screenChassisAngle);

    // Helper: local chassis world offset (lx, ly) to world coordinates
    auto chassisPointToWorld = [&](float lx, float ly) -> Physics::Vec2 {
        float cosW = std::cos(angle);
        float sinW = std::sin(angle);
        return {vehicle.chassisPos().x + (lx * cosW - ly * sinW),
                vehicle.chassisPos().y + (lx * sinW + ly * cosW)};
    };

    // 2. Suspension Mount Points on Chassis (matching wheel well centers)
    Physics::Vec2 rearMountScr = cam.worldToScreen(chassisPointToWorld(-0.78f, -0.06f));
    Physics::Vec2 frontMountScr = cam.worldToScreen(chassisPointToWorld(+0.78f, -0.06f));

    Physics::Vec2 rearWheelScr = cam.worldToScreen(vehicle.rearWheelPos());
    Physics::Vec2 frontWheelScr = cam.worldToScreen(vehicle.frontWheelPos());

    // Safety fallback: ensure wheels stay strictly anchored to their struts
    float maxStrutPx = 0.60f * zoom;
    float minStrutPx = 0.10f * zoom;
    if (std::isnan(rearWheelScr.x) || (rearWheelScr - rearMountScr).length() > maxStrutPx || (rearWheelScr - rearMountScr).length() < minStrutPx) {
        rearWheelScr = rearMountScr + Physics::Vec2(-sinA, cosA) * (0.28f * zoom);
    }
    if (std::isnan(frontWheelScr.x) || (frontWheelScr - frontMountScr).length() > maxStrutPx || (frontWheelScr - frontMountScr).length() < minStrutPx) {
        frontWheelScr = frontMountScr + Physics::Vec2(-sinA, cosA) * (0.28f * zoom);
    }

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

    // 4. Render Authentic Wheels (Inside wheel wells, rotating with physical wheel angle)
    // Wheel sprite is 74x72, anchor at center (37, 36)
    float wheelScale = spriteScale * (44.0f / 74.0f);
    blitRotated(fb, s_wheelImg, rearWheelScr.x, rearWheelScr.y, 37.0f, 36.0f, vehicle.rearWheelAngle(), wheelScale);
    blitRotated(fb, s_wheelImg, frontWheelScr.x, frontWheelScr.y, 37.0f, 36.0f, vehicle.frontWheelAngle(), wheelScale);

    // 5. Render Driver (Bill Newton with his red backwards cap and scruffy chin)
    // Driver position in cockpit: (-0.27m, +0.38m) relative to chassis center
    Physics::Vec2 driverHeadScr = cam.worldToScreen(chassisPointToWorld(-0.27f, +0.38f));
    // Bill's head sways with g-forces and terrain jumps
    float driverHeadScreenAngle = screenChassisAngle + vehicle.driverHeadAngle();
    float driverScale = spriteScale * (44.0f / 95.0f);
    // Anchor at neck base of driver sprite: (45, 85)
    blitRotated(fb, s_driverHeadImg, driverHeadScr.x, driverHeadScr.y, 45.0f, 85.0f, driverHeadScreenAngle, driverScale);

    // 6. Render Authentic Red Jeep Chassis Body (Drawn on top so wheels sit inside wheel arches!)
    // Anchor at chassis center: (126.4, 70.0)
    blitRotated(fb, s_chassisImg, chassisScr.x, chassisScr.y, 126.4f, 70.0f, screenChassisAngle, spriteScale);
}

void SpriteRenderer::renderGarageVehicle(Framebuffer& fb, int cx, int cy, float bounceY, const Core::ProfileManager& profile) {
    ensureLoaded();

    float scale = 0.68f; // Prominent crisp garage scale
    float liftY = cy + 45;

    // 1. Hydraulic Garage Lift Platform
    int liftW = 230;
    int liftH = 14;
    int liftX = cx - liftW / 2;
    fb.fillRect(liftX, static_cast<int>(liftY), liftW, liftH, 0xFF212B36);
    fb.drawRect(liftX, static_cast<int>(liftY), liftW, liftH, 0xFF455A64);

    // Yellow Hazard Stripes
    for (int s = liftX + 6; s < liftX + liftW - 12; s += 16) {
        fb.fillRect(s, static_cast<int>(liftY + 2), 8, liftH - 4, UI::Theme::GOLD);
    }
    fb.fillRect(cx - 75, static_cast<int>(liftY + liftH), 16, 35, 0xFF37474F);
    fb.fillRect(cx + 59, static_cast<int>(liftY + liftH), 16, 35, 0xFF37474F);

    // 2. Wheels Resting on Lift
    int tireLvl = profile.getUpgradeLevel(Core::ProfileManager::UPGRADE_TIRES);
    float tireUpgrMult = 1.0f + std::min(0.18f, tireLvl * 0.015f);
    float wheelScale = scale * (44.0f / 74.0f) * tireUpgrMult;
    float wheelRadiusPx = 22.0f * tireUpgrMult * scale;
    float wheelCenterY = liftY - wheelRadiusPx;

    float rearWheelX = cx - 62.6f * scale;
    float frontWheelX = cx + 62.6f * scale;

    // 3. Chassis on Suspension (bounces playfully when clicked!)
    float chassisCenterY = wheelCenterY - (26.1f * scale) + bounceY;

    // Shock struts
    fb.drawLine(static_cast<int>(rearWheelX), static_cast<int>(chassisCenterY + 26.1f * scale),
                static_cast<int>(rearWheelX), static_cast<int>(wheelCenterY), 0xFFCFD8DC);
    fb.drawLine(static_cast<int>(frontWheelX), static_cast<int>(chassisCenterY + 26.1f * scale),
                static_cast<int>(frontWheelX), static_cast<int>(wheelCenterY), 0xFFCFD8DC);

    // Wheels
    blitRotated(fb, s_wheelImg, rearWheelX, wheelCenterY, 37.0f, 36.0f, 0.0f, wheelScale);
    blitRotated(fb, s_wheelImg, frontWheelX, wheelCenterY, 37.0f, 36.0f, 0.0f, wheelScale);

    // Driver Bill
    float driverScale = scale * (44.0f / 95.0f);
    float driverX = cx - 21.4f * scale;
    float driverY = chassisCenterY - 32.0f * scale;
    blitRotated(fb, s_driverHeadImg, driverX, driverY, 45.0f, 85.0f, bounceY * 0.04f, driverScale);

    // Chassis
    blitRotated(fb, s_chassisImg, cx, chassisCenterY, 126.4f, 70.0f, 0.0f, scale);
}

} // namespace Graphics
