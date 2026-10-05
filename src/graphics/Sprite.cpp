#include "Sprite.h"
#include "UITheme.h"
#include <cmath>

namespace Graphics {

void SpriteRenderer::drawRotatedWheel(Framebuffer& fb, int cx, int cy, int radius, float angleRad) {
    // 1. Tire Black Rim
    fb.fillCircle(cx, cy, radius, 0xFF263238);
    fb.drawCircle(cx, cy, radius, 0xFF0D1217);

    // 2. Hubcap Silver Disc
    int hubR = radius / 2;
    fb.fillCircle(cx, cy, hubR, 0xFFCFD8DC);
    fb.drawCircle(cx, cy, hubR, 0xFF78909C);
    fb.fillCircle(cx, cy, 3, UI::Theme::GOLD);

    // 3. Rotating spokes (4 cross spokes)
    for (int i = 0; i < 4; ++i) {
        float a = angleRad + i * (static_cast<float>(M_PI) / 2.0f);
        int sx = static_cast<int>(cx + std::cos(a) * (radius - 2));
        int sy = static_cast<int>(cy + std::sin(a) * (radius - 2));
        fb.drawLine(cx, cy, sx, sy, 0xFF37474F);
    }
}

void SpriteRenderer::renderVehicle(Framebuffer& fb, const Camera& cam, const Physics::Vehicle& vehicle) {
    float zoom = cam.zoom();

    // 1. Wheels
    Physics::Vec2 rearScr = cam.worldToScreen(vehicle.rearWheelPos());
    Physics::Vec2 frontScr = cam.worldToScreen(vehicle.frontWheelPos());
    int wheelRad = static_cast<int>(vehicle.rearWheelRadius() * zoom);

    // 2. Chassis Screen Position & Rotation
    Physics::Vec2 chassisScr = cam.worldToScreen(vehicle.chassisPos());
    float angle = vehicle.chassisAngle();
    float cosA = std::cos(-angle);
    float sinA = std::sin(-angle);

    // Local-to-Screen transformation helper
    auto toScreen = [&](float lx, float ly) -> Physics::Vec2 {
        float rx = lx * cosA - ly * sinA;
        float ry = lx * sinA + ly * cosA;
        return {chassisScr.x + rx * zoom, chassisScr.y - ry * zoom};
    };

    // 3. Suspension Struts
    Physics::Vec2 rearMount = toScreen(-0.9f, -0.2f);
    Physics::Vec2 frontMount = toScreen(+0.9f, -0.2f);

    fb.drawLine(static_cast<int>(rearMount.x), static_cast<int>(rearMount.y),
                static_cast<int>(rearScr.x), static_cast<int>(rearScr.y), 0xFFB0BEC5);
    fb.drawLine(static_cast<int>(frontMount.x), static_cast<int>(frontMount.y),
                static_cast<int>(frontScr.x), static_cast<int>(frontScr.y), 0xFFB0BEC5);

    // 4. Wheels
    drawRotatedWheel(fb, static_cast<int>(rearScr.x), static_cast<int>(rearScr.y), wheelRad, vehicle.rearWheelAngle());
    drawRotatedWheel(fb, static_cast<int>(frontScr.x), static_cast<int>(frontScr.y), wheelRad, vehicle.frontWheelAngle());

    // 5. Chassis Body (Rotated Red Retro Jeep)
    // Draw rotated polygon corners
    Physics::Vec2 c1 = toScreen(-1.1f, -0.25f);
    Physics::Vec2 c2 = toScreen(+1.1f, -0.25f);
    Physics::Vec2 c3 = toScreen(+1.1f, +0.20f);
    Physics::Vec2 c4 = toScreen(-1.1f, +0.20f);

    // Outline
    fb.drawLine(static_cast<int>(c1.x), static_cast<int>(c1.y), static_cast<int>(c2.x), static_cast<int>(c2.y), 0xFFD32F2F);
    fb.drawLine(static_cast<int>(c2.x), static_cast<int>(c2.y), static_cast<int>(c3.x), static_cast<int>(c3.y), 0xFFD32F2F);
    fb.drawLine(static_cast<int>(c3.x), static_cast<int>(c3.y), static_cast<int>(c4.x), static_cast<int>(c4.y), 0xFFD32F2F);
    fb.drawLine(static_cast<int>(c4.x), static_cast<int>(c4.y), static_cast<int>(c1.x), static_cast<int>(c1.y), 0xFFD32F2F);

    // Solid fill using midpoint lines
    for (float t = -1.05f; t <= 1.05f; t += 0.08f) {
        Physics::Vec2 b = toScreen(t, -0.22f);
        Physics::Vec2 top = toScreen(t, +0.18f);
        fb.drawLine(static_cast<int>(b.x), static_cast<int>(b.y), static_cast<int>(top.x), static_cast<int>(top.y), 0xFFD32F2F);
        // White racing stripe
        Physics::Vec2 s1 = toScreen(t, -0.05f);
        Physics::Vec2 s2 = toScreen(t, +0.05f);
        fb.drawLine(static_cast<int>(s1.x), static_cast<int>(s1.y), static_cast<int>(s2.x), static_cast<int>(s2.y), 0xFFFFFFFF);
    }

    // Roll cage
    Physics::Vec2 r1 = toScreen(-0.4f, +0.20f);
    Physics::Vec2 r2 = toScreen(-0.2f, +0.65f);
    Physics::Vec2 r3 = toScreen(+0.4f, +0.65f);
    Physics::Vec2 r4 = toScreen(+0.6f, +0.20f);

    fb.drawLine(static_cast<int>(r1.x), static_cast<int>(r1.y), static_cast<int>(r2.x), static_cast<int>(r2.y), 0xFF37474F);
    fb.drawLine(static_cast<int>(r2.x), static_cast<int>(r2.y), static_cast<int>(r3.x), static_cast<int>(r3.y), 0xFF37474F);
    fb.drawLine(static_cast<int>(r3.x), static_cast<int>(r3.y), static_cast<int>(r4.x), static_cast<int>(r4.y), 0xFF37474F);

    // 6. Driver Body & Head
    Physics::Vec2 driverHip = toScreen(-0.1f, +0.20f);
    Physics::Vec2 driverHead = cam.worldToScreen(vehicle.driverHeadPos());

    // Blue torso
    fb.drawLine(static_cast<int>(driverHip.x), static_cast<int>(driverHip.y),
                static_cast<int>(driverHead.x), static_cast<int>(driverHead.y), 0xFF1976D2);
    fb.drawLine(static_cast<int>(driverHip.x + 1), static_cast<int>(driverHip.y),
                static_cast<int>(driverHead.x + 1), static_cast<int>(driverHead.y), 0xFF1976D2);

    // Yellow Helmet Head with Goggles
    int headRadius = static_cast<int>(0.22f * zoom);
    fb.fillCircle(static_cast<int>(driverHead.x), static_cast<int>(driverHead.y), headRadius, 0xFFFFCA28);
    fb.drawCircle(static_cast<int>(driverHead.x), static_cast<int>(driverHead.y), headRadius, 0xFFFFA000);
    // Goggles
    fb.fillRect(static_cast<int>(driverHead.x), static_cast<int>(driverHead.y - 2), 6, 4, 0xFF212121);
}

} // namespace Graphics
