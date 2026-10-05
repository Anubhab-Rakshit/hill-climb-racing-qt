#include "RetroButton.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <algorithm>

namespace UI {

static uint32_t lightenColor(uint32_t c, int amount) {
    uint32_t a = (c >> 24) & 0xFF;
    uint32_t r = std::min(255, static_cast<int>((c >> 16) & 0xFF) + amount);
    uint32_t g = std::min(255, static_cast<int>((c >> 8) & 0xFF) + amount);
    uint32_t b = std::min(255, static_cast<int>(c & 0xFF) + amount);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t darkenColor(uint32_t c, int amount) {
    uint32_t a = (c >> 24) & 0xFF;
    uint32_t r = std::max(0, static_cast<int>((c >> 16) & 0xFF) - amount);
    uint32_t g = std::max(0, static_cast<int>((c >> 8) & 0xFF) - amount);
    uint32_t b = std::max(0, static_cast<int>(c & 0xFF) - amount);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

RetroButton::RetroButton(int x, int y, int w, int h, const std::string& text, uint32_t baseColor)
    : m_x(x)
    , m_y(y)
    , m_w(w)
    , m_h(h)
    , m_text(text)
    , m_baseColor(baseColor)
    , m_textColor(Theme::TEXT_WHITE)
    , m_hovered(false)
    , m_pressed(false)
    , m_onClick(nullptr)
{
}

bool RetroButton::contains(int px, int py) const {
    return (px >= m_x && px < m_x + m_w && py >= m_y && py < m_y + m_h);
}

void RetroButton::onMouseMove(int px, int py) {
    m_hovered = contains(px, py);
    if (!m_hovered) {
        m_pressed = false;
    }
}

bool RetroButton::onMouseDown(int px, int py) {
    if (contains(px, py)) {
        m_pressed = true;
        return true;
    }
    return false;
}

void RetroButton::onMouseUp(int px, int py) {
    if (m_pressed && contains(px, py)) {
        m_pressed = false;
        if (m_onClick) {
            m_onClick();
        }
    }
    m_pressed = false;
}

void RetroButton::render(Graphics::Framebuffer& fb, int fontScale) {
    uint32_t fillCol = m_baseColor;
    if (m_hovered && !m_pressed) {
        fillCol = lightenColor(m_baseColor, 35);
    } else if (m_pressed) {
        fillCol = darkenColor(m_baseColor, 30);
    }

    uint32_t lightBevel = lightenColor(fillCol, 60);
    uint32_t darkBevel = darkenColor(fillCol, 70);
    uint32_t blackBorder = 0xFF000000;

    // Outer 1px black border
    fb.drawRect(m_x, m_y, m_w, m_h, blackBorder);

    // Bevel logic (Inverted if pressed)
    uint32_t topBevel = m_pressed ? darkBevel : lightBevel;
    uint32_t botBevel = m_pressed ? lightBevel : darkBevel;

    // Fill body
    fb.fillRect(m_x + 3, m_y + 3, m_w - 6, m_h - 6, fillCol);

    // Top and Left bevel (2px thick)
    fb.fillRect(m_x + 1, m_y + 1, m_w - 2, 2, topBevel);
    fb.fillRect(m_x + 1, m_y + 1, 2, m_h - 2, topBevel);

    // Bottom and Right bevel (2px thick)
    fb.fillRect(m_x + 1, m_y + m_h - 3, m_w - 2, 2, botBevel);
    fb.fillRect(m_x + m_w - 3, m_y + 1, 2, m_h - 2, botBevel);

    // Corner pixel notches for arcade aesthetic
    fb.setPixelFast(m_x + 1, m_y + 1, blackBorder);
    fb.setPixelFast(m_x + m_w - 2, m_y + 1, blackBorder);
    fb.setPixelFast(m_x + 1, m_y + m_h - 2, blackBorder);
    fb.setPixelFast(m_x + m_w - 2, m_y + m_h - 2, blackBorder);

    // Text rendering with drop shadow and press offset
    int textYOffset = (m_h - Graphics::RasterFont::getTextHeight(fontScale)) / 2;
    int pressOffset = m_pressed ? 2 : 0;
    int cx = m_x + m_w / 2 + pressOffset;
    int cy = m_y + textYOffset + pressOffset;

    // Text drop shadow
    Graphics::RasterFont::drawStringCentered(fb, cx + 1, cy + 1, m_text, 0xFF0A0F1A, fontScale);
    // Main text
    Graphics::RasterFont::drawStringCentered(fb, cx, cy, m_text, m_textColor, fontScale);
}

} // namespace UI
