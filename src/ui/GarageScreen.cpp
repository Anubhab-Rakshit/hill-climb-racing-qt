#include "GarageScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <algorithm>

namespace UI {

GarageScreen::GarageScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_carBounceY(0.0f)
    , m_carBounceVel(0.0f)
    , m_btnBack(20, 16, 110, 36, "< MENU", 0xFF37474F)
    , m_btnDrive(790, 16, 150, 38, "DRIVE >", Theme::GREEN_GAS)
    , m_btnUpgradeEngine(0, 0, 190, 34, "UPGRADE", Theme::GOLD)
    , m_btnUpgradeSuspension(0, 0, 190, 34, "UPGRADE", Theme::GOLD)
    , m_btnUpgradeTires(0, 0, 190, 34, "UPGRADE", Theme::GOLD)
    , m_btnUpgrade4WD(0, 0, 190, 34, "UPGRADE", Theme::GOLD)
{
    setDimensions(m_width, m_height);
}

void GarageScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(20, 16);
    m_btnDrive.setPosition(m_width - 170, 16);

    int cardW = 210;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 320;

    m_btnUpgradeEngine.setPosition(startX + 10, cardY + 120);
    m_btnUpgradeSuspension.setPosition(startX + (cardW + cardGap) * 1 + 10, cardY + 120);
    m_btnUpgradeTires.setPosition(startX + (cardW + cardGap) * 2 + 10, cardY + 120);
    m_btnUpgrade4WD.setPosition(startX + (cardW + cardGap) * 3 + 10, cardY + 120);
}

void GarageScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
    m_btnDrive.onMouseMove(px, py);
    m_btnUpgradeEngine.onMouseMove(px, py);
    m_btnUpgradeSuspension.onMouseMove(px, py);
    m_btnUpgradeTires.onMouseMove(px, py);
    m_btnUpgrade4WD.onMouseMove(px, py);
}

bool GarageScreen::onMouseDown(int px, int py, Core::ProfileManager& profile) {
    if (m_btnBack.onMouseDown(px, py)) return true;
    if (m_btnDrive.onMouseDown(px, py)) return true;

    // Check click on vehicle preview to trigger playful suspension bounce
    int cx = m_width / 2;
    int cy = 180;
    if (px >= cx - 120 && px <= cx + 120 && py >= cy - 60 && py <= cy + 60) {
        m_carBounceVel = -150.0f;
        return true;
    }

    if (m_btnUpgradeEngine.onMouseDown(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_ENGINE)) {
            m_carBounceVel = -120.0f;
        }
        return true;
    }
    if (m_btnUpgradeSuspension.onMouseDown(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_SUSPENSION)) {
            m_carBounceVel = -160.0f;
        }
        return true;
    }
    if (m_btnUpgradeTires.onMouseDown(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_TIRES)) {
            m_carBounceVel = -100.0f;
        }
        return true;
    }
    if (m_btnUpgrade4WD.onMouseDown(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_4WD)) {
            m_carBounceVel = -140.0f;
        }
        return true;
    }

    return false;
}

void GarageScreen::onMouseUp(int px, int py) {
    m_btnBack.onMouseUp(px, py);
    m_btnDrive.onMouseUp(px, py);
    m_btnUpgradeEngine.onMouseUp(px, py);
    m_btnUpgradeSuspension.onMouseUp(px, py);
    m_btnUpgradeTires.onMouseUp(px, py);
    m_btnUpgrade4WD.onMouseUp(px, py);
}

void GarageScreen::update(float dt) {
    // Damped harmonic spring bounce for vehicle preview
    float k = 180.0f;
    float c = 12.0f;
    float force = -k * m_carBounceY - c * m_carBounceVel;
    m_carBounceVel += force * dt;
    m_carBounceY += m_carBounceVel * dt;
}

void GarageScreen::drawVehiclePreview(Graphics::Framebuffer& fb, int cx, int cy, const Core::ProfileManager& profile) {
    // Garage floor platform
    fb.fillRect(cx - 160, cy + 38, 320, 10, 0xFF1E2838);
    fb.drawRect(cx - 160, cy + 38, 320, 10, Theme::CARD_BORDER);

    // Bounce offset
    int by = static_cast<int>(m_carBounceY);
    int carY = cy + by;

    // 1. Suspension struts
    int rearWheelX = cx - 55;
    int frontWheelX = cx + 55;
    int wheelY = cy + 28;

    fb.drawLine(rearWheelX, wheelY, rearWheelX + 10, carY + 8, 0xFF90A4AE);
    fb.drawLine(frontWheelX, wheelY, frontWheelX - 10, carY + 8, 0xFF90A4AE);

    // 2. Chassis Body (Red Retro Jeep)
    int bodyX = cx - 75;
    int bodyY = carY - 20;
    fb.fillRect(bodyX, bodyY, 150, 28, 0xFFD32F2F); // Crimson red chassis
    fb.drawRect(bodyX, bodyY, 150, 28, 0xFF8E0000);
    // White racing stripe
    fb.fillRect(bodyX, bodyY + 10, 150, 6, Theme::TEXT_WHITE);

    // Windshield & Roll cage
    fb.drawLine(cx - 20, bodyY, cx - 10, bodyY - 24, 0xFF37474F);
    fb.drawLine(cx - 10, bodyY - 24, cx + 40, bodyY - 24, 0xFF37474F);
    fb.drawLine(cx + 40, bodyY - 24, cx + 45, bodyY, 0xFF37474F);

    // Driver (Head & Torso)
    fb.fillRect(cx - 5, bodyY - 14, 18, 14, 0xFF1976D2); // Blue shirt
    fb.fillCircle(cx + 4, bodyY - 22, 9, 0xFFFFCA28);     // Yellow helmet
    fb.fillRect(cx + 4, bodyY - 23, 7, 5, 0xFF212121);    // Goggles

    // 3. Wheels (Tire Upgrade reflects wheel size/tread!)
    int tireLvl = profile.getUpgradeLevel(Core::ProfileManager::UPGRADE_TIRES);
    int wheelRadius = 18 + std::min(4, tireLvl / 4);

    auto drawTire = [&](int wx, int wy) {
        fb.fillCircle(wx, wy, wheelRadius, 0xFF212121);
        fb.drawCircle(wx, wy, wheelRadius, 0xFF101010);
        // Hubcap
        fb.fillCircle(wx, wy, wheelRadius / 2, 0xFFB0BEC5);
        fb.drawCircle(wx, wy, wheelRadius / 2, 0xFF455A64);
        fb.fillCircle(wx, wy, 2, Theme::GOLD);
    };

    drawTire(rearWheelX, wheelY);
    drawTire(frontWheelX, wheelY);

    // Vehicle Spec Readout Banner below car
    std::string powerStr = "TORQUE: " + std::to_string(static_cast<int>(profile.getEngineTorqueMultiplier() * 100)) + "%  |  GRIP: " +
                           std::to_string(static_cast<int>(profile.getTireGripMultiplier() * 100)) + "%  |  AWD SPLIT: " +
                           std::to_string(static_cast<int>(profile.get4wdTorqueSplit() * 100 * 2)) + "%";
    Graphics::RasterFont::drawStringCentered(fb, cx, cy + 62, powerStr, Theme::CYAN_UPGRADE, 1);
}

