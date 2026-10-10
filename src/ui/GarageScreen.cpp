#include "GarageScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "Sprite.h"
#include "VehicleSprites.h"
#include <algorithm>
#include <cmath>

namespace UI {

struct CardMeta {
    Core::ProfileManager::UpgradeType type;
    const char* name;
    const char* subtitle;
    const char* desc;
};

static const CardMeta s_cards[Core::ProfileManager::UPGRADE_COUNT] = {
    {Core::ProfileManager::UPGRADE_ENGINE, "ENGINE", "HP & TORQUE", "+Peak Climbing Torque"},
    {Core::ProfileManager::UPGRADE_SUSPENSION, "SUSPENSION", "SHOCK STABILITY", "Absorbs Bumps & Hard Jumps"},
    {Core::ProfileManager::UPGRADE_TIRES, "TIRES", "PACEJKA GRIP", "Maximum Steep Hill Traction"},
    {Core::ProfileManager::UPGRADE_4WD, "4WD SYSTEM", "TORQUE SPLIT", "Transfers Power To Front Wheels"},
    {Core::ProfileManager::UPGRADE_BRAKES, "BRAKES", "STOPPING POWER", "Downhill Anti-Roll & Stoppie Lock"},
    {Core::ProfileManager::UPGRADE_TRANSMISSION, "GEARBOX", "TOP SPEED GEARING", "Higher Max Speed & High-RPM Pull"},
    {Core::ProfileManager::UPGRADE_CHASSIS, "CHASSIS", "LIGHTWEIGHT ALLOY", "-25% Weight & Crash Resilience"},
    {Core::ProfileManager::UPGRADE_DOWNFORCE, "AERODYNAMICS", "AIRFOIL WING", "High-Speed Grip & Stunt Flips"}
};

GarageScreen::GarageScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_carBounceY(0.0f)
    , m_carBounceVel(0.0f)
    , m_flashCardIdx(-1)
    , m_flashTimer(0.0f)
    , m_viewedVehicle(Physics::VehicleType::OFFROADER)
    , m_activeTab(0)
    , m_btnBack(20, 14, 110, 36, "< MENU", 0xFF37474F)
    , m_btnDrive(780, 14, 160, 36, "DRIVE >", Theme::GREEN_GAS)
    , m_btnPrevVeh(0, 0, 36, 32, "<", 0xFF2A394A)
    , m_btnNextVeh(0, 0, 36, 32, ">", 0xFF2A394A)
    , m_btnUnlockVeh(0, 0, 136, 32, "UNLOCK", Theme::GOLD)
    , m_btnPrevDriver(0, 0, 30, 26, "<", 0xFF2A394A)
    , m_btnNextDriver(0, 0, 30, 26, ">", 0xFF2A394A)
    , m_btnTabPowertrain(0, 0, 220, 30, "1. POWERTRAIN & TRACTION", Theme::GOLD)
    , m_btnTabChassis(0, 0, 220, 30, "2. HANDLING & CHASSIS", 0xFF263238)
{
    m_btnBack.setKeyHint("ESC");
    m_btnDrive.setKeyHint("ENTER");
    for (int i = 0; i < Core::ProfileManager::UPGRADE_COUNT; ++i) {
        m_btnUpgrades[i] = RetroButton(0, 0, 186, 38, "UPGRADE", Theme::GOLD);
    }
    setDimensions(m_width, m_height);
}

void GarageScreen::setButtonSound(std::function<void()> cb) {
    m_btnBack.setOnSound(cb);
    m_btnDrive.setOnSound(cb);
    m_btnPrevVeh.setOnSound(cb);
    m_btnNextVeh.setOnSound(cb);
    m_btnUnlockVeh.setOnSound(cb);
    m_btnPrevDriver.setOnSound(cb);
    m_btnNextDriver.setOnSound(cb);
    m_btnTabPowertrain.setOnSound(cb);
    m_btnTabChassis.setOnSound(cb);
    for (int i = 0; i < Core::ProfileManager::UPGRADE_COUNT; ++i) {
        m_btnUpgrades[i].setOnSound(cb);
    }
}

void GarageScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(20, 14);
    m_btnDrive.setPosition(m_width - 170, 14);

    int cx = m_width / 2;

