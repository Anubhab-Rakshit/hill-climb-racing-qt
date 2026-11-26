#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include <functional>
#include <utility>

namespace UI {

class AchievementsScreen {
public:
    AchievementsScreen();

    void setDimensions(int width, int height);
    void setOnClose(std::function<void()> cb) { m_btnBack.setOnClick(std::move(cb)); }
    void setButtonSound(std::function<void()> cb) { m_btnBack.setOnSound(std::move(cb)); }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);
    void update(float dt);
    void render(Graphics::Framebuffer& fb);

private:
    int m_width;
    int m_height;
    float m_animTime;
    RetroButton m_btnBack;
};

} // namespace UI
