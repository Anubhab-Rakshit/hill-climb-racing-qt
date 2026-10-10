#include "PauseOverlay.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"

namespace UI {

PauseOverlay::PauseOverlay()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_btnResume(0, 0, 220, 44, "RESUME", Theme::GREEN_GAS)
    , m_btnRestart(0, 0, 220, 44, "RESTART", Theme::GOLD)
    , m_btnGarage(0, 0, 220, 44, "GARAGE", Theme::CYAN_UPGRADE)
    , m_btnMenu(0, 0, 220, 44, "MAIN MENU", 0xFF37474F)
{
    m_btnResume.setKeyHint("ESC");
    m_btnRestart.setKeyHint("R");
    setDimensions(m_width, m_height);
}

void PauseOverlay::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 230;
    int btnH = 44;
    int cx = width / 2 - btnW / 2;
    int startY = height / 2 - 76;

    m_btnResume.setPosition(cx, startY);
    m_btnResume.setSize(btnW, btnH);

    m_btnRestart.setPosition(cx, startY + 54);
    m_btnRestart.setSize(btnW, btnH);

    m_btnGarage.setPosition(cx, startY + 108);
    m_btnGarage.setSize(btnW, btnH);

    m_btnMenu.setPosition(cx, startY + 162);
    m_btnMenu.setSize(btnW, btnH);
}

void PauseOverlay::onMouseMove(int px, int py) {
    m_btnResume.onMouseMove(px, py);
    m_btnRestart.onMouseMove(px, py);
    m_btnGarage.onMouseMove(px, py);
    m_btnMenu.onMouseMove(px, py);
}

bool PauseOverlay::onMouseDown(int px, int py) {
    if (m_btnResume.onMouseDown(px, py)) return true;
    if (m_btnRestart.onMouseDown(px, py)) return true;
    if (m_btnGarage.onMouseDown(px, py)) return true;
    if (m_btnMenu.onMouseDown(px, py)) return true;
    return false;
}

void PauseOverlay::onMouseUp(int px, int py) {
    m_btnResume.onMouseUp(px, py);
    m_btnRestart.onMouseUp(px, py);
    m_btnGarage.onMouseUp(px, py);
    m_btnMenu.onMouseUp(px, py);
}

void PauseOverlay::render(Graphics::Framebuffer& fb) {
    // 1. Semi-transparent dark overlay across whole screen
    fb.fillRect(0, 0, m_width, m_height, 0xD0060A12);

    // 2. Centered Modal Box
    int boxW = 360;
    int boxH = 330;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2;

    UIComponents::drawArcadePanel(fb, boxX, boxY, boxW, boxH, Theme::CARD_BG, Theme::CYAN_UPGRADE, Theme::CYAN_UPGRADE, 4, true, true);

    // Corner Rivets
    fb.fillCircle(boxX + 8, boxY + 8, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + boxW - 9, boxY + 8, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + 8, boxY + boxH - 9, 2, Theme::CHROME_MID);
    fb.fillCircle(boxX + boxW - 9, boxY + boxH - 9, 2, Theme::CHROME_MID);

    // Title Plaque
    int cx = m_width / 2;
    // Pause Double-Bar Icon
    fb.fillRect(cx - 100, boxY + 22, 5, 18, Theme::GOLD);
    fb.fillRect(cx - 90, boxY + 22, 5, 18, Theme::GOLD);
    fb.fillRect(cx + 85, boxY + 22, 5, 18, Theme::GOLD);
    fb.fillRect(cx + 95, boxY + 22, 5, 18, Theme::GOLD);

    // Drop shadow title
    Graphics::RasterFont::drawStringCentered(fb, cx + 2, boxY + 24, "GAME PAUSED", 0xFF05080E, 3);
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 22, "GAME PAUSED", Theme::GOLD, 3);

    // Buttons
    m_btnResume.render(fb, 2);
    m_btnRestart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnMenu.render(fb, 2);
}

} // namespace UI