    // Vehicle Carousel
    m_btnPrevVeh.setPosition(cx - 240, 52);
    m_btnPrevVeh.setSize(36, 32);
    m_btnNextVeh.setPosition(cx + 80, 52);
    m_btnNextVeh.setSize(36, 32);
    m_btnUnlockVeh.setPosition(cx + 124, 52);
    m_btnUnlockVeh.setSize(136, 32);

    // Driver Carousel
    m_btnPrevDriver.setPosition(cx - 160, 88);
    m_btnPrevDriver.setSize(30, 26);
    m_btnNextDriver.setPosition(cx + 130, 88);
    m_btnNextDriver.setSize(30, 26);

    // Category Tabs
    m_btnTabPowertrain.setPosition(cx - 225, 292);
    m_btnTabPowertrain.setSize(220, 30);
    m_btnTabChassis.setPosition(cx + 5, 292);
    m_btnTabChassis.setSize(220, 30);

    // 8 Upgrade Buttons Layout
    int cardW = 210;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 328;
    int btnY = cardY + 128;
    int btnW = cardW - 24;
    int btnH = 38;

    for (int i = 0; i < 4; ++i) {
        int x = startX + i * (cardW + cardGap) + 12;
        // Tab 0: 0..3
        m_btnUpgrades[i].setPosition(x, btnY);
        m_btnUpgrades[i].setSize(btnW, btnH);
        // Tab 1: 4..7
        m_btnUpgrades[i + 4].setPosition(x, btnY);
        m_btnUpgrades[i + 4].setSize(btnW, btnH);
    }
}

void GarageScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
    m_btnDrive.onMouseMove(px, py);
    m_btnPrevVeh.onMouseMove(px, py);
    m_btnNextVeh.onMouseMove(px, py);
    m_btnUnlockVeh.onMouseMove(px, py);
    m_btnPrevDriver.onMouseMove(px, py);
    m_btnNextDriver.onMouseMove(px, py);
    m_btnTabPowertrain.onMouseMove(px, py);
    m_btnTabChassis.onMouseMove(px, py);

    int startIdx = (m_activeTab == 0) ? 0 : 4;
    for (int i = startIdx; i < startIdx + 4; ++i) {
        m_btnUpgrades[i].onMouseMove(px, py);
    }
}

bool GarageScreen::onMouseDown(int px, int py, Core::ProfileManager& profile) {
    if (m_btnBack.onMouseDown(px, py)) return true;
    if (m_btnDrive.onMouseDown(px, py)) return true;

    // Check click on vehicle preview to trigger suspension bounce
    int cx = m_width / 2;
    int cy = 190;
    if (px >= cx - 140 && px <= cx + 140 && py >= cy - 60 && py <= cy + 60) {
        m_carBounceVel = -180.0f;
        return true;
    }

    if (m_btnPrevVeh.onMouseDown(px, py)) return true;
    if (m_btnNextVeh.onMouseDown(px, py)) return true;
    if (!profile.isVehicleUnlocked(m_viewedVehicle) && m_btnUnlockVeh.onMouseDown(px, py)) return true;

    if (m_btnPrevDriver.onMouseDown(px, py)) return true;
    if (m_btnNextDriver.onMouseDown(px, py)) return true;

    if (m_btnTabPowertrain.onMouseDown(px, py)) return true;
    if (m_btnTabChassis.onMouseDown(px, py)) return true;

    int startIdx = (m_activeTab == 0) ? 0 : 4;
    for (int i = startIdx; i < startIdx + 4; ++i) {
        if (m_btnUpgrades[i].onMouseDown(px, py)) return true;
    }

    return false;
}

