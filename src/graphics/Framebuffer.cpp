#include "Framebuffer.h"
#include <cstdlib>

namespace Graphics {

Framebuffer::Framebuffer(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_image(width, height, QImage::Format_RGB32)
{
    clear(0xFF000000);
}

void Framebuffer::resize(int width, int height) {
    if (m_width == width && m_height == height) return;
    m_width = width;
    m_height = height;
    m_image = QImage(width, height, QImage::Format_RGB32);
    clear(0xFF000000);
}

void Framebuffer::clear(uint32_t color) {
    m_image.fill(color);
}

void Framebuffer::fillRect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;

    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(m_width, x + w);
    int y1 = std::min(m_height, y + h);

    if (x0 >= x1 || y0 >= y1) return;

    uint8_t a = (color >> 24) & 0xFF;
    bool isOpaque = (a == 255);

    for (int cy = y0; cy < y1; ++cy) {
        uint32_t* line = scanLine(cy);
        if (isOpaque) {
            std::fill(line + x0, line + x1, color);
        } else {
            for (int cx = x0; cx < x1; ++cx) {
                blendPixelFast(cx, cy, color);
            }
        }
    }
}

void Framebuffer::drawRect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    drawLine(x, y, x + w - 1, y, color);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    drawLine(x, y, x, y + h - 1, color);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void Framebuffer::drawLine(int x0, int y0, int x1, int y1, uint32_t color) {
    // Bresenham's line algorithm
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        setPixelFast(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Framebuffer::drawCircle(int xc, int yc, int r, uint32_t color) {
    if (r <= 0) {
        setPixelFast(xc, yc, color);
        return;
    }
    // Midpoint circle algorithm
    int x = 0;
    int y = r;
    int d = 1 - r;

    auto plot8 = [&](int cx, int cy, int px, int py) {
        setPixelFast(cx + px, cy + py, color);
        setPixelFast(cx - px, cy + py, color);
        setPixelFast(cx + px, cy - py, color);
        setPixelFast(cx - px, cy - py, color);
        setPixelFast(cx + py, cy + px, color);
        setPixelFast(cx - py, cy + px, color);
        setPixelFast(cx + py, cy - px, color);
        setPixelFast(cx - py, cy - px, color);
    };

    plot8(xc, yc, x, y);
    while (x < y) {
        ++x;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            --y;
            d += 2 * (x - y) + 1;
        }
        plot8(xc, yc, x, y);
    }
}

void Framebuffer::fillCircle(int xc, int yc, int r, uint32_t color) {
    if (r <= 0) {
        setPixelFast(xc, yc, color);
        return;
    }
    int r2 = r * r;
    int y0 = std::max(0, yc - r);
    int y1 = std::min(m_height - 1, yc + r);

    uint8_t a = (color >> 24) & 0xFF;
    bool isOpaque = (a == 255);
    uint32_t opCol = 0xFF000000 | (color & 0x00FFFFFF);

    for (int y = y0; y <= y1; ++y) {
        int dy = y - yc;
        int dx = static_cast<int>(std::sqrt(r2 - dy * dy));
        int x0 = std::max(0, xc - dx);
        int x1 = std::min(m_width - 1, xc + dx);
        if (isOpaque) {
            uint32_t* line = scanLine(y);
            std::fill(line + x0, line + x1 + 1, opCol);
        } else {
            for (int x = x0; x <= x1; ++x) {
                blendPixelFast(x, y, color);
            }
        }
    }
}

void Framebuffer::fillVerticalGradient(int x, int y, int w, int h, uint32_t topColor, uint32_t bottomColor) {
    if (w <= 0 || h <= 0) return;
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(m_width, x + w);
    int y1 = std::min(m_height, y + h);

    if (x0 >= x1 || y0 >= y1) return;

    float tr = (topColor >> 16) & 0xFF;
    float tg = (topColor >> 8) & 0xFF;
    float tb = topColor & 0xFF;
    float ta = (topColor >> 24) & 0xFF;

    float br = (bottomColor >> 16) & 0xFF;
    float bg = (bottomColor >> 8) & 0xFF;
    float bb = bottomColor & 0xFF;
    float ba = (bottomColor >> 24) & 0xFF;

    float invH = 1.0f / static_cast<float>(h);

    for (int cy = y0; cy < y1; ++cy) {
        float t = (cy - y) * invH;
        uint32_t r = static_cast<uint32_t>(tr + (br - tr) * t);
        uint32_t g = static_cast<uint32_t>(tg + (bg - tg) * t);
        uint32_t b = static_cast<uint32_t>(tb + (bb - tb) * t);
        uint32_t a = static_cast<uint32_t>(ta + (ba - ta) * t);

        uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;
        uint32_t* line = scanLine(cy);
        std::fill(line + x0, line + x1, color);
    }
}

void Framebuffer::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }

    if (y0 == y2) return; // Degenerate horizontal segment

    int totalHeight = y2 - y0;
    uint8_t a = (color >> 24) & 0xFF;
    bool isOpaque = (a == 255);
    uint32_t opCol = 0xFF000000 | (color & 0x00FFFFFF);

    for (int i = 0; i <= totalHeight; ++i) {
        int cy = y0 + i;
        if (cy < 0 || cy >= m_height) continue;

        bool secondHalf = (i > (y1 - y0) || y1 == y0);
        int segmentHeight = secondHalf ? (y2 - y1) : (y1 - y0);
        if (segmentHeight <= 0) segmentHeight = 1;

        float alpha = static_cast<float>(i) / totalHeight;
        float beta = static_cast<float>(i - (secondHalf ? (y1 - y0) : 0)) / segmentHeight;

        int ax = static_cast<int>(x0 + (x2 - x0) * alpha);
        int bx = secondHalf ? static_cast<int>(x1 + (x2 - x1) * beta)
                            : static_cast<int>(x0 + (x1 - x0) * beta);

        if (ax > bx) std::swap(ax, bx);
        int startX = std::max(0, ax);
        int endX = std::min(m_width - 1, bx);

        if (startX <= endX) {
            if (isOpaque) {
                uint32_t* line = scanLine(cy);
                std::fill(line + startX, line + endX + 1, opCol);
            } else {
                for (int cx = startX; cx <= endX; ++cx) {
                    blendPixelFast(cx, cy, color);
                }
            }
        }
    }
}

} // namespace Graphics
