#include "Camera.h"
#include <cmath>
#include <cstdlib>

namespace Graphics {

Camera::Camera(int screenWidth, int screenHeight)
    : m_screenW(screenWidth)
    , m_screenH(screenHeight)
    , m_pos({0.0f, 0.0f})
    , m_shakeOffset({0.0f, 0.0f})
    , m_zoom(44.0f) // 44 pixels per meter base scale for authentic Hill Climb framing
    , m_trauma(0.0f)
{
}

void Camera::addTrauma(float amount) {
    m_trauma = std::clamp(m_trauma + amount, 0.0f, 1.0f);
}

void Camera::update(float dt, const Physics::Vec2& targetPos, float targetSpeedX) {
    // 1. Dynamic Look-ahead based on forward speed
    float lookAheadX = std::clamp(targetSpeedX * 0.35f, 0.0f, 7.0f);
    Physics::Vec2 desiredPos = {targetPos.x + lookAheadX, targetPos.y + 0.9f};

    // Smooth position lerp
    float lerpSpeed = 7.0f;
    m_pos.x += (desiredPos.x - m_pos.x) * (1.0f - std::exp(-lerpSpeed * dt));
    m_pos.y += (desiredPos.y - m_pos.y) * (1.0f - std::exp(-lerpSpeed * dt));

    // 2. Speed-dependent Zoom Out
    float targetZoom = 46.0f - std::clamp(std::abs(targetSpeedX) * 0.40f, 0.0f, 12.0f);
    m_zoom += (targetZoom - m_zoom) * (1.0f - std::exp(-4.0f * dt));

    // 3. Traumatic Shake
    if (m_trauma > 0.001f) {
        float shake = m_trauma * m_trauma; // Non-linear shake curve
        float maxOffset = 0.6f; // meters
        float r1 = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
        float r2 = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
        m_shakeOffset = {r1 * maxOffset * shake, r2 * maxOffset * shake};
        m_trauma = std::max(0.0f, m_trauma - 1.5f * dt);
    } else {
        m_shakeOffset = {0.0f, 0.0f};
    }
}

Physics::Vec2 Camera::worldToScreen(const Physics::Vec2& world) const {
    float sx = m_screenW * 0.5f + (world.x - x()) * m_zoom;
    float sy = m_screenH * 0.55f - (world.y - y()) * m_zoom;
    return {sx, sy};
}

Physics::Vec2 Camera::screenToWorld(const Physics::Vec2& screen) const {
    float wx = x() + (screen.x - m_screenW * 0.5f) / m_zoom;
    float wy = y() - (screen.y - m_screenH * 0.55f) / m_zoom;
    return {wx, wy};
}

} // namespace Graphics
