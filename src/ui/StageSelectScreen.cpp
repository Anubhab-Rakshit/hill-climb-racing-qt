#include "StageSelectScreen.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include <cmath>
#include <string>
#include <algorithm>

namespace UI {

struct StageCardMeta {
    std::string id;
    std::string name;
    std::string desc;
    std::string gravity;
    int difficultyStars;
    std::string unlockRequirement;
    uint32_t stripeColor;
};

static const StageCardMeta s_stages[6] = {
    {"countryside", "COUNTRYSIDE", "Smooth Rolling Hills", "1.0G (Normal)", 2, "Starter Stage", Theme::GREEN_GAS},
    {"desert", "DESERT DUNES", "Loose Sand & Slip-Faces", "1.0G (Loose)", 3, "Clear Countryside", Theme::GOLD},
    {"arctic", "ARCTIC TUNDRA", "Slippery Glacial Crevasses", "1.0G (Ice)", 4, "Clear Desert", 0xFF00E5FF},
    {"mountain", "MOUNTAIN RIDGE", "Craggy Rocky Escarpments", "1.0G (Rocky)", 4, "Clear Arctic", 0xFF78909C},
    {"moon", "THE MOON", "Floaty Low-G Impact Craters", "0.16G (Lunar)", 5, "Clear Mountain", 0xFFECEFF1},
    {"volcano", "INFERNO PEAKS", "Dense Gravity & Calderas", "1.17G (Heavy)", 5, "Clear The Moon", 0xFFFF3D00}
};

StageSelectScreen::StageSelectScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_page(0)
    , m_btnBack(20, 14, 110, 36, "< MENU", 0xFF37474F)
    , m_btnPrevPage(0, 0, 80, 28, "< PREV", 0xFF263238)
    , m_btnNextPage(0, 0, 80, 28, "NEXT >", 0xFF263238)
{
    m_btnBack.setKeyHint("ESC");
    for (int i = 0; i < 6; ++i) {
        m_btnStageAction[i] = RetroButton(0, 0, 240, 42, "PLAY", Theme::GREEN_GAS);
        m_btnStageAction[i].setKeyHint("ENTER");
        std::string sId = s_stages[i].id;
        m_btnStageAction[i].setOnClick([this, sId]() {
            if (m_onSelectStage) m_onSelectStage(sId);
        });
    }

    setDimensions(m_width, m_height);
}

void StageSelectScreen::setButtonSound(std::function<void()> cb) {
    m_btnBack.setOnSound(cb);
    m_btnPrevPage.setOnSound(cb);
    m_btnNextPage.setOnSound(cb);
    for (int i = 0; i < 6; ++i) {
        m_btnStageAction[i].setOnSound(cb);
    }
}

void StageSelectScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(20, 14);

    int cx = m_width / 2;
    m_btnPrevPage.setPosition(cx - 130, 54);
    m_btnPrevPage.setSize(75, 26);
    m_btnNextPage.setPosition(cx + 55, 54);
    m_btnNextPage.setSize(75, 26);

    int cardW = 270;
    int cardGap = 20;
    int totalCardsW = 3 * cardW + 2 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 88;
    int btnW = cardW - 28;
    int btnH = 40;
    int btnY = cardY + 355;

    for (int i = 0; i < 3; ++i) {
        // Page 0 buttons (0..2)
        int x = startX + i * (cardW + cardGap) + 14;
        m_btnStageAction[i].setPosition(x, btnY);
        m_btnStageAction[i].setSize(btnW, btnH);

        // Page 1 buttons (3..5)
        m_btnStageAction[i + 3].setPosition(x, btnY);
        m_btnStageAction[i + 3].setSize(btnW, btnH);
    }
}

void StageSelectScreen::update(float dt) {
    m_animTime += dt;
}

void StageSelectScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
    m_btnPrevPage.onMouseMove(px, py);
    m_btnNextPage.onMouseMove(px, py);

    int startIdx = m_page * 3;
    for (int i = startIdx; i < startIdx + 3; ++i) {
        m_btnStageAction[i].onMouseMove(px, py);
    }
}

