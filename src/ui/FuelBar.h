#pragma once

#include "Framebuffer.h"

namespace UI {

/**
 * @brief Segmented Retro LED Fuel Gauge with low-fuel blink alarm.
 */
class FuelBar {
public:
    FuelBar(int x = 20, int y = 20, int w = 180, int h = 24);

    void setPosition(int x, int y) { m_x = x; m_y = y; }
    void setFuel(float fuelPercent); // 0.0f to 100.0f
    void update(float dt);
    void render(Graphics::Framebuffer& fb);

    float fuel() const { return m_fuelPercent; }

private:
    int m_x;
    int m_y;
    int m_w;
    int m_h;
    float m_fuelPercent;
    float m_blinkTimer;
    bool m_blinkState;
};

} // namespace UI
