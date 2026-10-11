#pragma once

#include "PhysicsTypes.h"
#include <algorithm>

namespace Graphics {

/**
 * @brief Dynamic 2D Tracking Camera with velocity look-ahead, speed zoom, and impact shake.
 */
class Camera {
public:
    Camera(int screenWidth = 960, int screenHeight = 540);

    void setScreenSize(int w, int h) { m_screenW = w; m_screenH = h; }
    void reset(const Physics::Vec2& pos);

    void update(float dt, const Physics::Vec2& targetPos, float targetSpeedX);
    void addTrauma(float amount);
    void setRenderAlpha(float alpha) { m_renderAlpha = std::clamp(alpha, 0.0f, 1.0f); }

    float x() const;
    float y() const;
    float zoom() const;

    Physics::Vec2 worldToScreen(const Physics::Vec2& world) const;
    Physics::Vec2 screenToWorld(const Physics::Vec2& screen) const;

private:
    int m_screenW;
    int m_screenH;
    Physics::Vec2 m_pos;
    Physics::Vec2 m_prevPos;
    Physics::Vec2 m_shakeOffset;
    Physics::Vec2 m_prevShakeOffset;
    float m_zoom;
    float m_prevZoom;
    float m_trauma;
    float m_renderAlpha;
};

} // namespace Graphics
