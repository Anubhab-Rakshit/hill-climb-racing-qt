#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Physics {

enum class SurfaceType {
    GRASS = 0,
    DIRT,
    MUD,
    SAND,
    GRAVEL,
    ROCK,
    ICE,
    SNOW,
    ASH,
    REGOLITH,
    TARMAC,
    NEON,
    COUNT
};

struct SurfaceMaterial {
    SurfaceType type;
    const char* id;
    const char* name;
    float friction;
    float rollingResistance;
    float sinkage;
    float deformationRate;
    float maxCompression;
    float restitution;
    uint32_t dustColor;
    uint32_t topColor;
    uint32_t bodyColor;
};

struct SurfaceProperties {
    SurfaceType type = SurfaceType::GRASS;
    float friction = 1.0f;
    float rollingResistance = 0.014f;
    float sinkage = 0.0f;
    float deformationRate = 0.08f;
    float maxCompression = 0.045f;
    float restitution = 0.0f;
    float blend = 1.0f;
};

class SurfaceMaterialRegistry {
public:
    static const SurfaceMaterial& get(SurfaceType type);
    static const SurfaceMaterial& get(const std::string& id);
    static SurfaceType fromId(const std::string& id, SurfaceType fallback = SurfaceType::GRASS);
    static const char* name(SurfaceType type);
    static const std::vector<SurfaceMaterial>& all();
};

SurfaceProperties propertiesFromMaterial(const SurfaceMaterial& material);

SurfaceProperties blendSurfaceProperties(const SurfaceProperties& a,
                                          const SurfaceProperties& b,
                                          float t);

} // namespace Physics
