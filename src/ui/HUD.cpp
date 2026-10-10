#include "HUD.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace UI {

HUD::HUD()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_fuelBar(20, 16, 200, 26)
    , m_speedometer(415, 472, 48, 0.0f, 140.0f, "SPEED", "KM/H", 0.75f)
    , m_tachometer(545, 472, 48, 0.0f, 8000.0f, "RPM", "x1000", 0.80f)
    , m_brakePedal(35, 430, 120, 85, "BRAKE", Theme::RED_BRAKE)
    , m_gasPedal(805, 430, 120, 85, "GAS", Theme::GREEN_GAS)
    , m_pauseButton(895, 16, 44, 34, "||", 0xFF37474F)
    , m_distance(0.0f)
    , m_recordDistance(500.0f)
    , m_coins(0)
    , m_clearedCheckpoints(0)
    , m_totalCheckpoints(5)
    , m_nextCheckpointName("THE FOOTHILLS")
    , m_nextCheckpointDist(250.0f)
{
    m_brakePedal.setKeyHint("[A]");
    m_gasPedal.setKeyHint("[D]");
    m_pauseButton.setKeyHint("ESC");
    setDimensions(m_width, m_height);
}

void HUD::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_fuelBar.setPosition(20, 16);
    m_speedometer.setCenter(m_width / 2 - 65, m_height - 64);
    m_tachometer.setCenter(m_width / 2 + 65, m_height - 64);

    m_brakePedal.setPosition(35, m_height - 100);
    m_gasPedal.setPosition(m_width - 155, m_height - 100);
    m_pauseButton.setPosition(m_width - 65, 16);
}

void HUD::setBrakeVirtualPressed(bool pressed) {
    m_brakePedal.setPressed(pressed);
}

void HUD::setGasVirtualPressed(bool pressed) {
    m_gasPedal.setPressed(pressed);
}

void HUD::addFloatingText(const std::string& text, float x, float y, uint32_t color) {
    FloatingText ft;
    ft.text = text;
    ft.x = x;
    ft.y = y;
    ft.life = 0.0f;
    ft.maxLife = 1.0f;
    ft.color = color;
    m_floatingTexts.push_back(ft);
}

void HUD::onMouseMove(int px, int py) {
    m_brakePedal.onMouseMove(px, py);
    m_gasPedal.onMouseMove(px, py);
    m_pauseButton.onMouseMove(px, py);
}

bool HUD::onMouseDown(int px, int py) {
    if (m_pauseButton.onMouseDown(px, py)) return true;
    if (m_brakePedal.onMouseDown(px, py)) return true;
    if (m_gasPedal.onMouseDown(px, py)) return true;
    return false;
}

void HUD::onMouseUp(int px, int py) {
    m_pauseButton.onMouseUp(px, py);
    m_brakePedal.onMouseUp(px, py);
    m_gasPedal.onMouseUp(px, py);
}

void HUD::update(float dt) {
    m_animTime += dt;
    m_fuelBar.update(dt);
    m_speedometer.update(dt);
    m_tachometer.update(dt);
    m_stuntBanner.update(dt);

    // Update floating score texts
    for (size_t i = 0; i < m_floatingTexts.size(); ) {
        m_floatingTexts[i].life += dt;
        m_floatingTexts[i].y -= 38.0f * dt;
        if (m_floatingTexts[i].life >= m_floatingTexts[i].maxLife) {
            m_floatingTexts[i] = m_floatingTexts.back();
            m_floatingTexts.pop_back();
        } else {
            ++i;
        }
    }
}

