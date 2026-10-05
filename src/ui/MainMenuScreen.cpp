#include "MainMenuScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <string>

namespace UI {

MainMenuScreen::MainMenuScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_btnStart(370, 240, 220, 48, "START DRIVE", Theme::GREEN_GAS)
    , m_btnGarage(370, 300, 220, 48, "GARAGE SHOP", Theme::GOLD)
    , m_btnStages(370, 360, 220, 48, "STAGE SELECT", 0xFF2979FF)
    , m_btnQuit(370, 420, 220, 44, "QUIT GAME", 0xFF37474F)
{
    setDimensions(m_width, m_height);
}

void MainMenuScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 240;
    int btnH = 46;
    int cx = width / 2 - btnW / 2;

    m_btnStart.setPosition(cx, 235);
    m_btnStart.setSize(btnW, btnH);

    m_btnGarage.setPosition(cx, 295);
    m_btnGarage.setSize(btnW, btnH);

    m_btnStages.setPosition(cx, 355);
    m_btnStages.setSize(btnW, btnH);

    m_btnQuit.setPosition(cx, 415);
    m_btnQuit.setSize(btnW, btnH);
}

void MainMenuScreen::onMouseMove(int px, int py) {
    m_btnStart.onMouseMove(px, py);
    m_btnGarage.onMouseMove(px, py);
    m_btnStages.onMouseMove(px, py);
    m_btnQuit.onMouseMove(px, py);
}

bool MainMenuScreen::onMouseDown(int px, int py) {
    if (m_btnStart.onMouseDown(px, py)) return true;
    if (m_btnGarage.onMouseDown(px, py)) return true;
    if (m_btnStages.onMouseDown(px, py)) return true;
    if (m_btnQuit.onMouseDown(px, py)) return true;
    return false;
}

void MainMenuScreen::onMouseUp(int px, int py) {
    m_btnStart.onMouseUp(px, py);
    m_btnGarage.onMouseUp(px, py);
    m_btnStages.onMouseUp(px, py);
    m_btnQuit.onMouseUp(px, py);
}

void MainMenuScreen::update(float dt) {
    m_animTime += dt;
}

void MainMenuScreen::render(Graphics::Framebuffer& fb, int totalCoins) {
    // 1. Sky Gradient Background
    fb.fillVerticalGradient(0, 0, m_width, m_height, Theme::SKY_TOP, Theme::SKY_HORIZON);

    // 2. Distant rolling hill silhouettes
    for (int x = 0; x < m_width; ++x) {
        float h1 = std::sin(x * 0.005f + m_animTime * 0.2f) * 35.0f + 400.0f;
        int y1 = static_cast<int>(h1);
        for (int y = y1; y < m_height; ++y) {
            fb.setPixelFast(x, y, 0xFF1B324B);
        }

        float h2 = std::sin(x * 0.012f + 1.5f) * 25.0f + 460.0f;
        int y2 = static_cast<int>(h2);
        for (int y = y2; y < m_height; ++y) {
            fb.setPixelFast(x, y, 0xFF142436);
        }
    }

    // 3. Top Banner: Coin Display
    int coinBoxW = 140;
    int coinBoxH = 32;
    int coinBoxX = m_width - coinBoxW - 20;
    int coinBoxY = 16;
    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE111926);
    fb.drawRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, Theme::CARD_BORDER);
    fb.fillCircle(coinBoxX + 15, coinBoxY + 16, 8, Theme::GOLD);
    Graphics::RasterFont::drawStringCentered(fb, coinBoxX + 15, coinBoxY + 13, "$", 0xFF5D4037, 1);
    char coinBuf[32];
    std::snprintf(coinBuf, sizeof(coinBuf), "%06d", totalCoins);
    Graphics::RasterFont::drawString(fb, coinBoxX + 32, coinBoxY + 8, coinBuf, Theme::GOLD, 2);

    // 4. Animated Title Logo
    int cx = m_width / 2;
    float bounceY = std::sin(m_animTime * 2.5f) * 4.0f;
    int titleY = static_cast<int>(60 + bounceY);

    // Drop Shadow
    Graphics::RasterFont::drawStringCentered(fb, cx + 3, titleY + 3, "HILL CLIMB", 0xFF080C14, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 3, titleY + 43, "RACING", 0xFF080C14, 4);

    // Foreground Gold / Amber Gradient effect
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY, "HILL CLIMB", Theme::GOLD, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY + 40, "RACING", 0xFFFF8F00, 4);

    // Subtitle badge
    int badgeW = 260;
    int badgeH = 20;
    fb.fillRect(cx - badgeW / 2, titleY + 95, badgeW, badgeH, 0xDD152030);
    fb.drawRect(cx - badgeW / 2, titleY + 95, badgeW, badgeH, Theme::CYAN_UPGRADE);
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY + 100, "100% PURE RASTER C++ ENGINE", Theme::TEXT_WHITE, 1);

    // 5. Menu Buttons
    m_btnStart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnStages.render(fb, 2);
    m_btnQuit.render(fb, 2);

    // 6. Footer Credits & Controls Hint
    Graphics::RasterFont::drawStringCentered(fb, cx, m_height - 24, "CONTROLS: [A]/[LEFT] BRAKE  |  [D]/[RIGHT] GAS  |  CLICK PEDALS", Theme::TEXT_MUTED, 1);
}

} // namespace UI
