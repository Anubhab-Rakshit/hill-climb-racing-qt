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
    void reset(const Physics::Vec2& pos) { m_pos = pos; m_shakeOffset = {0.0f, 0.0f}; m_trauma = 0.0f; }

    void update(float dt, const Physics::Vec2& targetPos, float targetSpeedX);
    void addTrauma(float amount);

    float x() const { return m_pos.x + m_shakeOffset.x; }
    float y() const { return m_pos.y + m_shakeOffset.y; }
    float zoom() const { return m_zoom; }

    Physics::Vec2 worldToScreen(const Physics::Vec2& world) const;
    Physics::Vec2 screenToWorld(const Physics::Vec2& screen) const;

private:
    int m_screenW;
    int m_screenH;
    Physics::Vec2 m_pos;
    Physics::Vec2 m_shakeOffset;
    float m_zoom;
    float m_trauma;
};

} // namespace Graphics
