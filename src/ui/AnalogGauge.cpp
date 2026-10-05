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
{
}

void AnalogGauge::setValue(float targetVal) {
    m_targetVal = std::clamp(targetVal, m_minVal, m_maxVal);
}

void AnalogGauge::update(float dt) {
    // Smooth needle damping (lerp with exponential decay)
    float speed = 12.0f;
    m_currentVal += (m_targetVal - m_currentVal) * (1.0f - std::exp(-speed * dt));
}

void AnalogGauge::render(Graphics::Framebuffer& fb) {
    // 1. Outer dial housing & bevel
    fb.fillCircle(m_xc, m_yc, m_radius, Theme::GAUGE_FACE);
    fb.drawCircle(m_xc, m_yc, m_radius, Theme::GAUGE_BORDER);
    fb.drawCircle(m_xc, m_yc, m_radius - 1, 0xFF1E2838);

    // 2. Tick marks (Sweep from 135 deg to 405 deg)
    const float startAngleDeg = 135.0f;
    const float sweepAngleDeg = 270.0f;
    const int numTicks = 11;

    for (int i = 0; i < numTicks; ++i) {
        float t = static_cast<float>(i) / (numTicks - 1);
        float angleDeg = startAngleDeg + t * sweepAngleDeg;
        float angleRad = angleDeg * (static_cast<float>(M_PI) / 180.0f);

        bool isRedline = (t >= m_redlineFrac);
        uint32_t tickColor = isRedline ? Theme::GAUGE_REDLINE : Theme::TEXT_MUTED;
        int tickLen = (i % 2 == 0) ? 6 : 4;

        int rOuter = m_radius - 3;
        int rInner = rOuter - tickLen;

        int x1 = static_cast<int>(m_xc + std::cos(angleRad) * rInner);
        int y1 = static_cast<int>(m_yc + std::sin(angleRad) * rInner);
        int x2 = static_cast<int>(m_xc + std::cos(angleRad) * rOuter);
        int y2 = static_cast<int>(m_yc + std::sin(angleRad) * rOuter);

        fb.drawLine(x1, y1, x2, y2, tickColor);
    }

    // 3. Dial Label
    Graphics::RasterFont::drawStringCentered(fb, m_xc, m_yc - m_radius / 2 - 2, m_label, Theme::TEXT_MUTED, 1);

    // 4. Swept Needle
    float valFrac = (m_currentVal - m_minVal) / (m_maxVal - m_minVal);
    valFrac = std::clamp(valFrac, 0.0f, 1.0f);

    float needleAngleDeg = startAngleDeg + valFrac * sweepAngleDeg;
    float needleAngleRad = needleAngleDeg * (static_cast<float>(M_PI) / 180.0f);

    int needleLen = m_radius - 7;
    int tipX = static_cast<int>(m_xc + std::cos(needleAngleRad) * needleLen);
    int tipY = static_cast<int>(m_yc + std::sin(needleAngleRad) * needleLen);

    // Draw thick needle
    fb.drawLine(m_xc, m_yc, tipX, tipY, Theme::GAUGE_NEEDLE);
    fb.drawLine(m_xc + 1, m_yc, tipX, tipY, Theme::GAUGE_NEEDLE);
    fb.drawLine(m_xc, m_yc + 1, tipX, tipY, Theme::GAUGE_NEEDLE);

    // Center pivot hub
    fb.fillCircle(m_xc, m_yc, 5, 0xFFE0E0E0);
    fb.drawCircle(m_xc, m_yc, 5, 0xFF424242);
    fb.fillCircle(m_xc, m_yc, 2, Theme::GAUGE_NEEDLE);

    // 5. Digital Readout Box below hub
    int valInt = static_cast<int>(m_currentVal);
    std::string valStr = std::to_string(valInt);
    int boxW = 44;
    int boxH = 12;
    int boxX = m_xc - boxW / 2;
    int boxY = m_yc + m_radius / 3;

    fb.fillRect(boxX, boxY, boxW, boxH, 0xDD0A0F17);
    fb.drawRect(boxX, boxY, boxW, boxH, 0xFF2A3A4D);
    Graphics::RasterFont::drawStringCentered(fb, m_xc, boxY + 2, valStr, Theme::GOLD, 1);
}

} // namespace UI
