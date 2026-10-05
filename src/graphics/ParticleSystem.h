#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "PhysicsTypes.h"
#include <vector>

namespace Graphics {

enum class ParticleType {
    SMOKE,
    DIRT,
    SPARKLE
};

struct Particle {
    Physics::Vec2 pos;
    Physics::Vec2 vel;
    float life;
    float maxLife;
    float size;
    uint32_t color;
    ParticleType type;
};

/**
 * @brief High-performance Raster Particle Engine.
 * Handles exhaust smoke puffs, ballistic dirt rooster tails, and coin sparkles.
 */
class ParticleSystem {
public:
    ParticleSystem();

    void emitSmoke(const Physics::Vec2& worldPos, const Physics::Vec2& carVel);
    void emitDirt(const Physics::Vec2& worldPos, const Physics::Vec2& wheelTangent);
    void emitSparkles(const Physics::Vec2& worldPos);

    void update(float dt);
    void render(Framebuffer& fb, const Camera& cam);
    void clear();

private:
    std::vector<Particle> m_particles;
};

} // namespace Graphics
