#include "StageSelectScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>

namespace UI {

StageSelectScreen::StageSelectScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_btnBack(20, 16, 110, 36, "< MENU", 0xFF37474F)
    , m_btnSelectCountryside(0, 0, 180, 40, "SELECT", Theme::GREEN_GAS)
    , m_btnSelectDesert(0, 0, 180, 40, "SELECT", Theme::GREEN_GAS)
    , m_btnSelectMoon(0, 0, 180, 40, "SELECT", Theme::GREEN_GAS)
    , m_btnSelectMountain(0, 0, 180, 40, "SELECT", Theme::GREEN_GAS)
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

    int cardW = 205;
    int cardGap = 18;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 100;

    m_btnSelectCountryside.setPosition(startX + 12, cardY + 310);
    m_btnSelectCountryside.setSize(cardW - 24, 42);

    m_btnSelectDesert.setPosition(startX + (cardW + cardGap) * 1 + 12, cardY + 310);
    m_btnSelectDesert.setSize(cardW - 24, 42);

    m_btnSelectMoon.setPosition(startX + (cardW + cardGap) * 2 + 12, cardY + 310);
    m_btnSelectMoon.setSize(cardW - 24, 42);

    m_btnSelectMountain.setPosition(startX + (cardW + cardGap) * 3 + 12, cardY + 310);
    m_btnSelectMountain.setSize(cardW - 24, 42);
}

void StageSelectScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
    m_btnSelectCountryside.onMouseMove(px, py);
    m_btnSelectDesert.onMouseMove(px, py);
    m_btnSelectMoon.onMouseMove(px, py);
    m_btnSelectMountain.onMouseMove(px, py);
}

bool StageSelectScreen::onMouseDown(int px, int py) {
    if (m_btnBack.onMouseDown(px, py)) return true;
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
    // Inner box
    fb.fillRect(x, y, w, h, 0xFF000000);

    for (int px = 0; px < w; ++px) {
        float t = static_cast<float>(px) / w;
        float hillY = 0.0f;
        uint32_t skyCol = 0;
        uint32_t groundCol = 0;

        if (stageIdx == 0) { // Countryside
            skyCol = 0xFF42A5F5;
            groundCol = 0xFF43A047;
            hillY = std::sin(t * 6.28f) * 18.0f + h * 0.55f;
        } else if (stageIdx == 1) { // Desert
            skyCol = 0xFFFFB74D;
            groundCol = 0xFFFFA000;
            hillY = std::sin(t * 8.0f) * 22.0f + h * 0.52f;
        } else if (stageIdx == 2) { // Moon
            skyCol = 0xFF0B0F19;
            groundCol = 0xFF90A4AE;
            hillY = std::sin(t * 4.0f) * 14.0f + h * 0.60f;
        } else { // Mountain
            skyCol = 0xFF263238;
            groundCol = 0xFF546E7A;
            hillY = std::sin(t * 12.0f) * 26.0f + h * 0.50f;
        }

        int hy = static_cast<int>(hillY);
        for (int py = 0; py < h; ++py) {
            fb.setPixelFast(x + px, y + py, (py < hy) ? skyCol : groundCol);
        }
    }

    fb.drawRect(x, y, w, h, Theme::CARD_BORDER);
}

void StageSelectScreen::render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile) {
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF0B121C, 0xFF142030);

    m_btnBack.render(fb, 2);
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, 22, "SELECT STAGE BIOME", Theme::GOLD, 3);

    int cardW = 205;
    int cardH = 370;
    int cardGap = 18;
    int totalCardsW = 4 * cardW + 3 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 90;

    struct StageInfo {
        std::string id;
        std::string name;
        std::string desc;
        std::string gravity;
        RetroButton* btn;
    };

    StageInfo stages[4] = {
        {"countryside", "COUNTRYSIDE", "Rolling grassy hills", "1.0G (Normal)", &m_btnSelectCountryside},
        {"desert", "DESERT DUNES", "Slippery sand dunes", "1.0G (Loose)", &m_btnSelectDesert},
        {"moon", "THE MOON", "Craters & low gravity", "0.16G (Float)", &m_btnSelectMoon},
        {"mountain", "MOUNTAIN RIDGE", "Violent rock inclines", "1.0G (Rocky)", &m_btnSelectMountain}
    };

    for (int i = 0; i < 4; ++i) {
        int x = startX + i * (cardW + cardGap);
        int y = cardY;

        fb.fillRect(x, y, cardW, cardH, Theme::CARD_BG);
        fb.drawRect(x, y, cardW, cardH, Theme::CARD_BORDER);
        fb.fillRect(x, y, cardW, 4, (i == 0) ? Theme::GREEN_GAS : (i == 1) ? Theme::GOLD : (i == 2) ? Theme::CYAN_UPGRADE : Theme::RED_BRAKE);

        // Title
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 16, stages[i].name, Theme::TEXT_WHITE, 2);

        // Thumbnail
        drawStageThumbnail(fb, x + 16, y + 42, cardW - 32, 100, i);

        // Info
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 154, stages[i].desc, Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 174, "GRAVITY: " + stages[i].gravity, Theme::CYAN_UPGRADE, 1);

        // Record
        float rec = profile.getRecordDistance(stages[i].id);
        std::string recStr = "RECORD: " + std::to_string(static_cast<int>(rec)) + "m";
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 200, recStr, Theme::GOLD, 1);

        bool unlocked = profile.isStageUnlocked(stages[i].id);
        if (unlocked) {
            stages[i].btn->setText("PLAY");
            stages[i].btn->setBaseColor(Theme::GREEN_GAS);
        } else {
            stages[i].btn->setText("LOCKED");
            stages[i].btn->setBaseColor(0xFF37474F);
        }

        stages[i].btn->render(fb, 2);
    }
}

} // namespace UI
