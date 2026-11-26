#include "AchievementsScreen.h"
#include "AchievementManager.h"
#include "AccountManager.h"
#include "RasterFont.h"
#include "UIComponents.h"
#include "UITheme.h"
#include <algorithm>
#include <string>

namespace UI {

static std::string clippedText(const std::string& text, int maxWidth) {
    if (Graphics::RasterFont::getTextWidth(text) <= maxWidth) return text;
    std::string out = text;
    while (!out.empty() && Graphics::RasterFont::getTextWidth(out + "...") > maxWidth) {
        out.pop_back();
    }
    return out + "...";
}

AchievementsScreen::AchievementsScreen()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_btnBack(20, 16, 110, 34, "< BACK", 0xFF2A3A4E)
{
    m_btnBack.setKeyHint("ESC");
    setDimensions(m_width, m_height);
}

void AchievementsScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;
    m_btnBack.setPosition(20, 16);
    m_btnBack.setSize(110, 34);
}

void AchievementsScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);
}

bool AchievementsScreen::onMouseDown(int px, int py) {
    return m_btnBack.onMouseDown(px, py);
}

void AchievementsScreen::onMouseUp(int px, int py) {
    m_btnBack.onMouseUp(px, py);
}

void AchievementsScreen::update(float dt) {
    m_animTime += dt;
}

void AchievementsScreen::render(Graphics::Framebuffer& fb) {
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF07111D, 0xFF142B40);

    UIComponents::drawArcadePanel(fb, 140, 10, m_width - 280, 50,
                                  0xFF11263A, Theme::CYAN_UPGRADE, 0xFF00E5FF, 3);

    Graphics::RasterFont::drawStringCentered(
        fb, m_width / 2, 24, "ACHIEVEMENTS & MILLSTONES", Theme::GOLD, 2);

    Core::AchievementManager manager;
    auto views = manager.snapshot();
    int unlocked = manager.unlockedCount();

    UIComponents::drawCoinBadge(fb, m_width - 180, 18, 150, 30,
                                Core::AccountManager::instance().activeAccount()->coins,
                                m_animTime);

    std::string summary = std::to_string(unlocked) + " / " +
                          std::to_string(views.size()) + " UNLOCKED";
    Graphics::RasterFont::drawString(fb, 150, 68, summary,
                                     unlocked == static_cast<int>(views.size()) ? Theme::GOLD : 0xFFB0C4DE);

    int cols = (m_width >= 820) ? 2 : 1;
    int rows = (static_cast<int>(views.size()) + cols - 1) / cols;
    int margin = 22;
    int gapX = 12;
    int gapY = 6;
    int top = 84;
    int cardW = (m_width - margin * 2 - gapX * (cols - 1)) / cols;
    int availableH = m_height - top - 42;
    int cardH = std::min(62, (availableH - gapY * (rows - 1)) / std::max(1, rows));

    for (size_t i = 0; i < views.size(); ++i) {
        const auto& view = views[i];
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;
        int x = margin + col * (cardW + gapX);
        int y = top + row * (cardH + gapY);

        uint32_t accent = view.unlocked ? view.definition.color : 0xFF455A64;
        UIComponents::drawArcadePanel(fb, x, y, cardW, cardH,
                                      view.unlocked ? 0xFF163247 : 0xFF101B27,
                                      accent, accent, view.unlocked ? 2 : 0);

        int textX = x + 42;
        int maxTextW = cardW - 52;
        Graphics::RasterFont::drawString(fb, textX, y + 6,
                                         clippedText(view.definition.title, maxTextW),
                                         view.unlocked ? Theme::GOLD : 0xFFCFD8DC);
        Graphics::RasterFont::drawString(fb, textX, y + 19,
                                         clippedText(view.definition.description, maxTextW),
                                         view.unlocked ? 0xFFB0C4DE : 0xFF78909C);

        if (view.unlocked) {
            Graphics::RasterFont::drawString(fb, x + cardW - 48, y + 6, "DONE", Theme::GREEN_GAS);
        }

        int barW = cardW - 56;
        int current = std::min(static_cast<int>(view.current), static_cast<int>(view.target));
        UIComponents::drawSegmentedBar(fb, textX, y + 36, barW, 10,
                                       current, std::max(1, static_cast<int>(view.target)),
                                       view.unlocked ? view.definition.color : 0xFF607D8B);

        std::string progress = std::to_string(current) + "/" + std::to_string(static_cast<int>(view.target));
        int pw = Graphics::RasterFont::getTextWidth(progress);
        Graphics::RasterFont::drawString(fb, x + cardW - pw - 10, y + 36, progress,
                                         view.unlocked ? 0xFFFFFFFF : 0xFF90A4AE);

        UIComponents::drawStar(fb, x + 24, y + 27, 11, view.unlocked,
                               view.definition.color, 0xFFFFFFFF);
    }

    std::string footer = "REWARDS: " + UIComponents::formatNumber(
        Core::AccountManager::instance().activeAccount()->coins) + " COINS AVAILABLE";
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, m_height - 24, footer, 0xFF78909C);

    m_btnBack.render(fb);
}

} // namespace UI
