#include "SurfaceMaterial.h"
#include <algorithm>

namespace Physics {

static const std::vector<SurfaceMaterial> s_materials = {
    {SurfaceType::GRASS, "grass", "GRASS", 1.25f, 0.014f, 0.004f, 0.080f, 0.045f, 0.00f, 0xFF6D4C41, 0xFF81C784, 0xFF43A047},
    {SurfaceType::DIRT, "dirt", "DIRT", 1.10f, 0.022f, 0.006f, 0.100f, 0.050f, 0.00f, 0xFF8D6E63, 0xFFA1887F, 0xFF6D4C41},
    {SurfaceType::MUD, "mud", "MUD", 0.78f, 0.055f, 0.018f, 0.160f, 0.075f, 0.00f, 0xFF4E342E, 0xFF5D4037, 0xFF3E2723},
    {SurfaceType::SAND, "sand", "SAND", 0.92f, 0.045f, 0.015f, 0.140f, 0.065f, 0.00f, 0xFFFFD54F, 0xFFFFF59D, 0xFFFFD54F},
    {SurfaceType::GRAVEL, "gravel", "GRAVEL", 1.08f, 0.030f, 0.008f, 0.070f, 0.035f, 0.05f, 0xFF90A4AE, 0xFFB0BEC5, 0xFF78909C},
    {SurfaceType::ROCK, "rock", "ROCK", 1.38f, 0.012f, 0.002f, 0.025f, 0.018f, 0.08f, 0xFF78909C, 0xFFECEFF1, 0xFF546E7A},
    {SurfaceType::ICE, "ice", "ICE", 0.42f, 0.006f, 0.001f, 0.010f, 0.008f, 0.02f, 0xFFE0F7FA, 0xFFE0F7FA, 0xFF80DEEA},
    {SurfaceType::SNOW, "snow", "SNOW", 0.72f, 0.035f, 0.012f, 0.090f, 0.045f, 0.00f, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFCFD8DC},
    {SurfaceType::ASH, "ash", "ASH", 0.88f, 0.050f, 0.016f, 0.130f, 0.060f, 0.00f, 0xFF616161, 0xFF9E9E9E, 0xFF424242},
    {SurfaceType::REGOLITH, "regolith", "REGOLITH", 1.02f, 0.040f, 0.014f, 0.100f, 0.050f, 0.00f, 0xFFB0BEC5, 0xFFECEFF1, 0xFF90A4AE},
    {SurfaceType::TARMAC, "tarmac", "TARMAC", 1.32f, 0.010f, 0.001f, 0.005f, 0.006f, 0.02f, 0xFF424242, 0xFF616161, 0xFF212121},
    {SurfaceType::NEON, "neon", "NEON", 1.40f, 0.008f, 0.000f, 0.004f, 0.005f, 0.03f, 0xFF00E5FF, 0xFF00E5FF, 0xFFFF007F}
};

const SurfaceMaterial& SurfaceMaterialRegistry::get(SurfaceType type) {
    const int idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(s_materials.size())) {
        return s_materials[static_cast<size_t>(idx)];
    }
    return s_materials[0];
}

const SurfaceMaterial& SurfaceMaterialRegistry::get(const std::string& id) {
    for (const auto& material : s_materials) {
        if (id == material.id) {
            return material;
        }
    }
    return s_materials[0];
}

SurfaceType SurfaceMaterialRegistry::fromId(const std::string& id, SurfaceType fallback) {
    for (const auto& material : s_materials) {
        if (id == material.id) {
            return material.type;
        }
    }
    return fallback;
}

const char* SurfaceMaterialRegistry::name(SurfaceType type) {
    return get(type).name;
}

const std::vector<SurfaceMaterial>& SurfaceMaterialRegistry::all() {
    return s_materials;
}

SurfaceProperties propertiesFromMaterial(const SurfaceMaterial& material) {
    SurfaceProperties out;
    out.type = material.type;
    out.friction = material.friction;
    out.rollingResistance = material.rollingResistance;
    out.sinkage = material.sinkage;
    out.deformationRate = material.deformationRate;
    out.maxCompression = material.maxCompression;
    out.restitution = material.restitution;
    out.blend = 1.0f;
    return out;
}

SurfaceProperties blendSurfaceProperties(const SurfaceProperties& a,
                                          const SurfaceProperties& b,
                                          float t) {
    const float w = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - w;
    SurfaceProperties out;
    out.type = (w >= 0.5f) ? b.type : a.type;
    out.friction = a.friction * inv + b.friction * w;
    out.rollingResistance = a.rollingResistance * inv + b.rollingResistance * w;
    out.sinkage = a.sinkage * inv + b.sinkage * w;
    out.deformationRate = a.deformationRate * inv + b.deformationRate * w;
    out.maxCompression = a.maxCompression * inv + b.maxCompression * w;
    out.restitution = a.restitution * inv + b.restitution * w;
    out.blend = w;
    return out;
}

} // namespace Physics