void GarageScreen::onMouseUp(int px, int py, Core::ProfileManager& profile) {
    m_btnBack.onMouseUp(px, py);
    m_btnDrive.onMouseUp(px, py);

    // Vehicle Carousel
    if (m_btnPrevVeh.isPressed() && m_btnPrevVeh.contains(px, py)) {
        int cur = static_cast<int>(m_viewedVehicle);
        int total = static_cast<int>(Physics::VehicleType::COUNT);
        m_viewedVehicle = static_cast<Physics::VehicleType>((cur + total - 1) % total);
        if (profile.isVehicleUnlocked(m_viewedVehicle)) {
            profile.setSelectedVehicle(m_viewedVehicle);
        }
        m_carBounceVel = -130.0f;
    }
    m_btnPrevVeh.onMouseUp(px, py);

    if (m_btnNextVeh.isPressed() && m_btnNextVeh.contains(px, py)) {
        int cur = static_cast<int>(m_viewedVehicle);
        int total = static_cast<int>(Physics::VehicleType::COUNT);
        m_viewedVehicle = static_cast<Physics::VehicleType>((cur + 1) % total);
        if (profile.isVehicleUnlocked(m_viewedVehicle)) {
            profile.setSelectedVehicle(m_viewedVehicle);
        }
        m_carBounceVel = -130.0f;
    }
    m_btnNextVeh.onMouseUp(px, py);

    // Unlock Vehicle Button
    if (!profile.isVehicleUnlocked(m_viewedVehicle) && m_btnUnlockVeh.isPressed() && m_btnUnlockVeh.contains(px, py)) {
        if (profile.unlockVehicleWithCoins(m_viewedVehicle)) {
            m_carBounceVel = -220.0f;
            m_flashCardIdx = 99;
            m_flashTimer = 0.5f;
        }
    }
    m_btnUnlockVeh.onMouseUp(px, py);

    // Driver Carousel
    if (m_btnPrevDriver.isPressed() && m_btnPrevDriver.contains(px, py)) {
        int cur = static_cast<int>(profile.selectedDriver());
        int total = static_cast<int>(Physics::DriverType::COUNT);
        profile.setSelectedDriver(static_cast<Physics::DriverType>((cur + total - 1) % total));
        m_carBounceVel = -70.0f;
    }
    m_btnPrevDriver.onMouseUp(px, py);

    if (m_btnNextDriver.isPressed() && m_btnNextDriver.contains(px, py)) {
        int cur = static_cast<int>(profile.selectedDriver());
        int total = static_cast<int>(Physics::DriverType::COUNT);
        profile.setSelectedDriver(static_cast<Physics::DriverType>((cur + 1) % total));
        m_carBounceVel = -70.0f;
    }
    m_btnNextDriver.onMouseUp(px, py);

    // Category Tabs
    if (m_btnTabPowertrain.isPressed() && m_btnTabPowertrain.contains(px, py)) {
        m_activeTab = 0;
    }
    m_btnTabPowertrain.onMouseUp(px, py);

    if (m_btnTabChassis.isPressed() && m_btnTabChassis.contains(px, py)) {
        m_activeTab = 1;
    }
    m_btnTabChassis.onMouseUp(px, py);

    // Upgrade Purchases
    int startIdx = (m_activeTab == 0) ? 0 : 4;
    for (int i = startIdx; i < startIdx + 4; ++i) {
        if (m_btnUpgrades[i].isPressed() && m_btnUpgrades[i].contains(px, py)) {
            if (profile.isVehicleUnlocked(m_viewedVehicle)) {
                if (profile.purchaseUpgrade(m_viewedVehicle, s_cards[i].type)) {
                    m_carBounceVel = -150.0f;
                    m_flashCardIdx = i;
                    m_flashTimer = 0.4f;
                }
            }
        }
        m_btnUpgrades[i].onMouseUp(px, py);
    }
}

void GarageScreen::update(float dt) {
    m_animTime += dt;
    if (m_flashTimer > 0.0f) {
        m_flashTimer -= dt;
    }

    // Damped harmonic spring bounce for vehicle preview
    float k = 160.0f;
    float c = 11.0f;
    float force = -k * m_carBounceY - c * m_carBounceVel;
    m_carBounceVel += force * dt;
    m_carBounceY += m_carBounceVel * dt;
}

