#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "ProfileManager.h"
#include <string>
#include <functional>

namespace UI {

/**
 * @brief Stage & Biome Selection Screen.
 * Allows choosing between Countryside, Desert, Moon, and Mountain Ridge tracks.
 * Features live animated biome previews and coin-based stage unlocking.
 */
class StageSelectScreen {
public:
    StageSelectScreen();

    void setDimensions(int width, int height);

    void setOnBack(std::function<void()> cb) { m_btnBack.setOnClick(cb); }
    void setOnSelectStage(std::function<void(const std::string& stageId)> cb) { m_onSelectStage = cb; }
    void setButtonSound(std::function<void()> cb) {
        m_btnBack.setOnSound(cb);
        m_btnSelectCountryside.setOnSound(cb);
        m_btnSelectDesert.setOnSound(cb);
        m_btnSelectMoon.setOnSound(cb);
        m_btnSelectMountain.setOnSound(cb);
    }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py, Core::ProfileManager& profile);
    void onMouseUp(int px, int py);

    void update(float dt);
    void render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile);

private:
    int m_width;
    int m_height;
    float m_animTime;

    RetroButton m_btnBack;
    RetroButton m_btnSelectCountryside;
    RetroButton m_btnSelectDesert;
    RetroButton m_btnSelectMoon;
    RetroButton m_btnSelectMountain;

    std::function<void(const std::string& stageId)> m_onSelectStage;

    void drawStageThumbnail(Graphics::Framebuffer& fb, int x, int y, int w, int h, int stageIdx);
};

} // namespace UI
