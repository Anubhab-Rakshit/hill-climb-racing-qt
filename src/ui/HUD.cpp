#include "HUD.h"
#include "RasterFont.h"
#include "UITheme.h"
#include "UIComponents.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace UI {

HUD::HUD()
    : m_width(Theme::VIRTUAL_WIDTH)
    , m_height(Theme::VIRTUAL_HEIGHT)
    , m_animTime(0.0f)
    , m_fuelBar(16, 12, 210, 36)
    , m_speedometer(418, 478, 48, 0.0f, 140.0f, "SPEED", "KM/H", 0.75f)
    , m_tachometer(542, 478, 48, 0.0f, 8000.0f, "RPM", "x1000", 0.80f)
    , m_brakePedal(24, 445, 115, 80, "BRAKE", Theme::RED_BRAKE)
    , m_reversePedal(148, 445, 115, 80, "REVERSE", 0xFFFF6D00)
    , m_gasPedal(821, 445, 115, 80, "GAS", Theme::GREEN_GAS)
    , m_pauseButton(902, 12, 42, 36, "||", 0xFF2A3644)
    , m_distance(0.0f)
    , m_recordDistance(500.0f)
    , m_coins(0)
    , m_clearedCheckpoints(0)
    , m_totalCheckpoints(5)
    , m_nextCheckpointName("THE FOOTHILLS")
    , m_nextCheckpointDist(250.0f)
{
    m_brakePedal.setKeyHint("[A]");
    m_reversePedal.setKeyHint("[S]");
    m_gasPedal.setKeyHint("[D]");
    m_pauseButton.setKeyHint("ESC");
    setDimensions(m_width, m_height);
}

void HUD::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_fuelBar.setPosition(16, 12);
    m_speedometer.setCenter(m_width / 2 - 62, m_height - 62);
    m_tachometer.setCenter(m_width / 2 + 62, m_height - 62);

    m_brakePedal.setPosition(24, m_height - 95);
    m_reversePedal.setPosition(148, m_height - 95);
    m_gasPedal.setPosition(m_width - 139, m_height - 95);
    m_pauseButton.setPosition(m_width - 16 - 42, 12);
    m_pauseButton.setSize(42, 36);
}

void HUD::setBrakeVirtualPressed(bool pressed) {
    m_brakePedal.setPressed(pressed);
}

void HUD::setReverseVirtualPressed(bool pressed) {
    m_reversePedal.setPressed(pressed);
}

void HUD::setGasVirtualPressed(bool pressed) {
    m_gasPedal.setPressed(pressed);
}

void HUD::addFloatingText(const std::string& text, float x, float y, uint32_t color) {
    FloatingText ft;
    ft.text = text;
    ft.x = x;
    ft.y = y;
    ft.life = 0.0f;
    ft.maxLife = 1.0f;
    ft.color = color;
    m_floatingTexts.push_back(ft);
}

void HUD::onMouseMove(int px, int py) {
    m_brakePedal.onMouseMove(px, py);
    m_reversePedal.onMouseMove(px, py);
    m_gasPedal.onMouseMove(px, py);
    m_pauseButton.onMouseMove(px, py);
}

bool HUD::onMouseDown(int px, int py) {
    if (m_pauseButton.onMouseDown(px, py)) return true;
    if (m_brakePedal.onMouseDown(px, py)) return true;
    if (m_reversePedal.onMouseDown(px, py)) return true;
    if (m_gasPedal.onMouseDown(px, py)) return true;
    return false;
}

void HUD::onMouseUp(int px, int py) {
    m_pauseButton.onMouseUp(px, py);
    m_brakePedal.onMouseUp(px, py);
    m_reversePedal.onMouseUp(px, py);
    m_gasPedal.onMouseUp(px, py);
}

void HUD::update(float dt) {
    m_animTime += dt;
    m_fuelBar.update(dt);
    m_speedometer.update(dt);
    m_tachometer.update(dt);
    m_stuntBanner.update(dt);

    // Update floating score texts
    for (size_t i = 0; i < m_floatingTexts.size(); ) {
        m_floatingTexts[i].life += dt;
        m_floatingTexts[i].y -= 38.0f * dt;
        if (m_floatingTexts[i].life >= m_floatingTexts[i].maxLife) {
            m_floatingTexts[i] = m_floatingTexts.back();
            m_floatingTexts.pop_back();
        } else {
            ++i;
        }
    }
}

