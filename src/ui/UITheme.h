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

    // World & Terrain Palette
    static constexpr uint32_t SKY_TOP        = 0xFF162B4D;
    static constexpr uint32_t SKY_HORIZON    = 0xFF3F6B99;
    static constexpr uint32_t GRASS_LUSH     = 0xFF43A047;
    static constexpr uint32_t GRASS_DARK     = 0xFF1B5E20;
    static constexpr uint32_t DIRT_RICH      = 0xFF6D4C41;
    static constexpr uint32_t DIRT_DEEP      = 0xFF3E2723;
};

} // namespace UI
