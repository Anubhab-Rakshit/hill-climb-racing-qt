#include "StageSelectScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <string>

namespace UI {

StageSelectScreen::StageSelectScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_btnBack(20, 16, 110, 36, "< MENU", 0xFF37474F)
    , m_btnSelectCountryside(0, 0, 180, 42, "PLAY", Theme::GREEN_GAS)
    , m_btnSelectDesert(0, 0, 180, 42, "PLAY", Theme::GREEN_GAS)
    , m_btnSelectMoon(0, 0, 180, 42, "PLAY", Theme::GREEN_GAS)
    , m_btnSelectMountain(0, 0, 180, 42, "PLAY", Theme::GREEN_GAS)
{
    setDimensions(m_width, m_height);

    m_btnSelectCountryside.setOnClick([this]() { if (m_onSelectStage) m_onSelectStage("countryside"); });
    m_btnSelectDesert.setOnClick([this]() { if (m_onSelectStage) m_onSelectStage("desert"); });
    m_btnSelectMoon.setOnClick([this]() { if (m_onSelectStage) m_onSelectStage("moon"); });
    m_btnSelectMountain.setOnClick([this]() { if (m_onSelectStage) m_onSelectStage("mountain"); });
}

void StageSelectScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(20, 16);

    int cardW = 210;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 90;

    m_btnSelectCountryside.setPosition(startX + 14, cardY + 312);
    m_btnSelectCountryside.setSize(cardW - 28, 42);

    m_btnSelectDesert.setPosition(startX + (cardW + cardGap) * 1 + 14, cardY + 312);
    m_btnSelectDesert.setSize(cardW - 28, 42);

    m_btnSelectMoon.setPosition(startX + (cardW + cardGap) * 2 + 14, cardY + 312);
    m_btnSelectMoon.setSize(cardW - 28, 42);

    m_btnSelectMountain.setPosition(startX + (cardW + cardGap) * 3 + 14, cardY + 312);
    m_btnSelectMountain.setSize(cardW - 28, 42);
}

void StageSelectScreen::update(float dt) {
    m_animTime += dt;
}

void StageSelectScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
    m_btnSelectCountryside.onMouseMove(px, py);
    m_btnSelectDesert.onMouseMove(px, py);
    m_btnSelectMoon.onMouseMove(px, py);
    m_btnSelectMountain.onMouseMove(px, py);
}

bool StageSelectScreen::onMouseDown(int px, int py, Core::ProfileManager& profile) {
    if (m_btnBack.onMouseDown(px, py)) return true;

    // Check unlocking for Moon and Mountain
    if (!profile.isStageUnlocked("moon") && m_btnSelectMoon.contains(px, py)) {
        if (profile.unlockStageWithCoins("moon")) {
            m_btnSelectMoon.setText("PLAY");
            m_btnSelectMoon.setBaseColor(Theme::GREEN_GAS);
        }
        return true;
    }
    if (!profile.isStageUnlocked("mountain") && m_btnSelectMountain.contains(px, py)) {
        if (profile.unlockStageWithCoins("mountain")) {
            m_btnSelectMountain.setText("PLAY");
            m_btnSelectMountain.setBaseColor(Theme::GREEN_GAS);
        }
        return true;
    }

    if (m_btnSelectCountryside.onMouseDown(px, py)) return true;
    if (m_btnSelectDesert.onMouseDown(px, py)) return true;
    if (m_btnSelectMoon.onMouseDown(px, py)) return true;
    if (m_btnSelectMountain.onMouseDown(px, py)) return true;
    return false;
}

void StageSelectScreen::onMouseUp(int px, int py) {
    m_btnBack.onMouseUp(px, py);
    m_btnSelectCountryside.onMouseUp(px, py);
    m_btnSelectDesert.onMouseUp(px, py);
    m_btnSelectMoon.onMouseUp(px, py);
    m_btnSelectMountain.onMouseUp(px, py);
}

void StageSelectScreen::drawStageThumbnail(Graphics::Framebuffer& fb, int x, int y, int w, int h, int stageIdx) {
    fb.fillRect(x, y, w, h, 0xFF000000);

    for (int px = 0; px < w; ++px) {
        float t = static_cast<float>(px) / w;
        float hillY = 0.0f;
        uint32_t skyCol = 0;
        uint32_t groundCol = 0;

        if (stageIdx == 0) { // Countryside
            skyCol = 0xFF42A5F5;
            groundCol = 0xFF43A047;
            hillY = std::sin(t * 6.28f + m_animTime * 1.5f) * 14.0f + h * 0.55f;
        } else if (stageIdx == 1) { // Desert
            skyCol = 0xFFFFB74D;
            groundCol = 0xFFFFA000;
            hillY = std::sin(t * 8.0f + m_animTime * 0.8f) * 18.0f + h * 0.52f;
        } else if (stageIdx == 2) { // Moon
            skyCol = 0xFF070B14;
            groundCol = 0xFF90A4AE;
            hillY = std::sin(t * 4.0f) * 12.0f + h * 0.60f;
        } else { // Mountain
            skyCol = 0xFF263238;
            groundCol = 0xFF546E7A;
            hillY = std::sin(t * 12.0f) * 22.0f + h * 0.50f;
        }

        int hy = static_cast<int>(hillY);
        for (int py = 0; py < h; ++py) {
            fb.setPixelFast(x + px, y + py, (py < hy) ? skyCol : groundCol);
        }
    }

    // Special thumbnail decorative elements
    if (stageIdx == 2) {
        // Distant Earth in moon sky
        fb.fillCircle(x + w - 24, y + 22, 10, 0xFF1E88E5);
        fb.fillCircle(x + w - 22, y + 20, 4, 0xFF43A047);
        // Twinkling stars
        int starPos[5][2] = {{12, 14}, {34, 25}, {60, 10}, {85, 28}, {110, 15}};
        for (int s = 0; s < 5; ++s) {
            if (static_cast<int>(m_animTime * 4.0f + s) % 3 != 0) {
                fb.setPixelFast(x + starPos[s][0], y + starPos[s][1], 0xFFFFFFFF);
            }
        }
    } else if (stageIdx == 1) {
        // Scorching Sun in desert
        fb.fillCircle(x + 24, y + 22, 8, 0xFFFFD54F);
    }

    fb.drawRect(x, y, w, h, Theme::CARD_BORDER);
}

