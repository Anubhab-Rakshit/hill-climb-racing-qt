#include "StuntBanner.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <algorithm>

namespace UI {

StuntBanner::StuntBanner()
    : m_title("")
    , m_bonusCoins(0)
    , m_color(Theme::GOLD)
    , m_lifeTimer(0.0f)
    , m_totalDuration(2.0f)
    , m_offsetY(0.0f)
    , m_active(false)
{
}

void StuntBanner::trigger(const std::string& title, int bonusCoins, uint32_t color) {
    m_title = title;
    m_bonusCoins = bonusCoins;
    m_color = color;
    m_lifeTimer = 0.0f;
    m_totalDuration = 2.0f;
    m_offsetY = 0.0f;
    m_active = true;
}

void StuntBanner::update(float dt) {
    if (!m_active) return;

    m_lifeTimer += dt;
    // Gentle upward float
    m_offsetY -= 15.0f * dt;

    if (m_lifeTimer >= m_totalDuration) {
        m_active = false;
    }
}

void StuntBanner::render(Graphics::Framebuffer& fb) {
    if (!m_active) return;

    int cx = fb.width() / 2;
    int cy = static_cast<int>(130 + m_offsetY);

    // Box dimensions
    int textW = Graphics::RasterFont::getTextWidth(m_title, 2);
    int boxW = std::max(180, textW + 36);
    int boxH = 42;
    int boxX = cx - boxW / 2;
    int boxY = cy - boxH / 2;

    // Dark pill container
    fb.fillRect(boxX, boxY, boxW, boxH, 0xE00A101C);
    fb.drawRect(boxX, boxY, boxW, boxH, m_color);
    fb.drawRect(boxX - 1, boxY - 1, boxW + 2, boxH + 2, 0xFF000000);

    // Title
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 8, m_title, m_color, 2);

    // Subtitle coins bonus
    if (m_bonusCoins > 0) {
        std::string bonusStr = "+$" + std::to_string(m_bonusCoins) + " BONUS!";
        Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 26, bonusStr, Theme::GREEN_GAS, 1);
    }
}

} // namespace UI
