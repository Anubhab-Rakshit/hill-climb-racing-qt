#pragma once

#include "Framebuffer.h"
#include "ProfileManager.h"
#include <string>
#include <cstdint>

namespace UI {

/**
 * @brief Reusable Pixel-Art UI Components and Visual Kit.
 * Unifies panels, badges, keycaps, icons, medals, and formatting across all screens.
 */
class UIComponents {
public:
    // Format number with commas (e.g. 125000 -> "125,000")
    static std::string formatNumber(int64_t val);

    // Standard Arcade Panel with corner notches, 3D bevels, and optional top accent stripe
    static void drawArcadePanel(Graphics::Framebuffer& fb,
                                int x, int y, int w, int h,
                                uint32_t bgCol,
                                uint32_t borderCol,
                                uint32_t accentCol = 0,
                                int accentH = 0,
                                bool drawBevel = true,
                                bool drawNotches = true);

    // Top-Right / Header Golden Currency Pill
    static void drawCoinBadge(Graphics::Framebuffer& fb,
                              int x, int y, int w, int h,
                              int coins,
                              float animTime = 0.0f);

    // 3D Keyboard Keycap Hint (e.g. "[A]", "[D]", "[ESC]")
    static void drawKeycap(Graphics::Framebuffer& fb,
                           int x, int y,
                           const std::string& keyText,
                           bool pressed = false);

    // Backlit Segmented Pip Meter
    static void drawSegmentedBar(Graphics::Framebuffer& fb,
                                 int x, int y, int w, int h,
                                 int currentVal, int maxVal,
                                 uint32_t activeCol,
                                 uint32_t inactiveCol = 0xFF192534,
                                 uint32_t hiCol = 0x88FFFFFF);

    // 5-Point Symmetrical Pixel Star
    static void drawStar(Graphics::Framebuffer& fb,
                         int cx, int cy, int radius,
                         bool filled,
                         uint32_t color = 0xFFFFD700,
                         uint32_t outlineColor = 0xFFFFA000);

    // Result / Achievement Medal (Rookie, Bronze, Silver, Gold, Platinum)
    static void drawMedalBadge(Graphics::Framebuffer& fb,
                               int cx, int cy,
                               int tier, // 0..4
                               float animTime = 0.0f);

    // Custom 16x16 / 18x18 Pixel-Art Icon for Upgrades
    static void drawUpgradeIcon(Graphics::Framebuffer& fb,
                                int x, int y,
                                Core::ProfileManager::UpgradeType type);

    // Biome Track Mini-Icon (Countryside, Desert, Arctic, Mountain, Moon, Volcano)
    static void drawBiomeIcon(Graphics::Framebuffer& fb,
                              int x, int y,
                              int stageIdx);

    // Checkered Racing Flag Icon
    static void drawCheckeredFlag(Graphics::Framebuffer& fb, int x, int y, int size = 12);

    // Fuel Pump Dispenser Icon
    static void drawFuelPumpIcon(Graphics::Framebuffer& fb, int x, int y, uint32_t color = 0xFFFFB300);
};

} // namespace UI
