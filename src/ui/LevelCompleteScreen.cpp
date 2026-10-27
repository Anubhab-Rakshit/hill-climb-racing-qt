#include "LevelCompleteScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include <iomanip>
#include <sstream>
#include <cmath>

namespace UI {

LevelCompleteScreen::LevelCompleteScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_biomeName("COUNTRYSIDE")
    , m_levelIndex(1)
    , m_levelName("THE FOOTHILLS")
    , m_distance(550.0f)
    , m_timeSeconds(0.0f)
    , m_coinsEarned(0)
    , m_bonusReward(2000)
    , m_flips(0)
    , m_airTime(0.0f)
    , m_unlockedNextBiome(false)
    , m_btnNextLevel(0, 0, 160, 46, "NEXT LEVEL", Theme::GREEN_GAS)
    , m_btnRetry(0, 0, 140, 46, "RETRY", Theme::GOLD)
    , m_btnGarage(0, 0, 140, 46, "GARAGE", 0xFF0288D1)
    , m_btnMenu(0, 0, 140, 46, "STAGES", 0xFF37474F)
{
    m_btnNextLevel.setKeyHint("SPACE");
    m_btnRetry.setKeyHint("R");
    m_btnGarage.setKeyHint("G");
    m_btnMenu.setKeyHint("ESC");
    setDimensions(m_width, m_height);
}

void LevelCompleteScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW1 = 170;
    int btnW2 = 135;
    int btnH = 46;
    int gap = 14;
    int totalW = btnW1 + 3 * btnW2 + 3 * gap;
    int startX = (width - totalW) / 2;
    int btnY = height / 2 + 138;

    m_btnNextLevel.setPosition(startX, btnY);
    m_btnNextLevel.setSize(btnW1, btnH);

    m_btnRetry.setPosition(startX + btnW1 + gap, btnY);
    m_btnRetry.setSize(btnW2, btnH);

    m_btnGarage.setPosition(startX + btnW1 + gap + (btnW2 + gap), btnY);
    m_btnGarage.setSize(btnW2, btnH);

    m_btnMenu.setPosition(startX + btnW1 + gap + (btnW2 + gap) * 2, btnY);
    m_btnMenu.setSize(btnW2, btnH);
}

void LevelCompleteScreen::setLevelStats(const std::string& biomeName,
                                       int levelIndex,
                                       const std::string& levelName,
                                       float distance,
                                       float timeSeconds,
                                       int coinsEarned,
                                       int bonusReward,
                                       int flips,
                                       float airTime,
                                       bool unlockedNextBiome) {
    m_biomeName = biomeName;
    m_levelIndex = levelIndex;
    m_levelName = levelName;
    m_distance = distance;
    m_timeSeconds = timeSeconds;
    m_coinsEarned = coinsEarned;
    m_bonusReward = bonusReward;
    m_flips = flips;
    m_airTime = airTime;
    m_unlockedNextBiome = unlockedNextBiome;

    if (m_levelIndex >= 5) {
        if (m_unlockedNextBiome) {
            m_btnNextLevel.setText("NEXT REGION");
        } else {
            m_btnNextLevel.setText("PLAY AGAIN");
        }
    } else {
        m_btnNextLevel.setText("NEXT LEVEL");
    }
}

void LevelCompleteScreen::update(float dt) {
    m_animTime += dt;
}

void LevelCompleteScreen::onMouseMove(int px, int py) {
    m_btnNextLevel.onMouseMove(px, py);
    m_btnRetry.onMouseMove(px, py);
    m_btnGarage.onMouseMove(px, py);
    m_btnMenu.onMouseMove(px, py);
}

bool LevelCompleteScreen::onMouseDown(int px, int py) {
    if (m_btnNextLevel.onMouseDown(px, py)) return true;
    if (m_btnRetry.onMouseDown(px, py)) return true;
    if (m_btnGarage.onMouseDown(px, py)) return true;
    if (m_btnMenu.onMouseDown(px, py)) return true;
    return false;
}

