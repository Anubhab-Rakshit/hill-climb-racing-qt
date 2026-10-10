#include "MainMenuScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "Sprite.h"
#include "VehicleConfig.h"
#include <cmath>
#include <string>
#include <algorithm>

namespace UI {

MainMenuScreen::MainMenuScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_btnStart(360, 230, 250, 48, "START RACE", Theme::GREEN_GAS)
    , m_btnGarage(360, 288, 250, 48, "GARAGE & TUNING", Theme::GOLD)
    , m_btnStages(360, 346, 250, 48, "STAGE SELECT", Theme::CYAN_UPGRADE)
    , m_btnQuit(360, 404, 250, 44, "QUIT GAME", 0xFF37474F)
{
    m_btnStart.setKeyHint("ENTER");
    m_btnQuit.setKeyHint("ESC");
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
    Core::ProfileManager dummy;
    dummy.addCoins(totalCoins);
    render(fb, dummy);
}

void MainMenuScreen::render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile) {
    // 1. Vibrant Bright Daylight Countryside Sky Gradient
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF1976D2, 0xFF90CAF9);

    // 2. Glowing Golden Sun in sky with rotating rays
    int sunX = 135;
    int sunY = 72;
    for (int a = 0; a < 8; ++a) {
        float rayAng = a * (3.14159f / 4.0f) + m_animTime * 0.30f;
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

    drawPixelCloud(static_cast<int>(cloud1X), 52, 24);
    drawPixelCloud(static_cast<int>(cloud2X), 112, 30);
    drawPixelCloud(static_cast<int>(cloud3X), 32, 18);

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

    // 5. Player's Selected Vehicle Parked on the Rolling Hill with Idling Bob!
    int vehX = 185;
    float hillY = std::sin(vehX * 0.014f + 1.2f) * 26.0f + 435.0f;
    float idleBob = std::sin(m_animTime * 3.5f) * 1.5f;
    int vy = static_cast<int>(hillY - 32 + idleBob);

    // Tire contact shadows
    fb.fillCircle(vehX - 35, static_cast<int>(hillY + 4), 18, 0x550B121C);
    fb.fillCircle(vehX + 35, static_cast<int>(hillY + 4), 18, 0x550B121C);

    // Render true pixel-art vehicle sprite & driver
    Graphics::SpriteRenderer::renderGarageVehicle(fb, vehX, vy, idleBob, profile, profile.selectedVehicle(), profile.selectedDriver());

    // Idle exhaust puffs from tailpipe
    float puffTimer = std::fmod(m_animTime * 2.2f, 1.0f);
    int pfx = vehX - 52 - static_cast<int>(puffTimer * 22.0f);
    int pfy = vy + 12 - static_cast<int>(puffTimer * 12.0f);
    int pr = static_cast<int>(puffTimer * 7.0f) + 2;
    fb.fillCircle(pfx, pfy, pr, 0x44B0BEC5);

    // Vehicle Name Plaque below vehicle
    const auto& vehCfg = Physics::VehicleRegistry::getConfig(profile.selectedVehicle());
    int vBadgeW = Graphics::RasterFont::getTextWidth(vehCfg.name, 1) + 20;
    int vBadgeX = vehX - vBadgeW / 2;
    int vBadgeY = static_cast<int>(hillY + 18);
    UIComponents::drawArcadePanel(fb, vBadgeX, vBadgeY, vBadgeW, 19, 0xEE0B121C, 0xFF2A3A4E, 0, 0, true, true);
    Graphics::RasterFont::drawStringCentered(fb, vehX, vBadgeY + 4, vehCfg.name, Theme::GOLD, 1);

    // 6. Top Right: Master Currency Pill
    int coinBoxW = 155;
    int coinBoxH = 34;
    int coinBoxX = m_width - coinBoxW - 24;
    int coinBoxY = 16;
    UIComponents::drawCoinBadge(fb, coinBoxX, coinBoxY, coinBoxW, coinBoxH, profile.coins(), m_animTime);

    // 7. Master 3D Arcade Title Banner
    int cx = m_width / 2;
    float bounceY = std::sin(m_animTime * 2.8f) * 4.0f;
    int titleY = static_cast<int>(46 + bounceY);

    // Title Backdrop Plaque
    int plaqueW = 440;
    int plaqueH = 110;
    int plaqueX = cx - plaqueW / 2;
    int plaqueY = titleY - 14;
    UIComponents::drawArcadePanel(fb, plaqueX, plaqueY, plaqueW, plaqueH, 0xD8080E18, 0xFF37474F, Theme::GOLD, 3, true, true);

    // Plaque Corner Rivets
    fb.fillCircle(plaqueX + 6, plaqueY + 6, 2, Theme::CHROME_MID);
    fb.fillCircle(plaqueX + plaqueW - 7, plaqueY + 6, 2, Theme::CHROME_MID);
    fb.fillCircle(plaqueX + 6, plaqueY + plaqueH - 7, 2, Theme::CHROME_MID);
    fb.fillCircle(plaqueX + plaqueW - 7, plaqueY + plaqueH - 7, 2, Theme::CHROME_MID);

    // Deep 3D Drop Shadow Layers for Lettering
    Graphics::RasterFont::drawStringCentered(fb, cx + 4, titleY + 4, "HILL CLIMB", 0xFF05080E, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 4, titleY + 40, "RACING", 0xFF05080E, 4);

    Graphics::RasterFont::drawStringCentered(fb, cx + 2, titleY + 2, "HILL CLIMB", 0xFFB26A00, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 2, titleY + 38, "RACING", 0xFFE65100, 4);

    // Primary Text (Metallic Gold / Sunset Amber)
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY, "HILL CLIMB", Theme::GOLD, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx, titleY + 36, "RACING", 0xFFFF9100, 4);

    // Highlight Shimmer on top edge
    Graphics::RasterFont::drawStringCentered(fb, cx - 1, titleY - 1, "HILL CLIMB", 0xFFFFE082, 4);

    // Subtitle Badge
    int badgeW = 280;
    int badgeH = 20;
    int badgeX = cx - badgeW / 2;
    int badgeY = titleY + 76;
    UIComponents::drawArcadePanel(fb, badgeX, badgeY, badgeW, badgeH, 0xEE0B1422, Theme::CYAN_UPGRADE, 0, 0, false, true);
    Graphics::RasterFont::drawStringCentered(fb, cx, badgeY + 4, "* PURE RASTER C++ ENGINE *", Theme::CYAN_UPGRADE, 1);

    // 8. Navigation Buttons
    m_btnStart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnStages.render(fb, 2);
    m_btnQuit.render(fb, 2);

    // 9. Footer Controls Hint Bar with Real Keycaps
    int footH = 28;
    int footY = m_height - footH;
    fb.fillRect(0, footY, m_width, footH, 0xEE090E18);
    fb.drawLine(0, footY, m_width, footY, 0xFF1E2B3E);

    int startKx = cx - 380;
    int ky = footY + 6;

    // A / LEFT: BRAKE
    UIComponents::drawKeycap(fb, startKx, ky, "A");
    UIComponents::drawKeycap(fb, startKx + 22, ky, "LEFT");
    Graphics::RasterFont::drawString(fb, startKx + 64, ky + 4, "BRAKE & TILT BACK", Theme::TEXT_WHITE, 1);

    // D / RIGHT: GAS
    int midKx = startKx + 225;
    UIComponents::drawKeycap(fb, midKx, ky, "D");
    UIComponents::drawKeycap(fb, midKx + 22, ky, "RIGHT");
    Graphics::RasterFont::drawString(fb, midKx + 68, ky + 4, "GAS & TILT FWD", Theme::TEXT_WHITE, 1);

    // ESC: PAUSE
    int rightKx = midKx + 220;
    UIComponents::drawKeycap(fb, rightKx, ky, "ESC");
    Graphics::RasterFont::drawString(fb, rightKx + 36, ky + 4, "PAUSE / BACK", Theme::TEXT_WHITE, 1);

    // ENTER: SELECT
    int enterKx = rightKx + 150;
    UIComponents::drawKeycap(fb, enterKx, ky, "ENTER");
    Graphics::RasterFont::drawString(fb, enterKx + 46, ky + 4, "START RACE", Theme::TEXT_WHITE, 1);
}

} // namespace UI
