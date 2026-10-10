#include "AnalogGauge.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace UI {

AnalogGauge::AnalogGauge(int xc, int yc, int radius,
                         float minVal, float maxVal,
                         const std::string& label, const std::string& unit,
                         float redlineFrac)
    : m_xc(xc)
    , m_yc(yc)
    , m_radius(radius)
    , m_minVal(minVal)
    , m_maxVal(maxVal)
    , m_currentVal(minVal)
    , m_targetVal(minVal)
    , m_label(label)
    , m_unit(unit)
    , m_redlineFrac(redlineFrac)
    , m_animTime(0.0f)
{
}

void AnalogGauge::setValue(float targetVal) {
    m_targetVal = std::clamp(targetVal, m_minVal, m_maxVal);
}

void AnalogGauge::update(float dt) {
    m_animTime += dt;
    // Smooth needle damping (lerp with exponential decay)
    float speed = 14.0f;
    m_currentVal += (m_targetVal - m_currentVal) * (1.0f - std::exp(-speed * dt));
}

void AnalogGauge::render(Graphics::Framebuffer& fb) {
    // 1. Outer Titanium / Chrome Bezel & Mounting Bolts
    int r = m_radius;

    // Drop shadow
    fb.fillCircle(m_xc + 2, m_yc + 2, r + 1, 0x8805080E);

    // Bezel layers
    fb.fillCircle(m_xc, m_yc, r + 2, 0xFF1A232E);
    fb.drawCircle(m_xc, m_yc, r + 2, 0xFF05080E);
    fb.fillCircle(m_xc, m_yc, r, Theme::CHROME_MID);
    fb.drawCircle(m_xc, m_yc, r, Theme::CHROME_BRIGHT);
    fb.drawCircle(m_xc, m_yc, r - 1, Theme::CHROME_DARK);

    // Dial face (Deep carbon/night texture)
    fb.fillCircle(m_xc, m_yc, r - 2, 0xFF0E1520);
    fb.drawCircle(m_xc, m_yc, r - 2, 0xFF1E2838);

    // 4 Corner Mounting Hex Rivets at 45, 135, 225, 315 deg
    const float rivetAngles[4] = {0.785f, 2.356f, 3.927f, 5.498f};
    for (float ang : rivetAngles) {
        int rx = static_cast<int>(m_xc + std::cos(ang) * (r + 0.5f));
        int ry = static_cast<int>(m_yc + std::sin(ang) * (r + 0.5f));
        fb.setPixelFast(rx, ry, Theme::CHROME_BRIGHT);
        fb.setPixelFast(rx + 1, ry, 0xFF1A232E);
    }

    // 2. Colored Perimeter Arc Sweep (135 deg to 405 deg = 270 deg)
    const float startAngleDeg = 135.0f;
    const float sweepAngleDeg = 270.0f;

    int arcR = r - 5;
    for (float a = 0.0f; a <= sweepAngleDeg; a += 3.0f) {
        float rad = (startAngleDeg + a) * (static_cast<float>(M_PI) / 180.0f);
        float frac = a / sweepAngleDeg;
        uint32_t arcCol;
        if (frac >= m_redlineFrac) {
            arcCol = Theme::GAUGE_REDLINE;
        } else if (frac >= m_redlineFrac * 0.7f) {
            arcCol = Theme::GOLD;
        } else {
            arcCol = (m_label == "RPM") ? Theme::GREEN_GAS : Theme::CYAN_UPGRADE;
        }
        int px = static_cast<int>(m_xc + std::cos(rad) * arcR);
        int py = static_cast<int>(m_yc + std::sin(rad) * arcR);
        fb.setPixelFast(px, py, arcCol);
    }

    // 3. Tick Marks
    const int numMajorTicks = 9;
    for (int i = 0; i < numMajorTicks; ++i) {
        float t = static_cast<float>(i) / (numMajorTicks - 1);
        float angleDeg = startAngleDeg + t * sweepAngleDeg;
        float angleRad = angleDeg * (static_cast<float>(M_PI) / 180.0f);

        bool isRedline = (t >= m_redlineFrac);
        uint32_t tickColor = isRedline ? Theme::GAUGE_REDLINE : Theme::TEXT_WHITE;
        int tickLen = (i % 2 == 0) ? 6 : 4;

        int rOuter = r - 6;
        int rInner = rOuter - tickLen;

        int x1 = static_cast<int>(m_xc + std::cos(angleRad) * rInner);
        int y1 = static_cast<int>(m_yc + std::sin(angleRad) * rInner);
        int x2 = static_cast<int>(m_xc + std::cos(angleRad) * rOuter);
        int y2 = static_cast<int>(m_yc + std::sin(angleRad) * rOuter);

        fb.drawLine(x1, y1, x2, y2, tickColor);
    }

    // 4. Dial Title & Unit
    Graphics::RasterFont::drawStringCentered(fb, m_xc, m_yc - r / 2 - 2, m_label, Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawStringCentered(fb, m_xc, m_yc - r / 2 + 7, m_unit, 0xFF546E7A, 1);

    // 5. Shift Light / Redline Flash Lamp
    float range = m_maxVal - m_minVal;
    float valFrac = (range > 0.0001f) ? ((m_currentVal - m_minVal) / range) : 0.0f;
    valFrac = std::clamp(valFrac, 0.0f, 1.0f);

    if (valFrac >= m_redlineFrac) {
        bool flash = (std::fmod(m_animTime * 12.0f, 2.0f) < 1.0f);
        uint32_t lampCol = flash ? 0xFFFF1744 : 0xFF880E4F;
        fb.fillCircle(m_xc, m_yc - r + 9, 3, lampCol);
        fb.setPixelFast(m_xc, m_yc - r + 9, 0xFFFFFFFF);
    }

    // 6. Swept Needle with Drop Shadow
    float needleAngleDeg = startAngleDeg + valFrac * sweepAngleDeg;
    float needleAngleRad = needleAngleDeg * (static_cast<float>(M_PI) / 180.0f);

    int needleLen = r - 8;
    int tipX = static_cast<int>(m_xc + std::cos(needleAngleRad) * needleLen);
    int tipY = static_cast<int>(m_yc + std::sin(needleAngleRad) * needleLen);

    // Needle drop shadow on dial face (offset by +2, +2)
    fb.drawLine(m_xc + 2, m_yc + 2, tipX + 2, tipY + 2, 0x7705080E);
    fb.drawLine(m_xc + 3, m_yc + 2, tipX + 3, tipY + 2, 0x7705080E);

    // Crisp high-visibility racing needle
    fb.drawLine(m_xc, m_yc, tipX, tipY, Theme::GAUGE_NEEDLE);
    fb.drawLine(m_xc + 1, m_yc, tipX, tipY, 0xFFFF7043);
    fb.drawLine(m_xc, m_yc + 1, tipX, tipY, 0xFFFF7043);

    // 7. Center Anodized Pivot Hub
    fb.fillCircle(m_xc, m_yc, 6, 0xFF21272F);
    fb.drawCircle(m_xc, m_yc, 6, Theme::CHROME_MID);
    fb.fillCircle(m_xc, m_yc, 3, 0xFFECEFF1);
    fb.setPixelFast(m_xc, m_yc, Theme::GAUGE_NEEDLE);

    // 8. Digital LCD Display Box
    int valInt = static_cast<int>(m_currentVal);
    std::string valStr = std::to_string(valInt);
    int boxW = 44;
    int boxH = 13;
    int boxX = m_xc - boxW / 2;
    int boxY = m_yc + r / 3;

    fb.fillRect(boxX, boxY, boxW, boxH, 0xEE090D14);
    fb.drawRect(boxX, boxY, boxW, boxH, 0xFF2A3A4D);
    Graphics::RasterFont::drawStringCentered(fb, m_xc + 1, boxY + 3, valStr, 0xFF05080E, 1);
    Graphics::RasterFont::drawStringCentered(fb, m_xc, boxY + 2, valStr, Theme::GOLD, 1);
}

} // namespace UI
