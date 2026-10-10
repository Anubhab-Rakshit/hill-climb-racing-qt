#pragma once

#include "Framebuffer.h"
#include <string>
#include <functional>

namespace UI {

/**
 * @brief Chunky 3D-beveled Arcade Pixel Button.
 * Features hover highlight, 2px physical press depression, and click callback.
 */
class RetroButton {
public:
    RetroButton(int x = 0, int y = 0, int w = 120, int h = 40,
                const std::string& text = "BUTTON",
                uint32_t baseColor = 0xFF2979FF);

    void setPosition(int x, int y) { m_x = x; m_y = y; }
    void setSize(int w, int h) { m_w = w; m_h = h; }
    void setText(const std::string& text) { m_text = text; }
    void setBaseColor(uint32_t color) { m_baseColor = color; }
    void setTextColor(uint32_t color) { m_textColor = color; }
    void setOnClick(std::function<void()> cb) { m_onClick = cb; }

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void setOnSound(std::function<void()> cb) { m_onSound = cb; }
    void setKeyHint(const std::string& hint) { m_keyHint = hint; }
    const std::string& keyHint() const { return m_keyHint; }
    void setCornerNotches(bool notches) { m_cornerNotches = notches; }

    bool contains(int px, int py) const;
    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    void render(Graphics::Framebuffer& fb, int fontScale = 2);

    bool isHovered() const { return m_hovered; }
    bool isPressed() const { return m_pressed; }
    void setPressed(bool p) { m_pressed = p; }

private:
    int m_x;
    int m_y;
    int m_w;
    int m_h;
    std::string m_text;
    std::string m_keyHint;
    uint32_t m_baseColor;
    uint32_t m_textColor;
    bool m_enabled = true;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_cornerNotches = true;
    std::function<void()> m_onClick;
    std::function<void()> m_onSound;
};

} // namespace UI