void GarageScreen::drawVehiclePreview(Graphics::Framebuffer& fb, int cx, int cy, const Core::ProfileManager& profile) {
    // 1. Hydraulic Workshop Lift Platform
    int liftW = 280;
    int liftX = cx - liftW / 2;
    int liftY = cy + 38;

    // Hydraulic Ram Cylinders
    fb.fillRect(cx - 72, liftY + 8, 16, 20, 0xFF78909C);
    fb.fillRect(cx - 70, liftY + 8, 12, 20, 0xFFCFD8DC);
    fb.fillRect(cx + 56, liftY + 8, 16, 20, 0xFF78909C);
    fb.fillRect(cx + 58, liftY + 8, 12, 20, 0xFFCFD8DC);

    // Lift Base Platform
    fb.fillRect(liftX, liftY, liftW, 8, 0xFF263238);
    fb.fillRect(liftX, liftY, liftW, 2, 0xFF90A4AE); // Top runner plate

    // Warning Hazard Chevron Stripes
    for (int hx = liftX + 4; hx < liftX + liftW - 4; hx += 16) {
        fb.fillRect(hx, liftY + 2, 8, 4, Theme::GOLD);
    }

    // Concrete Floor Divider Line
    fb.fillRect(0, liftY + 26, m_width, 2, 0xFF142030);

    // 2. Render Authentic Pixel Vehicle Sprite & Selected Driver
    Graphics::SpriteRenderer::renderGarageVehicle(fb, cx, cy, m_carBounceY, profile, m_viewedVehicle, profile.selectedDriver());

    const auto& cfg = Physics::VehicleRegistry::getConfig(m_viewedVehicle);

    // 3. Vehicle Tagline
    Graphics::RasterFont::drawStringCentered(fb, cx, cy + 54, cfg.tagline, Theme::GOLD, 1);

    // 4. Modular Telemetry Spec Chips (5 distinct cards)
    int torquePct = static_cast<int>(profile.getEngineTorqueMultiplier(m_viewedVehicle) * 100);
    int gripPct = static_cast<int>(profile.getTireGripMultiplier(m_viewedVehicle) * 100);
    int topSpeed = static_cast<int>(cfg.baseTopSpeedKmh * profile.getTransmissionMultiplier(m_viewedVehicle));
    int curMass = static_cast<int>(cfg.mass * profile.getChassisMassMultiplier(m_viewedVehicle));
    int awdPct = static_cast<int>((cfg.base4wdSplit + profile.get4wdTorqueSplit(m_viewedVehicle)) * 200);
    awdPct = std::min(100, awdPct);

    struct SpecChip {
        const char* name;
        std::string val;
        int pct;
        uint32_t col;
    };

    SpecChip chips[5] = {
        {"TORQUE", std::to_string(torquePct) + "%", std::clamp(torquePct / 2, 0, 100), Theme::GOLD},
        {"GRIP", std::to_string(gripPct) + "%", std::clamp(gripPct / 2, 0, 100), Theme::CYAN_UPGRADE},
        {"SPEED", std::to_string(topSpeed) + "KM/H", std::clamp(topSpeed / 2, 0, 100), 0xFFFF7043},
        {"WEIGHT", std::to_string(curMass) + "KG", std::clamp(curMass / 15, 0, 100), 0xFF81C784},
        {"4WD", std::to_string(awdPct) + "%", awdPct, 0xFFBA68C8}
    };

    int chipW = 120;
    int chipH = 22;
    int chipGap = 8;
    int totalChipsW = 5 * chipW + 4 * chipGap;
    int startChipX = cx - totalChipsW / 2;
    int chipY = cy + 66;

    for (int c = 0; c < 5; ++c) {
        int x = startChipX + c * (chipW + chipGap);
        UIComponents::drawArcadePanel(fb, x, chipY, chipW, chipH, 0xEE0E1724, 0xFF2A3A4E, 0, 0, false, false);

        Graphics::RasterFont::drawString(fb, x + 5, chipY + 3, chips[c].name, Theme::TEXT_MUTED, 1);
        int vW = Graphics::RasterFont::getTextWidth(chips[c].val, 1);
        Graphics::RasterFont::drawString(fb, x + chipW - vW - 5, chipY + 3, chips[c].val, chips[c].col, 1);

        // Mini level bar
        int barW = chipW - 10;
        int barFill = (chips[c].pct * barW) / 100;
        fb.fillRect(x + 5, chipY + 14, barW, 4, 0xFF192534);
        if (barFill > 0) {
            fb.fillRect(x + 5, chipY + 14, barFill, 4, chips[c].col);
            fb.fillRect(x + 5, chipY + 14, barFill, 1, 0x66FFFFFF);
        }
    }
}

