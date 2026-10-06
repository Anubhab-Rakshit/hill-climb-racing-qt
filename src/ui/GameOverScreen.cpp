#include "GameOverScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <iomanip>
#include <sstream>

namespace UI {

GameOverScreen::GameOverScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_reason("DRIVER DOWN!")
    , m_distance(0.0f)
    , m_coins(0)
    , m_flips(0)
    , m_airTime(0.0f)
    , m_isNewRecord(false)
    , m_btnRetry(0, 0, 175, 46, "RETRY [R]", Theme::GREEN_GAS)
    , m_btnGarage(0, 0, 175, 46, "GARAGE", Theme::GOLD)
    , m_btnMenu(0, 0, 175, 46, "MENU", 0xFF37474F)
{
    setDimensions(m_width, m_height);
}

void GameOverScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 175;
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

    // 2. Centered Summary Modal Card
    int boxW = 620;
    int boxH = 410;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2;

    fb.fillRect(boxX, boxY, boxW, boxH, Theme::CARD_BG);
    fb.drawRect(boxX, boxY, boxW, boxH, Theme::RED_BRAKE);
    fb.drawRect(boxX - 2, boxY - 2, boxW + 4, boxH + 4, 0xFF05080E);

    // Header Title (e.g. "DRIVER DOWN!" or "OUT OF FUEL!")
    uint32_t headerCol = (m_reason == "OUT OF FUEL!") ? Theme::GOLD : Theme::RED_BRAKE;
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2 + 2, boxY + 24, m_reason, 0xFF05080E, 3);
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, boxY + 22, m_reason, headerCol, 3);

    // Performance Medal (Bronze, Silver, Gold, Platinum)
    std::string medalTitle = "ROOKIE";
    uint32_t medalCol = 0xFF78909C;
    if (m_distance >= 1200.0f) {
        medalTitle = "PLATINUM CHAMPION";
        medalCol = 0xFF00E5FF;
    } else if (m_distance >= 600.0f) {
        medalTitle = "GOLD MEDAL";
        medalCol = Theme::GOLD;
    } else if (m_distance >= 300.0f) {
        medalTitle = "SILVER MEDAL";
        medalCol = 0xFFCFD8DC;
    } else if (m_distance >= 100.0f) {
        medalTitle = "BRONZE MEDAL";
        medalCol = 0xFFCD7F32;
    }

    if (m_isNewRecord) {
        int badgeW = 230;
        int badgeH = 22;
        int bx = m_width / 2 - badgeW / 2;
        int by = boxY + 60;
        fb.fillRect(bx, by, badgeW, badgeH, 0xFFD84315);
        fb.drawRect(bx, by, badgeW, badgeH, Theme::GOLD);
        Graphics::RasterFont::drawStringCentered(fb, m_width / 2, by + 5, "* NEW RECORD DISTANCE! *", Theme::TEXT_WHITE, 1);
    } else {
        int badgeW = 200;
        int badgeH = 20;
        int bx = m_width / 2 - badgeW / 2;
        int by = boxY + 62;
        fb.fillRect(bx, by, badgeW, badgeH, 0xEE111A26);
        fb.drawRect(bx, by, badgeW, badgeH, medalCol);
        Graphics::RasterFont::drawStringCentered(fb, m_width / 2, by + 4, medalTitle, medalCol, 1);
    }

    // Stats Table Grid
    int tableY = boxY + 98;
    int colLeft = boxX + 60;
    int colRight = boxX + boxW - 60;

    auto drawRow = [&](int y, const std::string& label, const std::string& value, uint32_t valCol) {
        Graphics::RasterFont::drawString(fb, colLeft, y, label, Theme::TEXT_MUTED, 2);
        int valW = Graphics::RasterFont::getTextWidth(value, 2);
        Graphics::RasterFont::drawString(fb, colRight - valW, y, value, valCol, 2);
        fb.drawLine(colLeft, y + 26, colRight, y + 26, 0xFF1C2738);
    };

    drawRow(tableY, "DISTANCE REACHED", std::to_string(static_cast<int>(m_distance)) + " m", Theme::TEXT_WHITE);
    drawRow(tableY + 36, "COINS COLLECTED", "+$" + std::to_string(m_coins), Theme::GOLD);
    drawRow(tableY + 72, "AERIAL FLIPS", std::to_string(m_flips) + " FLIPS", Theme::CYAN_UPGRADE);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << m_airTime << " s";
    drawRow(tableY + 108, "TOTAL AIR TIME", ss.str(), Theme::GREEN_GAS);

    // Action Buttons
    m_btnRetry.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnMenu.render(fb, 2);
}

} // namespace UI