void StageSelectScreen::render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile) {
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF0A121E, 0xFF142032);

    m_btnBack.render(fb, 2);
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, 22, "SELECT STAGE TRACK", Theme::GOLD, 3);

    // Player Coins in top right
    int coinBoxW = 140;
    int coinBoxH = 34;
    int coinBoxX = m_width - coinBoxW - 24;
    int coinBoxY = 16;
    fb.fillRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, 0xEE0B121C);
    fb.drawRect(coinBoxX, coinBoxY, coinBoxW, coinBoxH, Theme::CARD_BORDER);
    fb.fillCircle(coinBoxX + 16, coinBoxY + 17, 8, Theme::GOLD);
    Graphics::RasterFont::drawStringCentered(fb, coinBoxX + 16, coinBoxY + 14, "$", 0xFF5D4037, 1);
    char coinBuf[32];
    std::snprintf(coinBuf, sizeof(coinBuf), "%06d", profile.coins());
    Graphics::RasterFont::drawString(fb, coinBoxX + 34, coinBoxY + 9, coinBuf, Theme::GOLD, 2);

    int cardW = 210;
    int cardH = 370;
    int cardGap = 16;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 90;

    struct StageInfo {
        std::string id;
        std::string name;
        std::string desc;
        std::string gravity;
        std::string stars;
        RetroButton* btn;
    };

    StageInfo stages[4] = {
        {"countryside", "COUNTRYSIDE", "Smooth Rolling Hills", "1.0G (Normal)", "* *", &m_btnSelectCountryside},
        {"desert", "DESERT DUNES", "Slippery Loose Sand", "1.0G (Loose)", "* * *", &m_btnSelectDesert},
        {"moon", "THE MOON", "Floaty Low-G Craters", "0.16G (Lunar)", "* * * *", &m_btnSelectMoon},
        {"mountain", "MOUNTAIN RIDGE", "Steep Rocky Crags", "1.0G (Rocky)", "* * * * *", &m_btnSelectMountain}
    };

    for (int i = 0; i < 4; ++i) {
        int x = startX + i * (cardW + cardGap);
        int y = cardY;

        fb.fillRect(x, y, cardW, cardH, Theme::CARD_BG);
        fb.drawRect(x, y, cardW, cardH, Theme::CARD_BORDER);

        uint32_t stripeCol = (i == 0) ? Theme::GREEN_GAS : (i == 1) ? Theme::GOLD : (i == 2) ? Theme::CYAN_UPGRADE : Theme::RED_BRAKE;
        fb.fillRect(x, y, cardW, 4, stripeCol);

        // Title
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 15, stages[i].name, Theme::TEXT_WHITE, 2);

        // Thumbnail
        drawStageThumbnail(fb, x + 14, y + 40, cardW - 28, 96, i);

        // Info
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 148, stages[i].desc, Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 168, "GRAVITY: " + stages[i].gravity, Theme::CYAN_UPGRADE, 1);
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 188, "DIFFICULTY: " + stages[i].stars, Theme::GOLD, 1);

        // High Score Record
        float rec = profile.getRecordDistance(stages[i].id);
        std::string recStr = "RECORD: " + std::to_string(static_cast<int>(rec)) + "m";
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 214, recStr, Theme::GOLD, 2);

        // Unlock status
        bool unlocked = profile.isStageUnlocked(stages[i].id);
        if (unlocked) {
            stages[i].btn->setText("PLAY");
            stages[i].btn->setBaseColor(Theme::GREEN_GAS);
        } else {
            int cost = profile.getStageUnlockCost(stages[i].id);
            bool canAfford = (profile.coins() >= cost);
            stages[i].btn->setText("BUY $" + std::to_string(cost));
            stages[i].btn->setBaseColor(canAfford ? Theme::GOLD : 0xFF37474F);
        }

        stages[i].btn->setPosition(x + 14, cardY + 312);
        stages[i].btn->setSize(cardW - 28, 42);
        stages[i].btn->render(fb, 2);
    }
}

} // namespace UI
