#include "ParticleSystem.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace Graphics {

ParticleSystem::ParticleSystem() {
    m_particles.reserve(MAX_PARTICLES);
}

void ParticleSystem::clear() {
    m_particles.clear();
}

static inline float randomFloat() {
    return static_cast<float>(static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX));
}

void ParticleSystem::emitSmoke(const Physics::Vec2& worldPos, const Physics::Vec2& carVel) {
    if (m_particles.size() >= MAX_PARTICLES) return;

    Particle p;
    p.pos = worldPos;
    // Drifts backward and floats upward
    float rx = (randomFloat() - 0.5f) * 1.5f;
    float ry = randomFloat() * 1.8f + 0.5f;
    p.vel = {carVel.x * 0.2f - 2.0f + rx, ry};
    p.life = 0.0f;
    p.maxLife = 0.8f + randomFloat() * 0.4f;
    p.size = 2.0f;
    p.color = 0xAA90A4AE;
    p.type = ParticleType::SMOKE;

    m_particles.push_back(p);
}

void ParticleSystem::emitDirt(const Physics::Vec2& worldPos, const Physics::Vec2& wheelTangent) {
    if (m_particles.size() >= MAX_PARTICLES) return;

    for (int i = 0; i < 3; ++i) {
        Particle p;
        p.pos = worldPos;
        float spray = (randomFloat() - 0.5f) * 4.0f;
        p.vel = {wheelTangent.x * 0.8f + spray, std::abs(wheelTangent.y) * 0.8f + 2.5f + randomFloat() * 3.0f};
        p.life = 0.0f;
        p.maxLife = 0.4f + randomFloat() * 0.3f;
        p.size = 2.0f;
        p.color = (i % 2 == 0) ? 0xFF6D4C41 : 0xFF4E342E;
        p.type = ParticleType::DIRT_CLOD;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitWheelSpray(const Physics::Vec2& contactPos, float wheelRadius,
                                    const Physics::Vec2& tangent, float slipSpeed,
                                    float wheelAngVel, float normalForce,
                                    const std::string& biomeId) {
    if (m_particles.size() >= MAX_PARTICLES) return;

    float absSlip = std::abs(slipSpeed);
    float wheelSurfSpeed = std::abs(wheelAngVel) * wheelRadius;

    // Only emit when tyre is slipping or spinning fast against the surface
    if (absSlip < 0.6f && wheelSurfSpeed < 3.5f) return;

    float intensity = std::clamp(absSlip * 0.45f + (normalForce / 4000.0f) * 0.35f, 0.4f, 3.0f);

    // Ejection direction: backward along ground tangent, with upward rooster arc
    float spinDir = (wheelAngVel > 0.1f) ? -1.0f : ((wheelAngVel < -0.1f) ? 1.0f : -1.0f);
    Physics::Vec2 throwDir = tangent * spinDir;
    // Ground normal is perpendicular to tangent
    Physics::Vec2 groundNorm(-tangent.y, tangent.x);
    if (groundNorm.y < 0.0f) groundNorm = -groundNorm;

    // 1. Ballistic Dirt / Material Clods
    int clodCount = std::min(3, static_cast<int>(intensity * 1.5f));
    for (int i = 0; i < clodCount; ++i) {
        if (m_particles.size() >= MAX_PARTICLES) break;

        Particle p;
        p.pos = contactPos + Physics::Vec2((randomFloat() - 0.5f) * 0.15f, 0.02f);

        float ejectSpeed = 4.0f + intensity * 3.5f + randomFloat() * 3.0f;
        float spraySpread = (randomFloat() - 0.5f) * 0.6f;
        Physics::Vec2 dir = (throwDir * 0.75f + groundNorm * (0.85f + randomFloat() * 0.6f) + Physics::Vec2(spraySpread, 0.0f)).normalized();

        p.vel = dir * ejectSpeed;
        p.life = 0.0f;
        p.maxLife = 0.35f + randomFloat() * 0.35f;
        p.size = (randomFloat() > 0.4f) ? 3.0f : 2.0f;
        p.type = ParticleType::DIRT_CLOD;

        // Biome-specific clod colors
        if (biomeId == "desert") {
            p.color = (i % 2 == 0) ? 0xFFFFB300 : 0xFFFFD54F;
        } else if (biomeId == "arctic") {
            p.color = (i % 2 == 0) ? 0xFFECEFF1 : 0xFFCFD8DC;
        } else if (biomeId == "mountain") {
            p.color = (i % 2 == 0) ? 0xFF546E7A : 0xFF37474F;
        } else if (biomeId == "volcano") {
            p.color = (randomFloat() > 0.3f) ? 0xFF212121 : 0xFFFF5722; // Volcanic cinder
        } else if (biomeId == "moon") {
            p.color = (i % 2 == 0) ? 0xFF757575 : 0xFF616161;
            p.vel *= 0.65f; // Lower ejection velocity
        } else {
            // Countryside fertile soil
            p.color = (i % 2 == 0) ? 0xFF4E342E : 0xFF3E2723;
        }

        m_particles.push_back(p);
    }

    // 2. Fluttering Grass Blade Fragments (Countryside exclusive under tyre shear)
    if (biomeId == "countryside" && absSlip > 1.0f) {
        int grassCount = (randomFloat() > 0.4f) ? 1 : 2;
        for (int i = 0; i < grassCount; ++i) {
            if (m_particles.size() >= MAX_PARTICLES) break;

            Particle p;
            p.pos = contactPos + Physics::Vec2((randomFloat() - 0.5f) * 0.2f, 0.05f);

            float speed = 2.5f + randomFloat() * 3.5f;
            Physics::Vec2 dir = (throwDir * 0.6f + groundNorm * (1.0f + randomFloat() * 0.7f)).normalized();
            p.vel = dir * speed;
            p.life = 0.0f;
            p.maxLife = 0.45f + randomFloat() * 0.30f;
            p.size = 2.5f;
            p.angle = randomFloat() * 6.28f;
            p.angularVel = (randomFloat() - 0.5f) * 18.0f;
            p.color = (i % 2 == 0) ? 0xFF66BB6A : 0xFF81C784; // Vibrant green lawn flecks
            p.type = ParticleType::GRASS_FLECK;

            m_particles.push_back(p);
        }
    }

    // 3. Ambient Dust / Sand Plume
    m_dustCooldown += 0.016f;
    if (m_dustCooldown > 0.06f && m_particles.size() < MAX_PARTICLES) {
        m_dustCooldown = 0.0f;
        Particle p;
        p.pos = contactPos + Physics::Vec2((randomFloat() - 0.5f) * 0.15f, 0.05f);
        p.vel = throwDir * (1.2f + randomFloat() * 1.5f) + groundNorm * (0.6f + randomFloat() * 0.8f);
        p.life = 0.0f;
        p.maxLife = 0.60f + randomFloat() * 0.30f;
        p.size = 2.5f;
        p.type = ParticleType::DUST_PUFF;

        if (biomeId == "desert") p.color = 0x88FFE082;
        else if (biomeId == "arctic") p.color = 0x66FFFFFF;
        else if (biomeId == "volcano") p.color = 0x77424242;
        else if (biomeId == "mountain") p.color = 0x6690A4AE;
        else if (biomeId == "moon") p.color = 0x559E9E9E;
        else p.color = 0x778D6E63; // Countryside dusty soil

        m_particles.push_back(p);
    }
}

void ParticleSystem::emitSparkles(const Physics::Vec2& worldPos) {
    for (int i = 0; i < 10; ++i) {
        if (m_particles.size() >= MAX_PARTICLES) break;
        Particle p;
        p.pos = worldPos;
        float angle = randomFloat() * 6.28f;
        float speed = 2.0f + randomFloat() * 4.0f;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.life = 0.0f;
        p.maxLife = 0.5f;
        p.size = 3.0f;
        p.color = (i % 2 == 0) ? 0xFFFFD54F : 0xFFFFF9C4;
        p.type = ParticleType::SPARKLE;
        m_particles.push_back(p);
    }
}

void ParticleSystem::update(float dt) {
    for (size_t i = 0; i < m_particles.size(); ) {
        m_particles[i].life += dt;
        if (m_particles[i].life >= m_particles[i].maxLife) {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        auto& p = m_particles[i];
        p.angle += p.angularVel * dt;

        switch (p.type) {
            case ParticleType::DIRT_CLOD:
                p.vel.y -= 16.5f * dt; // Strong gravity
                p.vel.x *= 0.98f;     // Slight air resistance
                break;
            case ParticleType::GRASS_FLECK:
                p.vel.y -= 8.0f * dt;  // Gentle fluttering fall
                p.vel.x *= 0.94f;     // Aerodynamic flutter drag
                break;
            case ParticleType::DUST_PUFF:
                p.size += 6.5f * dt;   // Expanding puff
                p.vel *= 0.92f;        // Drag
                p.vel.y += 0.5f * dt;  // Slight thermal rise
                break;
            case ParticleType::SMOKE:
                p.size += 3.5f * dt;
                p.vel.y += 1.2f * dt;
                p.vel.x *= 0.95f;
                break;
            case ParticleType::SPARKLE:
                p.vel *= 0.90f;
                break;
        }

        p.pos += p.vel * dt;
        ++i;
    }
}

void ParticleSystem::render(Framebuffer& fb, const Camera& cam) {
    float zoom = cam.zoom();

    for (const auto& p : m_particles) {
        Physics::Vec2 scr = cam.worldToScreen(p.pos);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -15 || sx > fb.width() + 15 || sy < -15 || sy > fb.height() + 15) continue;

        float alphaFrac = 1.0f - (p.life / p.maxLife);
        uint32_t baseA = (p.color >> 24) & 0xFF;
        uint32_t a = static_cast<uint32_t>(alphaFrac * baseA);
        if (a == 0) continue;
        uint32_t col = (a << 24) | (p.color & 0x00FFFFFF);

        if (p.type == ParticleType::SMOKE || p.type == ParticleType::DUST_PUFF) {
            int r = static_cast<int>(p.size * (zoom / 22.0f));
            if (r > 0) fb.fillCircle(sx, sy, r, col);
        } else if (p.type == ParticleType::DIRT_CLOD) {
            // Shaded multi-pixel soil clump
            int s = static_cast<int>(p.size * (zoom / 22.0f));
            if (s <= 1) {
                fb.setPixelFast(sx, sy, col);
            } else {
                fb.fillRect(sx, sy, s, s, col);
                // Top-left highlight & bottom-right shadow
                uint32_t baseRgb = p.color & 0x00FFFFFFu;
                uint32_t hiCol = (a << 24) | std::min(0x00FFFFFFu, baseRgb + 0x00151515u);
                uint32_t shCol = (a << 24) | (baseRgb > 0x00151515u ? baseRgb - 0x00151515u : 0u);
                fb.setPixelFast(sx, sy, hiCol);
                fb.setPixelFast(sx + s - 1, sy + s - 1, shCol);
            }
        } else if (p.type == ParticleType::GRASS_FLECK) {
            // Tumbling oblong grass blade (2x1 or 1x2 or 3x1 pixel strip)
            bool horizontal = (std::abs(std::cos(p.angle)) > 0.5f);
            if (horizontal) {
                fb.fillRect(sx - 1, sy, 3, 1, col);
            } else {
                fb.fillRect(sx, sy - 1, 1, 3, col);
            }
        } else {
            fb.fillRect(sx, sy, 2, 2, col);
        }
    }
}

} // namespace Graphics
