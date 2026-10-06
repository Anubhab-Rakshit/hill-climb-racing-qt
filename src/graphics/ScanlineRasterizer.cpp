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
    float camX = cam.x();

    uint32_t skyTop = UI::Theme::SKY_TOP;
    uint32_t skyBot = UI::Theme::SKY_HORIZON;

    if (biomeId == "desert") {
        skyTop = 0xFFC95B24;
        skyBot = 0xFFFFB74D;
    } else if (biomeId == "moon") {
        skyTop = 0xFF050811;
        skyBot = 0xFF141C2B;
    } else if (biomeId == "mountain") {
        skyTop = 0xFF192532;
        skyBot = 0xFF455A64;
    }

    fb.fillVerticalGradient(0, 0, W, H, skyTop, skyBot);

    // Biome-specific celestial bodies & parallax sky elements
    if (biomeId == "moon") {
        // Glowing blue Earth suspended in space (slow parallax 0.01)
        int earthX = static_cast<int>(140 - camX * 0.01f);
        int earthY = 85;
        if (earthX > -40 && earthX < W + 40) {
            fb.fillCircle(earthX, earthY, 26, 0x331976D2); // Atmosphere corona
            fb.fillCircle(earthX, earthY, 20, 0xFF1E88E5); // Ocean disc
            fb.fillCircle(earthX - 4, earthY - 2, 9, 0xFF43A047); // Continent
            fb.fillCircle(earthX + 8, earthY + 5, 6, 0xFF388E3C); // Continent
            fb.fillCircle(earthX + 2, earthY - 14, 5, 0xFFFFFFFF); // Polar ice cap
        }

        // Twinkling star field
        for (int i = 0; i < 45; ++i) {
            int sx = static_cast<int>((i * 67 + 31) - camX * 0.02f) % W;
            if (sx < 0) sx += W;
            int sy = (i * 43 + 17) % (H / 2);
            fb.setPixelFast(sx, sy, (i % 3 == 0) ? 0xFF00E5FF : 0xFFFFFFFF);
        }
    } else if (biomeId == "desert") {
        // Blazing desert sun
        int sunX = static_cast<int>(W - 160 - camX * 0.015f);
        int sunY = 80;
        if (sunX > -50 && sunX < W + 50) {
            fb.fillCircle(sunX, sunY, 38, 0x33FFB300);
            fb.fillCircle(sunX, sunY, 26, 0x66FFD54F);
            fb.fillCircle(sunX, sunY, 18, 0xFFFFF9C4);
        }
    } else if (biomeId == "countryside") {
        // Glorious cartoon pixel clouds with fluffy white tops and soft shaded undersides
        auto drawCloud = [&](int cx, int cy, int size) {
            // Soft base shadow
            fb.fillCircle(cx, cy + 3, size + 2, 0xFFB0BEC5);
            fb.fillCircle(cx + size, cy + 5, (size * 4) / 5 + 2, 0xFFB0BEC5);
            fb.fillCircle(cx - size, cy + 5, (size * 3) / 5 + 2, 0xFFB0BEC5);
            fb.fillRect(cx - size, cy + 2, size * 2, size / 2 + 3, 0xFFB0BEC5);

            // Crisp white puffy body
            fb.fillCircle(cx, cy, size, 0xFFFFFFFF);
            fb.fillCircle(cx + size, cy + 2, (size * 4) / 5, 0xFFFFFFFF);
            fb.fillCircle(cx - size, cy + 3, (size * 3) / 5, 0xFFFFFFFF);
            fb.fillRect(cx - size, cy, size * 2, size / 2, 0xFFFFFFFF);
        };
        int c1X = static_cast<int>(W + 200 - std::fmod(camX * 0.04f, W + 400));
        int c2X = static_cast<int>(W + 400 - std::fmod(camX * 0.025f + 250, W + 500));
        drawCloud(c1X, 60, 22);
        drawCloud(c2X, 110, 28);
    }

    // Parallax Distant Mountain Range (Scroll factor 0.06)
    for (int x = 0; x < W; ++x) {
        float px = (camX * 0.06f) + (static_cast<float>(x) * 0.005f);
        float h = std::sin(px * 3.14f) * 65.0f + std::sin(px * 7.5f) * 22.0f + (H * 0.52f);
        int sy = static_cast<int>(h);
        sy = std::clamp(sy, 0, H);

        uint32_t mtnCol = (biomeId == "desert") ? 0xFFBF6E34 :
                          (biomeId == "moon") ? 0xFF192230 :
                          (biomeId == "mountain") ? 0xFF37474F : 0xFF3E5062;

        for (int y = sy; y < H; ++y) {
            fb.setPixelFast(x, y, mtnCol);
        }

        // Snow-capped peaks on mountain biome
        if (biomeId == "mountain" && sy < H * 0.44f) {
            fb.fillRect(x, sy, 1, 4, 0xFFCFD8DC);
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
        int bobY = sy + static_cast<int>(std::sin(gameTime * 4.5f + item.position.x) * 4.0f);

        if (item.type == Physics::ItemType::FUEL_CANISTER) {
            // Fuel Jerry Can Sprite (Red with white cross and silver nozzle)
            int canW = static_cast<int>(18 * (zoom / 22.0f));
            int canH = static_cast<int>(24 * (zoom / 22.0f));
            int cx = sx - canW / 2;
            int cy = bobY - canH / 2;

            fb.fillRect(cx, cy, canW, canH, 0xFFD32F2F);
            fb.drawRect(cx, cy, canW, canH, 0xFF8E0000);
            // Cap / Spout
            fb.fillRect(cx + canW - 6, cy - 4, 5, 4, 0xFFCFD8DC);
            // White Jerry Can Cross
            fb.drawLine(cx + 4, cy + 4, cx + canW - 4, cy + canH - 4, 0xFFFFFFFF);
            fb.drawLine(cx + canW - 4, cy + 4, cx + 4, cy + canH - 4, 0xFFFFFFFF);
            // Glow outline
            fb.drawRect(cx - 1, cy - 1, canW + 2, canH + 2, 0x44FF5252);
            // Label
            Graphics::RasterFont::drawStringCentered(fb, sx, cy - 11, "FUEL", 0xFFFF5252, 1);
        } else {
            // Coin (Gold, Silver, Bronze) with rotating shine
            int r = static_cast<int>(8 * (zoom / 22.0f));
            uint32_t coinCol = (item.type == Physics::ItemType::COIN_GOLD) ? UI::Theme::GOLD :
                               (item.type == Physics::ItemType::COIN_SILVER) ? 0xFFCFD8DC : 0xFFCD7F32;
            uint32_t rimCol = (item.type == Physics::ItemType::COIN_GOLD) ? 0xFFFFA000 :
                              (item.type == Physics::ItemType::COIN_SILVER) ? 0xFF90A4AE : 0xFF8D6E63;

            fb.fillCircle(sx, bobY, r, coinCol);
            fb.drawCircle(sx, bobY, r, rimCol);
            Graphics::RasterFont::drawStringCentered(fb, sx, bobY - 3, "$", 0xFF3E2723, 1);

            // Shimmer glint on gold coins
            if (item.type == Physics::ItemType::COIN_GOLD && (static_cast<int>(gameTime * 4.0f) % 2 == 0)) {
                fb.setPixelFast(sx - 3, bobY - 3, 0xFFFFFFFF);
            }
        }
    }
}

