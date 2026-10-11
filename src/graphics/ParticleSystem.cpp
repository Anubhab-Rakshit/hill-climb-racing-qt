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
    // Drifts backward gently with car velocity, low upward thermal drift
    float rx = (randomFloat() - 0.5f) * 0.6f;
    float ry = randomFloat() * 0.4f + 0.15f;
    p.vel = {carVel.x * 0.25f - 0.7f + rx, ry};
    p.life = 0.0f;
    p.maxLife = 0.24f + randomFloat() * 0.12f; // Short dissipation lifetime (under 0.36s)
    p.size = 1.0f;
    p.color = 0x8890A4AE;
    p.type = ParticleType::SMOKE;

    m_particles.push_back(p);
}

void ParticleSystem::emitDirt(const Physics::Vec2& worldPos, const Physics::Vec2& wheelTangent) {
    if (m_particles.size() >= MAX_PARTICLES) return;

    for (int i = 0; i < 2; ++i) {
        Particle p;
        p.pos = worldPos;
        float spray = (randomFloat() - 0.5f) * 1.5f;
        p.vel = {wheelTangent.x * 0.6f + spray, std::abs(wheelTangent.y) * 0.4f + 0.8f + randomFloat() * 1.0f};
        p.life = 0.0f;
        p.maxLife = 0.20f + randomFloat() * 0.14f;
        p.size = (randomFloat() > 0.6f) ? 1.6f : 1.0f;
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

    float intensity = std::clamp(absSlip * 0.40f + (normalForce / 4500.0f) * 0.30f, 0.4f, 2.2f);

    // Ejection direction: backward along ground tangent, with low ground-skimming rooster arc
    float spinDir = (wheelAngVel > 0.1f) ? -1.0f : ((wheelAngVel < -0.1f) ? 1.0f : -1.0f);
    Physics::Vec2 throwDir = tangent * spinDir;
    // Ground normal is perpendicular to tangent
    Physics::Vec2 groundNorm(-tangent.y, tangent.x);
    if (groundNorm.y < 0.0f) groundNorm = -groundNorm;

    // 1. Ballistic Material Clods (Ground-skimming micro-granules)
    int clodCount = std::min(2, static_cast<int>(intensity * 1.0f));
    for (int i = 0; i < clodCount; ++i) {
        if (m_particles.size() >= MAX_PARTICLES) break;

        Particle p;
        p.pos = contactPos + Physics::Vec2((randomFloat() - 0.5f) * 0.10f, 0.02f);

        // Low, controlled ejection velocity: stays near the wheel and terrain
        float ejectSpeed = 1.8f + intensity * 1.4f + randomFloat() * 1.0f;
        float spraySpread = (randomFloat() - 0.5f) * 0.3f;
        Physics::Vec2 dir = (throwDir * 0.88f + groundNorm * (0.18f + randomFloat() * 0.22f) + Physics::Vec2(spraySpread, 0.0f)).normalized();

        p.vel = dir * ejectSpeed;
        p.life = 0.0f;
        p.maxLife = 0.18f + randomFloat() * 0.16f; // Fast dissipation (0.18 - 0.34s)
        p.size = (randomFloat() > 0.5f) ? 1.6f : 1.0f;
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
            p.vel *= 0.55f; // Lower ejection velocity
        } else {
            // Countryside fertile soil
            p.color = (i % 2 == 0) ? 0xFF4E342E : 0xFF3E2723;
        }

        m_particles.push_back(p);
    }

    // 2. Fluttering Grass Blade Fragments (Countryside exclusive under tyre shear)
    if (biomeId == "countryside" && absSlip > 1.2f) {
        int grassCount = 1;
        for (int i = 0; i < grassCount; ++i) {
            if (m_particles.size() >= MAX_PARTICLES) break;

            Particle p;
            p.pos = contactPos + Physics::Vec2((randomFloat() - 0.5f) * 0.15f, 0.04f);

            float speed = 1.4f + randomFloat() * 1.8f;
            Physics::Vec2 dir = (throwDir * 0.7f + groundNorm * (0.3f + randomFloat() * 0.3f)).normalized();
            p.vel = dir * speed;
            p.life = 0.0f;
            p.maxLife = 0.24f + randomFloat() * 0.16f;
            p.size = 1.5f;
            p.angle = randomFloat() * 6.28f;
            p.angularVel = (randomFloat() - 0.5f) * 12.0f;
            p.color = (i % 2 == 0) ? 0xFF66BB6A : 0xFF81C784; // Vibrant green lawn flecks
            p.type = ParticleType::GRASS_FLECK;

            m_particles.push_back(p);
        }
    }

    // 3. Ambient Dust / Sand Plume (Suppressed in Moon vacuum)
    if (biomeId != "moon") {
        m_dustCooldown += 0.016f;
        if (m_dustCooldown > 0.08f && m_particles.size() < MAX_PARTICLES) {
            m_dustCooldown = 0.0f;
            Particle p;
            p.pos = contactPos + Physics::Vec2((randomFloat() - 0.5f) * 0.10f, 0.03f);
            p.vel = throwDir * (0.8f + randomFloat() * 0.9f) + groundNorm * (0.25f + randomFloat() * 0.35f);
            p.life = 0.0f;
            p.maxLife = 0.22f + randomFloat() * 0.14f;
            p.size = 1.2f;
            p.type = ParticleType::DUST_PUFF;

            if (biomeId == "desert") p.color = 0x66FFE082;
            else if (biomeId == "arctic") p.color = 0x55FFFFFF;
            else if (biomeId == "volcano") p.color = 0x55424242;
            else if (biomeId == "mountain") p.color = 0x5590A4AE;
            else p.color = 0x558D6E63; // Countryside dusty soil

            m_particles.push_back(p);
        }
    }
}

