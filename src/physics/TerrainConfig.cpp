#include "TerrainConfig.h"
#include "Terrain.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace Physics {

static const std::vector<BiomeSpec> s_biomes = {
    // 1. COUNTRYSIDE (Signature Lush Green Hills)
    {
        "countryside",
        "COUNTRYSIDE",
        "Smooth Rolling Grass Hills & Scenic Meadows",
        "desert",
        0, // Free starter
        9.81f,  // 1.0G
        1.25f,  // Firm grass traction
        1.00f,  // 1.0x Earth sea-level atmosphere
        1.0f,
        1.0f,
        // Sky & Parallax Colors
        0xFF42A5F5, 0xFFBBDEFB, 0xFF3E5062,
        // Surface & Strata Colors
        0xFF81C784, 0xFF43A047, 0xFF2E7D32, 0xFF5D4037, 0xFF3E2723,
        {
            {1, 250.0f, "THE FOOTHILLS", "Gentle Rolling Crests", 500, false},
            {2, 550.0f, "SKI JUMP MEADOWS", "High-Speed Launch Ramps", 1000, false},
            {3, 900.0f, "TECHNICAL MOGULS", "Rhythm Washboard Bumps", 1800, false},
            {4, 1300.0f, "MOUNTAIN WALL", "Steep Sustained Incline", 2800, false},
            {5, 1750.0f, "COUNTRYSIDE CLEARED", "Grand Finale & Desert Unlock!", 5000, true}
        }
    },

    // 2. DESERT DUNES (Loose Sand & Sharp Slip-Faces)
    {
        "desert",
        "DESERT DUNES",
        "Asymmetric Windward Sand Dunes & Sharp Crests",
        "arctic",
        1500,
        9.81f,  // 1.0G
        1.05f,  // Loose sand friction
        0.95f,  // Dry arid atmosphere
        1.15f,
        1.05f,
        // Sky & Parallax Colors
        0xFFC95B24, 0xFFFFB74D, 0xFFBF6E34,
        // Surface & Strata Colors
        0xFFFFF59D, 0xFFFFD54F, 0xFFFFA000, 0xFFFF8F00, 0xFFD84315,
        {
            {1, 250.0f, "SHIFTING SANDS", "Windward Dune Entry", 700, false},
            {2, 550.0f, "SLIP-FACE RIDGES", "Sharp Crest Drops", 1200, false},
            {3, 900.0f, "SAHARA CANYONS", "Deep Sandy Gullies", 2200, false},
            {4, 1300.0f, "DUST DEVIL PASS", "Long Climbing Dunes", 3200, false},
            {5, 1750.0f, "DESERT CONQUERED", "Dune Master & Arctic Unlock!", 6000, true}
        }
    },

    // 3. ARCTIC TUNDRA (Slick Ice Peaks & Glacial Crevasses)
    {
        "arctic",
        "ARCTIC TUNDRA",
        "Slippery Glacial Ice & Permafrost Ridges",
        "mountain",
        2500,
        9.81f,  // 1.0G
        0.82f,  // Slippery icy friction
        1.15f,  // Dense cold sub-zero atmosphere
        1.25f,
        1.10f,
        // Sky & Parallax Colors
        0xFF546E7A, 0xFFCFD8DC, 0xFF37474F,
        // Surface & Strata Colors
        0xFFE0F7FA, 0xFF80DEEA, 0xFF00ACC1, 0xFF00838F, 0xFF004D40,
        {
            {1, 250.0f, "FROZEN GLADE", "Slick Ice Entry", 900, false},
            {2, 550.0f, "GLACIAL CREVASSES", "Deep Trench Jumps", 1500, false},
            {3, 900.0f, "BLIZZARD PEAKS", "Wind-Swept Crags", 2500, false},
            {4, 1300.0f, "ICEFALL ESCARPMENT", "High-Slope Icy Ascent", 3600, false},
            {5, 1750.0f, "TUNDRA CLEARED", "Arctic Explorer & Mountain Unlock!", 7000, true}
        }
    },

    // 4. MOUNTAIN RIDGE (Craggy Alpine Ascents & Rocky Ledges)
    {
        "mountain",
        "MOUNTAIN RIDGE",
        "Steep Rocky Crags & Severe Alpine Ascents",
        "moon",
        3500,
        9.81f,  // 1.0G
        1.20f,  // Rocky granite grip
        0.85f,  // Thin high-altitude alpine air
        1.35f,
        1.15f,
        // Sky & Parallax Colors
        0xFF192532, 0xFF455A64, 0xFF263238,
        // Surface & Strata Colors
        0xFFECEFF1, 0xFF78909C, 0xFF546E7A, 0xFF37474F, 0xFF212121,
        {
            {1, 250.0f, "ROCKY FOOTHILLS", "Craggy Boulder Fields", 1000, false},
            {2, 550.0f, "TIMBERLINE CHASM", "Alpine Steps & Leaps", 1800, false},
            {3, 900.0f, "THE NEEDLE PEAK", "Jagged Vertical Climbs", 2800, false},
            {4, 1300.0f, "AVALANCHE GORGE", "Severe Boulder Steps", 4000, false},
            {5, 1750.0f, "SUMMIT CLEARED", "Mountain Legend & Moon Unlock!", 8000, true}
        }
    },

    // 5. THE MOON (Low-Gravity Floaty Craters & Stunt Launchpads)
    {
        "moon",
        "THE MOON",
        "Ultra-Low Gravity Lunar Impact Basins",
        "volcano",
        5000,
        1.62f,  // 0.16G Lunar gravity
        1.15f,  // Regolith grip
        0.00f,  // Strict 0.0x Lunar vacuum (no air resistance / no aero damping)
        1.40f,
        0.85f,  // Longer wavelengths for floaty low-g flight
        // Sky & Parallax Colors
        0xFF050811, 0xFF141C2B, 0xFF192230,
        // Surface & Strata Colors
        0xFFFFFFFF, 0xFFECEFF1, 0xFFCFD8DC, 0xFF90A4AE, 0xFF455A64,
        {
            {1, 250.0f, "SEA OF TRANQUILITY", "Gentle Lunar Rolling Dust", 1200, false},
            {2, 550.0f, "IMPACT BASIN", "Wide Zero-G Craters", 2000, false},
            {3, 900.0f, "APENNINE RIDGE", "Massive Launch Mounds", 3200, false},
            {4, 1300.0f, "FAR SIDE CANYONS", "Deep Lunar Rifts", 4500, false},
            {5, 1750.0f, "MOON BASE ALPHA", "Lunar Champion & Inferno Unlock!", 10000, true}
        }
    },

    // 6. INFERNO PEAKS (Heavy Volcanic Basalt & Magma Calderas)
    {
        "volcano",
        "INFERNO PEAKS",
        "Dense Gravitational Basalt & Molten Calderas",
        "",     // Ultimate stage!
        7000,
        11.50f, // 1.17G Dense planetary gravity
        1.30f,  // Basalt grip
        1.10f,  // Dense volcanic smog & ash
        1.45f,
        1.20f,
        // Sky & Parallax Colors
        0xFF210808, 0xFF5D1010, 0xFF3E0A0A,
        // Surface & Strata Colors
        0xFFFF8A80, 0xFFD32F2F, 0xFF7F0000, 0xFF3E0000, 0xFF1B0000,
        {
            {1, 250.0f, "OBSIDIAN TRAIL", "Hardened Basalt Floor", 1500, false},
            {2, 550.0f, "MAGMA VENT STEPS", "Thermal Shock Rises", 2500, false},
            {3, 900.0f, "CALDERA ASCENT", "Steep Heavy-Gravity Walls", 3800, false},
            {4, 1300.0f, "SULPHUR ESCARPMENT", "Irregular Basalt Ledges", 5000, false},
            {5, 1750.0f, "INFERNO CONQUERED", "Volcano Mastered!", 12000, true}
        }
    },

    // 7. NEON HIGHWAY (Synthwave Cyberpunk Grid)
    {
        "neon",
        "NEON HIGHWAY",
        "Cyberpunk Synthwave Grid & Glowing Speed Launchpads",
        "super_countryside",
        6000,
        9.81f,  // 1.0G
        1.35f,  // High-grip synth track
        0.90f,
        1.25f,
        1.10f,
        // Sky & Parallax Colors
        0xFF120024, 0xFF4A0E4E, 0xFF240038,
        // Surface & Strata Colors (Glowing Neon Cyan, Vibrant Pink, Deep Synth Violet)
        0xFF00E5FF, 0xFFFF007F, 0xFF7928CA, 0xFF1F0038, 0xFF0A0014,
        {
            {1, 250.0f, "CYBER GRID ENTRY", "Laser-Etched Highway", 1500, false},
            {2, 550.0f, "SYNTHWAVE RAMPS", "Glowing Hyperspace Launchers", 2500, false},
            {3, 900.0f, "DIGITAL CHASM", "High-Voltage Overpass", 3800, false},
            {4, 1300.0f, "NEON PEAK SKYLINE", "Downtown Cyber Ascent", 5000, false},
            {5, 1750.0f, "GRID FINALE", "Neon Grand Champion!", 14000, true}
        }
    },

    // 8. SUPER COUNTRYSIDE (Colossal Roller-Coaster Hills & Super Jumps)
    {
        "super_countryside",
        "SUPER COUNTRYSIDE",
        "Colossal Roller-Coaster Hills & Mega Launch Ramps",
        "seasons",
        8500,
        9.81f,  // 1.0G
        1.30f,  // High-traction engineered turf
        1.00f,
        1.65f,  // Giant amplitude!
        0.92f,  // Huge rolling wavelengths
        // Sky & Parallax Colors
        0xFF0288D1, 0xFF81D4FA, 0xFF1B5E20,
        // Surface & Strata Colors
        0xFFB9F6CA, 0xFF00E676, 0xFF00C853, 0xFF4E342E, 0xFF271C19,
        {
            {1, 250.0f, "MEGA RIDGE ENTRY", "High-Amplitude Hill Rise", 1800, false},
            {2, 550.0f, "SUPER LAUNCH RAMP", "Massive 80m Aerial Clears", 3000, false},
            {3, 900.0f, "DOUBLE CREST VALLEY", "Roller-Coaster G-Force Dips", 4500, false},
            {4, 1300.0f, "THE GIANT WALL", "Extreme 50-Degree Incline", 6000, false},
            {5, 1750.0f, "SUPER COUNTRYSIDE CLEARED", "King of the Hills!", 18000, true}
        }
    },

    // 9. CHANGING SEASONS (Spring, Summer, Autumn & Icy Winter)
    {
        "seasons",
        "CHANGING SEASONS",
        "Dynamic 4-Season Journey: Spring, Summer, Autumn & Icy Winter",
        "",     // Grand finale track!
        12000,
        9.81f,  // 1.0G
        1.15f,  // Balanced multi-season traction
        1.05f,
        1.35f,
        1.05f,
        // Sky & Parallax Colors
        0xFF37474F, 0xFFECEFF1, 0xFF455A64,
        // Surface & Strata Colors
        0xFFFFFFFF, 0xFF81C784, 0xFFFFB74D, 0xFF5D4037, 0xFF263238,
        {
            {1, 500.0f, "SPRING BLOSSOMS", "Fresh Green Meadows", 2000, false},
            {2, 1000.0f, "SUMMER GLOW", "Sun-Drenched Golden Dunes", 3500, false},
            {3, 1500.0f, "AUTUMN WINDS", "Amber Forest Leaves & Slick Mud", 5500, false},
            {4, 2000.0f, "WINTER BLIZZARD", "Frozen Peaks & Slippery Ice", 8000, false},
            {5, 2500.0f, "FOUR SEASONS MASTER", "Eternal Grand Prix Legend!", 25000, true}
        }
    }
};

