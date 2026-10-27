#include "ScanlineRasterizer.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <algorithm>

namespace Graphics {

ScanlineRasterizer::ScanlineRasterizer() {
}



void ScanlineRasterizer::renderItems(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime) {
    const auto& items = terrain.getItems();

    for (const auto& item : items) {
        if (item.collected) continue;

        Physics::Vec2 scr = cam.worldToScreen(item.position);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -35 || sx > fb.width() + 35 || sy < -35 || sy > fb.height() + 35) continue;

        // Gentle floating bob
        int bobY = sy + static_cast<int>(std::sin(gameTime * 4.2f + item.position.x * 0.8f) * 3.5f);

        if (item.type == Physics::ItemType::FUEL_CANISTER) {
            // Professional Pixel-Art Jerry Can (20 x 26 px)
            int cw = 20;
            int ch = 26;
            int cx = sx - cw / 2;
            int cy = bobY - ch / 2;

            // Ground shadow
            float groundY = terrain.getHeight(item.position.x);
            Physics::Vec2 gScr = cam.worldToScreen({item.position.x, groundY});
            if (gScr.x >= 0 && gScr.x < fb.width() && gScr.y >= 0 && gScr.y < fb.height()) {
                fb.fillCircle(static_cast<int>(gScr.x), static_cast<int>(gScr.y), 9, 0x44000000);
            }

            // Top handles (3-bar tubular carry handle)
            fb.fillRect(cx + 3, cy - 4, cw - 6, 4, 0xFFB71C1C);
            fb.fillRect(cx + 6, cy - 3, cw - 12, 3, 0x00000000); // Hollow grip cutout
            fb.drawRect(cx + 3, cy - 4, cw - 6, 4, 0xFF8E0000);

            // Angled filler spout on right shoulder
            fb.fillRect(cx + cw - 6, cy - 6, 5, 4, 0xFFCFD8DC);
            fb.drawRect(cx + cw - 6, cy - 6, 5, 4, 0xFF78909C);
            fb.fillRect(cx + cw - 5, cy - 7, 3, 2, 0xFFFFB300); // Brass fuel cap

            // Main Steel Canister Body
            fb.fillRect(cx, cy, cw, ch, 0xFFD32F2F); // Vibrant Jerry-can red
            fb.drawRect(cx, cy, cw, ch, 0xFF8E0000); // Dark pressed-steel border
            // Top-left metallic specular bevel
            fb.drawLine(cx + 1, cy + 1, cx + cw - 2, cy + 1, 0xFFFF5252);
            fb.drawLine(cx + 1, cy + 1, cx + 1, cy + ch - 2, 0xFFFF5252);

            // Stamped Recessed Stiffener 'X' indentation
            fb.drawLine(cx + 3, cy + 3, cx + cw - 4, cy + ch - 4, 0xFF8E0000);
            fb.drawLine(cx + 4, cy + 3, cx + cw - 3, cy + ch - 4, 0xFFEF5350);
            fb.drawLine(cx + cw - 4, cy + 3, cx + 3, cy + ch - 4, 0xFF8E0000);
            fb.drawLine(cx + cw - 3, cy + 3, cx + 4, cy + ch - 4, 0xFFEF5350);

            // High-Contrast White Jerry-Can Badge
            int badgeW = cw - 6;
            int badgeH = 9;
            int bx = cx + 3;
            int by = cy + (ch - badgeH) / 2;
            fb.fillRect(bx, by, badgeW, badgeH, 0xFFECEFF1);
            fb.drawRect(bx, by, badgeW, badgeH, 0xFFB0BEC5);
            Graphics::RasterFont::drawStringCentered(fb, sx, by + 1, "GAS", 0xFFD32F2F, 1);

            // Pulsing Ambient Beacon Halo
            float pulse = 0.5f + 0.5f * std::sin(gameTime * 5.0f + item.position.x);
            uint32_t haloA = static_cast<uint32_t>(pulse * 45.0f);
            fb.drawRect(cx - 1, cy - 1, cw + 2, ch + 2, (haloA << 24) | 0x00FF5252);
        } else {
            // Arcade Pseudo-3D Rotating Coin (Gold, Silver, Bronze)
            // Coin spins horizontally around vertical axis
            float spinAngle = gameTime * 4.5f + item.position.x * 0.7f;
            float cosSpin = std::cos(spinAngle);
            float absCos = std::abs(cosSpin);

            const int baseR = 10;
            // Half-width rx: clamped to minimum 4px to preserve coin disc character and avoid single-pixel needle artifacts
            int rx = std::max(4, static_cast<int>(absCos * baseR));
            int ry = baseR;

            // Ground shadow: realistic horizontal flattened oval disc beneath floating coin
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

            // Color palettes for 3 tiers
            uint32_t rimCol, bodyCol, hiCol, textCol;
            if (item.type == Physics::ItemType::COIN_GOLD) {
                rimCol = 0xFFB27D00;
                bodyCol = 0xFFFFD700;
                hiCol = 0xFFFFF59D;
                textCol = 0xFF3E2723;
            } else if (item.type == Physics::ItemType::COIN_SILVER) {
                rimCol = 0xFF607D8B;
                bodyCol = 0xFFCFD8DC;
                hiCol = 0xFFECEFF1;
                textCol = 0xFF263238;
            } else { // BRONZE
                rimCol = 0xFF795548;
                bodyCol = 0xFFCD7F32;
                hiCol = 0xFFD7CCC8;
                textCol = 0xFF3E2723;
            }

            // Draw Pseudo-3D Rotated Ellipse
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
                        fb.setPixelFast(px, py, hiCol); // Upper-left specular gleam
                    } else {
                        fb.setPixelFast(px, py, bodyCol);
                    }
                }
            }

            // Embossed Currency Symbol ($) or Edge-on Cylindrical Rim Flare
            if (rx >= 6) {
                Graphics::RasterFont::drawStringCentered(fb, sx, bobY - 3, "$", textCol, 1);
            } else {
                // Edge-on view: authentic metallic cylindrical rim flare and edge seam
                fb.drawLine(sx - 1, bobY - ry + 2, sx - 1, bobY + ry - 2, hiCol);
                fb.drawLine(sx + 1, bobY - ry + 2, sx + 1, bobY + ry - 2, rimCol);
            }

            // Specular diamond twinkle glint on Gold coins periodically
            if (item.type == Physics::ItemType::COIN_GOLD && (static_cast<int>(gameTime * 3.0f + item.position.x) % 5 == 0)) {
                fb.blendPixelFast(sx - rx / 2, bobY - ry / 2, 0xFFFFFFFF);
                fb.blendPixelFast(sx - rx / 2 + 1, bobY - ry / 2, 0x88FFFFFF);
                fb.blendPixelFast(sx - rx / 2 - 1, bobY - ry / 2, 0x88FFFFFF);
                fb.blendPixelFast(sx - rx / 2, bobY - ry / 2 + 1, 0x88FFFFFF);
                fb.blendPixelFast(sx - rx / 2, bobY - ry / 2 - 1, 0x88FFFFFF);
            }
        }
    }
}