void ScanlineRasterizer::render(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime) {
    const std::string& biomeId = terrain.getBiome();

    // 1. Parallax Sky & Distant Landscapes
    renderParallaxSky(fb, cam, biomeId);

    // 2. Vertical Column-Scanline Terrain Filling
    int W = fb.width();
    int H = fb.height();
    float camX = cam.x();
    float camY = cam.y();
    float zoom = cam.zoom();
    float invZoom = 1.0f / zoom;

    uint32_t crestCol    = 0xFF81C784;
    uint32_t grassTopCol = UI::Theme::GRASS_LUSH;
    uint32_t grassBodyCol = UI::Theme::GRASS_DARK;
    uint32_t dirtBodyCol = UI::Theme::DIRT_RICH;
    uint32_t bedrockCol  = UI::Theme::DIRT_DEEP;

    if (biomeId == "desert") {
        crestCol     = 0xFFFFF59D;
        grassTopCol  = 0xFFFFD54F;
        grassBodyCol = 0xFFFFA000;
        dirtBodyCol  = 0xFFFF8F00;
        bedrockCol   = 0xFFD84315;
    } else if (biomeId == "moon") {
        crestCol     = 0xFFFFFFFF;
        grassTopCol  = 0xFFECEFF1;
        grassBodyCol = 0xFFCFD8DC;
        dirtBodyCol  = 0xFF90A4AE;
        bedrockCol   = 0xFF455A64;
    } else if (biomeId == "mountain") {
        crestCol     = 0xFFECEFF1;
        grassTopCol  = 0xFF78909C;
        grassBodyCol = 0xFF546E7A;
        dirtBodyCol  = 0xFF37474F;
        bedrockCol   = 0xFF212121;
    }

    for (int x = 0; x < W; ++x) {
        float worldX = camX + (x - W * 0.5f) * invZoom;
        float worldY = terrain.getHeight(worldX);
        int surfaceScreenY = static_cast<int>(H * 0.55f - (worldY - camY) * zoom);

        int startY = std::max(0, surfaceScreenY);

        for (int y = startY; y < H; ++y) {
            int depth = y - surfaceScreenY;
            uint32_t pixelCol;

            if (depth == 0) {
                pixelCol = crestCol;
            } else if (depth < 5) {
                pixelCol = grassTopCol;
            } else if (depth < 14) {
                pixelCol = grassBodyCol;
            } else if (depth < 24) {
                pixelCol = 0xFF4E342E; // Dark rich loam layer
            } else if (depth < 85) {
                // Procedural gravel / soil specks
                int pattern = ((x ^ y) & 7);
                pixelCol = (pattern == 0) ? (dirtBodyCol - 0x000E0E0E) :
                           (pattern == 3) ? (dirtBodyCol + 0x00121212) : dirtBodyCol;
            } else {
                // Subterranean bedrock with stratified stone fissures
                int pattern = ((x & 15) == 0 || (y & 15) == 0);
                pixelCol = (pattern) ? (bedrockCol + 0x00141414) : bedrockCol;
            }

            fb.setPixelFast(x, y, pixelCol);
        }

        // Grass blade tufts & wildflowers on countryside ridge
        if (biomeId == "countryside" && surfaceScreenY > 3 && surfaceScreenY < H) {
            if (x % 6 == 0) {
                fb.setPixelFast(x, surfaceScreenY - 1, 0xFF66BB6A);
                fb.setPixelFast(x + 1, surfaceScreenY - 2, 0xFF81C784);
            }
            // Scattered wildflowers: yellow dandelions & white clovers
            if (x % 17 == 0) {
                fb.setPixelFast(x, surfaceScreenY - 1, 0xFFFFEE58);
            } else if (x % 31 == 0) {
                fb.setPixelFast(x, surfaceScreenY - 1, 0xFFFFFFFF);
            }
        }
    }

    // 3. Distance Milestone Signposts planted in the terrain
    renderMilestones(fb, cam, terrain);

    // 4. Collectibles (Coins & Fuel)
    renderItems(fb, cam, terrain, gameTime);
}