bool StageSelectScreen::onMouseDown(int px, int py, Core::ProfileManager& profile) {
    if (m_btnBack.onMouseDown(px, py)) return true;
    if (m_btnPrevPage.onMouseDown(px, py)) return true;
    if (m_btnNextPage.onMouseDown(px, py)) return true;

    int startIdx = m_page * 3;
    for (int i = startIdx; i < startIdx + 3; ++i) {
        const std::string& sId = s_stages[i].id;
        if (!profile.isStageUnlocked(sId) && m_btnStageAction[i].contains(px, py)) {
            if (profile.unlockStageWithCoins(sId)) {
                m_btnStageAction[i].setText("PLAY");
                m_btnStageAction[i].setBaseColor(Theme::GREEN_GAS);
            }
            return true;
        }
        if (m_btnStageAction[i].onMouseDown(px, py)) return true;
    }

    return false;
}

void StageSelectScreen::onMouseUp(int px, int py) {
    m_btnBack.onMouseUp(px, py);

    if (m_btnPrevPage.isPressed() && m_btnPrevPage.contains(px, py)) {
        m_page = 0;
    }
    m_btnPrevPage.onMouseUp(px, py);

    if (m_btnNextPage.isPressed() && m_btnNextPage.contains(px, py)) {
        m_page = 1;
    }
    m_btnNextPage.onMouseUp(px, py);

    int startIdx = m_page * 3;
    for (int i = startIdx; i < startIdx + 3; ++i) {
        m_btnStageAction[i].onMouseUp(px, py);
    }
}

void StageSelectScreen::drawStageThumbnail(Graphics::Framebuffer& fb, int x, int y, int w, int h, int stageIdx) {
    fb.fillRect(x, y, w, h, 0xFF05080E);

    for (int px = 0; px < w; ++px) {
        float t = static_cast<float>(px) / w;
        float hillY = 0.0f;
        uint32_t skyCol = 0;
        uint32_t groundCol = 0;
        uint32_t subCol = 0;

        if (stageIdx == 0) { // Countryside
            skyCol = 0xFF42A5F5;
            groundCol = 0xFF43A047;
            subCol = 0xFF2E7D32;
            hillY = std::sin(t * 6.28f + m_animTime * 1.5f) * 14.0f + h * 0.55f;
        } else if (stageIdx == 1) { // Desert
            skyCol = 0xFFFFB74D;
            groundCol = 0xFFFFA000;
            subCol = 0xFFE65100;
            hillY = std::sin(t * 8.0f + m_animTime * 0.8f) * 18.0f + h * 0.52f;
        } else if (stageIdx == 2) { // Arctic
            skyCol = 0xFF546E7A;
            groundCol = 0xFF80DEEA;
            subCol = 0xFF4DD0E1;
            hillY = std::sin(t * 7.0f + m_animTime * 0.6f) * 16.0f + h * 0.56f;
        } else if (stageIdx == 3) { // Mountain
            skyCol = 0xFF263238;
            groundCol = 0xFF546E7A;
            subCol = 0xFF37474F;
            hillY = std::sin(t * 12.0f) * 22.0f + h * 0.50f;
        } else if (stageIdx == 4) { // Moon
            skyCol = 0xFF070B14;
            groundCol = 0xFF90A4AE;
            subCol = 0xFF607D8B;
            hillY = std::sin(t * 4.0f) * 12.0f + h * 0.60f;
        } else { // Volcano
            skyCol = 0xFF2B0A0A;
            groundCol = 0xFFD32F2F;
            subCol = 0xFFB71C1C;
            hillY = std::sin(t * 10.0f + m_animTime * 1.1f) * 20.0f + h * 0.53f;
        }

        int hy = static_cast<int>(hillY);
        for (int py = 0; py < h; ++py) {
            uint32_t col;
            if (py < hy) {
                col = skyCol;
            } else if (py - hy < 6) {
                col = groundCol;
            } else {
                col = subCol;
            }
            fb.setPixelFast(x + px, y + py, col);
        }
    }

    // Special thumbnail decorative elements
    if (stageIdx == 0) {
        // Countryside Clouds
        fb.fillCircle(x + 30, y + 20, 10, 0xFFFFFFFF);
        fb.fillCircle(x + 42, y + 22, 8, 0xFFFFFFFF);
    } else if (stageIdx == 1) {
        // Scorching Sun
        fb.fillCircle(x + 28, y + 24, 10, 0xFFFFD54F);
        fb.fillCircle(x + 28, y + 24, 6, 0xFFFFF9C4);
    } else if (stageIdx == 2) {
        // Falling Arctic Snowflakes
        for (int s = 0; s < 8; ++s) {
            int sx = (s * 31 + static_cast<int>(m_animTime * 20.0f)) % (w - 10);
            int sy = (s * 13 + static_cast<int>(m_animTime * 35.0f)) % (h / 2);
            fb.fillRect(x + sx, y + sy, 2, 2, 0xFFFFFFFF);
        }
    } else if (stageIdx == 3) {
        // Mountain Snowcap ridge line
        for (int sx = 0; sx < w; ++sx) {
            float t = static_cast<float>(sx) / w;
            float hy = std::sin(t * 12.0f) * 22.0f + h * 0.50f;
            if (hy < h * 0.44f) {
                fb.fillRect(x + sx, y + static_cast<int>(hy), 1, 4, 0xFFECEFF1);
            }
        }
    } else if (stageIdx == 4) {
        // Distant Earth in moon sky
        fb.fillCircle(x + w - 28, y + 24, 11, 0xFF1E88E5);
        fb.fillCircle(x + w - 26, y + 22, 5, 0xFF43A047);
        // Twinkling stars
        int starPos[5][2] = {{12, 14}, {34, 25}, {60, 10}, {85, 28}, {110, 15}};
        for (int s = 0; s < 5; ++s) {
            if (static_cast<int>(m_animTime * 4.0f + s) % 3 != 0) {
                fb.setPixelFast(x + starPos[s][0], y + starPos[s][1], 0xFFFFFFFF);
            }
        }
    } else if (stageIdx == 5) {
        // Volcanic ash embers
        for (int s = 0; s < 7; ++s) {
            int ex = (s * 33 + static_cast<int>(m_animTime * 15.0f)) % (w - 10);
            int ey = (h / 2) - ((s * 11 + static_cast<int>(m_animTime * 25.0f)) % (h / 2));
            fb.setPixelFast(x + ex, y + ey, (s % 2 == 0) ? 0xFFFF5722 : 0xFFFFD54F);
        }
    }

    // Outer border with subtle glass shine
    fb.drawRect(x, y, w, h, Theme::CARD_BORDER);
    fb.fillRect(x + 2, y + 2, w - 4, 1, 0x33FFFFFF);
}

