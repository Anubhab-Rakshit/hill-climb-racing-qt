#include "UIComponents.h"
#include "RasterFont.h"
#include "UITheme.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace UI {

std::string UIComponents::formatNumber(int64_t val) {
    if (val == 0) return "0";
    bool neg = (val < 0);
    uint64_t num = neg ? static_cast<uint64_t>(-val) : static_cast<uint64_t>(val);

    std::string s = std::to_string(num);
    int insertPos = static_cast<int>(s.length()) - 3;
    while (insertPos > 0) {
        s.insert(static_cast<size_t>(insertPos), ",");
        insertPos -= 3;
    }
    return neg ? ("-" + s) : s;
}

void UIComponents::drawArcadePanel(Graphics::Framebuffer& fb,
                                   int x, int y, int w, int h,
                                   uint32_t bgCol,
                                   uint32_t borderCol,
                                   uint32_t accentCol,
                                   int accentH,
                                   bool drawBevel,
                                   bool drawNotches)
{
    if (w <= 4 || h <= 4) return;

    // 1. Soft Outer Drop Shadow
    fb.fillRect(x + 2, y + 2, w, h, 0x8805080E);

    // 2. Main Outer Border
    fb.drawRect(x, y, w, h, borderCol);

    // 3. Inner Fill
    fb.fillRect(x + 1, y + 1, w - 2, h - 2, bgCol);

    // 4. Corner Notches (Diagonal cut on 4 corners for arcade hardware look)
    if (drawNotches) {
        uint32_t shadowCol = 0xAA040810;
        fb.setPixelFast(x, y, shadowCol);
        fb.setPixelFast(x + w - 1, y, shadowCol);
        fb.setPixelFast(x, y + h - 1, shadowCol);
        fb.setPixelFast(x + w - 1, y + h - 1, shadowCol);

        fb.setPixelFast(x + 1, y + 1, borderCol);
        fb.setPixelFast(x + w - 2, y + 1, borderCol);
        fb.setPixelFast(x + 1, y + h - 2, borderCol);
        fb.setPixelFast(x + w - 2, y + h - 2, borderCol);
    }

    // 5. 3D Bevel Rim
    if (drawBevel) {
        uint32_t lightBevel = Theme::lighten(borderCol, 45);
        uint32_t darkBevel = Theme::darken(borderCol, 40);

        // Top and Left light bevel
        fb.fillRect(x + 2, y + 1, w - 4, 1, lightBevel);
        fb.fillRect(x + 1, y + 2, 1, h - 4, lightBevel);

        // Bottom and Right dark shadow bevel
        fb.fillRect(x + 2, y + h - 2, w - 4, 1, darkBevel);
        fb.fillRect(x + w - 2, y + 2, 1, h - 4, darkBevel);
    }

    // 6. Top Accent Stripe
    if (accentCol > 0 && accentH > 0) {
        int strH = std::min(accentH, h - 4);
        fb.fillRect(x + 2, y + 2, w - 4, strH, accentCol);
        // Gloss highlight specular line
        fb.fillRect(x + 4, y + 3, w - 8, 1, 0x66FFFFFF);
        // Dark accent divider bottom line
        fb.fillRect(x + 2, y + 2 + strH, w - 4, 1, Theme::darken(accentCol, 50));
    }

    // 7. Subtle Inner Border
    fb.drawRect(x + 2, y + 2, w - 4, h - 4, 0x22FFFFFF);
}

