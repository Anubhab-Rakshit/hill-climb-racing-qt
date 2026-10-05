#include "PauseOverlay.h"
#include "RasterFont.h"
#include "UITheme.h"

namespace UI {

PauseOverlay::PauseOverlay()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_btnResume(0, 0, 200, 44, "RESUME", Theme::GREEN_GAS)
    , m_btnRestart(0, 0, 200, 44, "RESTART", Theme::GOLD)
    , m_btnGarage(0, 0, 200, 44, "GARAGE", Theme::CYAN_UPGRADE)
    , m_btnMenu(0, 0, 200, 44, "MAIN MENU", 0xFF37474F)
{
    setDimensions(m_width, m_height);
}

void PauseOverlay::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int btnW = 220;
    int btnH = 44;
    int cx = width / 2 - btnW / 2;
    int startY = height / 2 - 80;

    m_btnResume.setPosition(cx, startY);
    m_btnResume.setSize(btnW, btnH);

    m_btnRestart.setPosition(cx, startY + 56);
    m_btnRestart.setSize(btnW, btnH);

    m_btnGarage.setPosition(cx, startY + 112);
    m_btnGarage.setSize(btnW, btnH);

    m_btnMenu.setPosition(cx, startY + 168);
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
    fb.fillRect(0, 0, m_width, m_height, 0xB8060A12);

    // 2. Centered Modal Box
    int boxW = 340;
    int boxH = 320;
    int boxX = (m_width - boxW) / 2;
    int boxY = (m_height - boxH) / 2;

    fb.fillRect(boxX, boxY, boxW, boxH, Theme::CARD_BG);
    fb.drawRect(boxX, boxY, boxW, boxH, Theme::CARD_BORDER_HI);
    fb.drawRect(boxX - 2, boxY - 2, boxW + 4, boxH + 4, 0xFF000000);

    // Title
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, boxY + 20, "GAME PAUSED", Theme::GOLD, 3);

    // Buttons
    m_btnResume.render(fb, 2);
    m_btnRestart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnMenu.render(fb, 2);
}

} // namespace UI
