#include "HUD.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace UI {

HUD::HUD()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_fuelBar(20, 20, 190, 24)
    , m_speedometer(420, 475, 46, 0.0f, 140.0f, "SPEED", "KM/H", 0.75f)
    , m_tachometer(540, 475, 46, 0.0f, 8000.0f, "RPM", "x1000", 0.80f)
    , m_brakePedal(35, 435, 120, 80, "BRAKE", Theme::RED_BRAKE)
    , m_gasPedal(805, 435, 120, 80, "GAS", Theme::GREEN_GAS)
    , m_pauseButton(895, 18, 44, 30, "||", 0xFF37474F)
    , m_distance(0.0f)
    , m_recordDistance(500.0f)
    , m_coins(0)
{
    setDimensions(m_width, m_height);
}

void HUD::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_fuelBar.setPosition(20, 20);
    m_speedometer.setCenter(m_width / 2 - 60, m_height - 65);
    m_tachometer.setCenter(m_width / 2 + 60, m_height - 65);

    m_brakePedal.setPosition(35, m_height - 105);
    m_gasPedal.setPosition(m_width - 155, m_height - 105);
    m_pauseButton.setPosition(m_width - 65, 18);
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
    m_fuelBar.update(dt);
    m_speedometer.update(dt);
    m_tachometer.update(dt);
    m_stuntBanner.update(dt);

    // Update floating score texts
    for (size_t i = 0; i < m_floatingTexts.size(); ) {
        m_floatingTexts[i].life += dt;
        m_floatingTexts[i].y -= 35.0f * dt;
        if (m_floatingTexts[i].life >= m_floatingTexts[i].maxLife) {
            m_floatingTexts[i] = m_floatingTexts.back();
            m_floatingTexts.pop_back();
        } else {
            ++i;
        }
    }
}

void HUD::render(Graphics::Framebuffer& fb) {
    // 1. Top Left: Fuel Bar
    m_fuelBar.render(fb);

    // 2. Top Center: Distance Odometer Card & Progress Track
    int distCardW = 220;
    int distCardH = 42;
    int distCardX = (m_width - distCardW) / 2;
    int distCardY = 14;

    fb.fillRect(distCardX, distCardY, distCardW, distCardH, 0xEE0A121E);
    fb.drawRect(distCardX, distCardY, distCardW, distCardH, Theme::CARD_BORDER);

    int distInt = static_cast<int>(m_distance);
    char distBuf[32];
    std::snprintf(distBuf, sizeof(distBuf), "%05dm", distInt);

    Graphics::RasterFont::drawString(fb, distCardX + 10, distCardY + 6, "DIST", Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawString(fb, distCardX + 48, distCardY + 5, distBuf, Theme::TEXT_WHITE, 2);

    int recInt = static_cast<int>(m_recordDistance);
    char recBuf[32];
    std::snprintf(recBuf, sizeof(recBuf), "REC %04dm", recInt);
    Graphics::RasterFont::drawString(fb, distCardX + 144, distCardY + 8, recBuf, Theme::GOLD, 1);

    // Mini Stage Progress Track Line
    int trackX = distCardX + 10;
    int trackY = distCardY + 30;
    int trackW = distCardW - 20;
    fb.fillRect(trackX, trackY, trackW, 4, 0xFF192534);

    float trackProg = std::clamp(m_distance / 1000.0f, 0.0f, 1.0f);
    int fillW = static_cast<int>(trackProg * trackW);
    if (fillW > 0) {
        fb.fillRect(trackX, trackY, fillW, 4, Theme::CYAN_UPGRADE);
    }
    // Indicator pip for car position
    fb.fillRect(trackX + fillW - 2, trackY - 2, 5, 8, Theme::GOLD);

    // 3. Top Right: Coin Balance Box
    int coinBoxW = 120;
    int coinBoxH = 32;
    int coinBoxX = m_width - coinBoxW - 80;
    int coinBoxY = 16;

    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE0B121C);
    fb.drawRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, Theme::CARD_BORDER);

    // Pixel gold coin icon
    int iconX = coinBoxX + 14;
    int iconY = coinBoxY + 16;
    fb.fillCircle(iconX, iconY, 7, Theme::GOLD);
    fb.drawCircle(iconX, iconY, 7, 0xFFFFA000);
    Graphics::RasterFont::drawStringCentered(fb, iconX, iconY - 3, "$", 0xFF6D4C41, 1);

    char coinBuf[32];
    std::snprintf(coinBuf, sizeof(coinBuf), "%06d", m_coins);
    Graphics::RasterFont::drawString(fb, coinBoxX + 28, coinBoxY + 8, coinBuf, Theme::GOLD, 2);

    // 4. Pause Button
    m_pauseButton.render(fb, 2);

    // 5. Floating Text FX (Popups for collected coins and fuel)
    for (const auto& ft : m_floatingTexts) {
        int fx = static_cast<int>(ft.x);
        int fy = static_cast<int>(ft.y);
        Graphics::RasterFont::drawStringCentered(fb, fx + 1, fy + 1, ft.text, 0xFF05080E, 2);
        Graphics::RasterFont::drawStringCentered(fb, fx, fy, ft.text, ft.color, 2);
    }

    // 6. Bottom Center: Gauges
    m_speedometer.render(fb);
    m_tachometer.render(fb);

    // 7. Bottom Corners: Interactive Arcade Pedals
    m_brakePedal.render(fb, 2);
    m_gasPedal.render(fb, 2);

    // 8. Stunt Notifications Banner (Top-Center floating)
    m_stuntBanner.render(fb);
}

} // namespace UI
