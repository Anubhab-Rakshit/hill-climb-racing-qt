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
            // Level 1: 550m
            {1, "THE FOOTHILLS", "Gentle Rolling Crests", 550.0f, 2000, {
                {1, 175.0f, "Rolling Meadows", "Introductory gentle waves", 500, false},
                {2, 350.0f, "Sunny Crest", "First launch kicker", 1000, false},
                {3, 550.0f, "Foothills Summit", "Level 1 Finish Line!", 2000, true}
            }},
            // Level 2: 750m
            {2, "SKI JUMP MEADOWS", "High-Speed Launch Ramps", 750.0f, 4000, {
                {1, 220.0f, "Lower Glade", "Rolling entry mounds", 800, false},
                {2, 480.0f, "Twin Ramps", "High-speed airtime kickers", 1500, false},
                {3, 750.0f, "Ski Jump Finish", "Level 2 Finish Line!", 4000, true}
            }},
            // Level 3: 1000m
            {3, "ROLLERCOASTER RIDGES", "Technical Rhythm Moguls", 1000.0f, 7000, {
                {1, 280.0f, "Rhythm Moguls", "Washboard chatter bumps", 1200, false},
                {2, 620.0f, "Steep Incline", "Technical hill climb", 2500, false},
                {3, 1000.0f, "Ridges Finish", "Level 3 Finish Line!", 7000, true}
            }},
            // Level 4: 1300m
            {4, "THE BIG LEAP", "Massive Valley Chasm Flight", 1300.0f, 11000, {
                {1, 350.0f, "Gorge Entry", "Deepening terrain undulations", 1800, false},
                {2, 800.0f, "Chasm Lip", "Massive valley flight kicker", 3800, false},
                {3, 1300.0f, "The Big Leap Finish", "Level 4 Finish Line!", 11000, true}
            }},
            // Level 5: 1650m
            {5, "MOUNT STEEPCLEARING", "Grand Finale & Desert Unlock!", 1650.0f, 20000, {
                {1, 420.0f, "Base Camp", "Steep technical incline", 2500, false},
                {2, 1000.0f, "High Escarpment", "Severe mountain wall", 6000, false},
                {3, 1650.0f, "Summit Grand Finale!", "Desert Region Unlocked!", 20000, true}
            }}
        },
        // Fallback checkpoints
        {
            {1, 175.0f, "Rolling Meadows", "Warmup", 500, false},
            {2, 350.0f, "Sunny Crest", "Kicker", 1000, false},
            {3, 550.0f, "Foothills Summit", "Finish", 2000, true}
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
            {1, "SHIFTING SANDS", "Windward Dune Entry", 550.0f, 2500, {
                {1, 175.0f, "Sand Ripple Rise", "Low traction ripples", 600, false},
                {2, 350.0f, "Windward Ridge", "Sharp sand crest", 1200, false},
                {3, 550.0f, "Oasis Gate", "Level 1 Finish Line!", 2500, true}
            }},
            {2, "DUNE KICKERS", "Sharp Crest Drops & Airtime", 750.0f, 5000, {
                {1, 220.0f, "Slip Crest", "Slip-face drop-off", 1000, false},
                {2, 480.0f, "Double Drop", "Twin sand kickers", 2000, false},
                {3, 750.0f, "Dune Jump Finish", "Level 2 Finish Line!", 5000, true}
            }},
            {3, "SAHARA CANYONS", "Deep Sandy Gullies & Speed Runs", 1000.0f, 8000, {
                {1, 280.0f, "Canyon Rim", "Deep gully plunge", 1500, false},
                {2, 620.0f, "Gully Speedrun", "High-speed momentum run", 3000, false},
                {3, 1000.0f, "Canyon Exit", "Level 3 Finish Line!", 8000, true}
            }},
            {4, "SLIP-FACE RIDGE", "Razor Crest Gap Leaps", 1300.0f, 13000, {
                {1, 350.0f, "Razor Edge", "Abrupt knife-edge peaks", 2200, false},
                {2, 800.0f, "Chasm Gap", "High-altitude dune flight", 4500, false},
                {3, 1300.0f, "Slip-Face Summit", "Level 4 Finish Line!", 13000, true}
            }},
            {5, "DUNES OF NO RETURN", "Dune Master & Arctic Unlock!", 1650.0f, 22000, {
                {1, 420.0f, "Endless Sand", "Steep 30-deg sand climb", 3000, false},
                {2, 1000.0f, "Mega-Dune Wall", "Massive parabolic ridge", 7000, false},
                {3, 1650.0f, "Sahara Master Finish!", "Arctic Region Unlocked!", 22000, true}
            }}
        },
        {
            {1, 175.0f, "Sand Ripple Rise", "Ripples", 600, false},
            {2, 350.0f, "Windward Ridge", "Ridge", 1200, false},
            {3, 550.0f, "Oasis Gate", "Finish", 2500, true}
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
            {1, "FROZEN GLADE", "Low-Friction Ice Entry", 550.0f, 3000, {
                {1, 175.0f, "Frosty Flat", "Low friction glide", 700, false},
                {2, 350.0f, "Slick Rise", "Ice ramp launch", 1400, false},
                {3, 550.0f, "Glade Threshold", "Level 1 Finish Line!", 3000, true}
            }},
            {2, "CREVASSE LEAPS", "Deep Glacial Trench Jumps", 750.0f, 5500, {
                {1, 220.0f, "Ice Shelf", "Glacial ledge jump", 1100, false},
                {2, 480.0f, "Chasm Launch", "Trench gap flight", 2200, false},
                {3, 750.0f, "Crevasse Clear", "Level 2 Finish Line!", 5500, true}
            }},
            {3, "BLIZZARD PEAKS", "High-Frequency Ice Chatter", 1000.0f, 9000, {
                {1, 280.0f, "Chatter Ridge", "Vibrating ice washboards", 1600, false},
                {2, 620.0f, "Permafrost Moguls", "Technical moguls", 3500, false},
                {3, 1000.0f, "Peak Plateau", "Level 3 Finish Line!", 9000, true}
            }},
            {4, "ICEFALL ESCARPMENT", "High-Slope Glacial Ascent", 1300.0f, 14000, {
                {1, 350.0f, "Icefall Step", "Steep ice ramp", 2400, false},
                {2, 800.0f, "Glacial Wall", "Severe slick climb", 5000, false},
                {3, 1300.0f, "Escarpment Rim", "Level 4 Finish Line!", 14000, true}
            }},
            {5, "GLACIER SUMMIT", "Arctic Explorer & Mountain Unlock!", 1650.0f, 24000, {
                {1, 420.0f, "High Shelf", "Freezing alpine crests", 3200, false},
                {2, 1000.0f, "Zero-G Ice Jump", "High-altitude glacial leap", 7500, false},
                {3, 1650.0f, "Arctic Master Summit!", "Mountain Region Unlocked!", 24000, true}
            }}
        },
        {
            {1, 175.0f, "Frosty Flat", "Glade", 700, false},
            {2, 350.0f, "Slick Rise", "Ramp", 1400, false},
            {3, 550.0f, "Glade Threshold", "Finish", 3000, true}
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
            {1, "ALPINE FOOTHILLS", "Craggy Boulder Fields", 550.0f, 3500, {
                {1, 175.0f, "Granite Steps", "Rough rocky ledges", 800, false},
                {2, 350.0f, "Pine Ridge", "Springboard ramp", 1600, false},
                {3, 550.0f, "Timber Threshold", "Level 1 Finish Line!", 3500, true}
            }},
            {2, "TIMBERLINE CHASM", "Alpine Steps & Springboards", 750.0f, 6000, {
                {1, 220.0f, "Crag Launch", "Jagged rock kicker", 1200, false},
                {2, 480.0f, "Ravine Flight", "Deep gorge flight", 2500, false},
                {3, 750.0f, "High Pass", "Level 2 Finish Line!", 6000, true}
            }},
            {3, "THE NEEDLE CREST", "Jagged Vertical Climbs", 1000.0f, 10000, {
                {1, 280.0f, "Needle Base", "Steep 28-deg rock climb", 1800, false},
                {2, 620.0f, "Knife Edge", "Sharp jagged ridgeline", 4000, false},
                {3, 1000.0f, "Needle Pinnacle", "Level 3 Finish Line!", 10000, true}
            }},
            {4, "AVALANCHE GORGE", "High-Speed Chasm Flight", 1300.0f, 16000, {
                {1, 350.0f, "Gorge Plunge", "Steep downhill acceleration", 2600, false},
                {2, 800.0f, "Avalanche Launchpad", "Massive rock springboard", 5500, false},
                {3, 1300.0f, "Gorge Clear", "Level 4 Finish Line!", 16000, true}
            }},
            {5, "SUMMIT PRECIPICE", "Mountain Legend & Moon Unlock!", 1650.0f, 26000, {
                {1, 420.0f, "Cloud Base", "Severe 34-deg granite incline", 3500, false},
                {2, 1000.0f, "Granite Cliff Wall", "Precision throttle wall", 8000, false},
                {3, 1650.0f, "Mount Everest Peak!", "Moon Region Unlocked!", 26000, true}
            }}
        },
        {
            {1, 175.0f, "Granite Steps", "Ledges", 800, false},
            {2, 350.0f, "Pine Ridge", "Springboard", 1600, false},
            {3, 550.0f, "Timber Threshold", "Finish", 3500, true}
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
            {1, "SEA OF TRANQUILITY", "Gentle Lunar Low-G Dust", 550.0f, 4000, {
                {1, 175.0f, "Regolith Rise", "Floaty low-g swells", 900, false},
                {2, 350.0f, "Float Mound", "Zero-G liftoff ramp", 1800, false},
                {3, 550.0f, "Tranquility Station", "Level 1 Finish Line!", 4000, true}
            }},
            {2, "IMPACT BASIN", "Deep Parabolic Crater Bowls", 750.0f, 7000, {
                {1, 220.0f, "Crater Rim", "Deep crater bowl plunge", 1400, false},
                {2, 480.0f, "Basin Bowl Launch", "Massive low-g orbital ramp", 3000, false},
                {3, 750.0f, "Crater Exit", "Level 2 Finish Line!", 7000, true}
            }},
            {3, "APENNINE RIDGE", "Massive Zero-G Launchpads", 1000.0f, 12000, {
                {1, 280.0f, "Apennine Step", "Stair-step crater walls", 2000, false},
                {2, 620.0f, "Orbital Launchpad", "8-second airtime launchpad", 4500, false},
                {3, 1000.0f, "Lunar Escarpment", "Level 3 Finish Line!", 12000, true}
            }},
            {4, "FAR SIDE RIFTS", "Multi-Ridge Orbital Leaps", 1300.0f, 18000, {
                {1, 350.0f, "Rift Entry", "Narrow dark-side ravines", 3000, false},
                {2, 800.0f, "Sub-Orbital Leap", "Multi-ridge chasm leap", 6500, false},
                {3, 1300.0f, "Dark Side Gate", "Level 4 Finish Line!", 18000, true}
            }},
            {5, "LUNAR ORBITAL ARC", "Lunar Champion & Inferno Unlock!", 1650.0f, 30000, {
                {1, 420.0f, "Orbital Trajectory Base", "Floaty vacuum ridges", 4000, false},
                {2, 1000.0f, "Grand Canyon Flight", "100-meter flight leap", 10000, false},
                {3, 1650.0f, "Apollo Landing Site!", "Inferno Peaks Unlocked!", 30000, true}
            }}
        },
        {
            {1, 175.0f, "Regolith Rise", "Low-G", 900, false},
            {2, 350.0f, "Float Mound", "Launch", 1800, false},
            {3, 550.0f, "Tranquility Station", "Finish", 4000, true}
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
            {1, "OBSIDIAN TRAIL", "Hardened Basalt Floor", 550.0f, 4500, {
                {1, 175.0f, "Basalt Rise", "Heavy gravity rises", 1000, false},
                {2, 350.0f, "Ash Plateau", "Thermal shock ramp", 2000, false},
                {3, 550.0f, "Obsidian Gate", "Level 1 Finish Line!", 4500, true}
            }},
            {2, "MAGMA VENT STEPS", "Thermal Shock Rises & Ramps", 750.0f, 8000, {
                {1, 220.0f, "Vent Shelf", "Basalt steps", 1600, false},
                {2, 480.0f, "Thermal Kicker", "High-power launch kicker", 3500, false},
                {3, 750.0f, "Vent Ridge Clear", "Level 2 Finish Line!", 8000, true}
            }},
            {3, "CALDERA WALL", "Sunken Magma Basin Ascent", 1000.0f, 14000, {
                {1, 280.0f, "Molten Basin Entry", "Sunken magma crater bowl", 2500, false},
                {2, 620.0f, "Caldera Wall Ascent", "Steep 32-deg magma ascent", 5500, false},
                {3, 1000.0f, "Caldera Rim", "Level 3 Finish Line!", 14000, true}
            }},
            {4, "SULPHUR ESCARPMENT", "Basalt Pillar Leaps", 1300.0f, 20000, {
                {1, 350.0f, "Sulphur Gully", "Basalt pillar terrain", 3500, false},
                {2, 800.0f, "Pillar Launch", "Pillar gap flight", 7500, false},
                {3, 1300.0f, "Escarpment Peak", "Level 4 Finish Line!", 20000, true}
            }},
            {5, "CALDERA PEAK", "Ultimate Hill Climb Master!", 1650.0f, 35000, {
                {1, 420.0f, "Inferno Wall Base", "Relentless 35-deg ascent", 5000, false},
                {2, 1000.0f, "Magma Summit Wall", "Extreme thermal wall", 12000, false},
                {3, 1650.0f, "The Ultimate Champion!", "Stage Mastered!", 35000, true}
            }}
        },
        {
            {1, 175.0f, "Basalt Rise", "Rises", 1000, false},
            {2, 350.0f, "Ash Plateau", "Plateau", 2000, false},
            {3, 550.0f, "Obsidian Gate", "Finish", 4500, true}
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

const LevelSpec& BiomeRegistry::getLevelSpec(const std::string& biomeId, int levelIndex) {
    const auto& b = getBiome(biomeId);
    return b.getLevel(levelIndex);
}

TerrainValidator::ValidationResult TerrainValidator::validateBiome(const std::string& biomeId) {
    ValidationResult result;
    std::ostringstream ss;
    ss << "=== 5-LEVEL REGIONAL VALIDATION REPORT FOR: " << biomeId << " ===\n";

    float worstUphill = 0.0f;
    float worstDownhill = 0.0f;
    float worstInterval = 0.0f;
    float worstFuelBuffer = 100.0f;
    int totalFuelCanisters = 0;

    for (int lvl = 1; lvl <= 5; ++lvl) {
        Terrain terrain;
        terrain.setBiome(biomeId, lvl);
        float trackLen = terrain.currentLevelLength();

        float maxUphill = 0.0f;
        float maxDownhill = 0.0f;
        for (float x = 0.0f; x <= trackLen + 40.0f; x += 0.5f) {
            float slope = terrain.getSlope(x);
            float deg = std::abs(std::atan(slope) * (180.0f / 3.14159265f));
            if (slope > 0.0f && deg > maxUphill) maxUphill = deg;
            if (slope < 0.0f && deg > maxDownhill) maxDownhill = deg;
        }
        if (maxUphill > worstUphill) worstUphill = maxUphill;
        if (maxDownhill > worstDownhill) worstDownhill = maxDownhill;

        if (maxUphill > 38.0f) {
            result.valid = false;
            ss << "FAIL: Level " << lvl << " Max uphill slope (" << maxUphill << " deg) exceeds climb limit (38 deg)!\n";
        }

        const auto& items = terrain.getItems();
        std::vector<float> fuelXs;
        for (const auto& item : items) {
            if (item.type == ItemType::FUEL_CANISTER) {
                fuelXs.push_back(item.position.x);
                float slopeAtFuel = std::abs(terrain.getSlope(item.position.x));
                if (slopeAtFuel > 0.35f) {
                    result.valid = false;
                    ss << "FAIL: Level " << lvl << " Fuel at x=" << item.position.x << " sits on steep slope (" << slopeAtFuel << ")!\n";
                }
            }
        }
        totalFuelCanisters += static_cast<int>(fuelXs.size());

        float maxInterval = 0.0f;
        float prevX = 0.0f;
        for (float fx : fuelXs) {
            float interval = fx - prevX;
            if (interval > maxInterval) maxInterval = interval;
            prevX = fx;
        }
        if (maxInterval > worstInterval) worstInterval = maxInterval;

        if (maxInterval > 520.0f) {
            result.valid = false;
            ss << "FAIL: Level " << lvl << " Max fuel interval (" << maxInterval << "m) exceeds tactical safe limit (520m)!\n";
        }

        float simFuel = 100.0f;
        float minRemaining = 100.0f;
        float currentX = 0.0f;
        size_t fuelIdx = 0;

        // Upgraded vehicle performance factor for higher levels (player levels up mechanics across stages)
        float upgradeSpeedBonus = 1.0f + 0.08f * (lvl - 1);

        while (currentX < trackLen) {
            float dt = 0.05f;
            float slope = terrain.getSlope(currentX);
            float baseSpeed = std::clamp(14.0f - slope * 9.5f, 7.0f, 19.5f);
            float speed = baseSpeed * upgradeSpeedBonus;
            currentX += speed * dt;

            // Tactical driving: coast when downhill slope < -0.04
            if (slope >= -0.04f) {
                float load = std::clamp(0.70f + std::max(0.0f, slope) * 2.20f - std::min(0.0f, slope) * 0.35f + 0.85f, 0.60f, 2.20f);
                float drainRate = (3.50f + 1.10f * load) * 0.92f;
                simFuel -= drainRate * dt;
            }
            if (simFuel < minRemaining) minRemaining = simFuel;

            if (simFuel <= 0.0f) {
                result.valid = false;
                ss << "FAIL: Level " << lvl << " Simulated tactical run ran out of fuel at x=" << currentX << "m!\n";
                break;
            }

            if (fuelIdx < fuelXs.size() && currentX >= fuelXs[fuelIdx]) {
                simFuel = 100.0f;
                fuelIdx++;
            }
        }
        if (minRemaining < worstFuelBuffer) worstFuelBuffer = minRemaining;
    }

    result.maxUphillSlopeDeg = worstUphill;
    result.maxDownhillSlopeDeg = worstDownhill;
    result.maxFuelIntervalMeters = worstInterval;
    result.fuelCanisterCount = totalFuelCanisters;
    result.minFuelRemainingPercent = worstFuelBuffer;

    if (result.valid) {
        ss << "PASS: All 5 Levels passed validation! Max Uphill: " << worstUphill
           << " deg, Max Downhill: " << worstDownhill
           << " deg, Max Interval: " << worstInterval
           << "m, Min Fuel Buffer: " << worstFuelBuffer << "%\n";
    }

    result.report = ss.str();
    return result;
}

} // namespace Physics
