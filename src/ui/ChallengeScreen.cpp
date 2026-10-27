#include "ChallengeScreen.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include <algorithm>

namespace UI {

ChallengeScreen::ChallengeScreen()
    : m_btnNextRound(0, 0, 240, 48, "NEXT CIRCUIT ROUND >", Theme::GREEN_GAS)
    , m_btnExitMenu(0, 0, 180, 44, "< EXIT CHAMPIONSHIP", 0xFF37474F)
{
    m_btnNextRound.setOnClick([this]() {
        if (m_onNextRound) m_onNextRound();
    });

    m_btnExitMenu.setOnClick([this]() {
        if (m_onExitToMenu) m_onExitToMenu();
    });

    setDimensions(m_width, m_height);
}

void ChallengeScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int cx = m_width / 2;
    int bottomY = m_height - 70;

    m_btnExitMenu.setPosition(cx - 240, bottomY);
    m_btnNextRound.setPosition(cx + 30, bottomY);
}

void ChallengeScreen::setButtonSound(std::function<void()> cb) {
    m_btnNextRound.setOnSound(cb);
    m_btnExitMenu.setOnSound(cb);
}

void ChallengeScreen::setStandings(const std::vector<GrandPrixRacerScore>& standings, int currentRound,
                                   int totalRounds, const std::string& currentTrack,
                                   const std::string& nextTrack, bool isChampionshipOver) {
    m_standings = standings;
    // Sort descending by points
    std::sort(m_standings.begin(), m_standings.end(), [](const GrandPrixRacerScore& a, const GrandPrixRacerScore& b) {
        if (a.points != b.points) return a.points > b.points;
        return a.bestDistance > b.bestDistance;
    });

    m_currentRound = currentRound;
    m_totalRounds = totalRounds;
    m_currentTrack = currentTrack;
    m_nextTrack = nextTrack;
    m_isChampionshipOver = isChampionshipOver;

    if (m_isChampionshipOver) {
        m_btnNextRound.setText("CHAMPIONSHIP COMPLETE!");
        m_btnNextRound.setBaseColor(Theme::GOLD);
    } else {
        m_btnNextRound.setText("NEXT: " + m_nextTrack + " >");
        m_btnNextRound.setBaseColor(Theme::GREEN_GAS);
    }
}

void ChallengeScreen::update(float dt) {
    m_animTime += dt;
}

void ChallengeScreen::onMouseMove(int px, int py) {
    m_btnNextRound.onMouseMove(px, py);
    m_btnExitMenu.onMouseMove(px, py);
}

bool ChallengeScreen::onMouseDown(int px, int py) {
    if (m_btnNextRound.onMouseDown(px, py)) return true;
    if (m_btnExitMenu.onMouseDown(px, py)) return true;
    return false;
}

void ChallengeScreen::onMouseUp(int px, int py) {
    m_btnNextRound.onMouseUp(px, py);
    m_btnExitMenu.onMouseUp(px, py);
}

void ChallengeScreen::render(Graphics::Framebuffer& fb) {
    // Backdrop
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF080F1C, 0xFF152236);

    int cx = m_width / 2;

    // Header Panel
    std::string titleStr = m_isChampionshipOver ? "GRAND PRIX CEREMONY - FINAL STANDINGS" :
                           "GRAND PRIX CIRCUIT - ROUND " + std::to_string(m_currentRound) + " OF " + std::to_string(m_totalRounds);
    uint32_t headCol = m_isChampionshipOver ? Theme::GOLD : Theme::CYAN_UPGRADE;

    UIComponents::drawArcadePanel(fb, cx - 320, 16, 640, 42, 0xEE0B1424, Theme::CARD_BORDER, headCol, 3, true, true);
    Graphics::RasterFont::drawStringCentered(fb, cx, 28, titleStr, headCol, 2);

    // Standings Table Card
    int tableW = 660;
    int tableH = 360;
    int tableX = cx - tableW / 2;
    int tableY = 72;

    UIComponents::drawArcadePanel(fb, tableX, tableY, tableW, tableH, 0xF20B1526, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

    // Table Header Row
    fb.fillRect(tableX + 12, tableY + 12, tableW - 24, 28, 0xFF142133);
    fb.drawRect(tableX + 12, tableY + 12, tableW - 24, 28, 0xFF2B3E56);

    Graphics::RasterFont::drawString(fb, tableX + 26, tableY + 20, "RANK", Theme::GOLD, 1);
    Graphics::RasterFont::drawString(fb, tableX + 90, tableY + 20, "RACER CALLSIGN", Theme::TEXT_WHITE, 1);
    Graphics::RasterFont::drawString(fb, tableX + 330, tableY + 20, "WINS", Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawString(fb, tableX + 430, tableY + 20, "BEST DIST", Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawString(fb, tableX + 560, tableY + 20, "POINTS", Theme::GOLD, 1);

    // Rows
    int rowY = tableY + 46;
    for (size_t i = 0; i < m_standings.size() && i < 6; ++i) {
        const auto& s = m_standings[i];
        bool isFirst = (i == 0);

        uint32_t rowBg = isFirst ? 0xDD1F2C3E : (i % 2 == 0) ? 0xAA0E1826 : 0xAA121D2C;
        uint32_t borderCol = isFirst ? Theme::GOLD : s.color;

        fb.fillRect(tableX + 12, rowY, tableW - 24, 42, rowBg);
        fb.drawRect(tableX + 12, rowY, tableW - 24, 42, borderCol);

        // Rank badge
        std::string rankStr = "#" + std::to_string(i + 1);
        uint32_t rCol = (i == 0) ? Theme::GOLD : (i == 1) ? 0xFFCFD8DC : (i == 2) ? 0xFFCD7F32 : Theme::TEXT_MUTED;
        Graphics::RasterFont::drawString(fb, tableX + 28, rowY + 14, rankStr, rCol, 2);

        // Player Tag & Color Dot
        fb.fillCircle(tableX + 78, rowY + 21, 6, s.color);
        Graphics::RasterFont::drawString(fb, tableX + 92, rowY + 14, s.name, 0xFFFFFFFF, 2);

        // Wins
        std::string winStr = std::to_string(s.wins);
        Graphics::RasterFont::drawString(fb, tableX + 340, rowY + 16, winStr, Theme::TEXT_WHITE, 1);

        // Best distance
        std::string distStr = std::to_string(static_cast<int>(s.bestDistance)) + "m";
        Graphics::RasterFont::drawString(fb, tableX + 430, rowY + 16, distStr, Theme::CYAN_UPGRADE, 1);

        // Points
        std::string ptsStr = std::to_string(s.points) + " PTS";
        Graphics::RasterFont::drawString(fb, tableX + 550, rowY + 14, ptsStr, Theme::GOLD, 2);

        rowY += 48;
    }

    m_btnExitMenu.render(fb, 2);
    m_btnNextRound.render(fb, 2);
}

} // namespace UI