void ScanlineRasterizer::renderMilestones(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain) {
    const auto& milestones = terrain.getMilestones();
    int W = fb.width();
    int H = fb.height();

    for (const auto& ms : milestones) {
        Physics::Vec2 scr = cam.worldToScreen(ms.position);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -60 || sx > W + 60 || sy < -100 || sy > H + 100) continue;

        // 1. Two Wooden Support Stakes Planted in the Ground
        int postW = 3;
        int postH = 40;
        int p1x = sx - 16;
        int p2x = sx + 13;
        int postTopY = sy - postH;

        fb.fillRect(p1x, postTopY, postW, postH, 0xFF4E342E);
        fb.fillRect(p1x + 1, postTopY, 1, postH, 0xFF795548); // Wood grain highlight
        fb.fillRect(p2x, postTopY, postW, postH, 0xFF4E342E);
        fb.fillRect(p2x + 1, postTopY, 1, postH, 0xFF795548);

        // 2. Wooden Signboard Plank
        int boardW = 52;
        int boardH = 22;
        int bx = sx - boardW / 2;
        int by = postTopY - 2;

        fb.fillRect(bx, by, boardW, boardH, 0xFF6D4C41);
        fb.fillRect(bx + 2, by + 2, boardW - 4, boardH - 4, 0xFF8D6E63); // Rich oak wood face
        fb.drawRect(bx, by, boardW, boardH, 0xFF3E2723); // Dark carved wood border

        // Horizontal wood grain lines
        fb.drawLine(bx + 4, by + 7, bx + boardW - 5, by + 7, 0xFF795548);
        fb.drawLine(bx + 4, by + 15, bx + boardW - 5, by + 15, 0xFF795548);

        // 4 Corner Iron Nails
        fb.setPixelFast(bx + 3, by + 3, 0xFF212121);
        fb.setPixelFast(bx + boardW - 4, by + 3, 0xFF212121);
        fb.setPixelFast(bx + 3, by + boardH - 4, 0xFF212121);
        fb.setPixelFast(bx + boardW - 4, by + boardH - 4, 0xFF212121);

        // Distance Label (e.g., "100m") with drop shadow
        std::string distStr = std::to_string(ms.distanceMeters) + "m";
        Graphics::RasterFont::drawStringCentered(fb, sx + 1, by + 5, distStr, 0xFF212121, 2);
        Graphics::RasterFont::drawStringCentered(fb, sx, by + 4, distStr, 0xFFFFF9C4, 2);

        // 3. Checkered Pennant / Flag on Top of the Signboard
        int flagPoleX = sx;
        int flagPoleTop = by - 16;
        fb.drawLine(flagPoleX, by, flagPoleX, flagPoleTop, 0xFFB0BEC5);

        // 8x6 Checkered Racing Flag waving from the pole
        for (int fx = 0; fx < 8; ++fx) {
            for (int fy = 0; fy < 6; ++fy) {
                bool isWhite = ((fx / 2 + fy / 2) % 2 == 0);
                fb.setPixelFast(flagPoleX + 1 + fx, flagPoleTop + fy, isWhite ? 0xFFFFFFFF : 0xFF212121);
            }
        }
    }
}

} // namespace Graphics