static void drawArcadeRacingPedal(Graphics::Framebuffer& fb,
                                  int px, int py, int pw, int ph,
                                  const std::string& label,
                                  const std::string& keyHint,
                                  uint32_t themeCol,
                                  bool pressed,
                                  bool hovered,
                                  int pedalType) {
    if (pw <= 0 || ph <= 0) return;

    // 1. Recessed floor drop shadow
    fb.fillRect(px + 2, py + 2, pw, ph, 0x8805080E);

    // 2. 3D Mechanical Depression Offset (shifts down & inward into footwell when active)
    int dx = pressed ? 1 : 0;
    int dy = pressed ? 3 : 0;
    int x = px + dx;
    int y = py + dy;

    // 3. Active LED Perimeter Halo Glow
    if (pressed) {
        fb.drawRect(px - 1, py - 1, pw + 2, ph + 2, themeCol);
        fb.drawRect(px - 2, py - 2, pw + 4, ph + 4, (themeCol & 0x00FFFFFF) | 0x55000000);
    } else if (hovered) {
        fb.drawRect(px - 1, py - 1, pw + 2, ph + 2, UI::Theme::lighten(themeCol, 40));
    }

    // 4. Heavy Gunmetal / Brushed Titanium Pedal Plate
    uint32_t plateCol = pressed ? 0xFF1E2834 : (hovered ? 0xFF2E3B4D : 0xFF253242);
    fb.fillRect(x, y, pw, ph, 0xFF0A0F17); // Dark chassis rim
    fb.fillRect(x + 2, y + 2, pw - 4, ph - 4, plateCol);

    // 5. 3D Chamfered Metallic Edge Bevels
    uint32_t bevelHi = UI::Theme::lighten(plateCol, 45);
    uint32_t bevelShd = UI::Theme::darken(plateCol, 55);
    fb.fillRect(x + 1, y + 1, pw - 2, 2, pressed ? bevelShd : bevelHi);
    fb.fillRect(x + 1, y + 1, 2, ph - 2, pressed ? bevelShd : bevelHi);
    fb.fillRect(x + 1, y + ph - 3, pw - 2, 2, pressed ? bevelHi : bevelShd);
    fb.fillRect(x + pw - 3, y + 1, 2, ph - 2, pressed ? bevelHi : bevelShd);

    // 6. Functional Perimeter Racing Outline
    fb.drawRect(x + 3, y + 3, pw - 6, ph - 6, themeCol);

    // 7. Four Corner Socket Head Hex Screws
    auto drawBolt = [&](int bx, int by) {
        fb.fillRect(bx - 1, by - 1, 3, 3, 0xFF78909C);
        fb.setPixelFast(bx, by, 0xFF141C24);
    };
    drawBolt(x + 7, y + 7);
    drawBolt(x + pw - 8, y + 7);
    drawBolt(x + 7, y + ph - 8);
    drawBolt(x + pw - 8, y + ph - 8);

    // 8. Non-Slip Rubber Grip Tread Plates
    if (pedalType == 0) {
        // BRAKE: Heavy-Duty Horizontal Rubber Traction Slats
        int treadW = pw - 28;
        int treadX = x + 14;
        const int slatYs[4] = {y + 11, y + 17, y + ph - 20, y + ph - 14};
        for (int ty : slatYs) {
            fb.fillRect(treadX, ty, treadW, 3, 0xFF101722);
            fb.fillRect(treadX, ty, treadW, 1, 0x44FFFFFF);
            if (pressed) fb.fillRect(treadX + 2, ty + 1, treadW - 4, 1, 0x88FF1744);
        }
    } else if (pedalType == 1) {
        // REVERSE: Safety Hazard Textured Tread with Amber Accents
        int treadW = pw - 28;
        int treadX = x + 14;
        const int slatYs[4] = {y + 11, y + 17, y + ph - 20, y + ph - 14};
        for (int ty : slatYs) {
            fb.fillRect(treadX, ty, treadW, 3, 0xFF1C1208);
            for (int hx = treadX; hx < treadX + treadW; hx += 8) {
                fb.fillRect(hx, ty, 4, 2, 0xFFFF6D00);
            }
        }
    } else {
        // GAS: Performance Accelerator Longitudinal Racing Ribs
        int ribH = ph - 26;
        int ribY = y + 13;
        const int ribXs[4] = {x + 12, x + 18, x + pw - 21, x + pw - 15};
        for (int rx : ribXs) {
            fb.fillRect(rx, ribY, 3, ribH, 0xFF101722);
            fb.fillRect(rx, ribY, 1, ribH, 0x44FFFFFF);
            if (pressed) fb.fillRect(rx + 1, ribY, 1, ribH, 0x8800E676);
        }
    }

    // 9. Stencil Typography Label with Drop Shadow
    int cx = x + pw / 2;
    int cy = y + (ph - Graphics::RasterFont::getTextHeight(2)) / 2;
    Graphics::RasterFont::drawStringCentered(fb, cx + 1, cy + 1, label, 0xFF05080E, 2);
    uint32_t textCol = hovered ? 0xFFFFFFFF : (pressed ? themeCol : 0xFFECEFF1);
    Graphics::RasterFont::drawStringCentered(fb, cx, cy, label, textCol, 2);

    // 10. Tactile Keycap Hint Badge in Corner (e.g. "[A]", "[S]", "[D]")
    if (!keyHint.empty()) {
        int kw = Graphics::RasterFont::getTextWidth(keyHint, 1) + 6;
        int kh = 11;
        int kx = x + pw - kw - 6;
        int ky = y + 5;
        fb.fillRect(kx, ky, kw, kh, 0xEE090E18);
        fb.drawRect(kx, ky, kw, kh, themeCol);
        Graphics::RasterFont::drawStringCentered(fb, kx + kw / 2, ky + 2, keyHint, UI::Theme::TEXT_WHITE, 1);
    }
}

