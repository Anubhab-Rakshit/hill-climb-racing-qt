#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include <string>
#include <functional>

namespace UI {

/**
 * @brief Level Complete celebration modal overlay.
 * Displays celebration banner, level statistics, coins earned, next unlock announcement,
 * and navigation buttons (Next Level, Retry, Garage, Stage Select).
 */
class LevelCompleteScreen {
public:
    LevelCompleteScreen();

    void setDimensions(int width, int height);

    void setLevelStats(const std::string& biomeName,
                       int levelIndex,
                       const std::string& levelName,
                       float distance,
                       float timeSeconds,
                       int coinsEarned,
                       int bonusReward,
                       int flips,
                       float airTime,
                       bool unlockedNextBiome);

    void setOnNextLevel(std::function<void()> cb) { m_btnNextLevel.setOnClick(cb); }
    void setOnRetry(std::function<void()> cb)     { m_btnRetry.setOnClick(cb); }
    void setOnGarage(std::function<void()> cb)    { m_btnGarage.setOnClick(cb); }
    void setOnMenu(std::function<void()> cb)      { m_btnMenu.setOnClick(cb); }

    void setButtonSound(std::function<void()> cb) {
        m_btnNextLevel.setOnSound(cb);
        m_btnRetry.setOnSound(cb);
        m_btnGarage.setOnSound(cb);
        m_btnMenu.setOnSound(cb);
    }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void update(float dt);
    void render(Graphics::Framebuffer& fb);

    int levelIndex() const { return m_levelIndex; }

private:
    int m_width;
    int m_height;
    float m_animTime;

    std::string m_biomeName;
    int m_levelIndex;
    std::string m_levelName;
    float m_distance;
    float m_timeSeconds;
    int m_coinsEarned;
    int m_bonusReward;
    int m_flips;
    float m_airTime;
    bool m_unlockedNextBiome;

    RetroButton m_btnNextLevel;
    RetroButton m_btnRetry;
    RetroButton m_btnGarage;
    RetroButton m_btnMenu;
};

} // namespace UI