void ScanlineRasterizer::render(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, float gameTime) {
    if (fb.width() <= 0 || fb.height() <= 0) return;
    const std::string& biomeId = terrain.getBiome();

    int W = fb.width();
    int H = fb.height();
    float camX = cam.x();
    float camY = cam.y();
    float zoom = cam.zoom();
    float invZoom = 1.0f / zoom;
    int stride = fb.stride();

    // 1. Precalculate Surface Profiles for All Columns
    static std::vector<int> s_surfaceY;
    static std::vector<float> s_worldX;
    static std::vector<Physics::TerrainDeformation> s_deforms;
    if (static_cast<int>(s_surfaceY.size()) != W) {
        s_surfaceY.resize(W);
        s_worldX.resize(W);
        s_deforms.resize(W);
    }

    for (int x = 0; x < W; ++x) {
        float wx = camX + (x - W * 0.5f) * invZoom;
        float wy = terrain.getHeight(wx);
        s_surfaceY[x] = static_cast<int>(H * 0.55f - (wy - camY) * zoom);
        s_worldX[x] = wx;
        s_deforms[x] = terrain.getDeformation(wx);
    }

    // 2. Parallax Sky, Celestial Bodies & Distant Mountains (Zero Overdraw: clips to s_surfaceY)
    m_envRenderer.renderSkyAndParallax(fb, cam, biomeId, m_dayNightCycle, s_surfaceY.data(), gameTime);

    // 3. Day-Night Lighting State for Terrain & Scenery
    LightingState lighting = m_dayNightCycle.getLighting(biomeId, W, H, camX);

    // 4. Vertical Column-Scanline Terrain Filling
    const auto& spec = terrain.getBiomeSpec();
    uint32_t crestCol    = spec.crestColor;
    uint32_t grassTopCol = spec.topColor;
    uint32_t grassBodyCol = spec.bodyColor;
    uint32_t dirtBodyCol = spec.soilColor;
    uint32_t bedrockCol  = spec.bedrockColor;

    // Modulate body and strata layers by ambient lighting once outside column loop
    uint32_t colLoamDark = DayNightCycle::applyAmbient(UI::Theme::darken(dirtBodyCol, 16), lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colLoamLight = DayNightCycle::applyAmbient(UI::Theme::lighten(dirtBodyCol, 16), lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colLoamMid = DayNightCycle::applyAmbient(dirtBodyCol, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colBedrockLight = DayNightCycle::applyAmbient(UI::Theme::lighten(bedrockCol, 22), lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colBedrock = DayNightCycle::applyAmbient(bedrockCol, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t colLoamCap = DayNightCycle::applyAmbient(UI::Theme::darken(dirtBodyCol, 12), lighting.ambientColor, lighting.ambientBrightness);

    for (int x = 0; x < W; ++x) {
        int surfaceScreenY = s_surfaceY[x];
        int startY = std::max(0, surfaceScreenY);
        if (startY >= H) continue;

        float worldX = s_worldX[x];
        const auto& deform = s_deforms[x];

        // Biome base colors and layer thicknesses
        uint32_t colCrest = crestCol;
        uint32_t colTop = grassTopCol;
        uint32_t colBody = grassBodyCol;
        int topThickness = 5;
        int bodyThickness = 14;

        if (biomeId == "countryside") {
            if (deform.grassFlattening > 0.15f || deform.grassTear > 0.15f) {
                // Grass trampled & bent by rolling tyres: crest consistently remains lush green
                colCrest = 0xFF43A047; // Rich compressed green crest
                colTop   = 0xFF388E3C; // Compressed green sub-layer
                topThickness = 3;      // Compressed layer thickness
                bodyThickness = 10;

                // Localized tyre interaction: subtle exposed fertile soil peeking through only under heavy slip/tear
                if (deform.grassTear > 0.45f) {
                    int worldCol = static_cast<int>(std::floor(std::abs(worldX) * 12.0f));
                    if (((worldCol % 5) == 0) || (((worldCol + 2) % 11) == 0)) {
                        colTop = 0xFF4E342E; // Small patch of exposed dark fertile soil beneath blades
                        topThickness = 2;
                    }
                }
            }
        } else if (biomeId == "desert") {
            if (deform.compression > 0.008f) {
                // Deep shadowed sand tire ruts / tracks
                colCrest = 0xFFC29B38; // Shadowed furrow
                colTop   = 0xFFB28728;
            }
        } else if (biomeId == "arctic") {
            if (deform.compression > 0.005f) {
                // Hard-packed icy blue frosted ruts
                colCrest = 0xFFB0BEC5;
                colTop   = 0xFF90A4AE;
            }
        } else if (biomeId == "mountain") {
            if (deform.compression > 0.008f) {
                // Displaced scree and gravel scuffs
                colCrest = 0xFF37474F;
                colTop   = 0xFF263238;
            }
        } else if (biomeId == "volcano") {
            if (deform.compression > 0.008f) {
                // Disturbed ash reveals basalt bedrock & warm cinders
                colCrest = 0xFF212121;
                colTop   = ((x & 7) == 0) ? 0xFFD84315 : 0xFF303030;
            }
        } else if (biomeId == "moon") {
            if (deform.compression > 0.008f) {
                // Dark compacted lunar regolith tracks
                colCrest = 0xFF424242;
                colTop   = 0xFF303030;
            }
        }

        // Modulate column-specific crest, top and body with ambient lighting
        colCrest = DayNightCycle::applyAmbient(colCrest, lighting.ambientColor, lighting.ambientBrightness);
        colTop   = DayNightCycle::applyAmbient(colTop, lighting.ambientColor, lighting.ambientBrightness);
        colBody  = DayNightCycle::applyAmbient(colBody, lighting.ambientColor, lighting.ambientBrightness);

        // World-space coordinate for consistent scrolling geological strata
        int wx = static_cast<int>(std::floor(std::abs(worldX) * 16.0f));

        uint32_t* pPixel = fb.scanLine(startY) + x;
        for (int y = startY; y < H; ++y) {
            int depth = y - surfaceScreenY;
            uint32_t pixelCol;

            if (depth == 0) {
                pixelCol = colCrest;
            } else if (depth < topThickness) {
                pixelCol = colTop;
            } else if (depth < bodyThickness) {
                pixelCol = colBody;
            } else if (depth < 24) {
                pixelCol = colLoamCap; // Biome-specific intermediate transitional layer
            } else if (depth < 85) {
                // Procedural gravel / mineral specks keyed to world coordinates
                int loamNoise = ((wx * 7 + depth * 13) ^ (wx * 3)) & 15;
                if (biomeId == "volcano" && ((wx * 19 + depth * 23) ^ (wx * 7)) % 43 == 0) {
                    pixelCol = DayNightCycle::applyAmbient(0xFFFF5722, lighting.ambientColor, 1.0f); // Glowing magma vein
                } else {
                    pixelCol = (loamNoise == 0) ? colLoamDark :
                               (loamNoise == 3) ? colLoamLight : colLoamMid;
                }
            } else {
                // Subterranean bedrock with stratified stone fissures keyed to world coordinates
                int bedrockNoise = ((wx * 11 + depth * 7) ^ (wx >> 2)) & 31;
                if (biomeId == "volcano" && ((wx * 29 + depth * 17) ^ (wx * 13)) % 59 == 0) {
                    pixelCol = DayNightCycle::applyAmbient(0xFFFF3D00, lighting.ambientColor, 1.0f); // Deep magma fissure
                } else {
                    pixelCol = (bedrockNoise == 0) ? colBedrockLight : colBedrock;
                }
            }

            *pPixel = pixelCol;
            pPixel += stride;
        }

        // Grass blade tufts & wildflowers on countryside ridge
        if (biomeId == "countryside" && surfaceScreenY > 3 && surfaceScreenY < H) {
            if (deform.grassFlattening < 0.15f && deform.grassTear < 0.15f) {
                // Pristine grass: upright tufts & wildflowers
                if (x % 6 == 0) {
                    fb.setPixelFast(x, surfaceScreenY - 1, DayNightCycle::applyAmbient(0xFF66BB6A, lighting.ambientColor, lighting.ambientBrightness));
                    fb.setPixelFast(x + 1, surfaceScreenY - 2, DayNightCycle::applyAmbient(0xFF81C784, lighting.ambientColor, lighting.ambientBrightness));
                }
                // Scattered wildflowers: yellow dandelions & white clovers
                if (x % 17 == 0) {
                    fb.setPixelFast(x, surfaceScreenY - 1, DayNightCycle::applyAmbient(0xFFFFEE58, lighting.ambientColor, lighting.ambientBrightness));
                } else if (x % 31 == 0) {
                    fb.setPixelFast(x, surfaceScreenY - 1, DayNightCycle::applyAmbient(0xFFFFFFFF, lighting.ambientColor, lighting.ambientBrightness));
                }
            } else {
                // Bent grass: flattened green blades along the ground
                if (x % 4 == 0) {
                    fb.setPixelFast(x, surfaceScreenY - 1, DayNightCycle::applyAmbient(0xFF388E3C, lighting.ambientColor, lighting.ambientBrightness));
                }
                // Subtle localized tyre interaction: tiny dislodged grass flecks
                if (deform.grassTear > 0.45f && (x % 9 == 0)) {
                    fb.setPixelFast(x, surfaceScreenY - 1, DayNightCycle::applyAmbient(0xFF558B2F, lighting.ambientColor, lighting.ambientBrightness));
                }
            }
        }
    }

    // 5. Foreground Scenery (Trees, Cacti, Boulders, Fences, Barns, Windmills, Landmarks)
    m_envRenderer.renderForegroundScenery(fb, cam, terrain, m_dayNightCycle, gameTime);

    // 6. Distance Milestone Signposts planted in the terrain
    renderMilestones(fb, cam, terrain, lighting);

    // 7. Collectibles (Coins & Fuel)
    renderItems(fb, cam, terrain, gameTime);

    // 8. Atmospheric Weather Particles (Snowflakes, Fireflies, Dust, Embers)
    m_envRenderer.renderAtmosphericParticles(fb, cam, biomeId, m_dayNightCycle, s_surfaceY.data(), gameTime);
}

void ScanlineRasterizer::renderMilestones(Framebuffer& fb, const Camera& cam, const Physics::Terrain& terrain, const LightingState& lighting) {
    const auto& milestones = terrain.getMilestones();
    int W = fb.width();
    int H = fb.height();

    uint32_t postCol = DayNightCycle::applyAmbient(0xFF4E342E, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t postHiCol = DayNightCycle::applyAmbient(0xFF795548, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t boardBaseCol = DayNightCycle::applyAmbient(0xFF6D4C41, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t boardFaceCol = DayNightCycle::applyAmbient(0xFF8D6E63, lighting.ambientColor, lighting.ambientBrightness);
    uint32_t boardBorderCol = DayNightCycle::applyAmbient(0xFF3E2723, lighting.ambientColor, lighting.ambientBrightness);

    for (const auto& ms : milestones) {
        Physics::Vec2 scr = cam.worldToScreen(ms.position);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -80 || sx > W + 80 || sy < -120 || sy > H + 120) continue;

        if (ms.isCheckpoint) {
            // Grand Overhead Checkpoint Archway: clearance for vehicle to pass comfortably under
            int archH = 145;
            int archW = 240;
            int p1x = sx - archW / 2;
            int p2x = sx + archW / 2 - 10;
            int topY = sy - archH;

            // Two Heavy Steel Lattice Gantry Towers
            auto drawTower = [&](int tx) {
                fb.fillRect(tx, topY, 10, archH, 0xFF37474F);
                fb.drawRect(tx, topY, 10, archH, 0xFF263238);
                // Heavy truss cross braces
                for (int b = topY + 6; b < sy - 12; b += 18) {
                    fb.drawLine(tx, b, tx + 9, b + 14, 0xFF78909C);
                    fb.drawLine(tx + 9, b, tx, b + 14, 0xFF78909C);
                    fb.drawLine(tx, b, tx + 9, b, 0xFF90A4AE); // Horizontal tie
                }
            };
            drawTower(p1x);
            drawTower(p2x);

            // Grand Overhead Banner Box
            int bannerW = archW + 24;
            int bannerH = 34;
            int bx = sx - bannerW / 2;
            int by = topY - 18;

            uint32_t bannerBg = ms.isFinishLine ? 0xFFC62828 : 0xFF1565C0;
            uint32_t borderCol = ms.isFinishLine ? UI::Theme::GOLD : 0xFF00E5FF;
            fb.fillRect(bx, by, bannerW, bannerH, bannerBg);
            fb.drawRect(bx, by, bannerW, bannerH, borderCol);
            fb.drawRect(bx + 1, by + 1, bannerW - 2, bannerH - 2, 0xFF0D47A1);

            // Checkered border strip on banner (top and bottom)
            for (int cx = bx + 2; cx < bx + bannerW - 2; cx += 8) {
                fb.fillRect(cx, by + 1, 4, 4, 0xFFFFFFFF);
                fb.fillRect(cx + 4, by + 1, 4, 4, 0xFF212121);
                fb.fillRect(cx, by + bannerH - 5, 4, 4, 0xFF212121);
                fb.fillRect(cx + 4, by + bannerH - 5, 4, 4, 0xFFFFFFFF);
            }

            if (ms.isFinishLine) {
                Graphics::RasterFont::drawStringCentered(fb, sx + 1, by + 10, "FINISH LINE", 0xFF05080E, 2);
                Graphics::RasterFont::drawStringCentered(fb, sx, by + 9, "FINISH LINE", UI::Theme::GOLD, 2);
            } else {
                std::string title = "CHECKPOINT " + std::to_string(ms.checkpointIndex);
                Graphics::RasterFont::drawStringCentered(fb, sx + 1, by + 10, title, 0xFF05080E, 2);
                Graphics::RasterFont::drawStringCentered(fb, sx, by + 9, title, 0xFFFFFFFF, 2);
            }

            // Subtitle banner under overhead beam
            std::string subStr = ms.checkpointName + " - " + std::to_string(ms.distanceMeters) + "m";
            int subW = Graphics::RasterFont::getTextWidth(subStr, 1) + 16;
            int subX = sx - subW / 2;
            int subY = by + bannerH + 4;
            fb.fillRect(subX, subY, subW, 14, 0xEE0B1422);
            fb.drawRect(subX, subY, subW, 14, UI::Theme::CARD_BORDER);
            Graphics::RasterFont::drawStringCentered(fb, sx, subY + 3, subStr, UI::Theme::GOLD, 1);

            // Dual Checkered Racing Flags atop towers
            auto drawFlag = [&](int fx, int fy) {
                fb.drawLine(fx, fy + 22, fx, fy, 0xFFCFD8DC);
                fb.fillCircle(fx, fy - 1, 2, UI::Theme::GOLD);
                for (int x = 0; x < 14; ++x) {
                    for (int y = 0; y < 10; ++y) {
                        bool white = ((x / 3 + y / 3) % 2 == 0);
                        fb.setPixelFast(fx + 1 + x, fy + y, white ? 0xFFFFFFFF : 0xFF1A1A1A);
                    }
                }
            };
            drawFlag(p1x - 3, topY - 32);
            drawFlag(p2x + 11, topY - 32);
            continue;
        }

        // 1. Two Wooden Support Stakes Planted in the Ground
        int postW = 5;
        int postH = 55;
        int p1x = sx - 22;
        int p2x = sx + 18;
        int postTopY = sy - postH;

        fb.fillRect(p1x, postTopY, postW, postH, postCol);
        fb.fillRect(p1x + 1, postTopY, 1, postH, postHiCol); // Wood grain highlight
        fb.fillRect(p2x, postTopY, postW, postH, postCol);
        fb.fillRect(p2x + 1, postTopY, 1, postH, postHiCol);

        // 2. Wooden Signboard Plank
        int boardW = 68;
        int boardH = 28;
        int bx = sx - boardW / 2;
        int by = postTopY - 4;

        fb.fillRect(bx, by, boardW, boardH, boardBaseCol);
        fb.fillRect(bx + 2, by + 2, boardW - 4, boardH - 4, boardFaceCol); // Rich oak wood face
        fb.drawRect(bx, by, boardW, boardH, boardBorderCol); // Dark carved wood border

        // Horizontal wood grain lines
        fb.drawLine(bx + 4, by + 8, bx + boardW - 5, by + 8, postHiCol);
        fb.drawLine(bx + 4, by + 19, bx + boardW - 5, by + 19, postHiCol);

        // 4 Corner Iron Nails
        fb.setPixelFast(bx + 3, by + 3, 0xFF212121);
        fb.setPixelFast(bx + boardW - 4, by + 3, 0xFF212121);
        fb.setPixelFast(bx + 3, by + boardH - 4, 0xFF212121);
        fb.setPixelFast(bx + boardW - 4, by + boardH - 4, 0xFF212121);

        // Distance Label (e.g., "100m") with drop shadow
        std::string distStr = std::to_string(ms.distanceMeters) + "m";
        Graphics::RasterFont::drawStringCentered(fb, sx + 1, by + 8, distStr, 0xFF212121, 2);
        Graphics::RasterFont::drawStringCentered(fb, sx, by + 7, distStr, 0xFFFFF9C4, 2);

        // 3. Checkered Pennant / Flag on Top of the Signboard
        int flagPoleX = sx;
        int flagPoleTop = by - 20;
        fb.drawLine(flagPoleX, by, flagPoleX, flagPoleTop, 0xFFB0BEC5);
        fb.fillCircle(flagPoleX, flagPoleTop - 1, 2, UI::Theme::GOLD);

        // Checkered Racing Flag waving from the pole
        for (int fx = 0; fx < 12; ++fx) {
            for (int fy = 0; fy < 8; ++fy) {
                bool isWhite = ((fx / 3 + fy / 2) % 2 == 0);
                fb.setPixelFast(flagPoleX + 1 + fx, flagPoleTop + fy, isWhite ? 0xFFFFFFFF : 0xFF212121);
            }
        }
    }
}

} // namespace Graphics