void UIComponents::drawCoinBadge(Graphics::Framebuffer& fb,
                                 int x, int y, int w, int h,
                                 int coins,
                                 float animTime)
{
    // Outer Arcade Panel Container
    drawArcadePanel(fb, x, y, w, h, 0xEE0B121C, 0xFF2A3A52, 0, 0, true, true);

    // 3D Spinning / Pulsing Golden Coin
    int coinX = x + 16;
    int coinY = y + h / 2;
    int maxCoinW = 8;
    float spinFactor = std::abs(std::cos(animTime * 3.5f));
    int coinW = static_cast<int>(spinFactor * maxCoinW) + 2;

    // Coin outer edge
    fb.fillRect(coinX - coinW, coinY - 8, coinW * 2 + 1, 17, 0xFFE65100);
    // Coin rim
    fb.fillRect(coinX - coinW + 1, coinY - 7, std::max(1, coinW * 2 - 1), 15, Theme::GOLD);
    // Coin core
    if (coinW >= 4) {
        fb.fillRect(coinX - coinW + 2, coinY - 6, coinW * 2 - 3, 13, 0xFFFFD54F);
        // Specular glint
        fb.fillRect(coinX - coinW + 2, coinY - 5, std::max(1, coinW - 2), 2, 0xFFFFF9C4);
        // Dollar sign engraving
        Graphics::RasterFont::drawStringCentered(fb, coinX, coinY - 3, "$", 0xFF5D4037, 1);
    }

    // Formatted Currency Text
    std::string valStr = formatNumber(coins);
    int textY = y + (h - Graphics::RasterFont::getTextHeight(2)) / 2;
    // Drop shadow
    Graphics::RasterFont::drawString(fb, x + 33, textY + 1, valStr, 0xFF05080E, 2);
    // Main gold text
    Graphics::RasterFont::drawString(fb, x + 32, textY, valStr, Theme::GOLD, 2);
}

void UIComponents::drawKeycap(Graphics::Framebuffer& fb,
                              int x, int y,
                              const std::string& keyText,
                              bool pressed)
{
    int fontScale = 1;
    int textW = Graphics::RasterFont::getTextWidth(keyText, fontScale);
    int w = std::max(16, textW + 8);
    int h = 16;

    int dy = pressed ? 1 : 0;
    uint32_t capBg = pressed ? 0xFF192534 : 0xFF2C3E50;
    uint32_t capBorder = pressed ? 0xFF0F1822 : 0xFF1A2530;
    uint32_t capLight = pressed ? 0xFF233244 : 0xFF4A627A;
    uint32_t capShadow = 0xFF0B1017;

    // Key base shadow
    fb.fillRect(x, y + 2, w, h - 2, capShadow);

    // Key cap body
    fb.fillRect(x + 1, y + dy, w - 2, h - 3, capBg);
    fb.drawRect(x, y + dy, w, h - 3, capBorder);

    // Bevels
    if (!pressed) {
        fb.fillRect(x + 1, y, w - 2, 1, capLight);
        fb.fillRect(x + 1, y, 1, h - 4, capLight);
    }

    // Centered Key Text
    int cx = x + w / 2;
    int cy = y + dy + (h - 3 - Graphics::RasterFont::getTextHeight(fontScale)) / 2;
    Graphics::RasterFont::drawStringCentered(fb, cx + 1, cy + 1, keyText, 0xFF000000, fontScale);
    Graphics::RasterFont::drawStringCentered(fb, cx, cy, keyText, pressed ? Theme::CYAN_UPGRADE : Theme::TEXT_WHITE, fontScale);
}

void UIComponents::drawSegmentedBar(Graphics::Framebuffer& fb,
                                    int x, int y, int w, int h,
                                    int currentVal, int maxVal,
                                    uint32_t activeCol,
                                    uint32_t inactiveCol,
                                    uint32_t hiCol)
{
    if (maxVal <= 0 || w <= 0 || h <= 0) return;

    int gap = 2;
    int segW = (w - (maxVal - 1) * gap) / maxVal;
    if (segW < 1) segW = 1;

    for (int i = 0; i < maxVal; ++i) {
        int sx = x + i * (segW + gap);
        if (i < currentVal) {
            fb.fillRect(sx, y, segW, h, activeCol);
            if (hiCol > 0 && h > 3) {
                fb.fillRect(sx, y, segW, 1, hiCol); // LED surface highlight
            }
        } else {
            fb.fillRect(sx, y, segW, h, inactiveCol);
        }
        // Pip border
        fb.drawRect(sx, y, segW, h, 0xFF0B121C);
    }
}

