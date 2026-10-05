#pragma once

#include "Framebuffer.h"
#include <string>
#include <cstdint>

namespace Graphics {

/**
 * @brief Embedded 5x7 / 8x8 pixel bitmap font renderer.
 * Draws crisp arcade typography directly into the software framebuffer without vector engines.
 */
class RasterFont {
public:
    static void drawChar(Framebuffer& fb, int x, int y, char c, uint32_t color, int scale = 1);
    static void drawString(Framebuffer& fb, int x, int y, const std::string& text, uint32_t color, int scale = 1, int letterSpacing = 1);
    static void drawStringCentered(Framebuffer& fb, int cx, int y, const std::string& text, uint32_t color, int scale = 1);
    static int getTextWidth(const std::string& text, int scale = 1, int letterSpacing = 1);
    static int getTextHeight(int scale = 1);
};

} // namespace Graphics
