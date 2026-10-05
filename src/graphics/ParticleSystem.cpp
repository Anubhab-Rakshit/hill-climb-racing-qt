#include "ParticleSystem.h"
#include <cstdlib>
#include <cmath>

namespace Graphics {

ParticleSystem::ParticleSystem() {
    m_particles.reserve(256);
}

void ParticleSystem::clear() {
    m_particles.clear();
}

static inline float randomFloat() {
    return static_cast<float>(static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX));
}

void ParticleSystem::emitSmoke(const Physics::Vec2& worldPos, const Physics::Vec2& carVel) {
    if (m_particles.size() > 200) return;

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
    if (m_particles.size() > 200) return;

    for (int i = 0; i < 3; ++i) {
        Particle p;
        p.pos = worldPos;
        float spray = (randomFloat() - 0.5f) * 4.0f;
        p.vel = {wheelTangent.x * 0.8f + spray, std::abs(wheelTangent.y) * 0.8f + 2.5f + randomFloat() * 3.0f};
        p.life = 0.0f;
        p.maxLife = 0.4f + randomFloat() * 0.3f;
        p.size = 2.0f;
        p.color = (i % 2 == 0) ? 0xFF6D4C41 : 0xFF4E342E;
        p.type = ParticleType::DIRT;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitSparkles(const Physics::Vec2& worldPos) {
    for (int i = 0; i < 10; ++i) {
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

        if (m_particles[i].type == ParticleType::DIRT) {
            m_particles[i].vel.y -= 14.0f * dt; // Gravity pull
        } else if (m_particles[i].type == ParticleType::SMOKE) {
            m_particles[i].size += 3.0f * dt; // Expanding puff
        }

        m_particles[i].pos += m_particles[i].vel * dt;
        ++i;
    }
}

void ParticleSystem::render(Framebuffer& fb, const Camera& cam) {
    for (const auto& p : m_particles) {
        Physics::Vec2 scr = cam.worldToScreen(p.pos);
        int sx = static_cast<int>(scr.x);
        int sy = static_cast<int>(scr.y);

        if (sx < -10 || sx > fb.width() + 10 || sy < -10 || sy > fb.height() + 10) continue;

        float alphaFrac = 1.0f - (p.life / p.maxLife);
        uint32_t a = static_cast<uint32_t>(alphaFrac * 255.0f);
        uint32_t col = (a << 24) | (p.color & 0x00FFFFFF);

        if (p.type == ParticleType::SMOKE) {
            int r = static_cast<int>(p.size * (cam.zoom() / 20.0f));
            if (r > 0) fb.fillCircle(sx, sy, r, col);
        } else {
            fb.fillRect(sx, sy, 2, 2, col);
        }
    }
}

} // namespace Graphics