const BiomeSpec& BiomeRegistry::getBiome(const std::string& id) {
    for (const auto& b : s_biomes) {
        if (b.id == id) return b;
    }
    return s_biomes[0];
}

const std::vector<BiomeSpec>& BiomeRegistry::getAllBiomes() {
    return s_biomes;
}

bool BiomeRegistry::exists(const std::string& id) {
    for (const auto& b : s_biomes) {
        if (b.id == id) return true;
    }
    return false;
}

TerrainValidator::ValidationResult TerrainValidator::validateBiome(const std::string& biomeId) {
    Terrain terrain;
    terrain.setBiome(biomeId);

    ValidationResult result;
    std::ostringstream ss;
    ss << "=== TERRAIN VALIDATION REPORT FOR: " << biomeId << " ===\n";

    // 1. Scan slopes across 0 to 2000 meters at 0.5m intervals
    float maxUphill = 0.0f;
    float maxDownhill = 0.0f;
    for (float x = 0.0f; x <= 2000.0f; x += 0.5f) {
        float slope = terrain.getSlope(x);
        float deg = std::abs(std::atan(slope) * (180.0f / 3.14159265f));
        if (slope > 0.0f && deg > maxUphill) maxUphill = deg;
        if (slope < 0.0f && deg > maxDownhill) maxDownhill = deg;
    }
    result.maxUphillSlopeDeg = maxUphill;
    result.maxDownhillSlopeDeg = maxDownhill;

    // Standard vehicle climbing threshold: max slope must not exceed 38 degrees
    if (maxUphill > 38.0f) {
        result.valid = false;
        ss << "FAIL: Max uphill slope (" << maxUphill << " deg) exceeds climb limit (38 deg)!\n";
    } else {
        ss << "PASS: Max uphill slope (" << maxUphill << " deg) is safely climbable.\n";
    }

    // 2. Scan fuel canisters
    const auto& items = terrain.getItems();
    std::vector<float> fuelXs;
    for (const auto& item : items) {
        if (item.type == ItemType::FUEL_CANISTER) {
            fuelXs.push_back(item.position.x);
            // Verify stability of fuel placement (slope magnitude must be <= 0.35)
            float slopeAtFuel = std::abs(terrain.getSlope(item.position.x));
            if (slopeAtFuel > 0.35f) {
                result.valid = false;
                ss << "FAIL: Fuel at x=" << item.position.x << " sits on steep slope (" << slopeAtFuel << ")!\n";
            }
        }
    }
    result.fuelCanisterCount = static_cast<int>(fuelXs.size());

    // 3. Verify maximum distance between consecutive fuels
    float maxInterval = 0.0f;
    float prevX = 0.0f;
    for (float fx : fuelXs) {
        float interval = fx - prevX;
        if (interval > maxInterval) maxInterval = interval;
        prevX = fx;
    }
    result.maxFuelIntervalMeters = maxInterval;

    // Fuel capacity is 100 units; at full throttle average drain is ~4.7 units/sec.
    // At minimum climbing speed 10 m/s, max safe distance is ~165 meters.
    if (maxInterval > 165.0f) {
        result.valid = false;
        ss << "FAIL: Max fuel interval (" << maxInterval << "m) exceeds safe limit (165m)!\n";
    } else {
        ss << "PASS: Max fuel interval (" << maxInterval << "m) guarantees reachability.\n";
    }

    // 4. Physical fuel consumption simulation between each fuel pair
    float simFuel = 100.0f;
    float minRemaining = 100.0f;
    float currentX = 0.0f;
    size_t fuelIdx = 0;

    while (currentX < 1800.0f) {
        float dt = 0.05f;
        float slope = terrain.getSlope(currentX);
        // Vehicle speed modeled by grade resistance: calibrated frame speed (~10.5 m/s flat, ~6.6 m/s on climb)
        float speed = std::clamp(10.5f - slope * 7.2f, 5.4f, 14.4f);
        currentX += speed * dt;

        // Drain rate at full throttle (calibrated to match Vehicle fuel burn per meter)
        simFuel -= (1.2f + 3.5f) * 0.60f * dt;
        if (simFuel < minRemaining) minRemaining = simFuel;

        if (simFuel <= 0.0f) {
            result.valid = false;
            ss << "FAIL: Simulated run ran out of fuel at x=" << currentX << "m!\n";
            break;
        }

        // Collect fuel
        if (fuelIdx < fuelXs.size() && currentX >= fuelXs[fuelIdx]) {
            simFuel = 100.0f;
            fuelIdx++;
        }
    }
    result.minFuelRemainingPercent = minRemaining;

    if (minRemaining > 10.0f) {
        ss << "PASS: Simulated run completed 1800m with min fuel buffer of " << minRemaining << "%.\n";
    } else if (result.valid) {
        ss << "WARN: Low fuel buffer (" << minRemaining << "%) but reachable.\n";
    }

    result.report = ss.str();
    return result;
}

} // namespace Physics