void HUD::render(Graphics::Framebuffer& fb) {
    // 1. Top Left: Fuel Gauge Pod
    m_fuelBar.render(fb);

    // 2. Top Center: Unified Rally Stage Telemetry & Odometer Module
    int distCardW = 320;
    int distCardH = 48;
    int distCardX = (m_width - distCardW) / 2;
    int distCardY = 10;

    UIComponents::drawArcadePanel(fb, distCardX, distCardY, distCardW, distCardH, 0xEE090E18, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

    int distInt = static_cast<int>(m_distance);
    char distBuf[32];
    std::snprintf(distBuf, sizeof(distBuf), "%05dm", distInt);

    Graphics::RasterFont::drawString(fb, distCardX + 12, distCardY + 6, "DIST", Theme::TEXT_MUTED, 1);
    Graphics::RasterFont::drawString(fb, distCardX + 48, distCardY + 5, distBuf, Theme::TEXT_WHITE, 2);

    int recInt = static_cast<int>(m_recordDistance);
    char recBuf[32];
    std::snprintf(recBuf, sizeof(recBuf), "REC %04dm", recInt);
    Graphics::RasterFont::drawString(fb, distCardX + 210, distCardY + 7, recBuf, Theme::GOLD, 1);

    // Mini Rally Stage Progress Track Line
    int trackX = distCardX + 12;
    int trackY = distCardY + 25;
    int trackW = distCardW - 36;
    fb.fillRect(trackX, trackY, trackW, 4, 0xFF192534);

    float trackProg = std::clamp(m_distance / 1750.0f, 0.0f, 1.0f);
    int fillW = static_cast<int>(trackProg * trackW);
    if (fillW > 0) {
        fb.fillRect(trackX, trackY, fillW, 4, Theme::CYAN_UPGRADE);
        fb.fillRect(trackX, trackY, fillW, 1, 0x88FFFFFF);
    }

    // 5 Checkpoint Pips along the track line
    const float cpDists[5] = {250.0f, 550.0f, 900.0f, 1300.0f, 1750.0f};
    for (int cp = 0; cp < 5; ++cp) {
        int cpPx = trackX + static_cast<int>((cpDists[cp] / 1750.0f) * trackW);
        uint32_t col = (m_clearedCheckpoints > cp) ? Theme::GOLD : 0xFF546E7A;
        fb.fillRect(cpPx - 1, trackY - 2, 3, 8, col);
    }
    // Indicator pip for car position
    fb.fillRect(trackX + fillW - 2, trackY - 3, 5, 10, Theme::GREEN_GAS);

    // Checkered Finish Line Flag Icon at end of track
    UIComponents::drawCheckeredFlag(fb, trackX + trackW + 4, trackY - 6, 12);

    // Integrated Subtitle Ribbon at bottom of telemetry card
    int tagY = distCardY + 34;
    if (!m_nextCheckpointName.empty() && m_distance < 1750.0f) {
        int remain = static_cast<int>(m_nextCheckpointDist - m_distance);
        std::string cpTag = "CP " + std::to_string(m_clearedCheckpoints + 1) + "/5: " +
                            std::to_string(std::max(0, remain)) + "m TO " + m_nextCheckpointName;
        Graphics::RasterFont::drawStringCentered(fb, distCardX + distCardW / 2, tagY + 2, cpTag, Theme::CYAN_UPGRADE, 1);
    } else if (m_clearedCheckpoints >= 5) {
        Graphics::RasterFont::drawStringCentered(fb, distCardX + distCardW / 2, tagY + 2, "* STAGE CLEARED - ENDLESS RUN *", Theme::GOLD, 1);
    }

    // 3. Top Right: Master Currency Pill
    int coinBoxW = 145;
    int coinBoxH = 36;
    int pauseW = 42;
    int pauseH = 36;
    int coinBoxX = m_width - 16 - pauseW - 8 - coinBoxW;
    int coinBoxY = 12;
    UIComponents::drawCoinBadge(fb, coinBoxX, coinBoxY, coinBoxW, coinBoxH, m_coins, m_animTime);

    // 4. Pause Button (perfectly aligned with coin pill)
    m_pauseButton.setPosition(m_width - 16 - pauseW, 12);
    m_pauseButton.setSize(pauseW, pauseH);
    m_pauseButton.render(fb, 2);

    // 5. Floating Text FX (Popups for collected coins and fuel)
    for (const auto& ft : m_floatingTexts) {
        int fx = static_cast<int>(ft.x);
        int fy = static_cast<int>(ft.y);
        Graphics::RasterFont::drawStringCentered(fb, fx + 1, fy + 1, ft.text, 0xFF05080E, 2);
        Graphics::RasterFont::drawStringCentered(fb, fx, fy, ft.text, ft.color, 2);
    }

    // 6. Bottom Center: Sculpted Cockpit Dual-Gauge Binnacle Contour
    // Low-profile carbon connector bridge between the two round instrument dials
    int bridgeW = 40;
    int bridgeH = 18;
    int bridgeX = m_width / 2 - bridgeW / 2;
    int bridgeY = m_height - 62 - bridgeH / 2;
    fb.fillRect(bridgeX, bridgeY, bridgeW, bridgeH, 0xDD080E18);
    fb.fillRect(bridgeX, bridgeY, bridgeW, 2, 0xFF2A3A4E);
    fb.fillRect(bridgeX, bridgeY + bridgeH - 2, bridgeW, 2, 0xFF0B1422);

    m_speedometer.render(fb);
    m_tachometer.render(fb);

    // 7. Bottom Corners: Authentic Arcade Racing Pedals with Metallic Grip Textures & LED Halos
    drawArcadeRacingPedal(fb, 24, m_height - 95, 115, 80,
                         "BRAKE", "[A]", Theme::RED_BRAKE,
                         m_brakePedal.isPressed(), m_brakePedal.isHovered(), 0);

    drawArcadeRacingPedal(fb, 148, m_height - 95, 115, 80,
                         "REVERSE", "[S]", 0xFFFF6D00,
                         m_reversePedal.isPressed(), m_reversePedal.isHovered(), 1);

    drawArcadeRacingPedal(fb, m_width - 139, m_height - 95, 115, 80,
                         "GAS", "[D]", Theme::GREEN_GAS,
                         m_gasPedal.isPressed(), m_gasPedal.isHovered(), 2);

    // 8. Stunt Notifications Banner (Top-Center floating)
    m_stuntBanner.render(fb);
}

} // namespace UI
