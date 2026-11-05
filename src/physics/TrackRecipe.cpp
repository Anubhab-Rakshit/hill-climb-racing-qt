#include "TrackRecipe.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

namespace Physics {

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct FeatureSpec {
    TerrainFeature feature;
    float amplitude;
    float frequency;
};

struct ZoneSpec {
    float startX;
    float endX;
    SurfaceType material;
    float blendWidth;
};

const std::array<float, 26> kSectionLengths = {
    60.0f, 70.0f, 110.0f, 100.0f, 100.0f, 100.0f, 140.0f, 120.0f, 120.0f,
    140.0f, 120.0f, 150.0f, 150.0f, 200.0f, 150.0f, 150.0f, 200.0f,
    200.0f, 200.0f, 200.0f, 200.0f, 300.0f, 300.0f, 400.0f, 500.0f, 500.0f
};

float clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

float smoothStep(float t) {
    const float x = clamp01(t);
    return x * x * (3.0f - 2.0f * x);
}

float smoothWindow(float t) {
    const float s = std::sin(clamp01(t) * kPi);
    return s * s;
}

float riseTo(float t, float riseFraction) {
    if (t < riseFraction) {
        return smoothStep(t / std::max(riseFraction, 0.001f));
    }
    return 1.0f;
}

float dropTo(float t, float dropFraction) {
    if (t < dropFraction) {
        return -smoothStep(t / std::max(dropFraction, 0.001f));
    }
    return -1.0f;
}

float bump(float t, float center, float halfWidth) {
    const float d = std::abs(t - center) / std::max(halfWidth, 0.001f);
    if (d >= 1.0f) {
        return 0.0f;
    }
    return 0.5f * (1.0f + std::cos(d * kPi));
}

uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float hashSigned(int32_t x, uint32_t seed) {
    const uint32_t h = hash32(static_cast<uint32_t>(x) ^ seed);
    return static_cast<float>(h & 0xFFFFU) / 32767.5f - 1.0f;
}

TrackRecipe makeRecipe(const char* biomeId,
                       uint32_t seed,
                       float baseAmplitude,
                       float baseFrequency,
                       float roughness,
                       std::initializer_list<FeatureSpec> features,
                       std::initializer_list<ZoneSpec> zones) {
    TrackRecipe recipe;
    recipe.biomeId = biomeId;
    recipe.seed = seed;
    recipe.length = 5000.0f;
    recipe.baseAmplitude = baseAmplitude;
    recipe.baseFrequency = baseFrequency;
    recipe.roughness = roughness;

    float x = 20.0f;
    size_t index = 0;
    for (const auto& spec : features) {
        if (index >= kSectionLengths.size()) {
            break;
        }
        TrackSection section;
        section.startX = x;
        section.length = kSectionLengths[index];
        section.feature = spec.feature;
        section.amplitude = spec.amplitude;
        section.frequency = spec.frequency;
        recipe.sections.push_back(section);
        x += section.length;
        ++index;
    }
    if (x < recipe.length) {
        TrackSection tail;
        tail.startX = x;
        tail.length = recipe.length - x;
        tail.feature = TerrainFeature::FLAT;
        recipe.sections.push_back(tail);
    }

    for (const auto& zone : zones) {
        recipe.surfaceZones.push_back({zone.startX, zone.endX, zone.material, zone.blendWidth});
    }
    return recipe;
}

std::vector<TrackRecipe> buildRecipes() {
    std::vector<TrackRecipe> recipes;
    recipes.reserve(9);

    recipes.push_back(makeRecipe(
        "countryside", 0xC0A1u, 2.0f, 0.020f, 0.90f,
        {
            {TerrainFeature::ROLLERS, 1.8f, 1.0f},
            {TerrainFeature::TABLETOP, 2.8f, 1.0f},
            {TerrainFeature::ROLLERS, 2.4f, 1.2f},
            {TerrainFeature::WHOOPS, 1.1f, 3.0f},
            {TerrainFeature::RIDGE, 3.2f, 1.0f},
            {TerrainFeature::PLATEAU, 3.8f, 1.0f},
            {TerrainFeature::WASHBOARD, 0.6f, 9.0f},
            {TerrainFeature::ROLLERS, 2.8f, 1.1f},
            {TerrainFeature::STEP_UP, 2.2f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 1.8f, 1.0f},
            {TerrainFeature::DROP_OFF, 2.4f, 1.0f},
            {TerrainFeature::RAVINE, 3.6f, 1.0f},
            {TerrainFeature::DOUBLE, 2.8f, 1.0f},
            {TerrainFeature::ROLLERS, 3.2f, 1.3f},
            {TerrainFeature::TABLETOP, 3.8f, 1.0f},
            {TerrainFeature::CLIFF, 2.8f, 1.0f},
            {TerrainFeature::RIDGE, 4.0f, 1.0f},
            {TerrainFeature::PLATEAU, 4.6f, 1.0f},
            {TerrainFeature::WHOOPS, 1.8f, 3.5f},
            {TerrainFeature::STEP_UP, 3.8f, 1.0f},
            {TerrainFeature::RAVINE, 4.6f, 1.0f},
            {TerrainFeature::ROLLERS, 4.8f, 1.2f},
            {TerrainFeature::DOUBLE, 4.8f, 1.0f},
            {TerrainFeature::RIDGE, 5.4f, 1.0f},
            {TerrainFeature::ROLLERS, 5.0f, 1.4f},
            {TerrainFeature::PLATEAU, 4.8f, 1.0f}
        },
        {
            {0.0f, 600.0f, SurfaceType::GRASS, 6.0f},
            {600.0f, 700.0f, SurfaceType::DIRT, 6.0f},
            {700.0f, 950.0f, SurfaceType::GRASS, 6.0f},
            {950.0f, 1100.0f, SurfaceType::GRAVEL, 5.0f},
            {1100.0f, 1400.0f, SurfaceType::DIRT, 6.0f},
            {1400.0f, 1750.0f, SurfaceType::GRASS, 6.0f},
            {1750.0f, 2000.0f, SurfaceType::GRAVEL, 5.0f},
            {2000.0f, 2400.0f, SurfaceType::DIRT, 6.0f},
            {2400.0f, 3000.0f, SurfaceType::GRASS, 6.0f},
            {3000.0f, 3400.0f, SurfaceType::GRAVEL, 5.0f},
            {3400.0f, 4000.0f, SurfaceType::GRASS, 6.0f},
            {4000.0f, 4500.0f, SurfaceType::DIRT, 6.0f},
            {4500.0f, 5000.0f, SurfaceType::GRASS, 6.0f}
        }));

    recipes.push_back(makeRecipe(
        "desert", 0xD35E27u, 2.4f, 0.018f, 1.10f,
        {
            {TerrainFeature::DUNE_CREST, 4.0f, 1.0f},
            {TerrainFeature::DUNE_CREST, 5.5f, 1.0f},
            {TerrainFeature::ROLLERS, 3.0f, 1.1f},
            {TerrainFeature::WHOOPS, 1.4f, 3.0f},
            {TerrainFeature::DUNE_CREST, 6.0f, 1.0f},
            {TerrainFeature::RAVINE, 3.0f, 1.0f},
            {TerrainFeature::WASHBOARD, 0.5f, 10.0f},
            {TerrainFeature::DUNE_CREST, 6.5f, 1.0f},
            {TerrainFeature::STEP_UP, 2.5f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 1.5f, 1.0f},
            {TerrainFeature::DROP_OFF, 3.5f, 1.0f},
            {TerrainFeature::DUNE_CREST, 7.0f, 1.0f},
            {TerrainFeature::DOUBLE, 3.0f, 1.0f},
            {TerrainFeature::ROLLERS, 4.0f, 1.2f},
            {TerrainFeature::TABLETOP, 4.5f, 1.0f},
            {TerrainFeature::CLIFF, 3.0f, 1.0f},
            {TerrainFeature::DUNE_CREST, 7.5f, 1.0f},
            {TerrainFeature::PLATEAU, 4.0f, 1.0f},
            {TerrainFeature::WHOOPS, 2.0f, 3.5f},
            {TerrainFeature::STEP_UP, 4.0f, 1.0f},
            {TerrainFeature::RAVINE, 5.0f, 1.0f},
            {TerrainFeature::DUNE_CREST, 8.0f, 1.0f},
            {TerrainFeature::DOUBLE, 5.0f, 1.0f},
            {TerrainFeature::RIDGE, 4.5f, 1.0f},
            {TerrainFeature::ROLLERS, 5.5f, 1.3f},
            {TerrainFeature::DUNE_CREST, 7.0f, 1.0f}
        },
        {
            {0.0f, 650.0f, SurfaceType::SAND, 8.0f},
            {650.0f, 800.0f, SurfaceType::ROCK, 5.0f},
            {800.0f, 1600.0f, SurfaceType::SAND, 8.0f},
            {1600.0f, 1800.0f, SurfaceType::ROCK, 5.0f},
            {1800.0f, 3200.0f, SurfaceType::SAND, 8.0f},
            {3200.0f, 3400.0f, SurfaceType::GRAVEL, 5.0f},
            {3400.0f, 4300.0f, SurfaceType::SAND, 8.0f},
            {4300.0f, 4500.0f, SurfaceType::ROCK, 5.0f},
            {4500.0f, 5000.0f, SurfaceType::SAND, 8.0f}
        }));

    recipes.push_back(makeRecipe(
        "arctic", 0xA2C71Cu, 2.1f, 0.019f, 0.80f,
        {
            {TerrainFeature::ICE_SHELF, 2.5f, 1.0f},
            {TerrainFeature::ROLLERS, 2.0f, 1.0f},
            {TerrainFeature::ICE_SHELF, 3.5f, 1.0f},
            {TerrainFeature::WASHBOARD, 0.4f, 8.0f},
            {TerrainFeature::RIDGE, 3.0f, 1.0f},
            {TerrainFeature::PLATEAU, 3.5f, 1.0f},
            {TerrainFeature::ICE_SHELF, 4.0f, 1.0f},
            {TerrainFeature::RAVINE, 4.5f, 1.0f},
            {TerrainFeature::STEP_UP, 2.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 1.5f, 1.0f},
            {TerrainFeature::DROP_OFF, 3.0f, 1.0f},
            {TerrainFeature::ICE_SHELF, 5.0f, 1.0f},
            {TerrainFeature::DOUBLE, 2.5f, 1.0f},
            {TerrainFeature::ROLLERS, 3.0f, 1.1f},
            {TerrainFeature::TABLETOP, 3.5f, 1.0f},
            {TerrainFeature::CLIFF, 3.5f, 1.0f},
            {TerrainFeature::ICE_SHELF, 5.5f, 1.0f},
            {TerrainFeature::PLATEAU, 4.0f, 1.0f},
            {TerrainFeature::WHOOPS, 1.5f, 3.0f},
            {TerrainFeature::STEP_UP, 3.5f, 1.0f},
            {TerrainFeature::RAVINE, 5.0f, 1.0f},
            {TerrainFeature::ICE_SHELF, 6.0f, 1.0f},
            {TerrainFeature::DOUBLE, 4.0f, 1.0f},
            {TerrainFeature::RIDGE, 5.0f, 1.0f},
            {TerrainFeature::ROLLERS, 5.0f, 1.2f},
            {TerrainFeature::ICE_SHELF, 5.5f, 1.0f}
        },
        {
            {0.0f, 800.0f, SurfaceType::SNOW, 7.0f},
            {800.0f, 1000.0f, SurfaceType::ICE, 4.0f},
            {1000.0f, 1500.0f, SurfaceType::SNOW, 7.0f},
            {1500.0f, 1800.0f, SurfaceType::ICE, 4.0f},
            {1800.0f, 2200.0f, SurfaceType::SNOW, 7.0f},
            {2200.0f, 2400.0f, SurfaceType::ROCK, 4.0f},
            {2400.0f, 2800.0f, SurfaceType::SNOW, 7.0f},
            {2800.0f, 3100.0f, SurfaceType::ICE, 4.0f},
            {3100.0f, 3800.0f, SurfaceType::SNOW, 7.0f},
            {3800.0f, 4100.0f, SurfaceType::ICE, 4.0f},
            {4100.0f, 4400.0f, SurfaceType::ROCK, 4.0f},
            {4400.0f, 5000.0f, SurfaceType::SNOW, 7.0f}
        }));

    recipes.push_back(makeRecipe(
        "mountain", 0x4D6F75u, 2.5f, 0.018f, 1.20f,
        {
            {TerrainFeature::ROCK_STEPS, 2.0f, 1.0f},
            {TerrainFeature::RIDGE, 3.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.5f, 1.0f},
            {TerrainFeature::WHOOPS, 1.2f, 3.0f},
            {TerrainFeature::RIDGE, 4.0f, 1.0f},
            {TerrainFeature::PLATEAU, 4.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 3.0f, 1.0f},
            {TerrainFeature::RAVINE, 4.0f, 1.0f},
            {TerrainFeature::STEP_UP, 3.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.5f, 1.0f},
            {TerrainFeature::DROP_OFF, 3.0f, 1.0f},
            {TerrainFeature::RIDGE, 5.0f, 1.0f},
            {TerrainFeature::DOUBLE, 3.0f, 1.0f},
            {TerrainFeature::ROLLERS, 4.0f, 1.2f},
            {TerrainFeature::TABLETOP, 4.5f, 1.0f},
            {TerrainFeature::CLIFF, 4.0f, 1.0f},
            {TerrainFeature::RIDGE, 5.5f, 1.0f},
            {TerrainFeature::PLATEAU, 5.0f, 1.0f},
            {TerrainFeature::WHOOPS, 2.0f, 3.5f},
            {TerrainFeature::STEP_UP, 4.5f, 1.0f},
            {TerrainFeature::RAVINE, 5.5f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 4.0f, 1.0f},
            {TerrainFeature::DOUBLE, 5.0f, 1.0f},
            {TerrainFeature::RIDGE, 6.0f, 1.0f},
            {TerrainFeature::ROLLERS, 5.5f, 1.3f},
            {TerrainFeature::PLATEAU, 5.5f, 1.0f}
        },
        {
            {0.0f, 500.0f, SurfaceType::ROCK, 5.0f},
            {500.0f, 700.0f, SurfaceType::GRAVEL, 5.0f},
            {700.0f, 1000.0f, SurfaceType::ROCK, 5.0f},
            {1000.0f, 1300.0f, SurfaceType::DIRT, 5.0f},
            {1300.0f, 1600.0f, SurfaceType::GRAVEL, 5.0f},
            {1600.0f, 2200.0f, SurfaceType::ROCK, 5.0f},
            {2200.0f, 2500.0f, SurfaceType::GRAVEL, 5.0f},
            {2500.0f, 3000.0f, SurfaceType::ROCK, 5.0f},
            {3000.0f, 3200.0f, SurfaceType::DIRT, 5.0f},
            {3200.0f, 3900.0f, SurfaceType::ROCK, 5.0f},
            {3900.0f, 4100.0f, SurfaceType::GRAVEL, 5.0f},
            {4100.0f, 5000.0f, SurfaceType::ROCK, 5.0f}
        }));

    recipes.push_back(makeRecipe(
        "moon", 0x30A17Cu, 2.8f, 0.016f, 0.75f,
        {
            {TerrainFeature::ROLLERS, 2.5f, 0.8f},
            {TerrainFeature::RAVINE, 3.5f, 1.0f},
            {TerrainFeature::DUNE_CREST, 4.0f, 1.0f},
            {TerrainFeature::WHOOPS, 1.0f, 2.5f},
            {TerrainFeature::RIDGE, 4.0f, 1.0f},
            {TerrainFeature::PLATEAU, 5.0f, 1.0f},
            {TerrainFeature::WASHBOARD, 0.5f, 7.0f},
            {TerrainFeature::RAVINE, 5.0f, 1.0f},
            {TerrainFeature::STEP_UP, 3.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.0f, 1.0f},
            {TerrainFeature::DROP_OFF, 4.0f, 1.0f},
            {TerrainFeature::DUNE_CREST, 6.0f, 1.0f},
            {TerrainFeature::DOUBLE, 4.0f, 1.0f},
            {TerrainFeature::ROLLERS, 5.0f, 1.0f},
            {TerrainFeature::TABLETOP, 5.5f, 1.0f},
            {TerrainFeature::CLIFF, 5.0f, 1.0f},
            {TerrainFeature::RIDGE, 6.0f, 1.0f},
            {TerrainFeature::PLATEAU, 6.0f, 1.0f},
            {TerrainFeature::WHOOPS, 2.0f, 3.0f},
            {TerrainFeature::STEP_UP, 5.0f, 1.0f},
            {TerrainFeature::RAVINE, 6.5f, 1.0f},
            {TerrainFeature::DUNE_CREST, 7.0f, 1.0f},
            {TerrainFeature::DOUBLE, 6.0f, 1.0f},
            {TerrainFeature::RIDGE, 7.0f, 1.0f},
            {TerrainFeature::ROLLERS, 6.0f, 1.1f},
            {TerrainFeature::PLATEAU, 6.5f, 1.0f}
        },
        {
            {0.0f, 700.0f, SurfaceType::REGOLITH, 8.0f},
            {700.0f, 850.0f, SurfaceType::ROCK, 5.0f},
            {850.0f, 1700.0f, SurfaceType::REGOLITH, 8.0f},
            {1700.0f, 1900.0f, SurfaceType::ROCK, 5.0f},
            {1900.0f, 3000.0f, SurfaceType::REGOLITH, 8.0f},
            {3000.0f, 3200.0f, SurfaceType::ROCK, 5.0f},
            {3200.0f, 4200.0f, SurfaceType::REGOLITH, 8.0f},
            {4200.0f, 4400.0f, SurfaceType::ROCK, 5.0f},
            {4400.0f, 5000.0f, SurfaceType::REGOLITH, 8.0f}
        }));

    recipes.push_back(makeRecipe(
        "volcano", 0x1F3A2Du, 2.6f, 0.017f, 1.15f,
        {
            {TerrainFeature::ASH_SLOPE, 2.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.5f, 1.0f},
            {TerrainFeature::ASH_SLOPE, 3.0f, 1.0f},
            {TerrainFeature::WHOOPS, 1.3f, 3.0f},
            {TerrainFeature::RIDGE, 4.0f, 1.0f},
            {TerrainFeature::PLATEAU, 4.0f, 1.0f},
            {TerrainFeature::ASH_SLOPE, 3.5f, 1.0f},
            {TerrainFeature::RAVINE, 4.5f, 1.0f},
            {TerrainFeature::STEP_UP, 3.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.5f, 1.0f},
            {TerrainFeature::DROP_OFF, 3.5f, 1.0f},
            {TerrainFeature::ASH_SLOPE, 5.0f, 1.0f},
            {TerrainFeature::DOUBLE, 3.5f, 1.0f},
            {TerrainFeature::ROLLERS, 4.5f, 1.2f},
            {TerrainFeature::TABLETOP, 5.0f, 1.0f},
            {TerrainFeature::CLIFF, 4.5f, 1.0f},
            {TerrainFeature::RIDGE, 5.5f, 1.0f},
            {TerrainFeature::PLATEAU, 5.0f, 1.0f},
            {TerrainFeature::WHOOPS, 2.0f, 3.5f},
            {TerrainFeature::STEP_UP, 5.0f, 1.0f},
            {TerrainFeature::RAVINE, 6.0f, 1.0f},
            {TerrainFeature::ASH_SLOPE, 6.0f, 1.0f},
            {TerrainFeature::DOUBLE, 5.5f, 1.0f},
            {TerrainFeature::RIDGE, 6.5f, 1.0f},
            {TerrainFeature::ROLLERS, 6.0f, 1.3f},
            {TerrainFeature::ASH_SLOPE, 6.0f, 1.0f}
        },
        {
            {0.0f, 400.0f, SurfaceType::ASH, 6.0f},
            {400.0f, 600.0f, SurfaceType::ROCK, 4.0f},
            {600.0f, 900.0f, SurfaceType::ASH, 6.0f},
            {900.0f, 1100.0f, SurfaceType::ROCK, 4.0f},
            {1100.0f, 1800.0f, SurfaceType::ASH, 6.0f},
            {1800.0f, 2000.0f, SurfaceType::ROCK, 4.0f},
            {2000.0f, 2800.0f, SurfaceType::ASH, 6.0f},
            {2800.0f, 3000.0f, SurfaceType::ROCK, 4.0f},
            {3000.0f, 3900.0f, SurfaceType::ASH, 6.0f},
            {3900.0f, 4100.0f, SurfaceType::ROCK, 4.0f},
            {4100.0f, 5000.0f, SurfaceType::ASH, 6.0f}
        }));

    recipes.push_back(makeRecipe(
        "neon", 0x4E30FFu, 2.3f, 0.021f, 0.70f,
        {
            {TerrainFeature::ROLLERS, 2.0f, 1.0f},
            {TerrainFeature::TABLETOP, 3.0f, 1.0f},
            {TerrainFeature::ROLLERS, 2.5f, 1.2f},
            {TerrainFeature::WASHBOARD, 0.6f, 9.0f},
            {TerrainFeature::RIDGE, 3.5f, 1.0f},
            {TerrainFeature::PLATEAU, 4.0f, 1.0f},
            {TerrainFeature::DOUBLE, 2.5f, 1.0f},
            {TerrainFeature::ROLLERS, 3.5f, 1.3f},
            {TerrainFeature::STEP_UP, 2.5f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.0f, 1.0f},
            {TerrainFeature::DROP_OFF, 3.0f, 1.0f},
            {TerrainFeature::RIDGE, 5.0f, 1.0f},
            {TerrainFeature::DOUBLE, 3.5f, 1.0f},
            {TerrainFeature::ROLLERS, 4.0f, 1.4f},
            {TerrainFeature::TABLETOP, 4.5f, 1.0f},
            {TerrainFeature::CLIFF, 3.5f, 1.0f},
            {TerrainFeature::RIDGE, 5.5f, 1.0f},
            {TerrainFeature::PLATEAU, 5.0f, 1.0f},
            {TerrainFeature::WHOOPS, 1.8f, 3.5f},
            {TerrainFeature::STEP_UP, 4.0f, 1.0f},
            {TerrainFeature::RAVINE, 5.0f, 1.0f},
            {TerrainFeature::ROLLERS, 5.5f, 1.2f},
            {TerrainFeature::DOUBLE, 5.0f, 1.0f},
            {TerrainFeature::RIDGE, 6.0f, 1.0f},
            {TerrainFeature::ROLLERS, 6.0f, 1.5f},
            {TerrainFeature::PLATEAU, 5.5f, 1.0f}
        },
        {
            {0.0f, 500.0f, SurfaceType::NEON, 6.0f},
            {500.0f, 700.0f, SurfaceType::TARMAC, 4.0f},
            {700.0f, 1500.0f, SurfaceType::NEON, 6.0f},
            {1500.0f, 1700.0f, SurfaceType::TARMAC, 4.0f},
            {1700.0f, 2600.0f, SurfaceType::NEON, 6.0f},
            {2600.0f, 2800.0f, SurfaceType::TARMAC, 4.0f},
            {2800.0f, 3700.0f, SurfaceType::NEON, 6.0f},
            {3700.0f, 3900.0f, SurfaceType::TARMAC, 4.0f},
            {3900.0f, 5000.0f, SurfaceType::NEON, 6.0f}
        }));

    recipes.push_back(makeRecipe(
        "super_countryside", 0x5C0A11u, 2.8f, 0.015f, 0.65f,
        {
            {TerrainFeature::ROLLERS, 3.0f, 1.0f},
            {TerrainFeature::TABLETOP, 4.0f, 1.0f},
            {TerrainFeature::ROLLERS, 4.0f, 1.2f},
            {TerrainFeature::WHOOPS, 1.5f, 3.0f},
            {TerrainFeature::RIDGE, 5.0f, 1.0f},
            {TerrainFeature::PLATEAU, 5.5f, 1.0f},
            {TerrainFeature::WASHBOARD, 0.7f, 9.0f},
            {TerrainFeature::ROLLERS, 4.5f, 1.1f},
            {TerrainFeature::STEP_UP, 3.0f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.5f, 1.0f},
            {TerrainFeature::DROP_OFF, 4.0f, 1.0f},
            {TerrainFeature::RIDGE, 6.0f, 1.0f},
            {TerrainFeature::DOUBLE, 4.5f, 1.0f},
            {TerrainFeature::ROLLERS, 5.5f, 1.3f},
            {TerrainFeature::TABLETOP, 6.0f, 1.0f},
            {TerrainFeature::CLIFF, 5.0f, 1.0f},
            {TerrainFeature::RIDGE, 7.0f, 1.0f},
            {TerrainFeature::PLATEAU, 6.5f, 1.0f},
            {TerrainFeature::WHOOPS, 2.2f, 3.5f},
            {TerrainFeature::STEP_UP, 5.5f, 1.0f},
            {TerrainFeature::RAVINE, 7.0f, 1.0f},
            {TerrainFeature::ROLLERS, 7.0f, 1.2f},
            {TerrainFeature::DOUBLE, 6.5f, 1.0f},
            {TerrainFeature::RIDGE, 8.0f, 1.0f},
            {TerrainFeature::ROLLERS, 7.5f, 1.4f},
            {TerrainFeature::PLATEAU, 7.0f, 1.0f}
        },
        {
            {0.0f, 800.0f, SurfaceType::GRASS, 8.0f},
            {800.0f, 1000.0f, SurfaceType::DIRT, 6.0f},
            {1000.0f, 1600.0f, SurfaceType::GRASS, 8.0f},
            {1600.0f, 1800.0f, SurfaceType::GRAVEL, 5.0f},
            {1800.0f, 2000.0f, SurfaceType::DIRT, 6.0f},
            {2000.0f, 2800.0f, SurfaceType::GRASS, 8.0f},
            {2800.0f, 3000.0f, SurfaceType::GRAVEL, 5.0f},
            {3000.0f, 3200.0f, SurfaceType::DIRT, 6.0f},
            {3200.0f, 4400.0f, SurfaceType::GRASS, 8.0f},
            {4400.0f, 4600.0f, SurfaceType::DIRT, 6.0f},
            {4600.0f, 5000.0f, SurfaceType::GRASS, 8.0f}
        }));

    recipes.push_back(makeRecipe(
        "seasons", 0x5EA50Fu, 2.2f, 0.020f, 0.95f,
        {
            {TerrainFeature::ROLLERS, 2.2f, 1.0f},
            {TerrainFeature::WHOOPS, 1.2f, 3.0f},
            {TerrainFeature::DUNE_CREST, 3.0f, 1.0f},
            {TerrainFeature::RIDGE, 3.5f, 1.0f},
            {TerrainFeature::PLATEAU, 3.5f, 1.0f},
            {TerrainFeature::RAVINE, 3.5f, 1.0f},
            {TerrainFeature::STEP_UP, 2.5f, 1.0f},
            {TerrainFeature::ROCK_STEPS, 2.0f, 1.0f},
            {TerrainFeature::DROP_OFF, 3.0f, 1.0f},
            {TerrainFeature::RIDGE, 4.5f, 1.0f},
            {TerrainFeature::DOUBLE, 3.0f, 1.0f},
            {TerrainFeature::ROLLERS, 4.0f, 1.2f},
            {TerrainFeature::TABLETOP, 4.5f, 1.0f},
            {TerrainFeature::CLIFF, 3.5f, 1.0f},
            {TerrainFeature::RIDGE, 5.0f, 1.0f},
            {TerrainFeature::PLATEAU, 5.0f, 1.0f},
            {TerrainFeature::WHOOPS, 1.8f, 3.5f},
            {TerrainFeature::STEP_UP, 4.0f, 1.0f},
            {TerrainFeature::RAVINE, 5.0f, 1.0f},
            {TerrainFeature::ROLLERS, 5.0f, 1.3f},
            {TerrainFeature::DOUBLE, 4.5f, 1.0f},
            {TerrainFeature::RIDGE, 5.5f, 1.0f},
            {TerrainFeature::ROLLERS, 5.5f, 1.4f},
            {TerrainFeature::TABLETOP, 5.5f, 1.0f},
            {TerrainFeature::RIDGE, 6.0f, 1.0f},
            {TerrainFeature::PLATEAU, 5.5f, 1.0f}
        },
        {
            {0.0f, 500.0f, SurfaceType::GRASS, 6.0f},
            {500.0f, 1000.0f, SurfaceType::DIRT, 6.0f},
            {1000.0f, 1500.0f, SurfaceType::MUD, 6.0f},
            {1500.0f, 2000.0f, SurfaceType::ICE, 5.0f},
            {2000.0f, 2500.0f, SurfaceType::SNOW, 6.0f},
            {2500.0f, 3000.0f, SurfaceType::GRASS, 6.0f},
            {3000.0f, 3500.0f, SurfaceType::DIRT, 6.0f},
            {3500.0f, 4000.0f, SurfaceType::MUD, 6.0f},
            {4000.0f, 4500.0f, SurfaceType::ICE, 5.0f},
            {4500.0f, 5000.0f, SurfaceType::SNOW, 6.0f}
        }));

    return recipes;
}

const std::vector<TrackRecipe>& recipes() {
    static const std::vector<TrackRecipe> value = buildRecipes();
    return value;
}

} // namespace

