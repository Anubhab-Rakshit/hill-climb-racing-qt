#include "GameOverScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include <iomanip>
#include <sstream>

namespace UI {

GameOverScreen::GameOverScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_reason("DRIVER DOWN!")
    , m_distance(0.0f)
    , m_coins(0)
    , m_flips(0)
    , m_airTime(0.0f)
    , m_isNewRecord(false)
    , m_btnRetry(0, 0, 180, 46, "RETRY", Theme::GREEN_GAS)
    , m_btnGarage(0, 0, 180, 46, "GARAGE", Theme::GOLD)
    , m_btnMenu(0, 0, 180, 46, "MENU", 0xFF37474F)
{
    m_btnRetry.setKeyHint("R");
    m_btnMenu.setKeyHint("ESC");
    setDimensions(m_width, m_height);
}

void GameOverScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 180;
    int btnH = 46;
    int gap = 18;
    int totalW = 3 * btnW + 2 * gap;
    int startX = (width - totalW) / 2;
    int btnY = height / 2 + 130;

    m_btnRetry.setPosition(startX, btnY);
    m_btnRetry.setSize(btnW, btnH);

    m_btnGarage.setPosition(startX + btnW + gap, btnY);
    m_btnGarage.setSize(btnW, btnH);

    m_btnMenu.setPosition(startX + (btnW + gap) * 2, btnY);
    m_btnMenu.setSize(btnW, btnH);
}

void GameOverScreen::update(float dt) {
    m_animTime += dt;
}

void GameOverScreen::setStats(float distance, int coins, int flips, float airTime, bool isNewRecord) {
    m_distance = distance;
    m_coins = coins;
    m_flips = flips;
    m_airTime = airTime;
    m_isNewRecord = isNewRecord;
}

void GameOverScreen::onMouseMove(int px, int py) {
    m_btnRetry.onMouseMove(px, py);
    m_btnGarage.onMouseMove(px, py);
    m_btnMenu.onMouseMove(px, py);
}

bool GameOverScreen::onMouseDown(int px, int py) {
    if (m_btnRetry.onMouseDown(px, py)) return true;
    if (m_btnGarage.onMouseDown(px, py)) return true;
    if (m_btnMenu.onMouseDown(px, py)) return true;
    return false;
}

void GameOverScreen::onMouseUp(int px, int py) {
    m_btnRetry.onMouseUp(px, py);
    m_btnGarage.onMouseUp(px, py);
    m_btnMenu.onMouseUp(px, py);
}

void GameOverScreen::render(Graphics::Framebuffer& fb) {
    // 1. Darkened scanline raster overlay
    fb.fillRect(0, 0, m_width, m_height, 0xD0080C14);
    for (int sy = 0; sy < m_height; sy += 3) {
        fb.fillRect(0, sy, m_width, 1, 0x18000000);
    }

    // 2. Centered Summary Modal Card
    int boxW = 620;
    int boxH = 410;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2;

    uint32_t stripeCol = (m_reason == "OUT OF FUEL!") ? Theme::GOLD : Theme::RED_BRAKE;
    UIComponents::drawArcadePanel(fb, boxX, boxY, boxW, boxH, Theme::CARD_BG, stripeCol, stripeCol, 4, true, true);

    // Corner Rivets
    fb.fillCircle(boxX + 8, boxY + 8, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + boxW - 9, boxY + 8, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + 8, boxY + boxH - 9, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + boxW - 9, boxY + boxH - 9, 2, Theme::CHROME_MID);

    int cx = m_width / 2;

    // Header Title (e.g. "DRIVER DOWN!" or "OUT OF FUEL!")
    Graphics::RasterFont::drawStringCentered(fb, cx + 2, boxY + 20, m_reason, 0xFF05080E, 3);
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 18, m_reason, stripeCol, 3);

    // Performance Medal Tier Calculation
    std::string medalTitle = "ROOKIE";
    uint32_t medalCol = Theme::MEDAL_ROOKIE;
    int tier = 0;

    if (m_distance >= 1200.0f) {
        medalTitle = "PLATINUM CHAMPION";
        medalCol = Theme::MEDAL_PLATINUM;
        tier = 4;
    } else if (m_distance >= 600.0f) {
        medalTitle = "GOLD MEDAL";
        medalCol = Theme::MEDAL_GOLD;
        tier = 3;
    } else if (m_distance >= 300.0f) {
        medalTitle = "SILVER MEDAL";
        medalCol = Theme::MEDAL_SILVER;
        tier = 2;
    } else if (m_distance >= 100.0f) {
        medalTitle = "BRONZE MEDAL";
        medalCol = Theme::MEDAL_BRONZE;
        tier = 1;
    }

    // Draw True Pixel-Art Medal Badge
    int medalCenterY = boxY + 68;
    UIComponents::drawMedalBadge(fb, cx - 120, medalCenterY, tier, m_animTime);

    // Medal Tier Banner next to badge
    int tierW = 210;
    int tierH = 22;
    int tierX = cx - 95;
    int tierY = medalCenterY - 11;
    UIComponents::drawArcadePanel(fb, tierX, tierY, tierW, tierH, 0xEE111A26, medalCol, 0, 0, false, true);
    Graphics::RasterFont::drawStringCentered(fb, tierX + tierW / 2, tierY + 5, medalTitle, medalCol, 1);

    // New Record Banner if beaten
    if (m_isNewRecord) {
        int recW = 240;
        int recH = 20;
        int recX = cx - recW / 2;
        int recY = boxY + 88;
        UIComponents::drawArcadePanel(fb, recX, recY, recW, recH, 0xFFD84315, Theme::GOLD, 0, 0, false, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, recY + 4, "* NEW RECORD DISTANCE! *", Theme::TEXT_WHITE, 1);
    }

    // Stats Table Grid
    int tableY = boxY + (m_isNewRecord ? 116 : 106);
    int colLeft = boxX + 60;
    int colRight = boxX + boxW - 60;
    int rowW = colRight - colLeft;

    auto drawRow = [&](int y, const std::string& label, const std::string& value, uint32_t valCol, bool altBg) {
        if (altBg) {
            fb.fillRect(colLeft - 8, y - 2, rowW + 16, 26, 0x221E2C3D);
        }
        // Label
        Graphics::RasterFont::drawString(fb, colLeft, y + 2, label, Theme::TEXT_MUTED, 2);
        // Formatted Value
        int valW = Graphics::RasterFont::getTextWidth(value, 2);
        Graphics::RasterFont::drawString(fb, colRight - valW, y + 2, value, valCol, 2);
        // Divider line
        fb.drawLine(colLeft - 8, y + 26, colRight + 8, y + 26, 0xFF1C2738);
    };

    drawRow(tableY, "DISTANCE REACHED", UIComponents::formatNumber(static_cast<int64_t>(m_distance)) + " m", Theme::TEXT_WHITE, false);
    drawRow(tableY + 32, "COINS COLLECTED", "+$" + UIComponents::formatNumber(m_coins), Theme::GOLD, true);
    drawRow(tableY + 64, "AERIAL FLIPS", UIComponents::formatNumber(m_flips) + " FLIPS", Theme::CYAN_UPGRADE, false);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << m_airTime << " s";
    drawRow(tableY + 96, "TOTAL AIR TIME", ss.str(), Theme::GREEN_GAS, true);

    // Action Buttons
    m_btnRetry.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnMenu.render(fb, 2);
}

} // namespace UI
