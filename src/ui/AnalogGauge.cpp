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

void AnalogGauge::bakeDialFace() {
    int r = m_radius;
    int size = (r + 4) * 2;
    m_faceCache.resize(size, size);
    m_faceCache.clear(0x00000000);

    int c = r + 4; // Local center inside cache

    // 1. Drop shadow
    m_faceCache.fillCircle(c + 2, c + 2, r + 1, 0x8805080E);

    // 2. Bezel layers
    m_faceCache.fillCircle(c, c, r + 2, 0xFF1A232E);
    m_faceCache.drawCircle(c, c, r + 2, 0xFF05080E);
    m_faceCache.fillCircle(c, c, r, Theme::CHROME_MID);
    m_faceCache.drawCircle(c, c, r, Theme::CHROME_BRIGHT);
    m_faceCache.drawCircle(c, c, r - 1, Theme::CHROME_DARK);

    // 3. Dial face (Deep carbon weave texture)
    m_faceCache.fillCircle(c, c, r - 2, 0xFF0E1520);
    for (int y = c - r + 3; y <= c + r - 3; ++y) {
        for (int x = c - r + 3; x <= c + r - 3; ++x) {
            int dx = x - c;
            int dy = y - c;
            if (dx * dx + dy * dy < (r - 3) * (r - 3)) {
                if (((x ^ y) & 2) == 0) {
                    m_faceCache.setPixelFast(x, y, 0xFF141D2C);
                }
            }
        }
    }
    m_faceCache.drawCircle(c, c, r - 2, 0xFF1E2838);

    // Convex mineral glass reflection across upper quadrant
    for (int y = c - r + 4; y < c; ++y) {
        for (int x = c - r + 4; x <= c + r - 4; ++x) {
            int dx = x - c;
            int dy = y - c;
            if (dx * dx + dy * dy < (r - 4) * (r - 4)) {
                if (dx > -r * 0.7f && dx < r * 0.5f && (dy + dx * 0.35f < -3)) {
                    m_faceCache.blendPixelFast(x, y, 0x22FFFFFF);
                }
            }
        }
    }

    // 4 Corner Mounting Hex Rivets at 45, 135, 225, 315 deg
    const float rivetAngles[4] = {0.785f, 2.356f, 3.927f, 5.498f};
    for (float ang : rivetAngles) {
        int rx = static_cast<int>(c + std::cos(ang) * (r + 0.5f));
        int ry = static_cast<int>(c + std::sin(ang) * (r + 0.5f));
        m_faceCache.setPixelFast(rx, ry, Theme::CHROME_BRIGHT);
        m_faceCache.setPixelFast(rx + 1, ry, 0xFF1A232E);
    }

    // 4. Colored Perimeter Arc Sweep (135 deg to 405 deg = 270 deg)
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
        int px = static_cast<int>(c + std::cos(rad) * arcR);
        int py = static_cast<int>(c + std::sin(rad) * arcR);
        m_faceCache.setPixelFast(px, py, arcCol);
    }

    // 5. Tick Marks
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

        int x1 = static_cast<int>(c + std::cos(angleRad) * rInner);
        int y1 = static_cast<int>(c + std::sin(angleRad) * rInner);
        int x2 = static_cast<int>(c + std::cos(angleRad) * rOuter);
        int y2 = static_cast<int>(c + std::sin(angleRad) * rOuter);

        m_faceCache.drawLine(x1, y1, x2, y2, tickColor);
    }

    // 6. Dial Title & Unit
    Graphics::RasterFont::drawStringCentered(m_faceCache, c, c - r / 2 - 2, m_label, Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawStringCentered(m_faceCache, c, c - r / 2 + 7, m_unit, 0xFF546E7A, 1);

    m_faceCached = true;
}

void AnalogGauge::render(Graphics::Framebuffer& fb) {
    if (!m_faceCached) {
        bakeDialFace();
    }

    int r = m_radius;
    int size = (r + 4) * 2;
    int dstX = m_xc - (r + 4);
    int dstY = m_yc - (r + 4);
    int c = r + 4;
    int rLimitSq = (r + 2) * (r + 2);

    // Fast blit cached static dial face with circular clipping mask
    for (int y = 0; y < size; ++y) {
        int targetY = dstY + y;
        if (targetY < 0 || targetY >= fb.height()) continue;
        const uint32_t* srcLine = m_faceCache.scanLine(y);
        uint32_t* dstLine = fb.scanLine(targetY);
        int dy = y - c;
        int dySq = dy * dy;
        for (int x = 0; x < size; ++x) {
            int dx = x - c;
            if (dx * dx + dySq > rLimitSq) continue;
            uint32_t col = srcLine[x];
            int targetX = dstX + x;
            if (targetX < 0 || targetX >= fb.width()) continue;
            dstLine[targetX] = col;
        }
    }

    // Dynamic components:
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
    const float startAngleDeg = 135.0f;
    const float sweepAngleDeg = 270.0f;
    float needleAngleDeg = startAngleDeg + valFrac * sweepAngleDeg;
    float needleAngleRad = needleAngleDeg * (static_cast<float>(M_PI) / 180.0f);

    int needleLen = r - 8;
    int tipX = static_cast<int>(m_xc + std::cos(needleAngleRad) * needleLen);
    int tipY = static_cast<int>(m_yc + std::sin(needleAngleRad) * needleLen);

    // Needle drop shadow on dial face (offset by +2, +2)
    fb.drawLine(m_xc + 2, m_yc + 2, tipX + 2, tipY + 2, 0x7705080E);
    fb.drawLine(m_xc + 3, m_yc + 2, tipX + 3, tipY + 2, 0x7705080E);

    // Crisp high-visibility racing needle with glowing illumination
    fb.drawLine(m_xc, m_yc, tipX, tipY, Theme::GAUGE_NEEDLE);
    fb.drawLine(m_xc + 1, m_yc, tipX, tipY, 0xFFFF7043);
    fb.drawLine(m_xc, m_yc + 1, tipX, tipY, 0xFFFF7043);
    fb.setPixelFast(tipX, tipY, 0xFFFFFFFF);
    fb.setPixelFast(tipX + 1, tipY, 0xFFFF8A65);

    // 7. Center Anodized Pivot Hub
    fb.fillCircle(m_xc, m_yc, 6, 0xFF21272F);
    fb.drawCircle(m_xc, m_yc, 6, Theme::CHROME_MID);
    fb.fillCircle(m_xc, m_yc, 3, 0xFFECEFF1);
    fb.setPixelFast(m_xc, m_yc, Theme::GAUGE_NEEDLE);

    // 8. Digital LCD Display Box
    int valInt = static_cast<int>(m_currentVal);
    std::string valStr = std::to_string(valInt);
    int boxW = 46;
    int boxH = 14;
    int boxX = m_xc - boxW / 2;
    int boxY = m_yc + r / 3;

    fb.fillRect(boxX, boxY, boxW, boxH, 0xEE080C14);
    fb.drawRect(boxX, boxY, boxW, boxH, 0xFF2A3A4D);
    fb.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 0xFF141F2C);
    // Unlit segment ghost
    Graphics::RasterFont::drawStringCentered(fb, m_xc, boxY + 3, "888", 0x1878909C, 1);
    Graphics::RasterFont::drawStringCentered(fb, m_xc + 1, boxY + 3, valStr, 0xFF05080E, 1);
    Graphics::RasterFont::drawStringCentered(fb, m_xc, boxY + 2, valStr, Theme::GOLD, 1);
}

} // namespace UI
