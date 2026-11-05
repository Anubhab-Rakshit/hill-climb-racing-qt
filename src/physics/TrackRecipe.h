#pragma once

#include "SurfaceMaterial.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Physics {

enum class TerrainFeature {
    FLAT = 0,
    ROLLERS,
    WHOOPS,
    WASHBOARD,
    TABLETOP,
    DOUBLE,
    STEP_UP,
    DROP_OFF,
    LEDGE,
    CLIFF,
    RAVINE,
    RIDGE,
    PLATEAU,
    DUNE_CREST,
    ICE_SHELF,
    ROCK_STEPS,
    ASH_SLOPE
};

struct TrackSection {
    float startX = 0.0f;
    float length = 1.0f;
    TerrainFeature feature = TerrainFeature::FLAT;
    float amplitude = 0.0f;
    float frequency = 1.0f;
};

struct SurfaceZone {
    float startX = 0.0f;
    float endX = 1.0f;
    SurfaceType material = SurfaceType::GRASS;
    float blendWidth = 4.0f;
};

struct TrackRecipe {
    std::string biomeId;
    uint32_t seed = 0;
    float length = 5000.0f;
    float baseAmplitude = 2.0f;
    float baseFrequency = 0.020f;
    float roughness = 1.0f;
    std::vector<TrackSection> sections;
    std::vector<SurfaceZone> surfaceZones;
};

class TrackRecipeRegistry {
public:
    static const TrackRecipe& get(const std::string& biomeId);
    static const std::vector<TrackRecipe>& all();
};

class TerrainGenerator {
public:
    static float heightAt(const TrackRecipe& recipe, float x);
    static float featureHeight(TerrainFeature feature, float t, float amplitude, float frequency);
    static float valueNoise1D(float x, uint32_t seed);
    static float fbm(float x, uint32_t seed, int octaves);
    static float ridgedNoise(float x, uint32_t seed, int octaves);
    static float sectionElevationDelta(const TrackSection& section);
};

} // namespace Physics
