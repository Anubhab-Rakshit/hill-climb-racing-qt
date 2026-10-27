#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "ProfileManager.h"
#include "TerrainConfig.h"
#include <string>
#include <functional>
#include <vector>

namespace UI {

/**
 * @brief Stage & Biome Selection Screen with Checkpoint Progression.
 * Supports browsing 6 unique biomes with animated previews, checkpoint completion badges,
 * and dual unlock paths (via stage completion or coin economy).
 */
class StageSelectScreen {
public:
    StageSelectScreen();

    void setDimensions(int width, int height);

    void setOnBack(std::function<void()> cb) { m_btnBack.setOnClick(cb); }
    void setOnSelectStage(std::function<void(const std::string& stageId)> cb) { m_onSelectStage = cb; }
    void setButtonSound(std::function<void()> cb);

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py, Core::ProfileManager& profile);
    void onMouseUp(int px, int py);

    void update(float dt);
    void render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile);

    int currentPage() const { return m_page; }
    void setPage(int p) { m_page = (p < 0) ? 0 : (p > 1) ? 1 : p; }

private:
    int m_width;
    int m_height;
    float m_animTime;
    int m_page; // 0 = stages 0..2, 1 = stages 3..5

    RetroButton m_btnBack;
    RetroButton m_btnPrevPage;
    RetroButton m_btnNextPage;

    // 6 Stage Select Buttons
    RetroButton m_btnStageAction[6];

    std::function<void(const std::string& stageId)> m_onSelectStage;

    void drawStageThumbnail(Graphics::Framebuffer& fb, int x, int y, int w, int h, int stageIdx);
};

} // namespace UI
