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
    , m_enabled(true)
    , m_hovered(false)
    , m_pressed(false)
    , m_onClick(nullptr)
    , m_onSound(nullptr)
{
}

bool RetroButton::contains(int px, int py) const {
    return (px >= m_x && px < m_x + m_w && py >= m_y && py < m_y + m_h);
}

void RetroButton::onMouseMove(int px, int py) {
    m_hovered = contains(px, py);
}

bool RetroButton::onMouseDown(int px, int py) {
    if (!m_enabled) return false;
    if (contains(px, py)) {
        m_pressed = true;
        return true;
    }
    return false;
}

void RetroButton::onMouseUp(int px, int py) {
    if (!m_enabled) {
        m_pressed = false;
        return;
    }
    if (m_pressed && contains(px, py)) {
        m_pressed = false;
        if (m_onSound) {
            m_onSound();
        }
        if (m_onClick) {
            m_onClick();
        }
    }
    m_pressed = false;
}

void RetroButton::render(Graphics::Framebuffer& fb, int fontScale) {
    if (!m_enabled) {
        // Disabled / Locked appearance
        uint32_t disBg = 0xFF212B36;
        uint32_t disBorder = 0xFF37474F;
        fb.fillRect(m_x, m_y, m_w, m_h, disBg);
        fb.drawRect(m_x, m_y, m_w, m_h, disBorder);

        int textYOffset = (m_h - Graphics::RasterFont::getTextHeight(fontScale)) / 2;
        int cx = m_x + m_w / 2;
        int cy = m_y + textYOffset;
        Graphics::RasterFont::drawStringCentered(fb, cx, cy, m_text, 0xFF546E7A, fontScale);
        return;
    }

    uint32_t fillCol = m_baseColor;
    if (m_hovered && !m_pressed) {
        fillCol = lightenColor(m_baseColor, 40);
    } else if (m_pressed) {
        fillCol = darkenColor(m_baseColor, 35);
    }

    uint32_t lightBevel = lightenColor(fillCol, 70);
    uint32_t darkBevel = darkenColor(fillCol, 80);
    uint32_t blackBorder = 0xFF080C14;

    // Hover glow border (1px neon border if hovered)
    if (m_hovered && !m_pressed) {
        fb.drawRect(m_x - 1, m_y - 1, m_w + 2, m_h + 2, lightenColor(m_baseColor, 90));
    }

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

    // Top highlight specular line (subtle arcade gloss)
    if (!m_pressed) {
        fb.fillRect(m_x + 6, m_y + 4, m_w - 12, 1, 0x44FFFFFF);
    }

    // Text rendering with drop shadow and press offset
    int textYOffset = (m_h - Graphics::RasterFont::getTextHeight(fontScale)) / 2;
    int pressOffset = m_pressed ? 2 : 0;
    int cx = m_x + m_w / 2 + pressOffset;
    int cy = m_y + textYOffset + pressOffset;

    // Text drop shadow
    Graphics::RasterFont::drawStringCentered(fb, cx + 1, cy + 1, m_text, 0xFF05080E, fontScale);
    // Main text
    uint32_t textCol = (m_hovered && !m_pressed) ? 0xFFFFFFFF : m_textColor;
    Graphics::RasterFont::drawStringCentered(fb, cx, cy, m_text, textCol, fontScale);
}

} // namespace UI
