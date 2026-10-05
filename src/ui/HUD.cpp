#include "HUD.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <iomanip>
#include <sstream>

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
    if (pressed) {
        m_brakePedal.onMouseDown(m_brakePedal.isHovered() ? 0 : 36, m_height - 100);
    } else {
        m_brakePedal.onMouseUp(0, 0);
    }
}

void HUD::setGasVirtualPressed(bool pressed) {
    if (pressed) {
        m_gasPedal.onMouseDown(m_gasPedal.isHovered() ? 0 : m_width - 150, m_height - 100);
    } else {
        m_gasPedal.onMouseUp(0, 0);
    }
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
}

void HUD::render(Graphics::Framebuffer& fb) {
    // 1. Top Left: Fuel Bar
    m_fuelBar.render(fb);

    // 2. Top Center: Distance Odometer Card
    int distCardW = 210;
    int distCardH = 34;
    int distCardX = (m_width - distCardW) / 2;
    int distCardY = 16;

    fb.fillRect(distCardX, distCardY, distCardW, distCardH, 0xDD0C1320);
    fb.drawRect(distCardX, distCardY, distCardW, distCardH, Theme::CARD_BORDER);

    int distInt = static_cast<int>(m_distance);
    char distBuf[32];
    std::snprintf(distBuf, sizeof(distBuf), "%05dm", distInt);

    Graphics::RasterFont::drawString(fb, distCardX + 10, distCardY + 6, "DIST", Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawString(fb, distCardX + 50, distCardY + 6, distBuf, Theme::TEXT_WHITE, 2);

    int recInt = static_cast<int>(m_recordDistance);
    char recBuf[32];
    std::snprintf(recBuf, sizeof(recBuf), "REC %04dm", recInt);
    Graphics::RasterFont::drawString(fb, distCardX + 138, distCardY + 22, recBuf, Theme::GOLD, 1);

    // 3. Top Right: Coin Balance Box
    int coinBoxW = 120;
    int coinBoxH = 30;
    int coinBoxX = m_width - coinBoxW - 80;
    int coinBoxY = 18;

    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE111926);
    fb.drawRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, Theme::CARD_BORDER);

    // Pixel gold coin icon
    int iconX = coinBoxX + 12;
    int iconY = coinBoxY + 15;
    fb.fillCircle(iconX, iconY, 7, Theme::GOLD);
    fb.drawCircle(iconX, iconY, 7, 0xFFFFA000);
    Graphics::RasterFont::drawStringCentered(fb, iconX, iconY - 3, "$", 0xFF6D4C41, 1);

    char coinBuf[32];
    std::snprintf(coinBuf, sizeof(coinBuf), "%06d", m_coins);
    Graphics::RasterFont::drawString(fb, coinBoxX + 28, coinBoxY + 8, coinBuf, Theme::GOLD, 2);

    // 4. Pause Button
    m_pauseButton.render(fb, 2);

    // 5. Bottom Center: Gauges
    m_speedometer.render(fb);
    m_tachometer.render(fb);

    // 6. Bottom Corners: Interactive Arcade Pedals
    m_brakePedal.render(fb, 2);
    m_gasPedal.render(fb, 2);

    // 7. Stunt Notifications Banner (Top-Center floating)
    m_stuntBanner.render(fb);
}

} // namespace UI
