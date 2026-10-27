#include "FuelBar.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
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
    if (m_flashTimer > 0.0f) {
        m_flashTimer = std::max(0.0f, m_flashTimer - dt);
    }
    if (m_fuelPercent < 25.0f) {
        m_blinkTimer += dt;
        if (m_blinkTimer >= 0.22f) {
            m_blinkTimer = 0.0f;
            m_blinkState = !m_blinkState;
        }
    } else {
        m_blinkState = false;
        m_blinkTimer = 0.0f;
    }
}

void FuelBar::render(Graphics::Framebuffer& fb) {
    // 1. Outer Aerodynamic Arcade Pod Container
    uint32_t borderCol = (m_flashTimer > 0.0f) ? 0xFF00E676 :
                         (m_fuelPercent < 25.0f && m_blinkState) ? 0xFFFF1744 : Theme::CARD_BORDER;
    uint32_t accentCol = (m_flashTimer > 0.0f) ? 0xFF00E676 :
                         (m_fuelPercent < 25.0f && m_blinkState) ? 0xFFFF1744 : 0;
    UIComponents::drawArcadePanel(fb, m_x, m_y, m_w, m_h, 0xEE090E18, borderCol, accentCol, 2, true, true);

    // 2. Pixel Fuel Pump Icon (centered vertically)
    int iconY = m_y + (m_h - 14) / 2;
    uint32_t pumpCol = (m_fuelPercent < 25.0f) ? (m_blinkState ? 0xFFFF1744 : 0xFFB71C1C) : Theme::GOLD;
    UIComponents::drawFuelPumpIcon(fb, m_x + 8, iconY, pumpCol);

    // "FUEL" Label
    int textY = m_y + (m_h - 9) / 2;
    Graphics::RasterFont::drawString(fb, m_x + 24, textY, "FUEL", Theme::TEXT_WHITE, 1);

    // 3. Segmented Bar Area
    int barStartX = m_x + 54;
    int barH = m_h - 14;
    int barStartY = m_y + (m_h - barH) / 2;
    int barW = m_w - 62;

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
            fb.fillRect(sx, sy, segW, 2, 0x88FFFFFF);
        } else {
            // Inactive segment slot
            fb.fillRect(sx, sy, segW, barH, 0xFF141D28);
        }
        fb.drawRect(sx, sy, segW, barH, 0xFF080C14);
    }

    // 4. Low Fuel Alert Warning Banner (docked neatly directly below the fuel capsule)
    if (m_fuelPercent < 25.0f && m_blinkState) {
        int alertX = m_x;
        int alertY = m_y + m_h + 3;
        int alertW = 120;
        int alertH = 18;

        UIComponents::drawArcadePanel(fb, alertX, alertY, alertW, alertH, 0xEEB71C1C, 0xFFFF5252, 0xFFFF5252, 2, true, true);
        Graphics::RasterFont::drawStringCentered(fb, alertX + alertW / 2, alertY + 4, "! LOW FUEL !", Theme::TEXT_WHITE, 1);
    }
}

} // namespace UI