void GarageScreen::render(Graphics::Framebuffer& fb, Core::ProfileManager& profile) {
    // 1. Dark Gradient Background
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF0E1724, 0xFF172336);

    // 2. Header
    m_btnBack.render(fb, 2);
    m_btnDrive.render(fb, 2);

    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, 22, "GARAGE & TUNING SHOP", Theme::GOLD, 3);

    // Coin Balance
    int coinBoxW = 140;
    int coinBoxH = 34;
    int coinBoxX = m_width / 2 + 190;
    int coinBoxY = 18;
    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE111926);
    fb.drawRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, Theme::CARD_BORDER);
    fb.fillCircle(coinBoxX + 16, coinBoxY + 17, 8, Theme::GOLD);
    Graphics::RasterFont::drawStringCentered(fb, coinBoxX + 16, coinBoxY + 14, "$", 0xFF5D4037, 1);
    char coinBuf[32];
    std::snprintf(coinBuf, sizeof(coinBuf), "%06d", profile.coins());
    Graphics::RasterFont::drawString(fb, coinBoxX + 34, coinBoxY + 9, coinBuf, Theme::GOLD, 2);

    // 3. Vehicle Interactive Preview
    drawVehiclePreview(fb, m_width / 2, 175, profile);

    // 4. Upgrade Cards
    int cardW = 210;
    int cardH = 168;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 320;

    struct CardData {
        Core::ProfileManager::UpgradeType type;
        const char* name;
        const char* desc;
        RetroButton* btn;
    };

    CardData cards[4] = {
        {Core::ProfileManager::UPGRADE_ENGINE, "ENGINE", "+Peak Hill Climb Torque", &m_btnUpgradeEngine},
        {Core::ProfileManager::UPGRADE_SUSPENSION, "SUSPENSION", "Absorbs Rough Landings", &m_btnUpgradeSuspension},
        {Core::ProfileManager::UPGRADE_TIRES, "TIRES", "Maximum Slope Traction", &m_btnUpgradeTires},
        {Core::ProfileManager::UPGRADE_4WD, "4WD SYSTEM", "Power To Front Axle", &m_btnUpgrade4WD}
    };

    for (int i = 0; i < 4; ++i) {
        int x = startX + i * (cardW + cardGap);
        int y = cardY;

        // Card container
        fb.fillRect(x, y, cardW, cardH, Theme::CARD_BG);
        fb.drawRect(x, y, cardW, cardH, Theme::CARD_BORDER);
        fb.fillRect(x, y, cardW, 4, Theme::CYAN_UPGRADE);

        // Title
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 14, cards[i].name, Theme::TEXT_WHITE, 2);

        // Level readout
        int lvl = profile.getUpgradeLevel(cards[i].type);
        std::string lvlStr = "LEVEL " + std::to_string(lvl) + " / 20";
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 36, lvlStr, Theme::GOLD, 1);

        // Segmented Progress Pips Bar (20 segments)
        int pipStartX = x + 15;
        int pipY = y + 54;
        int pipW = 7;
        int pipH = 12;
        int pipGap = 2;

        for (int p = 0; p < 20; ++p) {
            int px = pipStartX + p * (pipW + pipGap);
            if (p < lvl) {
                fb.fillRect(px, pipY, pipW, pipH, Theme::GREEN_GAS);
            } else {
                fb.fillRect(px, pipY, pipW, pipH, 0xFF1C2738);
            }
        }

        // Description
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 78, cards[i].desc, Theme::TEXT_MUTED, 1);

        // Upgrade Button
        cards[i].btn->setPosition(x + 15, y + 104);
        cards[i].btn->setSize(cardW - 30, 44);

        if (lvl >= 20) {
            cards[i].btn->setText("MAXED OUT");
            cards[i].btn->setBaseColor(0xFF37474F);
        } else {
            int cost = profile.getUpgradeCost(cards[i].type);
            cards[i].btn->setText("BUY $" + std::to_string(cost));
            bool canAfford = profile.canAffordUpgrade(cards[i].type);
            cards[i].btn->setBaseColor(canAfford ? Theme::GOLD : 0xFF455A64);
        }

        cards[i].btn->render(fb, 2);
    }
}

} // namespace UI