void StageSelectScreen::render(Graphics::Framebuffer& fb, const Core::ProfileManager& profile) {
    // 1. Dark Gradient Background
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF080E18, 0xFF121B2A);

    // 2. Header
    m_btnBack.render(fb, 2);

    int cx = m_width / 2;
    int headW = 300;
    UIComponents::drawArcadePanel(fb, cx - headW / 2, 12, headW, 36, 0xEE0B121C, 0xFF37474F, Theme::GOLD, 2, true, true);
    Graphics::RasterFont::drawStringCentered(fb, cx, 22, "SELECT STAGE TRACK", Theme::GOLD, 2);

    // Master Currency Pill
    int coinBoxW = 155;
    int coinBoxH = 34;
    int coinBoxX = m_width - coinBoxW - 24;
    int coinBoxY = 14;
    UIComponents::drawCoinBadge(fb, coinBoxX, coinBoxY, coinBoxW, coinBoxH, profile.coins(), m_animTime);

    // Paging Controls
    m_btnPrevPage.setEnabled(m_page > 0);
    m_btnPrevPage.setBaseColor((m_page > 0) ? 0xFF2A394A : 0xFF1E2834);
    m_btnPrevPage.render(fb, 1);

    m_btnNextPage.setEnabled(m_page < 1);
    m_btnNextPage.setBaseColor((m_page < 1) ? 0xFF2A394A : 0xFF1E2834);
    m_btnNextPage.render(fb, 1);

    std::string pageStr = "PAGE " + std::to_string(m_page + 1) + " OF 2";
    Graphics::RasterFont::drawStringCentered(fb, cx, 61, pageStr, Theme::CYAN_UPGRADE, 1);

    int cardW = 270;
    int cardH = 405;
    int cardGap = 20;
    int totalCardsW = 3 * cardW + 2 * cardGap;
    int startX = (m_width - totalCardsW) / 2;
    int cardY = 88;

    int startIdx = m_page * 3;
    for (int i = 0; i < 3; ++i) {
        int idx = startIdx + i;
        const auto& stage = s_stages[idx];
        int x = startX + i * (cardW + cardGap);
        int y = cardY;

        // Card Frame
        UIComponents::drawArcadePanel(fb, x, y, cardW, cardH, Theme::CARD_BG, Theme::CARD_BORDER, stage.stripeColor, 4, true, true);

        // Biome Mini-Icon in top-left of card
        UIComponents::drawBiomeIcon(fb, x + 12, y + 10, idx);

        // Title
        Graphics::RasterFont::drawString(fb, x + 34, y + 11, stage.name, Theme::TEXT_WHITE, 2);

        // Thumbnail
        drawStageThumbnail(fb, x + 14, y + 36, cardW - 28, 105, idx);

        // Info Chips
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 150, stage.desc, Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 168, "GRAVITY: " + stage.gravity, Theme::CYAN_UPGRADE, 1);

        // Difficulty Rating with Pixel Stars
        int starStartX = x + cardW / 2 - (5 * 10) / 2;
        int starY = y + 186;
        Graphics::RasterFont::drawString(fb, x + 30, starY - 1, "DIFFICULTY:", Theme::TEXT_MUTED, 1);
        for (int s = 0; s < 5; ++s) {
            UIComponents::drawStar(fb, starStartX + 35 + s * 12, starY + 2, 4, (s < stage.difficultyStars), Theme::GOLD, 0xFF37474F);
        }

        // Checkpoint Progression Display Box
        int cpCleared = profile.getStageCheckpointProgress(stage.id);
        bool completed = profile.isStageCompleted(stage.id);

        int progBoxW = cardW - 28;
        int progBoxH = 46;
        int progBoxX = x + 14;
        int progBoxY = y + 204;

        UIComponents::drawArcadePanel(fb, progBoxX, progBoxY, progBoxW, progBoxH, 0xFF121B27, 0xFF2A3A4E, 0, 0, false, false);

        if (completed) {
            Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, progBoxY + 8, "STAGE CLEARED (5/5)", Theme::GREEN_GAS, 1);
            // 5 Glowing Gold Pixel Stars
            int starsStartX = x + cardW / 2 - (5 * 18) / 2 + 8;
            for (int s = 0; s < 5; ++s) {
                UIComponents::drawStar(fb, starsStartX + s * 18, progBoxY + 28, 5, true, Theme::GOLD, 0xFFFFA000);
            }
        } else {
            std::string cpStr = "CHECKPOINTS: " + std::to_string(cpCleared) + " / 5";
            Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, progBoxY + 8, cpStr, Theme::GOLD, 1);

            // 5 Segmented Illuminated Progress Bars
            UIComponents::drawSegmentedBar(fb, progBoxX + 16, progBoxY + 24, progBoxW - 32, 12, cpCleared, 5,
                                          Theme::GREEN_GAS, 0xFF192534, 0x88FFFFFF);
        }

        // High Score Record Box
        float rec = profile.getRecordDistance(stage.id);
        std::string recStr = "RECORD: " + UIComponents::formatNumber(static_cast<int64_t>(rec)) + "m";
        Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 266, recStr, Theme::GOLD, 2);

        // Unlock status & Action Button
        bool unlocked = profile.isStageUnlocked(stage.id);
        auto& btn = m_btnStageAction[idx];

        if (unlocked) {
            btn.setText("PLAY");
            btn.setBaseColor(Theme::GREEN_GAS);
            btn.setEnabled(true);
        } else {
            int cost = profile.getStageUnlockCost(stage.id);
            bool canAfford = (profile.coins() >= cost);
            btn.setText("UNLOCK $" + UIComponents::formatNumber(cost));
            btn.setBaseColor(canAfford ? Theme::GOLD : 0xFF37474F);
            btn.setEnabled(canAfford);

            // Requirement hint under button
            std::string reqHint = "(" + stage.unlockRequirement + ")";
            Graphics::RasterFont::drawStringCentered(fb, x + cardW / 2, y + 338, reqHint, Theme::TEXT_MUTED, 1);
        }

        btn.setPosition(x + 14, y + 355);
        btn.setSize(cardW - 28, 40);
        btn.render(fb, 2);
    }
}

} // namespace UI
