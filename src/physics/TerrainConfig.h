#pragma once

#include "PhysicsTypes.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Physics {

struct CheckpointInfo {
    int index;                 // 1 to 5
    float distance;            // meters (e.g. 250, 550, 900, 1300, 1750)
    std::string name;          // e.g. "THE FOOTHILLS"
    std::string subtitle;      // e.g. "Gentle Rolling Crests"
    int coinReward;            // bonus coins
    bool isFinishLine;         // true for final checkpoint 5
};

struct BiomeSpec {
    std::string id;
    std::string name;
    std::string tagline;
    std::string nextBiomeId;   // Biome unlocked upon clearing checkpoint 5
    int unlockCostCoins;       // Alternative coin unlock price

    float gravity;             // m/s^2
    float friction;            // traction multiplier
    float baseHeightAmp;       // amplitude scale
    float baseFrequency;       // spatial frequency scale

    // Sky & Parallax palette
    uint32_t skyTopColor;
    uint32_t skyBottomColor;
    uint32_t mountainColor;

    // Terrain column scanline palette
    uint32_t crestColor;       // Top 1px ridge edge
    uint32_t topColor;         // Upper turf/surface
    uint32_t bodyColor;        // Sub-surface turf
    uint32_t soilColor;        // Loam / sediment
    uint32_t bedrockColor;     // Deep subterranean strata

    std::vector<CheckpointInfo> checkpoints;
};

class BiomeRegistry {
public:
    static const BiomeSpec& getBiome(const std::string& id);
    static const std::vector<BiomeSpec>& getAllBiomes();
    static bool exists(const std::string& id);
};

class TerrainValidator {
public:
    struct ValidationResult {
        bool valid = true;
        float maxUphillSlopeDeg = 0.0f;
        float maxDownhillSlopeDeg = 0.0f;
        int fuelCanisterCount = 0;
        float maxFuelIntervalMeters = 0.0f;
        float minFuelRemainingPercent = 100.0f;
        std::string report;
    };

    /**
     * @brief Performs comprehensive mathematical and physical validation of terrain traversal.
     * Verifies that all slopes are climbable, fuel canisters are reachable, and fuel economy is fair.
     */
    static ValidationResult validateBiome(const std::string& biomeId);
};

} // namespace Physics
