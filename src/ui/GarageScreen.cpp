#include "GarageScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "Sprite.h"
#include <algorithm>
#include <cmath>

namespace UI {

GarageScreen::GarageScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_carBounceY(0.0f)
    , m_carBounceVel(0.0f)
    , m_btnBack(20, 16, 110, 36, "< MENU", 0xFF37474F)
    , m_btnDrive(780, 16, 160, 38, "DRIVE >", Theme::GREEN_GAS)
    , m_btnUpgradeEngine(0, 0, 186, 42, "UPGRADE", Theme::GOLD)
    , m_btnUpgradeSuspension(0, 0, 186, 42, "UPGRADE", Theme::GOLD)
    , m_btnUpgradeTires(0, 0, 186, 42, "UPGRADE", Theme::GOLD)
    , m_btnUpgrade4WD(0, 0, 186, 42, "UPGRADE", Theme::GOLD)
{
    setDimensions(m_width, m_height);
}

void GarageScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(20, 16);
    m_btnDrive.setPosition(m_width - 180, 16);

    int cardW = 210;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 320;

    int btnY = cardY + 114;
    int btnW = cardW - 24;
    int btnH = 40;

    m_btnUpgradeEngine.setPosition(startX + 12, btnY);
    m_btnUpgradeEngine.setSize(btnW, btnH);

    m_btnUpgradeSuspension.setPosition(startX + (cardW + cardGap) * 1 + 12, btnY);
    m_btnUpgradeSuspension.setSize(btnW, btnH);

    m_btnUpgradeTires.setPosition(startX + (cardW + cardGap) * 2 + 12, btnY);
    m_btnUpgradeTires.setSize(btnW, btnH);

    m_btnUpgrade4WD.setPosition(startX + (cardW + cardGap) * 3 + 12, btnY);
    m_btnUpgrade4WD.setSize(btnW, btnH);
}

void GarageScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
    m_btnDrive.onMouseMove(px, py);
    m_btnUpgradeEngine.onMouseMove(px, py);
    m_btnUpgradeSuspension.onMouseMove(px, py);
    m_btnUpgradeTires.onMouseMove(px, py);
    m_btnUpgrade4WD.onMouseMove(px, py);
}

bool GarageScreen::onMouseDown(int px, int py, Core::ProfileManager& /*profile*/) {
    if (m_btnBack.onMouseDown(px, py)) return true;
    if (m_btnDrive.onMouseDown(px, py)) return true;

    // Check click on vehicle preview to trigger playful suspension bounce
    int cx = m_width / 2;
    int cy = 175;
    if (px >= cx - 130 && px <= cx + 130 && py >= cy - 60 && py <= cy + 60) {
        m_carBounceVel = -180.0f;
        return true;
    }

    if (m_btnUpgradeEngine.onMouseDown(px, py)) return true;
    if (m_btnUpgradeSuspension.onMouseDown(px, py)) return true;
    if (m_btnUpgradeTires.onMouseDown(px, py)) return true;
    if (m_btnUpgrade4WD.onMouseDown(px, py)) return true;

    return false;
}

void GarageScreen::onMouseUp(int px, int py, Core::ProfileManager& profile) {
    m_btnBack.onMouseUp(px, py);
    m_btnDrive.onMouseUp(px, py);

    // Single purchase on mouse release prevents unintended multi-click spending
    if (m_btnUpgradeEngine.isPressed() && m_btnUpgradeEngine.contains(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_ENGINE)) {
            m_carBounceVel = -140.0f;
        }
    }
    m_btnUpgradeEngine.onMouseUp(px, py);

    if (m_btnUpgradeSuspension.isPressed() && m_btnUpgradeSuspension.contains(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_SUSPENSION)) {
            m_carBounceVel = -180.0f;
        }
    }
    m_btnUpgradeSuspension.onMouseUp(px, py);

    if (m_btnUpgradeTires.isPressed() && m_btnUpgradeTires.contains(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_TIRES)) {
            m_carBounceVel = -120.0f;
        }
    }
    m_btnUpgradeTires.onMouseUp(px, py);

    if (m_btnUpgrade4WD.isPressed() && m_btnUpgrade4WD.contains(px, py)) {
        if (profile.purchaseUpgrade(Core::ProfileManager::UPGRADE_4WD)) {
            m_carBounceVel = -150.0f;
        }
    }
    m_btnUpgrade4WD.onMouseUp(px, py);
}

