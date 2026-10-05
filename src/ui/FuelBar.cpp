#include "FuelBar.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <algorithm>
#include <string>

namespace UI {

FuelBar::FuelBar(int x, int y, int w, int h)
    : m_x(x)
    , m_y(y)
    , m_w(w)
    , m_h(h)
    , m_fuelPercent(100.0f)
    , m_blinkTimer(0.0f)
    , m_blinkState(false)
{
}

void FuelBar::setFuel(float fuelPercent) {
    m_fuelPercent = std::clamp(fuelPercent, 0.0f, 100.0f);
}

void FuelBar::update(float dt) {
    if (m_fuelPercent < 25.0f) {
        m_blinkTimer += dt;
        if (m_blinkTimer >= 0.25f) {
            m_blinkTimer = 0.0f;
            m_blinkState = !m_blinkState;
        }
    } else {
        m_blinkState = false;
        m_blinkTimer = 0.0f;
    }
}

void FuelBar::render(Graphics::Framebuffer& fb) {
    // 1. Outer Container Box
    fb.fillRect(m_x, m_y, m_w, m_h, 0xEE101622);
    fb.drawRect(m_x, m_y, m_w, m_h, Theme::CARD_BORDER);

    // 2. "FUEL" Label
    Graphics::RasterFont::drawString(fb, m_x + 6, m_y + 8, "FUEL", Theme::TEXT_MUTED, 1);

    // 3. Segmented Bar Area
    int barStartX = m_x + 40;
    int barStartY = m_y + 4;
    int barW = m_w - 48;
    int barH = m_h - 8;

    int totalSegments = 16;
    int segGap = 2;
    int segW = (barW - (totalSegments - 1) * segGap) / totalSegments;

    int activeSegments = static_cast<int>((m_fuelPercent / 100.0f) * totalSegments + 0.5f);

    for (int i = 0; i < totalSegments; ++i) {
        int sx = barStartX + i * (segW + segGap);
        int sy = barStartY;

        if (i < activeSegments) {
            uint32_t segColor;
            float segFrac = static_cast<float>(i) / totalSegments;

            if (segFrac < 0.25f) {
                segColor = (m_blinkState) ? 0xFFFF1744 : 0xFFD50000;
            } else if (segFrac < 0.55f) {
                segColor = Theme::GOLD;
            } else {
                segColor = Theme::GREEN_GAS;
            }

            fb.fillRect(sx, sy, segW, barH, segColor);
            // Highlight bar top
            fb.fillRect(sx, sy, segW, 2, 0x55FFFFFF);
        } else {
            // Inactive segment slot
            fb.fillRect(sx, sy, segW, barH, 0xFF1C2533);
        }
    }

    // 4. Low Fuel Alert Warning Banner
    if (m_fuelPercent < 25.0f && m_blinkState) {
        int alertX = m_x + m_w + 10;
        int alertY = m_y + 4;
        fb.fillRect(alertX - 4, alertY - 2, 92, 18, 0xEEB71C1C);
        fb.drawRect(alertX - 4, alertY - 2, 92, 18, 0xFFFF5252);
        Graphics::RasterFont::drawString(fb, alertX, alertY + 3, "! LOW FUEL !", Theme::TEXT_WHITE, 1);
    }
}

} // namespace UI
