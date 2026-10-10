#pragma once

#include <cstdint>

namespace UI {

/**
 * @brief Curated Retro-Arcade Design System and Visual Tokens.
 * Guarantees a cohesive, premium visual aesthetic across all screens.
 */
struct Theme {
    // Canvas & Framebuffer
    static constexpr int VIRTUAL_WIDTH = 960;
    static constexpr int VIRTUAL_HEIGHT = 540;

    // Background & Surfaces
    static constexpr uint32_t BG_DARK        = 0xFF0E141F;
    static constexpr uint32_t BG_OVERLAY     = 0xD0080C14;
    static constexpr uint32_t CARD_BG        = 0xEE16202E;
    static constexpr uint32_t CARD_BG_HOVER  = 0xFF1E2B3D;
    static constexpr uint32_t CARD_BORDER    = 0xFF2A3A52;
    static constexpr uint32_t CARD_BORDER_HI = 0xFF00E5FF;

    // Brand & Status Accents
    static constexpr uint32_t GOLD           = 0xFFFFB300;
    static constexpr uint32_t GREEN_GAS      = 0xFF00E676;
    static constexpr uint32_t RED_BRAKE      = 0xFFFF3D00;
    static constexpr uint32_t CYAN_UPGRADE   = 0xFF00E5FF;
    static constexpr uint32_t PURPLE_STUNT   = 0xFFB388FF;

    // Typography
    static constexpr uint32_t TEXT_WHITE     = 0xFFF5F7FA;
    static constexpr uint32_t TEXT_MUTED     = 0xFF7E8E9F;
    static constexpr uint32_t TEXT_DARK      = 0xFF1A202C;

    // Gauge Colors
    static constexpr uint32_t GAUGE_FACE     = 0xEE121824;
    static constexpr uint32_t GAUGE_BORDER   = 0xFF37474F;
    static constexpr uint32_t GAUGE_NEEDLE   = 0xFFFF3D00;
    static constexpr uint32_t GAUGE_REDLINE  = 0xFFFF1744;

    // Medals & Badges
    static constexpr uint32_t MEDAL_BRONZE   = 0xFFCD7F32;
    static constexpr uint32_t MEDAL_SILVER   = 0xFFCFD8DC;
    static constexpr uint32_t MEDAL_GOLD     = 0xFFFFD700;
    static constexpr uint32_t MEDAL_PLATINUM = 0xFF00E5FF;
    static constexpr uint32_t MEDAL_ROOKIE   = 0xFF78909C;

    // Metallic & Bezel Accents
    static constexpr uint32_t CHROME_BRIGHT  = 0xFFECEFF1;
    static constexpr uint32_t CHROME_MID     = 0xFF90A4AE;
    static constexpr uint32_t CHROME_DARK    = 0xFF37474F;
    static constexpr uint32_t RIVET_DARK     = 0xFF21272F;

    // World & Terrain Palette
    static constexpr uint32_t SKY_TOP        = 0xFF1E88E5; // Vibrant Sky Azure
    static constexpr uint32_t SKY_HORIZON    = 0xFF81D4FA; // Soft Sunny Horizon
    static constexpr uint32_t GRASS_LUSH     = 0xFF4CAF50; // Vibrant Lush Emerald
    static constexpr uint32_t GRASS_DARK     = 0xFF2E7D32; // Deep Grass Sublayer
    static constexpr uint32_t DIRT_RICH      = 0xFF795548; // Warm Chocolate Loam
    static constexpr uint32_t DIRT_DEEP      = 0xFF4E342E; // Deep Terracotta Bedrock

    // Utility Color Functions
    static inline uint32_t lighten(uint32_t c, int amount) {
        uint32_t a = (c >> 24) & 0xFF;
        int r = static_cast<int>((c >> 16) & 0xFF) + amount;
        int g = static_cast<int>((c >> 8) & 0xFF) + amount;
        int b = static_cast<int>(c & 0xFF) + amount;
        r = std::clamp(r, 0, 255);
        g = std::clamp(g, 0, 255);
        b = std::clamp(b, 0, 255);
        return (a << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    }

    static inline uint32_t darken(uint32_t c, int amount) {
        return lighten(c, -amount);
    }

    static inline uint32_t blend(uint32_t c1, uint32_t c2, float t) {
        if (t <= 0.0f) return c1;
        if (t >= 1.0f) return c2;
        uint32_t a1 = (c1 >> 24) & 0xFF, r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
        uint32_t a2 = (c2 >> 24) & 0xFF, r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
        uint32_t a = static_cast<uint32_t>(a1 + (a2 - a1) * t);
        uint32_t r = static_cast<uint32_t>(r1 + (r2 - r1) * t);
        uint32_t g = static_cast<uint32_t>(g1 + (g2 - g1) * t);
        uint32_t b = static_cast<uint32_t>(b1 + (b2 - b1) * t);
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    static inline uint32_t withAlpha(uint32_t c, uint8_t alpha) {
        return (static_cast<uint32_t>(alpha) << 24) | (c & 0x00FFFFFF);
    }
};

} // namespace UI
