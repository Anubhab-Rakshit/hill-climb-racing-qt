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
    , m_btnStart(540, 150, 340, 52, "START RACE", Theme::GREEN_GAS)
    , m_btnGarage(540, 212, 340, 48, "GARAGE & TUNING", Theme::GOLD)
    , m_btnStages(540, 270, 340, 48, "STAGE SELECT", Theme::CYAN_UPGRADE)
    , m_btnQuit(540, 328, 340, 42, "QUIT GAME", 0xFF2A3A4E)
{
    m_btnStart.setKeyHint("ENTER");
    m_btnGarage.setKeyHint("G");
    m_btnStages.setKeyHint("S");
    m_btnQuit.setKeyHint("ESC");
    setDimensions(m_width, m_height);
}

void MainMenuScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 340;
    int btnX = width - btnW - 70; // 550 on 960 canvas
    int startY = 150;

    m_btnStart.setPosition(btnX, startY);
    m_btnStart.setSize(btnW, 52);

    m_btnGarage.setPosition(btnX, startY + 62);
    m_btnGarage.setSize(btnW, 48);

    m_btnStages.setPosition(btnX, startY + 120);
    m_btnStages.setSize(btnW, 48);

    m_btnQuit.setPosition(btnX, startY + 178);
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
    // 1. Radiant Countryside Sky Gradient (Atmospheric daylight)
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF1976D2, 0xFF90CAF9);

    // 2. Animated Rotating Sun with Corona & Shimmering Rays
    int sunX = 115;
    int sunY = 65;
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

    // 3. Drifting Puffy Pixel Clouds
    auto drawPixelCloud = [&](int cx, int cy, int size) {
        fb.fillCircle(cx, cy + 3, size + 2, 0xFFB0BEC5);
        fb.fillCircle(cx + size, cy + 5, (size * 4) / 5 + 2, 0xFFB0BEC5);
        fb.fillCircle(cx - size, cy + 5, (size * 3) / 5 + 2, 0xFFB0BEC5);
        fb.fillRect(cx - size, cy + 2, size * 2, size / 2 + 3, 0xFFB0BEC5);

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

    // 4. Parallax Mountain Layers & Scenic Hills
    for (int x = 0; x < m_width; ++x) {
        // Layer 1: Distant Alpine Peaks
        float h0 = std::sin(x * 0.005f + 0.5f) * 45.0f + 295.0f;
        int y0 = static_cast<int>(h0);
        for (int y = y0; y < m_height; ++y) {
            fb.setPixelFast(x, y, 0xFF455A64);
        }
        if (y0 < 275) {
            fb.fillRect(x, y0, 1, 5, 0xFFECEFF1); // Snowcaps
        }

        // Layer 2: Mid-ground Pine Foothills
        float h1 = std::sin(x * 0.008f + m_animTime * 0.10f) * 32.0f + 345.0f;
        int y1 = static_cast<int>(h1);
        for (int y = y1; y < m_height; ++y) {
            fb.setPixelFast(x, y, (y - y1 < 4) ? 0xFF388E3C : 0xFF2E7D32);
        }

        // Layer 3: Foreground Emerald Lawn with Geological Strata
        float h2 = std::sin(x * 0.011f + 0.8f) * 26.0f + 395.0f;
        int y2 = static_cast<int>(h2);
        for (int y = y2; y < m_height; ++y) {
            int depth = y - y2;
            uint32_t col;
            if (depth == 0) col = 0xFF81C784; // Sunlit crest
            else if (depth < 6) col = 0xFF4CAF50; // Lush emerald grass
            else if (depth < 14) col = 0xFF2E7D32; // Root layer
            else if (depth < 45) {
                int speck = ((x ^ y) & 7);
                col = (speck == 0) ? 0xFF6D4C41 : 0xFF795548;
            } else {
                col = 0xFF4E342E; // Deep bedrock
            }
            fb.setPixelFast(x, y, col);
        }

        // Grass blade tufts on foreground ridge
        if (x % 7 == 0 && y2 > 2 && y2 < m_height) {
            fb.setPixelFast(x, y2 - 1, 0xFF81C784);
            fb.setPixelFast(x + 1, y2 - 2, 0xFFA5D6A7);
        }
    }

    // 5. ZONE A (LEFT): Hero Vehicle Showcase on Scenic Plateau
    int vehX = 265;
    float hillY = std::sin(vehX * 0.011f + 0.8f) * 26.0f + 395.0f;
    float idleBob = std::sin(m_animTime * 3.5f) * 1.5f;
    int vy = static_cast<int>(hillY - 26 + idleBob);

    // High-tech illuminated showcase runner pad
    int padW = 220;
    int padH = 6;
    int padX = vehX - padW / 2;
    int padY = static_cast<int>(hillY + 6);
    fb.fillRect(padX, padY, padW, padH, 0xEE142232);
    fb.drawRect(padX, padY, padW, padH, 0xFF2A3D54);
    fb.fillRect(padX + 2, padY, padW - 4, 1, Theme::CYAN_UPGRADE);

    // Tire ground contact shadows
    fb.fillCircle(vehX - 35, static_cast<int>(hillY + 4), 16, 0x660B121C);
    fb.fillCircle(vehX + 35, static_cast<int>(hillY + 4), 16, 0x660B121C);

    // Render player's actual customized vehicle (drawLift = false for natural meadow stance)
    Graphics::SpriteRenderer::renderGarageVehicle(fb, vehX, vy, idleBob, profile,
                                                  profile.selectedVehicle(), profile.selectedDriver(),
                                                  /*drawLift=*/false);

    // Tailpipe idle exhaust puffs drifting in wind (combustion engines only)
    if (profile.selectedVehicle() != Physics::VehicleType::ELECTRIC_OFFROADER) {
        float puffTimer = std::fmod(m_animTime * 2.2f, 1.0f);
        int pfx = vehX - 52 - static_cast<int>(puffTimer * 18.0f);
        int pfy = vy + 10 - static_cast<int>(puffTimer * 8.0f);
        int pr = std::clamp(static_cast<int>(puffTimer * 3.5f) + 1, 1, 4);
        uint32_t pA = static_cast<uint32_t>((1.0f - puffTimer) * 60.0f);
        fb.blendPixelFast(pfx, pfy, (pA << 24) | 0x00B0BEC5);
        if (pr > 1) {
            fb.blendPixelFast(pfx + 1, pfy, ((pA / 2) << 24) | 0x00B0BEC5);
            fb.blendPixelFast(pfx, pfy - 1, ((pA / 2) << 24) | 0x00B0BEC5);
        }
    }

    // Telemetry Showcase Card directly beneath vehicle
    const auto& vehCfg = Physics::VehicleRegistry::getConfig(profile.selectedVehicle());
    int cardW = 390;
    int cardH = 88;
    int cardX = vehX - cardW / 2;
    int cardY = static_cast<int>(hillY + 16);

    UIComponents::drawArcadePanel(fb, cardX, cardY, cardW, cardH, 0xF00B1422, Theme::CARD_BORDER, Theme::GOLD, 3, true, true);

    // Active Vehicle Title & Tagline
    Graphics::RasterFont::drawString(fb, cardX + 14, cardY + 10, vehCfg.name, Theme::GOLD, 2);
    // [ACTIVE RIG] Pill
    int tagW = 86;
    int tagX = cardX + cardW - tagW - 12;
    UIComponents::drawArcadePanel(fb, tagX, cardY + 8, tagW, 18, 0xFF143820, Theme::GREEN_GAS, 0, 0, false, true);
    Graphics::RasterFont::drawStringCentered(fb, tagX + tagW / 2, cardY + 12, "ACTIVE RIG", Theme::GREEN_GAS, 1);

    Graphics::RasterFont::drawString(fb, cardX + 14, cardY + 30, vehCfg.tagline, Theme::TEXT_MUTED, 1);

    // 3 Performance Spec Chips
    int chipY = cardY + 48;
    auto drawMiniStat = [&](int x, const std::string& label, int level, uint32_t barCol) {
        int w = 114;
        fb.fillRect(x, chipY, w, 28, 0xEE111A26);
        fb.drawRect(x, chipY, w, 28, 0xFF2A3A4E);
        Graphics::RasterFont::drawString(fb, x + 6, chipY + 4, label, Theme::TEXT_MUTED, 1);
        UIComponents::drawSegmentedBar(fb, x + 6, chipY + 16, w - 12, 7, level, 15, barCol);
    };

    int engLvl = profile.getUpgradeLevel(profile.selectedVehicle(), Core::ProfileManager::UPGRADE_ENGINE);
    int tireLvl = profile.getUpgradeLevel(profile.selectedVehicle(), Core::ProfileManager::UPGRADE_TIRES);
    int suspLvl = profile.getUpgradeLevel(profile.selectedVehicle(), Core::ProfileManager::UPGRADE_SUSPENSION);

    drawMiniStat(cardX + 14, "ENGINE", engLvl, Theme::GOLD);
    drawMiniStat(cardX + 138, "TIRES", tireLvl, Theme::CYAN_UPGRADE);
    drawMiniStat(cardX + 262, "SUSPENSION", suspLvl, Theme::GREEN_GAS);

    // 6. ZONE B (RIGHT): Master Navigation Console & Career Ticker
    // Breathing pulse halo on START button
    float pulseAlpha = (std::sin(m_animTime * 4.0f) * 0.5f + 0.5f);
    uint32_t pulseCol = UI::Theme::blend(0x0000E676, 0x6600E676, pulseAlpha);
    fb.drawRect(m_btnStart.x() - 1, m_btnStart.y() - 1, m_btnStart.width() + 2, m_btnStart.height() + 2, pulseCol);

    m_btnStart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnStages.render(fb, 2);
    m_btnQuit.render(fb, 2);

    // Career Highlights Box beneath buttons
    int btnW = 340;
    int btnX = m_width - btnW - 70;
    int tickY = 385;
    int tickH = 88;
    UIComponents::drawArcadePanel(fb, btnX, tickY, btnW, tickH, 0xF00B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 3, true, true);

    Graphics::RasterFont::drawString(fb, btnX + 14, tickY + 10, "CAREER HIGHLIGHTS", Theme::CYAN_UPGRADE, 1);

    // Record stats
    int bestDist = profile.getRecordDistance("countryside");
    std::string recStr = "COUNTRYSIDE BEST: " + UIComponents::formatNumber(bestDist) + "m";
    Graphics::RasterFont::drawString(fb, btnX + 14, tickY + 28, recStr, Theme::TEXT_WHITE, 1);

    // Vehicle collection count
    int unlockedCount = 0;
    for (int v = 0; v < static_cast<int>(Physics::VehicleType::COUNT); ++v) {
        if (profile.isVehicleUnlocked(static_cast<Physics::VehicleType>(v))) unlockedCount++;
    }
    std::string collStr = "FLEET: " + std::to_string(unlockedCount) + "/10 VEHICLES UNLOCKED";
    Graphics::RasterFont::drawString(fb, btnX + 14, tickY + 46, collStr, Theme::GOLD, 1);

    // Checkpoints cleared
    int cps = profile.getStageCheckpointProgress("countryside");
    std::string cpStr = "PROGRESS: CHECKPOINT " + std::to_string(cps) + "/5 CLEARED";
    Graphics::RasterFont::drawString(fb, btnX + 14, tickY + 64, cpStr, Theme::GREEN_GAS, 1);

    // 7. TOP BAR: Driver Identity + Title Marquee + Master Coin Badge
    // Top-Left: Pilot Profile Pill
    const auto& drvCfg = Physics::VehicleRegistry::getDriver(profile.selectedDriver());
    int pillW = 210;
    int pillH = 34;
    int pillX = 24;
    int pillY = 16;
    UIComponents::drawArcadePanel(fb, pillX, pillY, pillW, pillH, 0xEE0B1422, Theme::CARD_BORDER, drvCfg.themeColor, 2, true, true);
    // Pilot helmet icon pip
    fb.fillCircle(pillX + 18, pillY + 17, 7, drvCfg.themeColor);
    fb.fillCircle(pillX + 18, pillY + 17, 4, 0xFFFFFFFF);
    std::string drvTitle = "PILOT: " + drvCfg.name;
    Graphics::RasterFont::drawString(fb, pillX + 34, pillY + 11, drvTitle, Theme::TEXT_WHITE, 1);

    // Top-Right: Master Currency Badge
    int coinBoxW = 155;
    int coinBoxH = 34;
    int coinBoxX = m_width - coinBoxW - 24;
    int coinBoxY = 16;
    UIComponents::drawCoinBadge(fb, coinBoxX, coinBoxY, coinBoxW, coinBoxH, profile.coins(), m_animTime);

    // Top-Center: Master Arcade Title Marquee
    int cx = m_width / 2;
    float bounceY = std::sin(m_animTime * 2.5f) * 3.0f;
    int titleY = static_cast<int>(18 + bounceY);

    // Marquee Frame
    int marqueeW = 460;
    int marqueeH = 96;
    int marqueeX = cx - marqueeW / 2;
    int marqueeY = titleY;
    UIComponents::drawArcadePanel(fb, marqueeX, marqueeY, marqueeW, marqueeH, 0xE8080E18, 0xFF37474F, Theme::GOLD, 4, true, true);

    // Chrome Rivets at 4 corners
    fb.fillCircle(marqueeX + 7, marqueeY + 7, 2, Theme::CHROME_MID);
    fb.fillCircle(marqueeX + marqueeW - 8, marqueeY + 7, 2, Theme::CHROME_MID);
    fb.fillCircle(marqueeX + 7, marqueeY + marqueeH - 8, 2, Theme::CHROME_MID);
    fb.fillCircle(marqueeX + marqueeW - 8, marqueeY + marqueeH - 8, 2, Theme::CHROME_MID);

    // Dual 3D Drop Shadow & Vibrant Typography
    Graphics::RasterFont::drawStringCentered(fb, cx + 3, marqueeY + 13, "HILL CLIMB", 0xFF05080E, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 3, marqueeY + 45, "RACING", 0xFF05080E, 4);

    Graphics::RasterFont::drawStringCentered(fb, cx + 1, marqueeY + 11, "HILL CLIMB", 0xFFB26A00, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx + 1, marqueeY + 43, "RACING", 0xFFE65100, 4);

    Graphics::RasterFont::drawStringCentered(fb, cx, marqueeY + 10, "HILL CLIMB", Theme::GOLD, 4);
    Graphics::RasterFont::drawStringCentered(fb, cx, marqueeY + 42, "RACING", 0xFFFF9100, 4);

    // Top highlight specular sheen
    Graphics::RasterFont::drawStringCentered(fb, cx - 1, marqueeY + 9, "HILL CLIMB", 0xFFFFE082, 4);

    // Dynamic diagonal specular gleam across title marquee
    float gleamFrac = std::fmod(m_animTime * 0.70f, 3.2f);
    if (gleamFrac < 1.0f) {
        int gleamX = marqueeX + static_cast<int>(gleamFrac * (marqueeW + 60)) - 30;
        for (int gy = marqueeY + 6; gy < marqueeY + marqueeH - 6; ++gy) {
            int gx = gleamX - (gy - marqueeY) / 2;
            if (gx >= marqueeX + 6 && gx < marqueeX + marqueeW - 6) {
                fb.blendPixelFast(gx, gy, 0x44FFFFFF);
                fb.blendPixelFast(gx + 1, gy, 0x77FFFFFF);
                fb.blendPixelFast(gx + 2, gy, 0x44FFFFFF);
            }
        }
    }

    // Arcade Subtitle Badge
    int subW = 260;
    int subH = 18;
    int subX = cx - subW / 2;
    int subY = marqueeY + 72;
    UIComponents::drawArcadePanel(fb, subX, subY, subW, subH, 0xEE0B1422, Theme::CYAN_UPGRADE, 0, 0, false, true);
    Graphics::RasterFont::drawStringCentered(fb, cx, subY + 3, "* RETRO ARCADE MOTORSPORT *", Theme::CYAN_UPGRADE, 1);

    // 8. Footer Controls Hint Bar with 3D Keycaps
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

    // ESC: PAUSE / QUIT
    int rightKx = midKx + 220;
    UIComponents::drawKeycap(fb, rightKx, ky, "ESC");
    Graphics::RasterFont::drawString(fb, rightKx + 36, ky + 4, "QUIT / BACK", Theme::TEXT_WHITE, 1);

    // ENTER: SELECT
    int enterKx = rightKx + 150;
    UIComponents::drawKeycap(fb, enterKx, ky, "ENTER");
    Graphics::RasterFont::drawString(fb, enterKx + 46, ky + 4, "START RACE", Theme::TEXT_WHITE, 1);
}

} // namespace UI
