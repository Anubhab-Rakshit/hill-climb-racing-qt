#include "EvaluatorOverlay.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"

namespace UI {

EvaluatorOverlay::EvaluatorOverlay() {
}

void EvaluatorOverlay::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;
}

void EvaluatorOverlay::render(Graphics::Framebuffer& fb, const std::string& currentVehicleName,
                              const std::string& currentBiomeName) {
    // Top VIP Badge
    int badgeW = 390;
    int badgeH = 22;
    int bx = 16;
    int by = 12;
    UIComponents::drawArcadePanel(fb, bx, by, badgeW, badgeH, 0xDD0A1424, Theme::CARD_BORDER, Theme::GOLD, 2, false, true);
    Graphics::RasterFont::drawString(fb, bx + 10, by + 4, "[VIP EVALUATOR MODE - UNLOCKED]", Theme::GOLD, 1);

    // Live active vehicle and biome tag
    std::string infoStr = currentVehicleName + " | " + currentBiomeName;
    Graphics::RasterFont::drawString(fb, bx + 185, by + 4, infoStr, Theme::CYAN_UPGRADE, 1);

    // Bottom Tester Hotkey Bar
    int barH = 26;
    int barY = m_height - barH;
    fb.fillRect(0, barY, m_width, barH, 0xEE090E18);
    fb.drawLine(0, barY, m_width, barY, Theme::GOLD);

    int kx = 18;
    int ky = barY + 5;

    // F: Fuel
    UIComponents::drawKeycap(fb, kx, ky, "F");
    std::string fuelStr = m_infiniteFuel ? "INF FUEL: ON" : "INF FUEL: OFF";
    uint32_t fuelCol = m_infiniteFuel ? Theme::GREEN_GAS : Theme::RED_BRAKE;
    Graphics::RasterFont::drawString(fb, kx + 18, ky + 4, fuelStr, fuelCol, 1);

    // V: Vehicle Swap
    kx += 125;
    UIComponents::drawKeycap(fb, kx, ky, "V");
    Graphics::RasterFont::drawString(fb, kx + 18, ky + 4, "NEXT VEHICLE (1-12)", Theme::TEXT_WHITE, 1);

    // 1-9: Track Hop
    kx += 165;
    UIComponents::drawKeycap(fb, kx, ky, "1-9");
    Graphics::RasterFont::drawString(fb, kx + 28, ky + 4, "HOP TRACK (1-9)", Theme::CYAN_UPGRADE, 1);

    // T: Turbo
    kx += 140;
    UIComponents::drawKeycap(fb, kx, ky, "T");
    std::string turboStr = m_turboBoost ? "TURBO: ACTIVE" : "HOLD TURBO";
    Graphics::RasterFont::drawString(fb, kx + 18, ky + 4, turboStr, m_turboBoost ? Theme::GOLD : Theme::TEXT_MUTED, 1);

    // Z: Slow-Mo
    kx += 115;
    UIComponents::drawKeycap(fb, kx, ky, "Z");
    std::string slowStr = m_slowMotion ? "SLOW-MO: ON" : "SLOW-MO";
    Graphics::RasterFont::drawString(fb, kx + 18, ky + 4, slowStr, m_slowMotion ? Theme::CYAN_UPGRADE : Theme::TEXT_MUTED, 1);

    // ESC: Menu
    kx += 110;
    UIComponents::drawKeycap(fb, kx, ky, "ESC");
    Graphics::RasterFont::drawString(fb, kx + 32, ky + 4, "MENU", Theme::TEXT_MUTED, 1);
}

} // namespace UI