float TerrainGenerator::valueNoise1D(float x, uint32_t seed) {
    const int32_t i0 = static_cast<int32_t>(std::floor(x));
    const float frac = x - static_cast<float>(i0);
    const float a = hashSigned(i0, seed);
    const float b = hashSigned(i0 + 1, seed);
    const float t = frac * frac * frac * (frac * (frac * 6.0f - 15.0f) + 10.0f);
    return a + (b - a) * t;
}

float TerrainGenerator::fbm(float x, uint32_t seed, int octaves) {
    float sum = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += valueNoise1D(x * frequency, seed + static_cast<uint32_t>(i) * 0x9E3779B9u) * amplitude;
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return (norm > 0.0f) ? sum / norm : 0.0f;
}

float TerrainGenerator::ridgedNoise(float x, uint32_t seed, int octaves) {
    float sum = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        const float n = valueNoise1D(x * frequency, seed + static_cast<uint32_t>(i) * 0x85EBCA6Bu);
        const float ridge = 1.0f - std::abs(n);
        sum += (ridge * ridge * 2.0f - 1.0f) * amplitude;
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return (norm > 0.0f) ? sum / norm : 0.0f;
}

float TerrainGenerator::featureHeight(TerrainFeature feature, float t, float amplitude, float frequency) {
    const float u = clamp01(t);
    switch (feature) {
        case TerrainFeature::ROLLERS:
            return std::sin(u * 2.0f * kPi * frequency) * smoothWindow(u) * amplitude;
        case TerrainFeature::WHOOPS: {
            const float phase = u * 2.0f * kPi * frequency;
            return (std::sin(phase) + 0.25f * std::sin(phase * 2.0f)) * smoothWindow(u) * amplitude;
        }
        case TerrainFeature::WASHBOARD:
            return std::sin(u * 2.0f * kPi * frequency) * smoothWindow(u) * amplitude;
        case TerrainFeature::TABLETOP: {
            if (u < 0.18f) return smoothStep(u / 0.18f) * amplitude;
            if (u < 0.82f) return amplitude;
            return (1.0f - smoothStep((u - 0.82f) / 0.18f)) * amplitude;
        }
        case TerrainFeature::DOUBLE:
            return (bump(u, 0.22f, 0.18f) + bump(u, 0.78f, 0.18f)) * amplitude;
        case TerrainFeature::STEP_UP:
            return riseTo(u, 0.25f) * amplitude;
        case TerrainFeature::DROP_OFF:
            return dropTo(u, 0.28f) * amplitude;
        case TerrainFeature::LEDGE:
            return riseTo(u, 0.12f) * amplitude;
        case TerrainFeature::CLIFF:
            return dropTo(u, 0.18f) * amplitude;
        case TerrainFeature::RAVINE: {
            if (u < 0.5f) return -smoothStep(u / 0.5f) * amplitude;
            return -(1.0f - smoothStep((u - 0.5f) / 0.5f)) * amplitude;
        }
        case TerrainFeature::RIDGE: {
            if (u < 0.45f) return smoothStep(u / 0.45f) * amplitude;
            return (1.0f - smoothStep((u - 0.45f) / 0.55f)) * amplitude;
        }
        case TerrainFeature::PLATEAU: {
            if (u < 0.22f) return smoothStep(u / 0.22f) * amplitude;
            if (u < 0.78f) return amplitude;
            return (1.0f - smoothStep((u - 0.78f) / 0.22f)) * amplitude;
        }
        case TerrainFeature::DUNE_CREST: {
            if (u < 0.68f) return smoothStep(u / 0.68f) * amplitude;
            return (1.0f - smoothStep((u - 0.68f) / 0.32f)) * amplitude;
        }
        case TerrainFeature::ICE_SHELF:
            return riseTo(u, 0.18f) * amplitude;
        case TerrainFeature::ROCK_STEPS: {
            constexpr int steps = 4;
            const float scaled = u * static_cast<float>(steps);
            const int stepIndex = std::min(steps - 1, static_cast<int>(scaled));
            const float frac = scaled - static_cast<float>(stepIndex);
            return ((static_cast<float>(stepIndex) + smoothStep(frac)) / static_cast<float>(steps)) * amplitude;
        }
        case TerrainFeature::ASH_SLOPE: {
            if (u < 0.30f) return smoothStep(u / 0.30f) * amplitude;
            if (u < 0.70f) return amplitude;
            return (1.0f - smoothStep((u - 0.70f) / 0.30f)) * amplitude;
        }
        case TerrainFeature::FLAT:
        default:
            return 0.0f;
    }
}

