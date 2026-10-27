#include "StuntBanner.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
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
    int boxW = std::max(200, textW + 48);
    int boxH = 44;
    int boxX = cx - boxW / 2;
    int boxY = cy - boxH / 2;

    // Deep drop shadow behind banner panel
    fb.fillRect(boxX + 3, boxY + 4, boxW, boxH, 0x8804070D);

    // Glowing border halo when freshly triggered
    if (m_lifeTimer < 0.6f) {
        float glowFrac = 1.0f - (m_lifeTimer / 0.6f);
        uint32_t glowAlpha = static_cast<uint32_t>(glowFrac * 120.0f);
        uint32_t glowCol = (glowAlpha << 24) | (m_color & 0x00FFFFFF);
        fb.drawRect(boxX - 2, boxY - 2, boxW + 4, boxH + 4, glowCol);
    }

    // Arcade panel with corner notches and border
    UIComponents::drawArcadePanel(fb, boxX, boxY, boxW, boxH, 0xF00A101C, m_color, m_color, 3, true, true);

    // Flanking decorative golden achievement stars
    UIComponents::drawStar(fb, boxX + 16, cy, 6, true, Theme::GOLD, 0xFFFFA000);
    UIComponents::drawStar(fb, boxX + boxW - 16, cy, 6, true, Theme::GOLD, 0xFFFFA000);

    // Title with drop shadow
    Graphics::RasterFont::drawStringCentered(fb, cx + 1, boxY + 9, m_title, 0xFF05080E, 2);
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 8, m_title, m_color, 2);

    // Dynamic diagonal specular gleam passing across banner
    if (m_lifeTimer < 0.8f) {
        float gleamT = m_lifeTimer / 0.8f;
        int gleamX = boxX + static_cast<int>(gleamT * (boxW + 30)) - 15;
        for (int gy = boxY + 3; gy < boxY + boxH - 3; ++gy) {
            int gx = gleamX + (boxY + boxH - gy) / 2;
            if (gx >= boxX + 3 && gx < boxX + boxW - 3) {
                fb.blendPixelFast(gx, gy, 0x44FFFFFF);
                fb.blendPixelFast(gx + 1, gy, 0x99FFFFFF);
                fb.blendPixelFast(gx + 2, gy, 0x44FFFFFF);
            }
        }
    }

    // Subtitle coins bonus
    if (m_bonusCoins > 0) {
        std::string bonusStr = "+$" + UIComponents::formatNumber(m_bonusCoins) + " BONUS!";
        Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 28, bonusStr, Theme::GREEN_GAS, 1);
    }
}

} // namespace UI
