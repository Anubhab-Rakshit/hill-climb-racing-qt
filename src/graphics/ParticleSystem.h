#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "PhysicsTypes.h"
#include <vector>
#include <string>

namespace Graphics {

enum class ParticleType {
    SMOKE,
    DIRT_CLOD,
    GRASS_FLECK,
    DUST_PUFF,
    SPARKLE
};

struct Particle {
    Physics::Vec2 pos;
    Physics::Vec2 vel;
    float life = 0.0f;
    float maxLife = 0.5f;
    float size = 2.0f;
    float angle = 0.0f;
    float angularVel = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    ParticleType type = ParticleType::DIRT_CLOD;
};

/**
 * @brief High-performance Raster Particle Engine.
 * Handles exhaust smoke, ballistic dirt clods, fluttering grass flecks, dust plumes, and sparks.
 */
class ParticleSystem {
public:
    ParticleSystem();

    void emitSmoke(const Physics::Vec2& worldPos, const Physics::Vec2& carVel);
    void emitDirt(const Physics::Vec2& worldPos, const Physics::Vec2& wheelTangent);
    void emitWheelSpray(const Physics::Vec2& contactPos, float wheelRadius,
                        const Physics::Vec2& tangent, float slipSpeed,
                        float wheelAngVel, float normalForce,
                        const std::string& biomeId);
    void emitSparkles(const Physics::Vec2& worldPos);

    void update(float dt);
    void render(Framebuffer& fb, const Camera& cam);
    void clear();

private:
    static constexpr size_t MAX_PARTICLES = 320;
    std::vector<Particle> m_particles;
    float m_dustCooldown = 0.0f;
};

} // namespace Graphics
