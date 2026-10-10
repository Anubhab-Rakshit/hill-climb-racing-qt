#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "ProfileManager.h"
#include <functional>

namespace UI {

/**
 * @brief Main Menu Title Screen.
 * Features animated title typography, rolling hill backdrop, and navigation buttons.
 */
class MainMenuScreen {
public:
    MainMenuScreen();

    void setDimensions(int width, int height);

    void setOnStart(std::function<void()> cb) { m_btnStart.setOnClick(cb); }
    void setOnGarage(std::function<void()> cb) { m_btnGarage.setOnClick(cb); }
    void setOnStages(std::function<void()> cb) { m_btnStages.setOnClick(cb); }
    void setOnQuit(std::function<void()> cb) { m_btnQuit.setOnClick(cb); }
    void setButtonSound(std::function<void()> cb) {
        m_btnStart.setOnSound(cb);
        m_btnGarage.setOnSound(cb);
        m_btnStages.setOnSound(cb);
        m_btnQuit.setOnSound(cb);
    }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void update(float dt);
    void render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile);
    void render(Graphics::Framebuffer& fb, int totalCoins);

private:
    int m_width;
    int m_height;
    float m_animTime;

    RetroButton m_btnStart;
    RetroButton m_btnGarage;
    RetroButton m_btnStages;
    RetroButton m_btnQuit;
};

} // namespace UI
