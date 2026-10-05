#include "ScanlineRasterizer.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <algorithm>

namespace Graphics {

ScanlineRasterizer::ScanlineRasterizer() {
}

void ScanlineRasterizer::renderParallaxSky(Framebuffer& fb, const Camera& cam, const std::string& biomeId) {
    int W = fb.width();
    int H = fb.height();

    uint32_t skyTop = UI::Theme::SKY_TOP;
    uint32_t skyBot = UI::Theme::SKY_HORIZON;

    if (biomeId == "desert") {
        skyTop = 0xFFD87D39;
        skyBot = 0xFFFFCC80;
    } else if (biomeId == "moon") {
        skyTop = 0xFF050811;
        skyBot = 0xFF172033;
    } else if (biomeId == "mountain") {
        skyTop = 0xFF1C2833;
        skyBot = 0xFF455A64;
    }

    fb.fillVerticalGradient(0, 0, W, H, skyTop, skyBot);

    // Parallax Distant Mountains Silhouette (Scroll factor 0.08)
    float camX = cam.x();
    for (int x = 0; x < W; ++x) {
        float px = (camX * 0.08f) + (static_cast<float>(x) * 0.005f);
        float h = std::sin(px * 3.14f) * 60.0f + std::sin(px * 7.5f) * 25.0f + (H * 0.55f);
        int sy = static_cast<int>(h);
        sy = std::clamp(sy, 0, H);

        uint32_t mtnCol = (biomeId == "desert") ? 0xFFBF7A40 : (biomeId == "moon") ? 0xFF1E2838 : 0xFF243B53;
        for (int y = sy; y < H; ++y) {
            fb.setPixelFast(x, y, mtnCol);
        }
    }
}

void ScanlineRasterizer::renderItems(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime) {
    const auto& items = terrain.getItems();
    float zoom = cam.zoom();

    for (const auto& item : items) {
        if (item.collected) continue;

        Physics::Vec2 scr = cam.worldToScreen(item.position);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -30 || sx > fb.width() + 30 || sy < -30 || sy > fb.height() + 30) continue;

        // Gentle floating bob
        int bobY = sy + static_cast<int>(std::sin(gameTime * 4.0f + item.position.x) * 3.0f);

        if (item.type == Physics::ItemType::FUEL_CANISTER) {
            // Fuel Canister Sprite (Red Jerry Can)
            int canW = static_cast<int>(18 * (zoom / 22.0f));
            int canH = static_cast<int>(24 * (zoom / 22.0f));
            int cx = sx - canW / 2;
            int cy = bobY - canH / 2;

            fb.fillRect(cx, cy, canW, canH, 0xFFD32F2F);
            fb.drawRect(cx, cy, canW, canH, 0xFF8E0000);
            // Cap / Spout
            fb.fillRect(cx + canW - 6, cy - 4, 5, 4, 0xFF424242);
            // White Jerry Can Cross
            fb.drawLine(cx + 4, cy + 4, cx + canW - 4, cy + canH - 4, 0xFFFFFFFF);
            fb.drawLine(cx + canW - 4, cy + 4, cx + 4, cy + canH - 4, 0xFFFFFFFF);
            // Label
            Graphics::RasterFont::drawStringCentered(fb, sx, cy - 10, "FUEL", UI::Theme::RED_BRAKE, 1);
        } else {
            // Coin (Gold, Silver, Bronze)
            int r = static_cast<int>(8 * (zoom / 22.0f));
            uint32_t coinCol = (item.type == Physics::ItemType::COIN_GOLD) ? UI::Theme::GOLD :
                               (item.type == Physics::ItemType::COIN_SILVER) ? 0xFFCFD8DC : 0xFFCD7F32;
            uint32_t rimCol = (item.type == Physics::ItemType::COIN_GOLD) ? 0xFFFFA000 :
                              (item.type == Physics::ItemType::COIN_SILVER) ? 0xFF90A4AE : 0xFF8D6E63;

            fb.fillCircle(sx, bobY, r, coinCol);
            fb.drawCircle(sx, bobY, r, rimCol);
            Graphics::RasterFont::drawStringCentered(fb, sx, bobY - 3, "$", 0xFF3E2723, 1);
        }
    }
}

void ScanlineRasterizer::render(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime) {
    const std::string& biomeId = terrain.getBiome();

    // 1. Parallax Sky & Mountains
    renderParallaxSky(fb, cam, biomeId);

    // 2. Vertical Column-Scanline Terrain Filling
    int W = fb.width();
    int H = fb.height();
    float camX = cam.x();
    float camY = cam.y();
    float zoom = cam.zoom();
    float invZoom = 1.0f / zoom;

    uint32_t grassTopCol = UI::Theme::GRASS_LUSH;
    uint32_t grassBodyCol = UI::Theme::GRASS_DARK;
    uint32_t dirtBodyCol = UI::Theme::DIRT_RICH;
    uint32_t bedrockCol = UI::Theme::DIRT_DEEP;

    if (biomeId == "desert") {
        grassTopCol = 0xFFFFD54F;
        grassBodyCol = 0xFFFFA000;
        dirtBodyCol = 0xFFFF8F00;
        bedrockCol = 0xFFE65100;
    } else if (biomeId == "moon") {
        grassTopCol = 0xFFECEFF1;
        grassBodyCol = 0xFFCFD8DC;
        dirtBodyCol = 0xFF90A4AE;
        bedrockCol = 0xFF455A64;
    } else if (biomeId == "mountain") {
        grassTopCol = 0xFF78909C;
        grassBodyCol = 0xFF546E7A;
        dirtBodyCol = 0xFF37474F;
        bedrockCol = 0xFF212121;
    }

    for (int x = 0; x < W; ++x) {
        float worldX = camX + (x - W * 0.5f) * invZoom;
        float worldY = terrain.getHeight(worldX);
        int surfaceScreenY = static_cast<int>(H * 0.55f - (worldY - camY) * zoom);

        int startY = std::max(0, surfaceScreenY);

        for (int y = startY; y < H; ++y) {
            int depth = y - surfaceScreenY;
            uint32_t pixelCol;

            if (depth < 5) {
                pixelCol = grassTopCol;
            } else if (depth < 14) {
                pixelCol = grassBodyCol;
            } else if (depth < 70) {
                // Procedural soil texture
                int pattern = ((x ^ y) & 7);
                pixelCol = (pattern == 0) ? dirtBodyCol - 0x00080808 : dirtBodyCol;
            } else {
                // Subterranean bedrock
                int pattern = ((x & 15) == 0 || (y & 15) == 0);
                pixelCol = (pattern) ? bedrockCol + 0x00101010 : bedrockCol;
            }

            fb.setPixelFast(x, y, pixelCol);
        }

        // Highlight top ridge pixel
        if (surfaceScreenY >= 0 && surfaceScreenY < H) {
            fb.setPixelFast(x, surfaceScreenY, 0xFFFFFFFF);
        }
    }

    // 3. Collectibles (Coins & Fuel)
    renderItems(fb, cam, terrain, gameTime);
}

} // namespace Graphics
