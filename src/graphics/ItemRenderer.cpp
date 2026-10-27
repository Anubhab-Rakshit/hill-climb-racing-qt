#include "ItemRenderer.h"
#include "Palette.h"
#include "Framebuffer.h"
#include "Camera.h"
#include "Terrain.h"
#include "RasterFont.h"

namespace Graphics {

void ItemRenderer::drawItem(Framebuffer &fb,
                           const Camera &cam,
                           const Physics::Terrain &terrain,
                           const Physics::WorldItem &item,
                           float gameTime) {
    // Compute screen position
    Physics::Vec2 scr = cam.worldToScreen(item.position);
    int sx = static_cast<int>(scr.x);
    int sy = static_cast<int>(scr.y);

    // Simple frustum cull (same thresholds as original code)
    if (sx < -35 || sx > fb.width() + 35 || sy < -35 || sy > fb.height() + 35)
        return;

    // Apply a gentle bobbing animation for floating items
    int bobY = sy + static_cast<int>(std::sin(gameTime * 4.2f + item.position.x * 0.8f) * 3.5f);

    if (item.type == Physics::ItemType::FUEL_CANISTER) {
        // ---- Fuel Canister -------------------------------------------------
        const int cw = 20;
        const int ch = 26;
        int cx = sx - cw / 2;
        int cy = bobY - ch / 2;

        // Ground shadow – uses palette constant
        float groundY = terrain.getHeight(item.position.x);
        Physics::Vec2 gScr = cam.worldToScreen({item.position.x, groundY});
        if (gScr.x >= 0 && gScr.x < fb.width() && gScr.y >= 0 && gScr.y < fb.height()) {
            fb.fillCircle(static_cast<int>(gScr.x), static_cast<int>(gScr.y), 9, Palette::FuelCanShadow);
        }

        // Handles
        fb.fillRect(cx + 3, cy - 4, cw - 6, 4, Palette::FuelCanHandle);
        fb.fillRect(cx + 6, cy - 3, cw - 12, 3, 0x00000000); // hollow grip
        fb.drawRect(cx + 3, cy - 4, cw - 6, 4, Palette::FuelCanHandleBorder);

        // Spout & cap
        fb.fillRect(cx + cw - 6, cy - 6, 5, 4, Palette::FuelCanSpout);
        fb.drawRect(cx + cw - 6, cy - 6, 5, 4, Palette::FuelCanSpoutBorder);
        fb.fillRect(cx + cw - 5, cy - 7, 3, 2, Palette::FuelCanCap);

        // Body & border
        fb.fillRect(cx, cy, cw, ch, Palette::FuelCanBody);
        fb.drawRect(cx, cy, cw, ch, Palette::FuelCanBodyBorder);
        // Bevel
        fb.drawLine(cx + 1, cy + 1, cx + cw - 2, cy + 1, Palette::FuelCanBevel);
        fb.drawLine(cx + 1, cy + 1, cx + 1, cy + ch - 2, Palette::FuelCanBevel);

        // X indentation
        fb.drawLine(cx + 3, cy + 3, cx + cw - 4, cy + ch - 4, Palette::FuelCanXLine1);
        fb.drawLine(cx + 4, cy + 3, cx + cw - 3, cy + ch - 4, Palette::FuelCanXLine2);
        fb.drawLine(cx + cw - 4, cy + 3, cx + 3, cy + ch - 4, Palette::FuelCanXLine1);
        fb.drawLine(cx + cw - 3, cy + 3, cx + 4, cy + ch - 4, Palette::FuelCanXLine2);

        // Badge
        int badgeW = cw - 6;
        int badgeH = 9;
        int bx = cx + 3;
        int by = cy + (ch - badgeH) / 2;
        fb.fillRect(bx, by, badgeW, badgeH, Palette::FuelCanBadgeFill);
        fb.drawRect(bx, by, badgeW, badgeH, Palette::FuelCanBadgeBorder);
        Graphics::RasterFont::drawStringCentered(fb, sx, by + 1, "GAS", Palette::FuelCanBadgeText, 1);

        // Halo – colour kept in palette, alpha calculated per‑frame
        float pulse = 0.5f + 0.5f * std::sin(gameTime * 5.0f + item.position.x);
        uint32_t haloA = static_cast<uint32_t>(pulse * 45.0f);
        fb.drawRect(cx - 1, cy - 1, cw + 2, ch + 2, (haloA << 24) | Palette::FuelCanHaloBase);
        return;
    }

    // ---- Coin rendering ---------------------------------------------------
    // Spin angle determines apparent width (rx) of the disc
    float spinAngle = gameTime * 4.5f + item.position.x * 0.7f;
    float cosSpin = std::cos(spinAngle);
    float absCos = std::abs(cosSpin);
    const int baseR = 10;
    int rx = std::max(4, static_cast<int>(absCos * baseR));
    int ry = baseR;

    // Ground shadow for coins (same style as original)
    float groundY = terrain.getHeight(item.position.x);
    Physics::Vec2 gScr = cam.worldToScreen({item.position.x, groundY});
    int gX = static_cast<int>(gScr.x);
    int gY = static_cast<int>(gScr.y);
    if (gX >= 0 && gX < fb.width() && gY >= 0 && gY < fb.height()) {
        for (int dy = -2; dy <= 2; ++dy) {
            int sw = (dy == 0) ? 7 : (std::abs(dy) == 1 ? 5 : 3);
            for (int dx = -sw; dx <= sw; ++dx) {
                fb.blendPixelFast(gX + dx, gY + dy, 0x38000000);
            }
        }
    }

    // Choose colour palette per coin type (kept as literals for now – can be moved to Palette later)
    uint32_t rimCol, bodyCol, hiCol, textCol;
    if (item.type == Physics::ItemType::COIN_GOLD) {
        rimCol = Palette::CoinGoldRim; bodyCol = Palette::CoinGoldBody; hiCol = Palette::CoinGoldHi; textCol = Palette::CoinGoldText;
    } else if (item.type == Physics::ItemType::COIN_SILVER) {
        rimCol = Palette::CoinSilverRim; bodyCol = Palette::CoinSilverBody; hiCol = Palette::CoinSilverHi; textCol = Palette::CoinSilverText;
    } else { // bronze
        rimCol = Palette::CoinBronzeRim; bodyCol = Palette::CoinBronzeBody; hiCol = Palette::CoinBronzeHi; textCol = Palette::CoinBronzeText;
    }

    // Draw the pseudo‑3D ellipse
    for (int dy = -ry; dy <= ry; ++dy) {
        float yFrac = static_cast<float>(dy) / ry;
        float xBound = std::sqrt(std::max(0.0f, 1.0f - yFrac * yFrac)) * rx;
        int minX = static_cast<int>(-xBound);
        int maxX = static_cast<int>(xBound);
        for (int dx = minX; dx <= maxX; ++dx) {
            int px = sx + dx;
            int py = bobY + dy;
            if (px < 0 || px >= fb.width() || py < 0 || py >= fb.height()) continue;
            bool isEdge = (dx == minX || dx == maxX || dy == -ry || dy == ry);
            if (isEdge) {
                fb.setPixelFast(px, py, rimCol);
            } else if (dx < 0 && dy < 0) {
                fb.setPixelFast(px, py, hiCol);
            } else {
                fb.setPixelFast(px, py, bodyCol);
            }
        }
    }

    // Currency symbol or edge flare
    if (rx >= 6) {
        Graphics::RasterFont::drawStringCentered(fb, sx, bobY - 3, "$", textCol, 1);
    } else {
        fb.drawLine(sx - 1, bobY - ry + 2, sx - 1, bobY + ry - 2, hiCol);
        fb.drawLine(sx + 1, bobY - ry + 2, sx + 1, bobY + ry - 2, rimCol);
    }

    // Gold sparkle (kept as original condition)
    if (item.type == Physics::ItemType::COIN_GOLD && (static_cast<int>(gameTime * 3.0f + item.position.x) % 5 == 0)) {
        fb.blendPixelFast(sx - rx / 2, bobY - ry / 2, 0xFFFFFFFF);
        fb.blendPixelFast(sx - rx / 2 + 1, bobY - ry / 2, 0x88FFFFFF);
        fb.blendPixelFast(sx - rx / 2 - 1, bobY - ry / 2, 0x88FFFFFF);
        fb.blendPixelFast(sx - rx / 2, bobY - ry / 2 + 1, 0x88FFFFFF);
        fb.blendPixelFast(sx - rx / 2, bobY - ry / 2 - 1, 0x88FFFFFF);
    }
}

} // namespace Graphics