void GarageScreen::update(float dt) {
    // Damped harmonic spring bounce for vehicle preview
    float k = 160.0f;
    float c = 11.0f;
    float force = -k * m_carBounceY - c * m_carBounceVel;
    m_carBounceVel += force * dt;
    m_carBounceY += m_carBounceVel * dt;
}

void GarageScreen::drawVehiclePreview(Graphics::Framebuffer& fb, int cx, int cy, const Core::ProfileManager& profile) {
    Graphics::SpriteRenderer::renderGarageVehicle(fb, cx, cy, m_carBounceY, profile);

    // Vehicle Spec Readout Banner below lift
    std::string powerStr = "TORQUE: " + std::to_string(static_cast<int>(profile.getEngineTorqueMultiplier() * 100)) + "%   |   GRIP: " +
                           std::to_string(static_cast<int>(profile.getTireGripMultiplier() * 100)) + "%   |   AWD: " +
                           std::to_string(static_cast<int>(profile.get4wdTorqueSplit() * 200)) + "%";
    Graphics::RasterFont::drawStringCentered(fb, cx, cy + 92, powerStr, Theme::CYAN_UPGRADE, 1);
}

void GarageScreen::render(Graphics::Framebuffer& fb, Core::ProfileManager& profile) {
    // 1. Dark Gradient Background
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF0B1422, 0xFF142134);

    // 2. Header
    m_btnBack.render(fb, 2);
    m_btnDrive.render(fb, 2);

    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, 22, "GARAGE & TUNING SHOP", Theme::GOLD, 3);

    // Coin Balance
    int coinBoxW = 140;
    int coinBoxH = 34;
    int coinBoxX = m_width / 2 + 180;
    int coinBoxY = 18;
    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE0B121C);
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
        {Core::ProfileManager::UPGRADE_ENGINE, "ENGINE", "+Peak Climbing Torque", &m_btnUpgradeEngine},
        {Core::ProfileManager::UPGRADE_SUSPENSION, "SUSPENSION", "Absorbs Rough Jumps", &m_btnUpgradeSuspension},
        {Core::ProfileManager::UPGRADE_TIRES, "TIRES", "Maximum Slope Grip", &m_btnUpgradeTires},
        {Core::ProfileManager::UPGRADE_4WD, "4WD SYSTEM", "Power To Front Axle", &m_btnUpgrade4WD}
    };

    for (int i = 0; i < 4; ++i) {
        int x = startX + i * (cardW + cardGap);
        int y = cardY;

        fb.fillRect(x, y, cardW, cardH, Theme::CARD_BG);
        fb.drawRect(x, y, cardW, cardH, Theme::CARD_BORDER);
        fb.fillRect(x, y, cardW, 4, Theme::CYAN_UPGRADE);

        // Title
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 14, cards[i].name, Theme::TEXT_WHITE, 2);

        // Level readout
        int lvl = profile.getUpgradeLevel(cards[i].type);
        std::string lvlStr = "LEVEL " + std::to_string(lvl) + " / 20";
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 36, lvlStr, Theme::GOLD, 1);

        // Segmented Illuminated LED Bar (20 segments)
        int pipStartX = x + 14;
        int pipY = y + 54;
        int pipW = 7;
        int pipH = 12;
        int pipGap = 2;

        for (int p = 0; p < 20; ++p) {
            int px = pipStartX + p * (pipW + pipGap);
            if (p < lvl) {
                fb.fillRect(px, pipY, pipW, pipH, Theme::GREEN_GAS);
                fb.fillRect(px, pipY, pipW, 2, 0x88FFFFFF); // Illuminated LED highlight
            } else {
                fb.fillRect(px, pipY, pipW, pipH, 0xFF192534);
            }
        }

        // Description
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 78, cards[i].desc, Theme::TEXT_MUTED, 1);

        // Button state
        int btnW = cardW - 24;
        int btnH = 40;
        int btnY = cardY + 114;
        cards[i].btn->setPosition(x + 12, btnY);
        cards[i].btn->setSize(btnW, btnH);

        if (lvl >= 20) {
            cards[i].btn->setText("MAX LEVEL");
            cards[i].btn->setBaseColor(0xFF37474F);
            cards[i].btn->setEnabled(false);
        } else {
            int cost = profile.getUpgradeCost(cards[i].type);
            cards[i].btn->setText("BUY $" + std::to_string(cost));
            bool canAfford = profile.canAffordUpgrade(cards[i].type);
            cards[i].btn->setBaseColor(canAfford ? Theme::GOLD : 0xFF455A64);
            cards[i].btn->setEnabled(canAfford);
        }

        cards[i].btn->render(fb, 2);
    }
}

} // namespace UI
