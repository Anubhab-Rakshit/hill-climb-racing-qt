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
 */
class StageSelectScreen {
public:
    StageSelectScreen();

    void setDimensions(int width, int height);

    void setOnBack(std::function<void()> cb) { m_btnBack.setOnClick(cb); }
    void setOnSelectStage(std::function<void(const std::string& stageId)> cb) { m_onSelectStage = cb; }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile);

private:
    int m_width;
    int m_height;

    RetroButton m_btnBack;
    RetroButton m_btnSelectCountryside;
    RetroButton m_btnSelectDesert;
    RetroButton m_btnSelectMoon;
    RetroButton m_btnSelectMountain;

    std::function<void(const std::string& stageId)> m_onSelectStage;

    void drawStageThumbnail(Graphics::Framebuffer& fb, int x, int y, int w, int h, int stageIdx);
};

} // namespace UI