float TerrainGenerator::sectionElevationDelta(const TrackSection& section) {
    switch (section.feature) {
        case TerrainFeature::STEP_UP:
        case TerrainFeature::LEDGE:
        case TerrainFeature::ICE_SHELF:
        case TerrainFeature::ROCK_STEPS:
            return section.amplitude;
        case TerrainFeature::DROP_OFF:
        case TerrainFeature::CLIFF:
            return -section.amplitude;
        default:
            return 0.0f;
    }
}

float TerrainGenerator::heightAt(const TrackRecipe& recipe, float x) {
    if (x < 20.0f) {
        return 0.0f;
    }

    const float lx = x - 20.0f;
    const float entry = smoothStep(lx / 20.0f);
    float offset = 0.0f;
    float sectionValue = 0.0f;

    for (const auto& section : recipe.sections) {
        if (x < section.startX) {
            break;
        }
        if (x >= section.startX + section.length) {
            offset += sectionElevationDelta(section);
            continue;
        }
        const float t = (x - section.startX) / std::max(section.length, 0.001f);
        sectionValue = featureHeight(section.feature, t, section.amplitude, section.frequency);
        break;
    }

    const float seedOffset = static_cast<float>(recipe.seed & 0xFFFFu) * 0.001f;
    const float warp = valueNoise1D(lx * 0.013f + seedOffset, recipe.seed) * 6.0f;
    const float broad = fbm((lx + warp) * 0.035f, recipe.seed, 4) * recipe.baseAmplitude * 0.55f;
    const float ridge = ridgedNoise((lx + warp * 0.4f) * 0.020f, recipe.seed ^ 0x9E3779B9u, 3) *
                        recipe.baseAmplitude * 0.25f;
    const float baseWave = std::sin(lx * recipe.baseFrequency +
                                    0.60f * valueNoise1D(lx * 0.010f, recipe.seed ^ 0xA5A5A5A5u)) *
                           recipe.baseAmplitude;
    const float roughness = (valueNoise1D(lx / 8.0f, recipe.seed + 11u) * 0.35f +
                             valueNoise1D(lx / 2.0f, recipe.seed + 17u) * 0.22f +
                             valueNoise1D(lx / 0.5f, recipe.seed + 23u) * 0.10f) *
                            recipe.roughness;

    const float terrain = offset + sectionValue + baseWave + broad + ridge + roughness;
    return terrain * entry;
}

const TrackRecipe& TrackRecipeRegistry::get(const std::string& biomeId) {
    for (const auto& recipe : recipes()) {
        if (recipe.biomeId == biomeId) {
            return recipe;
        }
    }
    return recipes()[0];
}

const std::vector<TrackRecipe>& TrackRecipeRegistry::all() {
    return recipes();
}

} // namespace Physics