void LevelCompleteScreen::onMouseUp(int px, int py) {
    m_btnNextLevel.onMouseUp(px, py);
    m_btnRetry.onMouseUp(px, py);
    m_btnGarage.onMouseUp(px, py);
    m_btnMenu.onMouseUp(px, py);
}

void LevelCompleteScreen::render(Graphics::Framebuffer& fb) {
    // 1. Dark translucent scanline backdrop overlay
    fb.fillRect(0, 0, m_width, m_height, 0xD8080E18);
    for (int sy = 0; sy < m_height; sy += 3) {
        fb.fillRect(0, sy, m_width, 1, 0x1A000000);
    }

    // 2. Centered Victory Summary Modal Card
    int boxW = 660;
    int boxH = 430;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2 - 8;

    UIComponents::drawArcadePanel(fb, boxX, boxY, boxW, boxH, Theme::CARD_BG, Theme::CARD_BORDER, Theme::GOLD, 4, true, true);

    // Corner decorative rivets
    fb.fillCircle(boxX + 8, boxY + 8, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + boxW - 9, boxY + 8, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + 8, boxY + boxH - 9, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + boxW - 9, boxY + boxH - 9, 2, Theme::CHROME_MID);

    int cx = m_width / 2;

    // Victory Starburst rays animation behind title
    for (int r = 0; r < 8; ++r) {
        float angle = r * (3.14159f / 4.0f) + m_animTime * 1.5f;
        int rayLen = 50 + static_cast<int>(std::sin(m_animTime * 4.0f + r) * 8.0f);
        int rx = cx + static_cast<int>(std::cos(angle) * rayLen);
        int ry = (boxY + 38) + static_cast<int>(std::sin(angle) * rayLen * 0.45f);
        fb.drawLine(cx, boxY + 38, rx, ry, 0x33FFD700);
    }

    // Header Title: "LEVEL COMPLETED!"
    std::string mainTitle = (m_levelIndex >= 5) ? "STAGE MASTERED!" : "LEVEL COMPLETED!";
    uint32_t titleCol = Theme::GOLD;
    Graphics::RasterFont::drawStringCentered(fb, cx + 2, boxY + 20, mainTitle, 0xFF05080E, 3);
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 18, mainTitle, titleCol, 3);

    // Subtitle: e.g. "COUNTRYSIDE - LEVEL 1: THE FOOTHILLS"
    std::string subTitle = m_biomeName + " - LEVEL " + std::to_string(m_levelIndex) + ": " + m_levelName;
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 54, subTitle, Theme::CYAN_UPGRADE, 2);

    // 3 Golden Victory Stars
    int starY = boxY + 85;
    for (int s = -1; s <= 1; ++s) {
        int sx = cx + s * 45;
        float bob = std::sin(m_animTime * 3.0f + s * 1.2f) * 3.0f;
        UIComponents::drawStar(fb, sx, starY + static_cast<int>(bob), 9, true, Theme::GOLD, 0xFFFFA000);
    }

    // Unlock Status Banner
    int unlW = 480;
    int unlH = 26;
    int unlX = cx - unlW / 2;
    int unlY = boxY + 110;

    if (m_levelIndex >= 5) {
        std::string unlMsg = m_unlockedNextBiome ? "★ CONGRATULATIONS! NEXT REGION UNLOCKED! ★"
                                                : "★ ULTIMATE CHAMPION! ALL 5 LEVELS CLEARED! ★";
        UIComponents::drawArcadePanel(fb, unlX, unlY, unlW, unlH, 0xEE1B5E20, Theme::GOLD, 0, 0, false, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, unlY + 7, unlMsg, Theme::GOLD, 1);
    } else {
        std::string unlMsg = "★ LEVEL " + std::to_string(m_levelIndex + 1) + " UNLOCKED! ★";
        UIComponents::drawArcadePanel(fb, unlX, unlY, unlW, unlH, 0xEE1B5E20, Theme::GREEN_GAS, 0, 0, false, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, unlY + 7, unlMsg, Theme::TEXT_WHITE, 1);
    }

    // Statistics Panels (Left & Right arcade cards)
    int statCardW = 285;
    int statCardH = 120;
    int statCardY = boxY + 146;
    int leftCardX = cx - statCardW - 12;
    int rightCardX = cx + 12;

    // Left Card: Course Performance (Distance, Time, Flips, Air Time)
    UIComponents::drawArcadePanel(fb, leftCardX, statCardY, statCardW, statCardH, 0xDD121B27, 0xFF2A3A4E, 0, 0, false, false);
    Graphics::RasterFont::drawString(fb, leftCardX + 14, statCardY + 12, "COURSE STATISTICS", Theme::CYAN_UPGRADE, 1);

    int m = static_cast<int>(m_timeSeconds) / 60;
    float s = m_timeSeconds - m * 60;
    std::ostringstream ssTime;
    ssTime << "TIME: " << std::setfill('0') << std::setw(2) << m << ":"
           << std::setfill('0') << std::setw(2) << static_cast<int>(s) << "."
           << std::setfill('0') << std::setw(2) << static_cast<int>((s - static_cast<int>(s)) * 100);

    Graphics::RasterFont::drawString(fb, leftCardX + 14, statCardY + 36, ssTime.str(), Theme::TEXT_WHITE, 1);
    Graphics::RasterFont::drawString(fb, leftCardX + 14, statCardY + 56,
                                     "DISTANCE: " + UIComponents::formatNumber(static_cast<int64_t>(m_distance)) + "m (100%)",
                                     Theme::GREEN_GAS, 1);

    std::ostringstream ssAir;
    ssAir << std::fixed << std::setprecision(1) << "AIR TIME: " << m_airTime << "s";
    Graphics::RasterFont::drawString(fb, leftCardX + 14, statCardY + 76, ssAir.str(), Theme::TEXT_WHITE, 1);
    Graphics::RasterFont::drawString(fb, leftCardX + 14, statCardY + 96, "STUNT FLIPS: " + std::to_string(m_flips), Theme::GOLD, 1);

    // Right Card: Financial Rewards (Run Coins, Clear Bonus, Total Earned)
    UIComponents::drawArcadePanel(fb, rightCardX, statCardY, statCardW, statCardH, 0xDD121B27, 0xFF2A3A4E, 0, 0, false, false);
    Graphics::RasterFont::drawString(fb, rightCardX + 14, statCardY + 12, "COIN REWARDS", Theme::GOLD, 1);

    Graphics::RasterFont::drawString(fb, rightCardX + 14, statCardY + 36,
                                     "COINS COLLECTED: +" + UIComponents::formatNumber(m_coinsEarned),
                                     Theme::TEXT_WHITE, 1);
    Graphics::RasterFont::drawString(fb, rightCardX + 14, statCardY + 56,
                                     "COMPLETION BONUS: +" + UIComponents::formatNumber(m_bonusReward),
                                     Theme::GOLD, 1);

    int totalEarned = m_coinsEarned + m_bonusReward;
    int totBoxW = statCardW - 28;
    int totBoxY = statCardY + 80;
    fb.fillRect(rightCardX + 14, totBoxY, totBoxW, 26, 0xEE1E281E);
    fb.drawRect(rightCardX + 14, totBoxY, totBoxW, 26, Theme::GREEN_GAS);
    Graphics::RasterFont::drawString(fb, rightCardX + 22, totBoxY + 7, "TOTAL WON:", Theme::TEXT_WHITE, 1);
    Graphics::RasterFont::drawString(fb, rightCardX + 130, totBoxY + 7,
                                     "+$" + UIComponents::formatNumber(totalEarned),
                                     Theme::GOLD, 2);

    // 4. Render Action Buttons
    m_btnNextLevel.render(fb, 2);
    m_btnRetry.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnMenu.render(fb, 2);
}

} // namespace UI
