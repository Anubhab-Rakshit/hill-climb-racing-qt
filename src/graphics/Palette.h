#pragma once

#include <cstdint>

namespace Graphics {
    namespace Palette {
        // Fuel can colors
        constexpr uint32_t FuelCanShadow      = 0x44000000; // semi‑transparent dark shadow
        constexpr uint32_t FuelCanHandle      = 0xFFB71C1C; // red handle
        constexpr uint32_t FuelCanHandleBorder= 0xFF8E0000; // darker border
        constexpr uint32_t FuelCanSpout       = 0xFFCFD8DC; // light grey spout
        constexpr uint32_t FuelCanSpoutBorder = 0xFF78909C; // darker grey border
        constexpr uint32_t FuelCanCap         = 0xFFFFB300; // brass cap
        constexpr uint32_t FuelCanBody        = 0xFFD32F2F; // bright red body
        constexpr uint32_t FuelCanBodyBorder  = 0xFF8E0000; // dark steel border
        constexpr uint32_t FuelCanBevel       = 0xFFFF5252; // specular bevel colour
        constexpr uint32_t FuelCanXLine1      = 0xFF8E0000; // X indentation primary colour
        constexpr uint32_t FuelCanXLine2      = 0xFFEF5350; // X indentation secondary colour
        constexpr uint32_t FuelCanBadgeFill   = 0xFFECEFF1; // white badge background
        constexpr uint32_t FuelCanBadgeBorder = 0xFFB0BEC5; // light grey border
        constexpr uint32_t FuelCanBadgeText   = 0xFFD32F2F; // text colour (same as body)
        constexpr uint32_t FuelCanHaloBase    = 0x00FF5252; // halo base colour (red with zero alpha)
        // Coin colors (Gold, Silver, Bronze)
        constexpr uint32_t CoinGoldRim    = 0xFFB27D00;
        constexpr uint32_t CoinGoldBody   = 0xFFFFD700;
        constexpr uint32_t CoinGoldHi     = 0xFFFFF59D;
        constexpr uint32_t CoinGoldText   = 0xFF3E2723;
        constexpr uint32_t CoinSilverRim  = 0xFF607D8B;
        constexpr uint32_t CoinSilverBody = 0xFFCFD8DC;
        constexpr uint32_t CoinSilverHi   = 0xFFECEFF1;
        constexpr uint32_t CoinSilverText = 0xFF263238;
        constexpr uint32_t CoinBronzeRim  = 0xFF795548;
        constexpr uint32_t CoinBronzeBody = 0xFFCD7F32;
        constexpr uint32_t CoinBronzeHi   = 0xFFD7CCC8;
        constexpr uint32_t CoinBronzeText = 0xFF3E2723; // same as gold text
    } // namespace Palette
} // namespace Graphics

