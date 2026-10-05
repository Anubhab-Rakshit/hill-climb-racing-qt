#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include <string>
#include <functional>

namespace UI {

/**
 * @brief Game Over Screen displaying crash reason, distance reached, coins, stunts, and retry.
 */
class GameOverScreen {
public:
    GameOverScreen();

    void setDimensions(int width, int height);

    void setCrashReason(const std::string& reason) { m_reason = reason; }
    void setStats(float distance, int coins, int flips, float airTime, bool isNewRecord);

    void setOnRetry(std::function<void()> cb) { m_btnRetry.setOnClick(cb); }
    void setOnGarage(std::function<void()> cb) { m_btnGarage.setOnClick(cb); }
    void setOnMenu(std::function<void()> cb) { m_btnMenu.setOnClick(cb); }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void render(Graphics::Framebuffer& fb);

private:
    int m_width;
    int m_height;
    std::string m_reason;
    float m_distance;
    int m_coins;
    int m_flips;
    float m_airTime;
    bool m_isNewRecord;

    RetroButton m_btnRetry;
    RetroButton m_btnGarage;
    RetroButton m_btnMenu;
};

} // namespace UI
