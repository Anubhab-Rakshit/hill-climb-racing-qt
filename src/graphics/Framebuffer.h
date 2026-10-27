#pragma once

#include <QImage>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Graphics {

/**
 * @brief High-performance software framebuffer rendering directly into a 32-bit ARGB pixel grid.
 * Adheres strictly to the pure-rasterization requirement (zero vector graphics / SVG / OpenGL).
 */
class Framebuffer {
public:
    Framebuffer(int width = 960, int height = 540);

    void resize(int width, int height);

    int width() const { return m_width; }
    int height() const { return m_height; }

    const QImage& getImage() const { return m_image; }
    QImage& getImage() { return m_image; }

    uint32_t* scanLine(int y) {
        return reinterpret_cast<uint32_t*>(m_image.scanLine(y));
    }

    const uint32_t* scanLine(int y) const {
        return reinterpret_cast<const uint32_t*>(m_image.scanLine(y));
    }

    int stride() const {
        return m_image.bytesPerLine() / static_cast<int>(sizeof(uint32_t));
    }

    uint32_t* bits() {
        return reinterpret_cast<uint32_t*>(m_image.bits());
    }

    const uint32_t* bits() const {
        return reinterpret_cast<const uint32_t*>(m_image.bits());
    }

    // Direct pixel operations with inline bounds checking
    inline void setPixelFast(int x, int y, uint32_t color) {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            reinterpret_cast<uint32_t*>(m_image.scanLine(y))[x] = 0xFF000000 | (color & 0x00FFFFFF);
        }
    }

    inline uint32_t getPixelFast(int x, int y) const {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            return reinterpret_cast<const uint32_t*>(m_image.scanLine(y))[x];
        }
        return 0;
    }

    // Fast bitshift alpha blending onto opaque 32-bit pixel: (t + (t >> 8)) >> 8 exactly matches t / 255
    static inline uint32_t blendOpaqueDst(uint32_t dst, uint32_t sr, uint32_t sg, uint32_t sb, uint32_t sa, uint32_t invA) {
        uint32_t dr = (dst >> 16) & 0xFF;
        uint32_t dg = (dst >> 8) & 0xFF;
        uint32_t db = dst & 0xFF;

        uint32_t tr = sr * sa + dr * invA + 128;
        uint32_t tg = sg * sa + dg * invA + 128;
        uint32_t tb = sb * sa + db * invA + 128;

        uint32_t outR = (tr + (tr >> 8)) >> 8;
        uint32_t outG = (tg + (tg >> 8)) >> 8;
        uint32_t outB = (tb + (tb >> 8)) >> 8;

        return 0xFF000000 | (outR << 16) | (outG << 8) | outB;
    }

    // Standard alpha blending onto opaque 32-bit pixel grid
    inline void blendPixelFast(int x, int y, uint32_t srcColor) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;

        uint32_t sa = (srcColor >> 24) & 0xFF;
        if (sa >= 255) {
            reinterpret_cast<uint32_t*>(m_image.scanLine(y))[x] = 0xFF000000 | (srcColor & 0x00FFFFFF);
            return;
        }
        if (sa == 0) return;

        uint32_t* pixel = &reinterpret_cast<uint32_t*>(m_image.scanLine(y))[x];
        uint32_t sr = (srcColor >> 16) & 0xFF;
        uint32_t sg = (srcColor >> 8) & 0xFF;
        uint32_t sb = srcColor & 0xFF;
        uint32_t invA = 255 - sa;

        *pixel = blendOpaqueDst(*pixel, sr, sg, sb, sa, invA);
    }

    // High performance raster primitives
    void clear(uint32_t color);
    void fillRect(int x, int y, int w, int h, uint32_t color);
    void drawRect(int x, int y, int w, int h, uint32_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint32_t color);
    void drawCircle(int xc, int yc, int r, uint32_t color);
    void fillCircle(int xc, int yc, int r, uint32_t color);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
    void fillVerticalGradient(int x, int y, int w, int h, uint32_t topColor, uint32_t bottomColor);
    void blit(int destX, int destY, const Framebuffer& src);

private:
    int m_width;
    int m_height;
    QImage m_image;
};

} // namespace Graphics