void ParticleSystem::emitSparkles(const Physics::Vec2& worldPos, uint32_t color) {
    for (int i = 0; i < 12; ++i) {
        if (m_particles.size() >= MAX_PARTICLES) break;
        Particle p;
        p.pos = worldPos;
        float angle = randomFloat() * 6.28f;
        float speed = 1.5f + randomFloat() * 3.5f;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.life = 0.0f;
        p.maxLife = 0.25f + randomFloat() * 0.15f;
        p.size = 2.0f;
        p.color = (i % 2 == 0) ? color : 0xFFFFFFFF;
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
                p.vel.y -= 22.0f * dt; // Strong downward gravity pulls clods directly to ground
                p.vel.x *= 0.94f;
                break;
            case ParticleType::GRASS_FLECK:
                p.vel.y -= 10.0f * dt;
                p.vel.x *= 0.92f;
                break;
            case ParticleType::DUST_PUFF:
                p.size += 2.0f * dt;   // Subtle gentle expansion
                p.vel *= 0.88f;        // Drag
                p.vel.y += 0.2f * dt;
                break;
            case ParticleType::SMOKE:
                p.size += 1.8f * dt;
                p.vel.y += 0.3f * dt;
                p.vel.x *= 0.90f;
                break;
            case ParticleType::SPARKLE:
                p.vel *= 0.86f;
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
            // Soft pixel-art cloud cluster with radial alpha falloff (never hard circles)
            int r = std::clamp(static_cast<int>(p.size * (zoom / 65.0f)), 1, 4);
            if (r <= 1) {
                fb.blendPixelFast(sx, sy, col);
            } else if (r == 2) {
                fb.blendPixelFast(sx, sy, col);
                uint32_t edgeCol = (col & 0x00FFFFFF) | ((a * 2 / 3) << 24);
                fb.blendPixelFast(sx + 1, sy, edgeCol);
                fb.blendPixelFast(sx - 1, sy, edgeCol);
                fb.blendPixelFast(sx, sy + 1, edgeCol);
                fb.blendPixelFast(sx, sy - 1, edgeCol);
            } else {
                for (int dy = -r; dy <= r; ++dy) {
                    for (int dx = -r; dx <= r; ++dx) {
                        int d2 = dx * dx + dy * dy;
                        if (d2 <= r * r) {
                            float falloff = 1.0f - std::sqrt(static_cast<float>(d2)) / (r + 0.5f);
                            uint32_t pA = static_cast<uint32_t>(a * falloff * 0.65f);
                            if (pA > 0) {
                                fb.blendPixelFast(sx + dx, sy + dy, (pA << 24) | (p.color & 0x00FFFFFF));
                            }
                        }
                    }
                }
            }
        } else if (p.type == ParticleType::DIRT_CLOD) {
            // Micro-pixel debris: 1-2 screen pixels (completely eliminates square blocks)
            fb.blendPixelFast(sx, sy, col);
            if (p.size > 1.4f) {
                uint32_t subCol = (col & 0x00FFFFFF) | ((a * 3 / 4) << 24);
                fb.blendPixelFast(sx + 1, sy, subCol);
                fb.blendPixelFast(sx, sy + 1, (col & 0x00FFFFFF) | ((a / 2) << 24));
            }
        } else if (p.type == ParticleType::GRASS_FLECK) {
            // Tumbling oblong grass blade: 2x1 or 1x2 micro-pixels
            fb.blendPixelFast(sx, sy, col);
            if (std::abs(std::cos(p.angle)) > 0.5f) {
                fb.blendPixelFast(sx + 1, sy, col);
            } else {
                fb.blendPixelFast(sx, sy + 1, col);
            }
        } else {
            // Sparkling star glint
            fb.blendPixelFast(sx, sy, col);
            if (p.life < p.maxLife * 0.45f) {
                fb.blendPixelFast(sx + 1, sy, 0xFFFFFFFF);
                fb.blendPixelFast(sx, sy - 1, 0xFFFFFFFF);
            }
        }
    }
}

} // namespace Graphics
