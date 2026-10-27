#include "Camera.h"
#include <cmath>
#include <cstdlib>

namespace Graphics {

Camera::Camera(int screenWidth, int screenHeight)
    : m_screenW(screenWidth)
    , m_screenH(screenHeight)
    , m_pos({0.0f, 0.0f})
    , m_prevPos({0.0f, 0.0f})
    , m_shakeOffset({0.0f, 0.0f})
    , m_prevShakeOffset({0.0f, 0.0f})
    , m_zoom(44.0f) // 44 pixels per meter base scale for authentic Hill Climb framing
    , m_prevZoom(44.0f)
    , m_trauma(0.0f)
    , m_renderAlpha(1.0f)
{
}

void Camera::reset(const Physics::Vec2& pos) {
    m_pos = pos;
    m_prevPos = pos;
    m_shakeOffset = {0.0f, 0.0f};
    m_prevShakeOffset = {0.0f, 0.0f};
    m_zoom = (m_screenH <= 300) ? 32.0f : 44.0f;
    m_prevZoom = m_zoom;
    m_trauma = 0.0f;
    m_renderAlpha = 1.0f;
}

void Camera::addTrauma(float amount) {
    m_trauma = std::clamp(m_trauma + amount, 0.0f, 1.0f);
}

void Camera::update(float dt, const Physics::Vec2& targetPos, float targetSpeedX) {
    // Cache previous physics tick state for rendering interpolation
    m_prevPos = m_pos;
    m_prevZoom = m_zoom;
    m_prevShakeOffset = m_shakeOffset;

    // 1. Dynamic Look-ahead based on calibrated forward speed
    float lookAheadX = std::clamp(targetSpeedX * 0.58f, 0.0f, 7.0f);
    float desiredOffsetY = (m_screenH <= 300) ? 0.35f : 0.9f;
    Physics::Vec2 desiredPos = {targetPos.x + lookAheadX, targetPos.y + desiredOffsetY};

    // Smooth position lerp
    float lerpSpeed = 7.0f;
    m_pos.x += (desiredPos.x - m_pos.x) * (1.0f - std::exp(-lerpSpeed * dt));
    m_pos.y += (desiredPos.y - m_pos.y) * (1.0f - std::exp(-lerpSpeed * dt));

    // 2. Speed-dependent Zoom Out (calibrated for 60% physical velocity)
    float baseZoom = (m_screenH <= 300) ? 33.0f : 46.0f;
    float maxZoomOut = (m_screenH <= 300) ? 6.0f : 12.0f;
    float targetZoom = baseZoom - std::clamp(std::abs(targetSpeedX) * 0.40f, 0.0f, maxZoomOut);
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

float Camera::x() const {
    float a = m_renderAlpha;
    return (m_prevPos.x + (m_pos.x - m_prevPos.x) * a) +
           (m_prevShakeOffset.x + (m_shakeOffset.x - m_prevShakeOffset.x) * a);
}

float Camera::y() const {
    float a = m_renderAlpha;
    return (m_prevPos.y + (m_pos.y - m_prevPos.y) * a) +
           (m_prevShakeOffset.y + (m_shakeOffset.y - m_prevShakeOffset.y) * a);
}

float Camera::zoom() const {
    return m_prevZoom + (m_zoom - m_prevZoom) * m_renderAlpha;
}

Physics::Vec2 Camera::worldToScreen(const Physics::Vec2& world) const {
    float curZoom = zoom();
    float centerYFrac = (m_screenH <= 300) ? 0.58f : 0.55f;
    float sx = m_screenW * 0.5f + (world.x - x()) * curZoom;
    float sy = m_screenH * centerYFrac - (world.y - y()) * curZoom;
    return {sx, sy};
}

Physics::Vec2 Camera::screenToWorld(const Physics::Vec2& screen) const {
    float curZoom = zoom();
    float centerYFrac = (m_screenH <= 300) ? 0.58f : 0.55f;
    float wx = x() + (screen.x - m_screenW * 0.5f) / curZoom;
    float wy = y() - (screen.y - m_screenH * centerYFrac) / curZoom;
    return {wx, wy};
}

} // namespace Graphics
