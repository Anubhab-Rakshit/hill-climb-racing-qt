#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include <functional>

namespace UI {

/**
 * @brief Darkened modal pause overlay with Resume, Restart, and Menu options.
 */
class PauseOverlay {
public:
    PauseOverlay();

    void setDimensions(int width, int height);

    void setOnResume(std::function<void()> cb) { m_btnResume.setOnClick(cb); }
    void setOnRestart(std::function<void()> cb) { m_btnRestart.setOnClick(cb); }
    void setOnGarage(std::function<void()> cb) { m_btnGarage.setOnClick(cb); }
    void setOnMenu(std::function<void()> cb) { m_btnMenu.setOnClick(cb); }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void render(Graphics::Framebuffer& fb);

private:
    int m_width;
    int m_height;

    RetroButton m_btnResume;
    RetroButton m_btnRestart;
    RetroButton m_btnGarage;
    RetroButton m_btnMenu;
};

} // namespace UI