void HUD::render(Graphics::Framebuffer& fb) {
    // 1. Top Left: Fuel Gauge Pod
    m_fuelBar.render(fb);

    // 2. Top Center: Rally Stage Navigation & Distance Odometer Card
    int distCardW = 240;
    int distCardH = 44;
    int distCardX = (m_width - distCardW) / 2;
    int distCardY = 14;

    UIComponents::drawArcadePanel(fb, distCardX, distCardY, distCardW, distCardH, 0xEE090E18, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

    int distInt = static_cast<int>(m_distance);
    char distBuf[32];
    std::snprintf(distBuf, sizeof(distBuf), "%05dm", distInt);

    Graphics::RasterFont::drawString(fb, distCardX + 10, distCardY + 6, "DIST", Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawString(fb, distCardX + 46, distCardY + 5, distBuf, Theme::TEXT_WHITE, 2);

    int recInt = static_cast<int>(m_recordDistance);
    char recBuf[32];
    std::snprintf(recBuf, sizeof(recBuf), "REC %04dm", recInt);
    Graphics::RasterFont::drawString(fb, distCardX + 152, distCardY + 8, recBuf, Theme::GOLD, 1);

    // Mini Rally Stage Progress Track Line
    int trackX = distCardX + 10;
    int trackY = distCardY + 31;
    int trackW = distCardW - 32;
    fb.fillRect(trackX, trackY, trackW, 4, 0xFF192534);

    float trackProg = std::clamp(m_distance / 1750.0f, 0.0f, 1.0f);
    int fillW = static_cast<int>(trackProg * trackW);
    if (fillW > 0) {
        fb.fillRect(trackX, trackY, fillW, 4, Theme::CYAN_UPGRADE);
        fb.fillRect(trackX, trackY, fillW, 1, 0x88FFFFFF);
    }

    // 5 Checkpoint Pips along the track line
    const float cpDists[5] = {250.0f, 550.0f, 900.0f, 1300.0f, 1750.0f};
    for (int cp = 0; cp < 5; ++cp) {
        int cpPx = trackX + static_cast<int>((cpDists[cp] / 1750.0f) * trackW);
        uint32_t col = (m_clearedCheckpoints > cp) ? Theme::GOLD : 0xFF546E7A;
        fb.fillRect(cpPx - 1, trackY - 2, 3, 8, col);
    }
    // Indicator pip for car position
    fb.fillRect(trackX + fillW - 2, trackY - 3, 5, 10, Theme::GREEN_GAS);

    // Checkered Finish Line Flag Icon at end of track
    UIComponents::drawCheckeredFlag(fb, trackX + trackW + 4, trackY - 6, 12);

    // Checkpoint Tag Banner below Odometer
    if (!m_nextCheckpointName.empty() && m_distance < 1750.0f) {
        int remain = static_cast<int>(m_nextCheckpointDist - m_distance);
        std::string cpTag = "CP " + std::to_string(m_clearedCheckpoints + 1) + "/5: " +
                            std::to_string(std::max(0, remain)) + "m TO " + m_nextCheckpointName;
        int tagW = Graphics::RasterFont::getTextWidth(cpTag, 1) + 16;
        int tagX = m_width / 2 - tagW / 2;
        int tagY = distCardY + distCardH + 4;
        UIComponents::drawArcadePanel(fb, tagX, tagY, tagW, 17, 0xEE0B1422, Theme::CARD_BORDER, 0, 0, false, true);
        Graphics::RasterFont::drawStringCentered(fb, m_width / 2, tagY + 3, cpTag, Theme::CYAN_UPGRADE, 1);
    } else if (m_clearedCheckpoints >= 5) {
        int tagW = 260;
        int tagX = m_width / 2 - tagW / 2;
        int tagY = distCardY + distCardH + 4;
        UIComponents::drawArcadePanel(fb, tagX, tagY, tagW, 17, 0xEE1B381A, Theme::GREEN_GAS, 0, 0, false, true);
        Graphics::RasterFont::drawStringCentered(fb, m_width / 2, tagY + 3, "* STAGE CLEARED - ENDLESS RUN *", Theme::GOLD, 1);
    }

    // 3. Top Right: Master Currency Pill
    int coinBoxW = 145;
    int coinBoxH = 34;
    int coinBoxX = m_width - coinBoxW - 80;
    int coinBoxY = 16;
    UIComponents::drawCoinBadge(fb, coinBoxX, coinBoxY, coinBoxW, coinBoxH, m_coins, m_animTime);

    // 4. Pause Button
    m_pauseButton.render(fb, 2);

    // 5. Floating Text FX (Popups for collected coins and fuel)
    for (const auto& ft : m_floatingTexts) {
        int fx = static_cast<int>(ft.x);
        int fy = static_cast<int>(ft.y);
        Graphics::RasterFont::drawStringCentered(fb, fx + 1, fy + 1, ft.text, 0xFF05080E, 2);
        Graphics::RasterFont::drawStringCentered(fb, fx, fy, ft.text, ft.color, 2);
    }

    // 6. Bottom Center: Racing Cockpit Dual-Gauge Pod
    int podW = 280;
    int podH = 105;
    int podX = m_width / 2 - podW / 2;
    int podY = m_height - podH;

    // Integrated Binnacle Dashboard Plaque
    UIComponents::drawArcadePanel(fb, podX, podY, podW, podH, 0xD0080E18, 0xFF2A3A4E, 0, 0, true, true);

    m_speedometer.render(fb);
    m_tachometer.render(fb);

    // 7. Bottom Corners: Interactive Arcade Racing Pedals with Grip Textures
    m_brakePedal.render(fb, 2);
    m_gasPedal.render(fb, 2);

    // Rubber Grip Tread Plates over Pedals
    // Brake pedal horizontal non-slip rubber ribs
    int bx = 35 + 8;
    int by = (m_height - 100) + 12 + (m_brakePedal.isPressed() ? 2 : 0);
    int bw = 120 - 16;
    for (int y = by + 28; y < by + 56; y += 7) {
        fb.fillRect(bx + 10, y, bw - 20, 3, 0xFF192534);
        fb.fillRect(bx + 10, y, bw - 20, 1, 0x44FFFFFF);
    }

    // Gas pedal vertical racing accelerator ribs
    int gx = (m_width - 155) + 8;
    int gy = (m_height - 100) + 12 + (m_gasPedal.isPressed() ? 2 : 0);
    int gw = 120 - 16;
    for (int x = gx + 18; x < gx + gw - 18; x += 12) {
        fb.fillRect(x, gy + 28, 4, 26, 0xFF192534);
        fb.fillRect(x, gy + 28, 1, 26, 0x44FFFFFF);
    }

    // 8. Stunt Notifications Banner (Top-Center floating)
    m_stuntBanner.render(fb);
}

} // namespace UI
