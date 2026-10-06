#include "MainMenuScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <string>
#include <algorithm>

namespace UI {

MainMenuScreen::MainMenuScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_btnStart(360, 230, 240, 48, "START RACE", Theme::GREEN_GAS)
    , m_btnGarage(360, 290, 240, 48, "GARAGE & TUNING", Theme::GOLD)
    , m_btnStages(360, 350, 240, 48, "STAGE SELECT", Theme::CYAN_UPGRADE)
    , m_btnQuit(360, 410, 240, 44, "QUIT GAME", 0xFF37474F)
{
    setDimensions(m_width, m_height);
}

void MainMenuScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 250;
    int btnH = 46;
    int cx = width / 2 - btnW / 2;

    m_btnStart.setPosition(cx, 230);
    m_btnStart.setSize(btnW, btnH);

    m_btnGarage.setPosition(cx, 288);
    m_btnGarage.setSize(btnW, btnH);

    m_btnStages.setPosition(cx, 346);
    m_btnStages.setSize(btnW, btnH);

    m_btnQuit.setPosition(cx, 404);
    m_btnQuit.setSize(btnW, 42);
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
    // 1. Vibrant Bright Daylight Countryside Sky Gradient
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF1E88E5, 0xFF90CAF9);

    // 2. Glowing Golden Sun in sky with rotating rays
    int sunX = 140;
    int sunY = 75;
    for (int a = 0; a < 8; ++a) {
        float rayAng = a * (3.14159f / 4.0f) + m_animTime * 0.35f;
        int r1x = static_cast<int>(sunX + std::cos(rayAng) * 22.0f);
        int r1y = static_cast<int>(sunY + std::sin(rayAng) * 22.0f);
        int r2x = static_cast<int>(sunX + std::cos(rayAng) * 36.0f);
        int r2y = static_cast<int>(sunY + std::sin(rayAng) * 36.0f);
        fb.drawLine(r1x, r1y, r2x, r2y, 0xFFFFE082);
    }
    fb.fillCircle(sunX, sunY, 26, 0x44FFF59D); // Soft halo
    fb.fillCircle(sunX, sunY, 19, 0xFFFFEE58); // Sun disc
    fb.fillCircle(sunX, sunY, 13, 0xFFFFF9C4); // Core glow

    // 3. Drifting Puffy Cartoon Pixel Clouds
    auto drawPixelCloud = [&](int cx, int cy, int size) {
        // Soft shaded underside
        fb.fillCircle(cx, cy + 3, size + 2, 0xFFB0BEC5);
        fb.fillCircle(cx + size, cy + 5, (size * 4) / 5 + 2, 0xFFB0BEC5);
        fb.fillCircle(cx - size, cy + 5, (size * 3) / 5 + 2, 0xFFB0BEC5);
        fb.fillRect(cx - size, cy + 2, size * 2, size / 2 + 3, 0xFFB0BEC5);

        // Crisp white puffy body
        fb.fillCircle(cx, cy, size, 0xFFFFFFFF);
        fb.fillCircle(cx + size, cy + 2, (size * 4) / 5, 0xFFFFFFFF);
        fb.fillCircle(cx - size, cy + 3, (size * 3) / 5, 0xFFFFFFFF);
        fb.fillRect(cx - size, cy, size * 2, size / 2, 0xFFFFFFFF);
    };

    float cloud1X = std::fmod(m_animTime * 12.0f, static_cast<float>(m_width + 200)) - 100.0f;
    float cloud2X = std::fmod(m_animTime * 7.0f + 400.0f, static_cast<float>(m_width + 200)) - 100.0f;
    float cloud3X = std::fmod(m_animTime * 18.0f + 700.0f, static_cast<float>(m_width + 200)) - 100.0f;

    drawPixelCloud(static_cast<int>(cloud1X), 55, 24);
    drawPixelCloud(static_cast<int>(cloud2X), 115, 30);
    drawPixelCloud(static_cast<int>(cloud3X), 35, 18);

    // 4. Parallax Mountain & Rolling Lush Green Hills
    for (int x = 0; x < m_width; ++x) {
        // Distant Mountain Ridge
        float h0 = std::sin(x * 0.005f + 0.5f) * 45.0f + 325.0f;
        int y0 = static_cast<int>(h0);
        for (int y = y0; y < m_height; ++y) {
            fb.setPixelFast(x, y, 0xFF455A64);
        }
        if (y0 < 305) {
            fb.fillRect(x, y0, 1, 5, 0xFFECEFF1); // Snowcaps
        }

        // Mid Hills (Meadow Green)
        float h1 = std::sin(x * 0.009f + m_animTime * 0.12f) * 35.0f + 375.0f;
        int y1 = static_cast<int>(h1);
        for (int y = y1; y < m_height; ++y) {
            fb.setPixelFast(x, y, (y - y1 < 4) ? 0xFF388E3C : 0xFF2E7D32);
        }

        // Foreground Rolling Lawn (Vibrant Emerald Top with Warm Chocolate Soil)
        float h2 = std::sin(x * 0.014f + 1.2f) * 26.0f + 435.0f;
        int y2 = static_cast<int>(h2);
        for (int y = y2; y < m_height; ++y) {
            int depth = y - y2;
            uint32_t col;
            if (depth == 0) col = 0xFF81C784; // Sunlit crest edge
            else if (depth < 6) col = 0xFF4CAF50; // Lush green grass
            else if (depth < 14) col = 0xFF2E7D32; // Deep root layer
            else if (depth < 45) {
                int speck = ((x ^ y) & 7);
                col = (speck == 0) ? 0xFF6D4C41 : 0xFF795548;
            } else {
                col = 0xFF4E342E; // Deep bedrock
            }
            fb.setPixelFast(x, y, col);
        }

        // Grass tufts on foreground ridge
        if (x % 7 == 0 && y2 > 2 && y2 < m_height) {
            fb.setPixelFast(x, y2 - 1, 0xFF81C784);
            fb.setPixelFast(x + 1, y2 - 2, 0xFFA5D6A7);
        }
    }

    // 5. Animated Cartoon Jeep parked on foreground menu hill
    int jeepX = 180;
    float jeepHillY = std::sin(jeepX * 0.014f + 1.2f) * 26.0f + 435.0f;
    float jeepBob = std::sin(m_animTime * 3.5f) * 2.0f;
    int jy = static_cast<int>(jeepHillY - 32 + jeepBob);

    // Chassis Body (Glossy Racing Red)
    fb.fillRect(jeepX - 35, jy, 70, 18, 0xFFD32F2F);
    fb.fillRect(jeepX - 35, jy, 70, 3, 0xFFFF5252); // Top gloss
    fb.fillRect(jeepX - 37, jy + 6, 74, 12, 0xFFC62828); // Lower fender flare
    fb.fillRect(jeepX - 10, jy - 16, 28, 16, 0xFFB71C1C); // Cabin hood / cabin back

    // Windshield & Roll Cage
    fb.fillRect(jeepX + 5, jy - 14, 12, 14, 0xFF81D4FA); // Glass
    fb.drawLine(jeepX - 10, jy - 16, jeepX - 10, jy, 0xFF212121); // Roll bar rear
    fb.drawLine(jeepX - 10, jy - 16, jeepX + 18, jy - 16, 0xFF212121); // Roll bar roof
    fb.drawLine(jeepX + 18, jy - 16, jeepX + 18, jy, 0xFF212121); // Roll bar pillar

    // Driver in White/Blue Helmet
    fb.fillCircle(jeepX - 2, jy - 10, 7, 0xFFECEFF1);
    fb.fillRect(jeepX + 1, jy - 12, 5, 4, 0xFF1E88E5); // Visor

    // Front Bumper & Headlight
    fb.fillRect(jeepX + 35, jy + 8, 6, 8, 0xFF212121);
    fb.fillRect(jeepX + 33, jy + 2, 4, 5, 0xFFFFF59D); // Headlight glow

    // Big Rugged Monster Truck Wheels with Rims
    auto drawMenuWheel = [&](int wx, int wy) {
        fb.fillCircle(wx, wy, 15, 0xFF212121); // Tire rubber
        fb.drawCircle(wx, wy, 15, 0xFF424242);
        fb.fillCircle(wx, wy, 9, 0xFFB0BEC5);  // Chrome rim
        fb.fillCircle(wx, wy, 4, 0xFF37474F);  // Hub
    };
    drawMenuWheel(jeepX - 24, jy + 18);
    drawMenuWheel(jeepX + 24, jy + 18);

    // 5. Top Right: Coin Balance Box
    int coinBoxW = 150;
    int coinBoxH = 34;
    int coinBoxX = m_width - coinBoxW - 24;
    int coinBoxY = 16;
    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE0B121C);
    fb.drawRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, Theme::CARD_BORDER);

    // Spinning / Shining Gold Coin
    int coinX = coinBoxX + 17;
    int coinY = coinBoxY + 17;
    int coinW = static_cast<int>(std::abs(std::cos(m_animTime * 3.5f)) * 7.0f) + 2;
    fb.fillRect(coinX - coinW, coinY - 7, coinW * 2, 14, Theme::GOLD);
    fb.drawRect(coinX - coinW, coinY - 7, coinW * 2, 14, 0xFFFFA000);
    if (coinW >= 5) {
        Graphics::RasterFont::drawStringCentered(fb, coinX, coinY - 3, "$", 0xFF4E342E, 1);
    }

    char coinBuf[32];
    std::snprintf(coinBuf, sizeof(coinBuf), "%06d", totalCoins);
    Graphics::RasterFont::drawString(fb, coinBoxX + 36, coinBoxY + 9, coinBuf, Theme::GOLD, 2);

    // 6. Master Animated Title Logo
    int cx = m_width / 2;
    float bounceY = std::sin(m_animTime * 2.8f) * 4.5f;
    int titleY = static_cast<int>(52 + bounceY);

    // Deep 3D Drop Shadow Layers
    Graphics::RasterFont::drawStringCentered(fb, cx + 4, titleY + 4, "HILL CLIMB", 0xFF05080E, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 4, titleY + 44, "RACING", 0xFF05080E, 4);

    Graphics::RasterFont::drawStringCentered(fb, cx + 2, titleY + 2, "HILL CLIMB", 0xFFB26A00, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 2, titleY + 42, "RACING", 0xFFE65100, 4);

    // Primary Text (Metallic Gold / Sunset Amber)
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY, "HILL CLIMB", Theme::GOLD, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY + 40, "RACING", 0xFFFF9100, 4);

    // Highlight Shimmer on top line
    Graphics::RasterFont::drawStringCentered(fb, cx - 1, titleY - 1, "HILL CLIMB", 0xFFFFE082, 4);

    // Engine Badge Subtitle
    int badgeW = 280;
    int badgeH = 22;
    int badgeX = cx - badgeW / 2;
    int badgeY = titleY + 92;
    fb.fillRect(badgeX, badgeY, badgeW, badgeH, 0xEE0B1422);
    fb.drawRect(badgeX, badgeY, badgeW, badgeH, Theme::CYAN_UPGRADE);
    Graphics::RasterFont::drawStringCentered(fb, cx, badgeY + 5, "* PURE RASTER C++ ENGINE *", Theme::CYAN_UPGRADE, 1);

    // 7. Navigation Buttons
    m_btnStart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnStages.render(fb, 2);
    m_btnQuit.render(fb, 2);

    // 8. Footer Controls Hint Bar
    fb.fillRect(0, m_height - 28, m_width, 28, 0xEE090E18);
    fb.drawLine(0, m_height - 28, m_width, m_height - 28, 0xFF1E2B3E);
    Graphics::RasterFont::drawStringCentered(fb, cx, m_height - 18,
        "[A]/[LEFT] BRAKE & TILT BACK   |   [D]/[RIGHT] GAS & TILT FWD   |   [ESC] PAUSE",
        Theme::TEXT_MUTED, 1);
}

} // namespace UI
