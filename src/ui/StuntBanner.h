#pragma once

#include "Framebuffer.h"
#include <string>

namespace UI {

/**
 * @brief Animated floating banner for aerial stunts and rewards (Backflip, Air Time, Flips).
 */
class StuntBanner {
public:
    StuntBanner();

    void trigger(const std::string& title, int bonusCoins, uint32_t color = 0xFFFFB300);
    void update(float dt);
    void render(Graphics::Framebuffer& fb);

    bool isActive() const { return m_active; }

private:
    std::string m_title;
    int m_bonusCoins;
    uint32_t m_color;
    float m_lifeTimer;
    float m_totalDuration;
    float m_offsetY;
    bool m_active;
};

} // namespace UI
