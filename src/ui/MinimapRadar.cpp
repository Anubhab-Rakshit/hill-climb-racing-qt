#include "MinimapRadar.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include <algorithm>
#include <cmath>

namespace UI {

MinimapRadar::MinimapRadar() {
}

void MinimapRadar::setDimensions(int x, int y, int width, int height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void MinimapRadar::setCourseLength(float finishDistanceMeters) {
    if (finishDistanceMeters > 50.0f) {
        m_courseLength = finishDistanceMeters;
    }
}

void MinimapRadar::render(Graphics::Framebuffer& fb, const std::vector<RadarRacer>& racers) {
    if (m_width <= 0 || m_height <= 0 || racers.empty()) return;

    // 1. Semi-transparent arcade background container
    UIComponents::drawArcadePanel(fb, m_x, m_y, m_width, m_height, 0xEE090F1A, Theme::CARD_BORDER, 0xFF37474F, 2, true, true);

    // Track baseline line
    int trackPad = 24;
    int trackX0 = m_x + trackPad;
    int trackX1 = m_x + m_width - trackPad;
    int trackW = trackX1 - trackX0;
    int midY = m_y + m_height / 2;

    // Subtle track bed line
    fb.fillRect(trackX0, midY - 1, trackW, 3, 0xFF192535);
    fb.drawLine(trackX0, midY, trackX1, midY, 0xFF2A3D55);

    // Start line icon (Green tick)
    fb.drawLine(trackX0, midY - 6, trackX0, midY + 6, Theme::GREEN_GAS);
    fb.drawLine(trackX0 + 1, midY - 5, trackX0 + 1, midY + 5, Theme::GREEN_GAS);

    // Quarter milestones ticks (25%, 50%, 75%)
    for (int q = 1; q <= 3; ++q) {
        int qx = trackX0 + (trackW * q) / 4;
        fb.drawLine(qx, midY - 3, qx, midY + 3, 0xFF3E5066);
    }

    // Finish line icon (Checkered flag badge)
    fb.drawLine(trackX1, midY - 7, trackX1, midY + 7, Theme::GOLD);
    UIComponents::drawCheckeredFlag(fb, trackX1 - 2, midY - 9, 8);

    // Find current leader for crown placement
    float maxDist = -1.0f;
    size_t leaderIdx = 0;
    for (size_t i = 0; i < racers.size(); ++i) {
        if (!racers[i].isCrashed && racers[i].distance > maxDist) {
            maxDist = racers[i].distance;
            leaderIdx = i;
        }
    }

    // Sort racers by distance so trailing racers render first, leader renders on top
    std::vector<std::pair<float, size_t>> sortedRacers;
    for (size_t i = 0; i < racers.size(); ++i) {
        sortedRacers.push_back({racers[i].distance, i});
    }
    std::sort(sortedRacers.begin(), sortedRacers.end());

    // 2. Render racer blips & pins
    for (const auto& entry : sortedRacers) {
        size_t idx = entry.second;
        const auto& racer = racers[idx];

        float frac = std::clamp(racer.distance / m_courseLength, 0.0f, 1.0f);
        int px = trackX0 + static_cast<int>(frac * trackW);

        uint32_t col = racer.color;
        if (racer.isCrashed) {
            col = 0xFF546E7A; // Muted gray when crashed
        }

        // Racer pin pinhead
        int pinY = midY;
        if (racer.isLocal) {
            // Local player has glowing diamond pin
            fb.fillRect(px - 3, pinY - 5, 7, 11, 0xFF000000);
            fb.fillRect(px - 2, pinY - 4, 5, 9, col);
            fb.drawRect(px - 3, pinY - 5, 7, 11, 0xFFFFFFFF);
            // Center core
            fb.fillRect(px - 1, pinY - 1, 3, 3, 0xFFFFFFFF);
        } else {
            // Remote opponent / AI has circular pin
            fb.fillCircle(px, pinY, 4, 0xFF000000);
            fb.fillCircle(px, pinY, 3, col);
            fb.drawCircle(px, pinY, 4, 0xFFECEFF1);
        }

        // Leader Crown above the leader
        if (idx == leaderIdx && racers.size() > 1 && !racer.isCrashed) {
            fb.setPixelFast(px, pinY - 9, Theme::GOLD);
            fb.setPixelFast(px - 2, pinY - 8, Theme::GOLD);
            fb.setPixelFast(px + 2, pinY - 8, Theme::GOLD);
            fb.drawLine(px - 2, pinY - 7, px + 2, pinY - 7, Theme::GOLD);
        }
    }

    // Legend tags at corners: "0m" and Course total
    Graphics::RasterFont::drawString(fb, m_x + 6, m_y + 8, "0m", Theme::TEXT_MUTED, 1);
    std::string lenStr = std::to_string(static_cast<int>(m_courseLength)) + "m";
    int lenW = Graphics::RasterFont::getTextWidth(lenStr, 1);
    Graphics::RasterFont::drawString(fb, m_x + m_width - lenW - 6, m_y + 8, lenStr, Theme::GOLD, 1);
}

} // namespace UI
