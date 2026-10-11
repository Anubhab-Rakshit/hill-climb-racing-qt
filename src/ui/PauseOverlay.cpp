#include "PauseOverlay.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"

namespace UI {

PauseOverlay::PauseOverlay()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_btnResume(0, 0, 220, 44, "RESUME", Theme::GREEN_GAS)
    , m_btnRestart(0, 0, 220, 44, "RESTART", Theme::GOLD)
    , m_btnGarage(0, 0, 220, 44, "GARAGE", Theme::CYAN_UPGRADE)
    , m_btnMenu(0, 0, 220, 44, "MAIN MENU", 0xFF37474F)
{
    m_btnResume.setKeyHint("ESC");
    m_btnRestart.setKeyHint("R");
    setDimensions(m_width, m_height);
}

void PauseOverlay::update(float dt) {
    m_animTime += dt;
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

    // Subtle scanline grid effect for authentic retro CRT pause overlay
    for (int y = 0; y < m_height; y += 4) {
        fb.drawLine(0, y, m_width, y, 0x1A000000);
    }

    // 2. Centered Modal Box
    int boxW = 380;
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
    // Pause Double-Bar Icons flanking title with ample clearance
    fb.fillRect(cx - 128, boxY + 22, 5, 18, Theme::GOLD);
    fb.fillRect(cx - 118, boxY + 22, 5, 18, Theme::GOLD);
    fb.fillRect(cx + 113, boxY + 22, 5, 18, Theme::GOLD);
    fb.fillRect(cx + 123, boxY + 22, 5, 18, Theme::GOLD);

    // Drop shadow title
    Graphics::RasterFont::drawStringCentered(fb, cx + 2, boxY + 24, "GAME PAUSED", 0xFF05080E, 3);
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + 22, "GAME PAUSED", Theme::GOLD, 3);

    // Tech divider line under title
    fb.drawLine(boxX + 30, boxY + 49, boxX + boxW - 30, boxY + 49, 0xFF1E2D3D);
    fb.drawLine(boxX + 40, boxY + 50, boxX + boxW - 40, boxY + 50, 0x4400E5FF);

    // Breathing pulse halo around RESUME button to draw intuitive player focus
    float pulse = 0.5f + 0.5f * std::sin(m_animTime * 4.5f);
    int haloAlpha = static_cast<int>(35 + 55 * pulse);
    uint32_t haloColor = (static_cast<uint32_t>(haloAlpha) << 24) | (Theme::GREEN_GAS & 0x00FFFFFF);
    fb.drawRect(m_btnResume.x() - 2, m_btnResume.y() - 2,
                m_btnResume.width() + 4, m_btnResume.height() + 4, haloColor);
    fb.drawRect(m_btnResume.x() - 3, m_btnResume.y() - 3,
                m_btnResume.width() + 6, m_btnResume.height() + 6, (haloColor & 0x00FFFFFF) | 0x18000000);

    // Buttons
    m_btnResume.render(fb, 2);
    m_btnRestart.render(fb, 2);
    m_btnGarage.render(fb, 2);
    m_btnMenu.render(fb, 2);

    // Subtle bottom key navigation hint
    Graphics::RasterFont::drawStringCentered(fb, cx, boxY + boxH - 15, "PRESS [ESC] OR CLICK RESUME TO CONTINUE", Theme::TEXT_MUTED, 1);
}

} // namespace UI