void GarageScreen::render(Graphics::Framebuffer& fb, Core::ProfileManager& profile) {
    // 1. Dark Gradient Background with Workshop Architectural Lines
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF080E18, 0xFF121B2A);

    // Subtle background grid
    for (int gy = 40; gy < m_height; gy += 40) {
        fb.drawLine(0, gy, m_width, gy, 0x122A3D54);
    }

    // 2. Header Plaque
    m_btnBack.render(fb, 2);
    m_btnDrive.render(fb, 2);

    int cx = m_width / 2;
    int headW = 320;
    int headX = cx - headW / 2;
    UIComponents::drawArcadePanel(fb, headX, 12, headW, 36, 0xEE0B121C, 0xFF37474F, Theme::GOLD, 2, true, true);
    Graphics::RasterFont::drawStringCentered(fb, cx, 22, "GARAGE & TUNING SHOP", Theme::GOLD, 2);

    // Master Currency Pill
    int coinBoxW = 155;
    int coinBoxH = 34;
    int coinBoxX = m_width - 335;
    int coinBoxY = 15;
    UIComponents::drawCoinBadge(fb, coinBoxX, coinBoxY, coinBoxW, coinBoxH, profile.coins(), m_animTime);

    // 3. Vehicle Selection Row
    m_btnPrevVeh.render(fb, 2);
    m_btnNextVeh.render(fb, 2);

    const auto& vehCfg = Physics::VehicleRegistry::getConfig(m_viewedVehicle);
    bool isUnlocked = profile.isVehicleUnlocked(m_viewedVehicle);
    bool isSelected = (profile.selectedVehicle() == m_viewedVehicle);

    // Vehicle Index Counter above banner
    std::string vIdxStr = "VEHICLE " + std::to_string(static_cast<int>(m_viewedVehicle) + 1) + " OF " +
                          std::to_string(static_cast<int>(Physics::VehicleType::COUNT));
    Graphics::RasterFont::drawStringCentered(fb, cx - 62, 41, vIdxStr, Theme::TEXT_MUTED, 1);

    // Vehicle Name Banner
    int vBannerX = cx - 200;
    int vBannerY = 52;
    int vBannerW = 276;
    int vBannerH = 32;
    UIComponents::drawArcadePanel(fb, vBannerX, vBannerY, vBannerW, vBannerH, 0xFF14202E, 0xFF2A3A52, Theme::GOLD, 2, true, true);
    Graphics::RasterFont::drawStringCentered(fb, vBannerX + vBannerW / 2, vBannerY + 9, vehCfg.name, Theme::TEXT_WHITE, 2);

    if (!isUnlocked) {
        int cost = vehCfg.unlockCost;
        m_btnUnlockVeh.setText("UNLOCK $" + UIComponents::formatNumber(cost));
        bool canAfford = (profile.coins() >= cost);
        m_btnUnlockVeh.setBaseColor(canAfford ? Theme::GOLD : 0xFF455A64);
        m_btnUnlockVeh.setEnabled(canAfford);
        m_btnUnlockVeh.render(fb, 2);
    } else {
        int badgeX = cx + 124;
        int badgeY = 52;
        int badgeW = 136;
        int badgeH = 32;
        uint32_t badgeBg = isSelected ? 0xFF143820 : 0xFF1A2634;
        uint32_t badgeBorder = isSelected ? Theme::GREEN_GAS : Theme::CARD_BORDER;
        UIComponents::drawArcadePanel(fb, badgeX, badgeY, badgeW, badgeH, badgeBg, badgeBorder, isSelected ? Theme::GREEN_GAS : 0, 2, true, true);

        // Glowing LED pip for active vehicle
        if (isSelected) {
            fb.fillCircle(badgeX + 14, badgeY + 16, 4, Theme::GREEN_GAS);
            fb.fillCircle(badgeX + 14, badgeY + 16, 2, 0xFFFFFFFF);
        }
        const char* badgeText = isSelected ? "ACTIVE VEHICLE" : "SELECT VEHICLE";
        Graphics::RasterFont::drawStringCentered(fb, badgeX + (isSelected ? 74 : 68), badgeY + 10, badgeText,
                                                 isSelected ? Theme::GREEN_GAS : Theme::TEXT_MUTED, 1);

        // If clicked on active badge when unlocked, select it
        if (!isSelected && m_viewedVehicle != profile.selectedVehicle()) {
            profile.setSelectedVehicle(m_viewedVehicle);
        }
    }

    // 4. Driver Selection Row
    m_btnPrevDriver.render(fb, 2);
    m_btnNextDriver.render(fb, 2);

    const auto& driverCfg = Physics::VehicleRegistry::getDriver(profile.selectedDriver());
    int dBoxX = cx - 126;
    int dBoxY = 88;
    int dBoxW = 252;
    int dBoxH = 26;
    UIComponents::drawArcadePanel(fb, dBoxX, dBoxY, dBoxW, dBoxH, 0xFF121B27, 0xFF2A3A52, 0, 0, true, true);

    // Mini Driver Portrait Icon
    const QImage& portrait = Graphics::VehicleSprites::getDriverPortrait(profile.selectedDriver());
    if (!portrait.isNull()) {
        for (int py = 0; py < portrait.height(); ++py) {
            const uint32_t* line = reinterpret_cast<const uint32_t*>(portrait.scanLine(py));
            for (int px = 0; px < portrait.width(); ++px) {
                uint32_t c = line[px];
                if ((c >> 24) > 0) {
                    fb.blendPixelFast(dBoxX + 6 + px, dBoxY + 3 + py, c);
                }
            }
        }
    }
    std::string driverTitle = "DRIVER: " + driverCfg.name;
    Graphics::RasterFont::drawString(fb, dBoxX + 32, dBoxY + 7, driverTitle, driverCfg.themeColor, 1);

    // 5. Vehicle Interactive Preview on Hydraulic Lift
    drawVehiclePreview(fb, cx, 190, profile);

    // 6. Category Tabs
    m_btnTabPowertrain.setBaseColor((m_activeTab == 0) ? Theme::GOLD : 0xFF213142);
    m_btnTabPowertrain.setTextColor((m_activeTab == 0) ? 0xFF0B1422 : Theme::TEXT_MUTED);
    m_btnTabPowertrain.render(fb, 1);

    m_btnTabChassis.setBaseColor((m_activeTab == 1) ? Theme::GOLD : 0xFF213142);
    m_btnTabChassis.setTextColor((m_activeTab == 1) ? 0xFF0B1422 : Theme::TEXT_MUTED);
    m_btnTabChassis.render(fb, 1);

    // 7. 4 Upgrade Cards for Active Tab
    int cardW = 210;
    int cardH = 176;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 328;

    int startIdx = (m_activeTab == 0) ? 0 : 4;
    for (int i = 0; i < 4; ++i) {
        int cardIdx = startIdx + i;
        const auto& meta = s_cards[cardIdx];
        int x = startX + i * (cardW + cardGap);
        int y = cardY;

        bool isFlashing = (m_flashTimer > 0.0f && m_flashCardIdx == cardIdx);
        uint32_t borderCol = isFlashing ? 0xFFFFFFFF : Theme::CARD_BORDER;

        // Card Frame
        UIComponents::drawArcadePanel(fb, x, y, cardW, cardH, Theme::CARD_BG, borderCol, Theme::CYAN_UPGRADE, 3, true, true);

        // Custom 18x18 Pixel Art Icon
        UIComponents::drawUpgradeIcon(fb, x + 10, y + 10, meta.type);

        // Title & Subtitle
        Graphics::RasterFont::drawString(fb, x + 34, y + 11, meta.name, Theme::TEXT_WHITE, 2);
        Graphics::RasterFont::drawString(fb, x + 34, y + 27, meta.subtitle, Theme::CYAN_UPGRADE, 1);

        // Level Readout
        int lvl = profile.getUpgradeLevel(m_viewedVehicle, meta.type);
        std::string lvlStr = "LEVEL " + std::to_string(lvl) + " / " + std::to_string(Core::ProfileManager::MAX_UPGRADE_LEVEL);
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 49, lvlStr, Theme::GOLD, 1);

        // 15 Illuminated LED Pips Meter
        UIComponents::drawSegmentedBar(fb, x + 12, y + 66, cardW - 24, 13, lvl,
                                      Core::ProfileManager::MAX_UPGRADE_LEVEL,
                                      Theme::GREEN_GAS, 0xFF192534, 0x88FFFFFF);

        // Description
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 96, meta.desc, Theme::TEXT_MUTED, 1);

        // Action Button State
        auto& btn = m_btnUpgrades[cardIdx];
        if (!isUnlocked) {
            btn.setText("VEHICLE LOCKED");
            btn.setBaseColor(0xFF37474F);
            btn.setEnabled(false);
        } else if (lvl >= Core::ProfileManager::MAX_UPGRADE_LEVEL) {
            btn.setText("MAX LEVEL");
            btn.setBaseColor(0xFF37474F);
            btn.setEnabled(false);
        } else {
            int cost = profile.getUpgradeCost(m_viewedVehicle, meta.type);
            btn.setText("BUY $" + UIComponents::formatNumber(cost));
            bool canAfford = profile.canAffordUpgrade(m_viewedVehicle, meta.type);
            btn.setBaseColor(canAfford ? Theme::GOLD : 0xFF455A64);
            btn.setEnabled(canAfford);
        }

        btn.render(fb, 2);
    }
}

} // namespace UI
