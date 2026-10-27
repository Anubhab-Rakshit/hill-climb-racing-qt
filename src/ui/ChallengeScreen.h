#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace UI {

struct GrandPrixRacerScore {
    std::string name;
    uint32_t color = 0xFF00E5FF;
    int points = 0;
    int wins = 0;
    float bestDistance = 0.0f;

    GrandPrixRacerScore() = default;
    GrandPrixRacerScore(const std::string& n, uint32_t c, int p, int w, float bd)
        : name(n), color(c), points(p), wins(w), bestDistance(bd) {}
};

/**
 * @brief Round-Robin Grand Prix Championship Standings & Trophy Screen.
 * Tracks cumulative points across consecutive tracks in both solo and multiplayer.
 */
class ChallengeScreen {
public:
    ChallengeScreen();

    void setDimensions(int width, int height);

    void setStandings(const std::vector<GrandPrixRacerScore>& standings, int currentRound, int totalRounds,
                      const std::string& currentTrack, const std::string& nextTrack, bool isChampionshipOver);

    void setOnNextRound(std::function<void()> cb) { m_onNextRound = cb; }
    void setOnExitToMenu(std::function<void()> cb) { m_onExitToMenu = cb; }
    void setButtonSound(std::function<void()> cb);

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void update(float dt);
    void render(Graphics::Framebuffer& fb);

private:
    int m_width = 960;
    int m_height = 540;
    float m_animTime = 0.0f;

    std::vector<GrandPrixRacerScore> m_standings;
    int m_currentRound = 1;
    int m_totalRounds = 5;
    std::string m_currentTrack = "countryside";
    std::string m_nextTrack = "desert";
    bool m_isChampionshipOver = false;

    RetroButton m_btnNextRound;
    RetroButton m_btnExitMenu;

    std::function<void()> m_onNextRound;
    std::function<void()> m_onExitToMenu;
};

} // namespace UI