void UIComponents::drawStar(Graphics::Framebuffer& fb,
                            int cx, int cy, int radius,
                            bool filled,
                            uint32_t color,
                            uint32_t outlineColor)
{
    // Draw crisp symmetrical 5-point pixel star
    if (radius <= 4) {
        // Small 7x7 star pattern
        static const int star7[7][7] = {
            {0, 0, 0, 1, 0, 0, 0},
            {0, 0, 1, 1, 1, 0, 0},
            {1, 1, 1, 1, 1, 1, 1},
            {0, 1, 1, 1, 1, 1, 0},
            {0, 0, 1, 1, 1, 0, 0},
            {0, 1, 1, 0, 1, 1, 0},
            {1, 1, 0, 0, 0, 1, 1}
        };
        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 7; ++c) {
                if (star7[r][c]) {
                    fb.setPixelFast(cx - 3 + c, cy - 3 + r, filled ? color : outlineColor);
                }
            }
        }
        return;
    }

    // Medium 11x11 star pattern for larger rating stars
    static const int star11[11][11] = {
        {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
        {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
        {0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0},
        {0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0},
        {1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
        {1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1}
    };

    for (int r = 0; r < 11; ++r) {
        for (int c = 0; c < 11; ++c) {
            if (star11[r][c]) {
                if (!filled) {
                    // Draw outline only
                    bool isBorder = (r == 0 || r == 10 || c == 0 || c == 10 ||
                                     (r > 0 && !star11[r-1][c]) || (r < 10 && !star11[r+1][c]) ||
                                     (c > 0 && !star11[r][c-1]) || (c < 10 && !star11[r][c+1]));
                    if (isBorder) {
                        fb.setPixelFast(cx - 5 + c, cy - 5 + r, outlineColor);
                    }
                } else {
                    fb.setPixelFast(cx - 5 + c, cy - 5 + r, color);
                }
            }
        }
    }
}

void UIComponents::drawMedalBadge(Graphics::Framebuffer& fb,
                                  int cx, int cy,
                                  int tier,
                                  float animTime)
{
    // tier: 0 = Rookie, 1 = Bronze, 2 = Silver, 3 = Gold, 4 = Platinum
    uint32_t ribbonCol = (tier >= 3) ? 0xFFD32F2F : (tier == 2) ? 0xFF1976D2 : (tier == 1) ? 0xFF00796B : 0xFF455A64;
    uint32_t medalCol;
    uint32_t rimCol;
    uint32_t shineCol;

    switch (tier) {
        case 4: // Platinum Champion
            medalCol = 0xFF00E5FF;
            rimCol   = 0xFF18FFFF;
            shineCol = 0xFFE0F7FA;
            break;
        case 3: // Gold
            medalCol = 0xFFFFD700;
            rimCol   = 0xFFFFA000;
            shineCol = 0xFFFFF9C4;
            break;
        case 2: // Silver
            medalCol = 0xFFCFD8DC;
            rimCol   = 0xFF90A4AE;
            shineCol = 0xFFECEFF1;
            break;
        case 1: // Bronze
            medalCol = 0xFFCD7F32;
            rimCol   = 0xFF8D6E63;
            shineCol = 0xFFFFCC80;
            break;
        default: // Rookie
            medalCol = 0xFF78909C;
            rimCol   = 0xFF455A64;
            shineCol = 0xFFB0BEC5;
            break;
    }

    // 1. Hanging Ribbon
    int ribW = 8;
    int ribH = 14;
    // Left ribbon fold
    fb.fillRect(cx - 10, cy - 20, ribW, ribH, ribbonCol);
    fb.fillRect(cx - 10, cy - 20, 2, ribH, Theme::lighten(ribbonCol, 30));
    // Right ribbon fold
    fb.fillRect(cx + 2, cy - 20, ribW, ribH, Theme::darken(ribbonCol, 20));
    fb.fillRect(cx + 8, cy - 20, 2, ribH, Theme::lighten(ribbonCol, 30));

    // 2. Medallion Body (Radius = 14)
    int r = 14;
    fb.fillCircle(cx, cy, r + 2, 0xFF05080E); // Drop shadow / border
    fb.fillCircle(cx, cy, r, rimCol);
    fb.fillCircle(cx, cy, r - 2, medalCol);

    // Inner embossed circle
    fb.drawCircle(cx, cy, r - 4, Theme::darken(medalCol, 40));

    // Specular Shine Sweep across medal
    float glintPhase = std::fmod(animTime * 1.5f, 3.0f);
    if (glintPhase < 1.0f) {
        int gx = static_cast<int>(cx - r + glintPhase * 2.0f * r);
        for (int gy = cy - r + 3; gy <= cy + r - 3; ++gy) {
            int dx = gx - cx;
            int dy = gy - cy;
            if (dx * dx + dy * dy < (r - 2) * (r - 2)) {
                fb.setPixelFast(gx, gy, shineCol);
                fb.setPixelFast(gx + 1, gy, shineCol);
            }
        }
    }

    // 3. Center Emblem
    if (tier >= 3) {
        // Gold / Platinum 5-point star in center
        drawStar(fb, cx, cy, 5, true, (tier == 4) ? 0xFFFFFFFF : 0xFFFFF59D, rimCol);
    } else if (tier == 2) {
        // Silver Laurel / Roman numeral II
        Graphics::RasterFont::drawStringCentered(fb, cx, cy - 3, "II", 0xFF37474F, 1);
    } else if (tier == 1) {
        // Bronze numeral I
        Graphics::RasterFont::drawStringCentered(fb, cx, cy - 3, "I", 0xFF3E2723, 1);
    } else {
        // Rookie dot
        fb.fillCircle(cx, cy, 3, 0xFF37474F);
    }
}

void UIComponents::drawUpgradeIcon(Graphics::Framebuffer& fb,
                                   int x, int y,
                                   Core::ProfileManager::UpgradeType type)
{
    // Draw 18x18 pixel art icons with dark background pod
    fb.fillRect(x, y, 18, 18, 0xDD0D1624);
    fb.drawRect(x, y, 18, 18, 0xFF2A3A4E);

    switch (type) {
        case Core::ProfileManager::UPGRADE_ENGINE: {
            // V8 Engine Block & Piston
            // Cylinder block
            fb.fillRect(x + 3, y + 5, 12, 9, 0xFFE0E0E0);
            fb.fillRect(x + 5, y + 3, 8, 3, 0xFFFFB300); // Valve cover
            fb.fillRect(x + 6, y + 10, 6, 5, 0xFF424242); // Crankcase
            // Piston head spark
            fb.setPixelFast(x + 8, y + 2, 0xFFFF3D00);
            fb.setPixelFast(x + 9, y + 2, 0xFFFFD54F);
            break;
        }
        case Core::ProfileManager::UPGRADE_SUSPENSION: {
            // Coil Spring & Shock Damper
            fb.fillRect(x + 8, y + 2, 2, 14, 0xFF90A4AE); // Damper shaft
            // Coils zig-zag
            fb.fillRect(x + 4, y + 4, 10, 2, 0xFFFFB300);
            fb.fillRect(x + 4, y + 8, 10, 2, 0xFFFFB300);
            fb.fillRect(x + 4, y + 12, 10, 2, 0xFFFFB300);
            // Mount rings
            fb.drawCircle(x + 9, y + 3, 2, 0xFFCFD8DC);
            fb.drawCircle(x + 9, y + 15, 2, 0xFFCFD8DC);
            break;
        }
        case Core::ProfileManager::UPGRADE_TIRES: {
            // Chunky Mud-Terrain Radial Tire Profile
            fb.drawCircle(x + 9, y + 9, 7, 0xFF212121);
            fb.fillCircle(x + 9, y + 9, 6, 0xFF424242);
            fb.fillCircle(x + 9, y + 9, 3, 0xFFB0BEC5); // Chrome wheel rim
            // Radial tire tread blocks
            fb.fillRect(x + 8, y + 1, 2, 2, 0xFF212121);
            fb.fillRect(x + 8, y + 15, 2, 2, 0xFF212121);
            fb.fillRect(x + 1, y + 8, 2, 2, 0xFF212121);
            fb.fillRect(x + 15, y + 8, 2, 2, 0xFF212121);
            break;
        }
        case Core::ProfileManager::UPGRADE_4WD: {
            // 4WD Transfer Case & Driveshafts
            // 4 Wheels
            fb.fillRect(x + 2, y + 2, 3, 5, 0xFF424242);
            fb.fillRect(x + 13, y + 2, 3, 5, 0xFF424242);
            fb.fillRect(x + 2, y + 11, 3, 5, 0xFF424242);
            fb.fillRect(x + 13, y + 11, 3, 5, 0xFF424242);
            // Driveshafts
            fb.fillRect(x + 8, y + 4, 2, 10, 0xFF00E5FF);
            fb.fillRect(x + 4, y + 4, 10, 2, 0xFF00E5FF);
            fb.fillRect(x + 4, y + 12, 10, 2, 0xFF00E5FF);
            // Center differential
            fb.fillCircle(x + 9, y + 9, 2, 0xFFFFB300);
            break;
        }
        case Core::ProfileManager::UPGRADE_BRAKES: {
            // Ventilated Brake Disc & Red Caliper
            fb.drawCircle(x + 9, y + 9, 6, 0xFFCFD8DC);
            fb.fillCircle(x + 9, y + 9, 2, 0xFF455A64);
            // Rotor drill holes
            fb.setPixelFast(x + 9, y + 5, 0xFF263238);
            fb.setPixelFast(x + 9, y + 13, 0xFF263238);
            fb.setPixelFast(x + 5, y + 9, 0xFF263238);
            // Brembo-style red racing caliper
            fb.fillRect(x + 11, y + 4, 5, 6, 0xFFFF1744);
            fb.fillRect(x + 12, y + 5, 3, 4, 0xFFFF5252);
            break;
        }
        case Core::ProfileManager::UPGRADE_TRANSMISSION: {
            // Gearbox Mesh Teeth
            fb.drawCircle(x + 7, y + 7, 4, 0xFFFFB300);
            fb.drawCircle(x + 12, y + 11, 3, 0xFF00E5FF);
            // Gear teeth
            fb.fillRect(x + 6, y + 1, 2, 2, 0xFFFFB300);
            fb.fillRect(x + 6, y + 11, 2, 2, 0xFFFFB300);
            fb.fillRect(x + 1, y + 6, 2, 2, 0xFFFFB300);
            fb.fillRect(x + 14, y + 11, 2, 2, 0xFF00E5FF);
            break;
        }
        case Core::ProfileManager::UPGRADE_CHASSIS: {
            // Lightweight Tubular Spaceframe / Rollcage
            fb.drawLine(x + 3, y + 14, x + 15, y + 14, 0xFF00E676); // Floor
            fb.drawLine(x + 4, y + 14, x + 8, y + 5, 0xFF00E676);   // A-pillar
            fb.drawLine(x + 14, y + 14, x + 12, y + 5, 0xFF00E676); // C-pillar
            fb.drawLine(x + 8, y + 5, x + 12, y + 5, 0xFF00E676);   // Roof
            fb.drawLine(x + 8, y + 5, x + 14, y + 14, 0xFF69F0AE);  // Cross brace
            break;
        }
        case Core::ProfileManager::UPGRADE_DOWNFORCE: {
            // Aerodynamic Airfoil Wing & Downforce Vector
            fb.fillRect(x + 2, y + 4, 14, 3, 0xFFE040FB); // Carbon wing blade
            fb.fillRect(x + 5, y + 7, 2, 6, 0xFF9E9E9E);  // Left upright
            fb.fillRect(x + 11, y + 7, 2, 6, 0xFF9E9E9E); // Right upright
            // Downward aero force arrow
            fb.drawLine(x + 9, y + 1, x + 9, y + 3, 0xFF00E5FF);
            fb.setPixelFast(x + 8, y + 2, 0xFF00E5FF);
            fb.setPixelFast(x + 10, y + 2, 0xFF00E5FF);
            break;
        }
        default:
            break;
    }
}

void UIComponents::drawBiomeIcon(Graphics::Framebuffer& fb,
                                 int x, int y,
                                 int stageIdx)
{
    fb.fillRect(x, y, 16, 16, 0xEE0B121C);
    fb.drawRect(x, y, 16, 16, 0xFF2A3A4E);

    switch (stageIdx) {
        case 0: // Countryside (Green Hill + Flower)
            fb.fillCircle(x + 8, y + 14, 6, 0xFF4CAF50);
            fb.fillCircle(x + 8, y + 5, 2, 0xFFFFEB3B);
            break;
        case 1: // Desert (Pyramid Dune + Sun)
            fb.fillCircle(x + 12, y + 4, 3, 0xFFFFB300);
            fb.drawLine(x + 2, y + 14, x + 8, y + 7, 0xFFFF9800);
            fb.drawLine(x + 8, y + 7, x + 14, y + 14, 0xFFF57C00);
            break;
        case 2: // Arctic (Snowflake Ice Crystal)
            fb.drawLine(x + 8, y + 2, x + 8, y + 13, 0xFF80DEEA);
            fb.drawLine(x + 3, y + 8, x + 13, y + 8, 0xFF80DEEA);
            fb.setPixelFast(x + 5, y + 5, 0xFFFFFFFF);
            fb.setPixelFast(x + 11, y + 5, 0xFFFFFFFF);
            fb.setPixelFast(x + 5, y + 11, 0xFFFFFFFF);
            fb.setPixelFast(x + 11, y + 11, 0xFFFFFFFF);
            break;
        case 3: // Mountain (Stone Escarpment)
            fb.drawLine(x + 3, y + 14, x + 8, y + 4, 0xFF78909C);
            fb.drawLine(x + 8, y + 4, x + 14, y + 14, 0xFF546E7A);
            fb.fillRect(x + 7, y + 4, 3, 3, 0xFFECEFF1); // Snowcap
            break;
        case 4: // Moon (Crater & Earth)
            fb.fillCircle(x + 8, y + 8, 6, 0xFFB0BEC5);
            fb.drawCircle(x + 8, y + 8, 6, 0xFF78909C);
            fb.fillCircle(x + 6, y + 6, 1, 0xFF546E7A); // Crater 1
            fb.fillCircle(x + 10, y + 9, 2, 0xFF546E7A); // Crater 2
            break;
        case 5: // Volcano (Caldera & Lava)
            fb.drawLine(x + 2, y + 14, x + 6, y + 6, 0xFF3E2723);
            fb.drawLine(x + 14, y + 14, x + 10, y + 6, 0xFF3E2723);
            fb.fillRect(x + 6, y + 6, 4, 3, 0xFFFF3D00); // Lava pool
            fb.setPixelFast(x + 7, y + 3, 0xFFFFEB3B);   // Spark
            fb.setPixelFast(x + 8, y + 2, 0xFFFF5722);
            break;
        default:
            break;
    }
}

void UIComponents::drawCheckeredFlag(Graphics::Framebuffer& fb, int x, int y, int size) {
    // Flag pole
    fb.fillRect(x, y, 2, size + 4, 0xFFECEFF1);
    // 8x6 Checkerboard
    int w = size;
    int h = (size * 3) / 4;
    int checkSize = 2;
    for (int cy = 0; cy < h; cy += checkSize) {
        for (int cx = 0; cx < w; cx += checkSize) {
            bool black = ((cx / checkSize) + (cy / checkSize)) % 2 == 0;
            fb.fillRect(x + 2 + cx, y + cy, checkSize, checkSize, black ? 0xFF05080E : 0xFFFFFFFF);
        }
    }
}

void UIComponents::drawFuelPumpIcon(Graphics::Framebuffer& fb, int x, int y, uint32_t color) {
    // Fuel pump body
    fb.fillRect(x + 2, y + 2, 7, 10, color);
    // Glass meter readout
    fb.fillRect(x + 4, y + 4, 3, 2, 0xFF102030);
    // Dispenser hose
    fb.fillRect(x + 9, y + 5, 2, 5, 0xFF424242);
    fb.fillRect(x + 11, y + 3, 2, 5, color);
    fb.fillRect(x + 10, y + 3, 2, 2, 0xFF212121); // Nozzle
    // Pump base
    fb.fillRect(x + 1, y + 12, 9, 2, Theme::darken(color, 40));
}

} // namespace UI
