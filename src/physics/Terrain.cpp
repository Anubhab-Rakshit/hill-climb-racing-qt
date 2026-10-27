#include "Terrain.h"
#include <cmath>
#include <algorithm>

namespace Physics {

Terrain::Terrain()
    : m_biomeId("countryside")
    , m_friction(1.25f)
    , m_gravity(9.81f)
{
    setBiome("countryside");
}

void Terrain::setBiome(const std::string& biomeId, int level) {
    m_biomeId = biomeId;
    m_level = std::clamp(level, 1, 5);
    m_spec = BiomeRegistry::getBiome(biomeId);
    m_levelSpec = m_spec.getLevel(m_level);
    m_activeCheckpoints = m_levelSpec.checkpoints;
    m_friction = m_spec.friction;
    m_gravity = m_spec.gravity;
    m_airDensity = m_spec.airDensity;

    // Precalculate static terrain base heights for 72,000 samples (zero runtime transcendental math)
    m_baseHeightCache.resize(DEFORM_SAMPLES);
    for (size_t i = 0; i < DEFORM_SAMPLES; ++i) {
        float px = DEFORM_MIN_X + i * DEFORM_DX;
        m_baseHeightCache[i] = computeRawBaseHeight(px);
    }

    resetDeformation();
    spawnItems();
}

void Terrain::setTestIncline(float angleDeg) {
    m_biomeId = "test_ramp";
    m_friction = 1.35f;
    m_gravity = 9.81f;
    m_airDensity = 1.0f;
    float rad = angleDeg * 3.14159265f / 180.0f;
    float slopeVal = std::tan(rad);
    m_baseHeightCache.resize(DEFORM_SAMPLES);
    for (size_t i = 0; i < DEFORM_SAMPLES; ++i) {
        float px = DEFORM_MIN_X + i * DEFORM_DX;
        m_baseHeightCache[i] = px * slopeVal;
    }
    resetDeformation();
    m_items.clear();
    m_milestones.clear();
}

void Terrain::reset() {
    for (auto& item : m_items) {
        item.collected = false;
    }
    resetDeformation();
}

void Terrain::resetDeformation() {
    if (m_deformGrid.size() != DEFORM_SAMPLES) {
        m_deformGrid.assign(DEFORM_SAMPLES, TerrainDeformation{});
    } else {
        std::fill(m_deformGrid.begin(), m_deformGrid.end(), TerrainDeformation{});
    }
    m_modifiedMinIdx = static_cast<int>(DEFORM_SAMPLES);
    m_modifiedMaxIdx = -1;
}

float Terrain::getDifficultyFactor(float x) const {
    float lx = std::max(0.0f, x - 20.0f);
    float maxL = std::max(100.0f, m_levelSpec.lengthMeters);
    return std::clamp(lx / maxL, 0.0f, 1.0f);
}

const CheckpointInfo* Terrain::getNextCheckpoint(float currentDist) const {
    for (const auto& cp : m_activeCheckpoints) {
        if (cp.distance > currentDist + 0.1f) {
            return &cp;
        }
    }
    return nullptr;
}

float Terrain::getBaseHeight(float x) const {
    if (m_baseHeightCache.empty()) {
        return computeRawBaseHeight(x);
    }
    float fx = (x - DEFORM_MIN_X) / DEFORM_DX;
    if (fx <= 0.0f) return 0.0f;
    int idx = static_cast<int>(fx);
    if (idx + 1 >= static_cast<int>(m_baseHeightCache.size())) {
        return (idx < static_cast<int>(m_baseHeightCache.size())) ? m_baseHeightCache[idx] : computeRawBaseHeight(x);
    }
    float t = fx - static_cast<float>(idx);
    return m_baseHeightCache[idx] * (1.0f - t) + m_baseHeightCache[idx + 1] * t;
}

float Terrain::computeRawBaseHeight(float x) const {
    // Starting runway is flat for smooth launch and acceleration
    if (x < 20.0f) {
        return 0.0f;
    }

    float lx = x - 20.0f;
    // Cosine smoothstep entry ramp: C1 continuous, zero slope at x=20
    float ramp = (lx < 20.0f) ? (0.5f - 0.5f * std::cos(lx * 3.14159265f / 20.0f)) : 1.0f;

    // Level progress difficulty: 0.0 at level start -> 1.0 at finish line
    float diff = getDifficultyFactor(x);
    float pDist = std::pow(diff, 1.35f);
    float lvlFactor = 1.0f + 0.028f * (m_level - 1);
    float ampMult = 1.0f + 0.36f * pDist;
    float L = std::max(200.0f, m_levelSpec.lengthMeters);

    // Smooth C1-continuous Hann envelope
    auto smoothWindow = [](float t) -> float {
        float s = std::sin(t * 3.14159265f);
        return s * s;
    };

    // 1. Asymmetric Curvy Harmonics (Sharp launch crests + dynamic slope transitions)
    auto curvyWave = [](float phase) -> float {
        return std::sin(phase) + 0.24f * std::sin(2.0f * phase - 0.35f) + 0.07f * std::sin(3.0f * phase + 0.70f);
    };

    // 2. Off-Road Surface Roughness & Moguls (Physical chatter, ruts and bumps)
    auto roughNoise = [](float xPos) -> float {
        return 0.14f * std::sin(0.24f * xPos) * std::cos(0.12f * xPos)
             + 0.07f * std::sin(0.50f * xPos + 1.2f)
             + 0.03f * std::sin(1.05f * xPos);
    };

    float roughScale = 0.18f + 0.035f * (m_level - 1);
    float roughBase = roughNoise(lx) * roughScale * (1.0f + 0.90f * pDist);

    // Dynamic washboard moguls emerging as distance traversed increases (suspension bounce & chatter)
    float washboard = 0.0f;
    if (lx > 140.0f) {
        float wFrac = std::min(1.0f, (lx - 140.0f) / (L * 0.45f));
        float wScale = 0.15f + 0.03f * (m_level - 1);
        washboard = std::sin(lx * 0.88f) * std::cos(lx * 0.35f) * (wScale * wFrac);
    }
    float rough = roughBase + washboard;

    // Physics-Grounded Launch Kicker with upward parabolic ramp and downhill landing transition
    auto kicker = [](float xVal, float startX, float rampLen, float landLen, float height) -> float {
        if (xVal <= startX || xVal >= startX + rampLen + landLen) return 0.0f;
        if (xVal < startX + rampLen) {
            float t = (xVal - startX) / rampLen;
            return height * t * t;
        } else {
            float u = (xVal - (startX + rampLen)) / landLen;
            float mLand = -0.26f;
            float h00 = 1.0f - 3.0f * u * u + 2.0f * u * u * u;
            float h10 = u - 2.0f * u * u + u * u * u;
            return std::max(0.0f, height * h00 + (mLand * landLen) * h10);
        }
    };

    if (m_biomeId == "desert") {
        // Asymmetric windward sand dunes with sharp slip-faces and canyon speed runs
        float phase = lx * (0.065f * (1.0f + 0.10f * pDist)) + 0.18f * std::sin(0.032f * lx);
        float d1 = curvyWave(phase) * (1.88f * ampMult * lvlFactor);
        float d2 = std::sin(lx * 0.13f) * (0.30f + 0.18f * pDist);

        float k1 = kicker(lx, L * 0.33f, 26.0f, 58.0f, 4.1f + 0.12f * m_level);
        float k2 = kicker(lx, L * 0.73f, 30.0f, 64.0f, 4.4f + 0.15f * m_level);

        float duneDrop = 0.0f;
        if (lx > L * 0.46f && lx < L * 0.58f) {
            float dt = (lx - L * 0.46f) / (L * 0.12f);
            duneDrop = -smoothWindow(dt) * (2.2f * pDist);
        }

        return (d1 + d2 + rough * 0.90f + k1 + k2 + duneDrop) * ramp;
    } else if (m_biomeId == "arctic") {
        // Glacial ice plateaus, slippery crevasses and high-speed ice chatter
        float phase = lx * (0.066f * (1.0f + 0.10f * pDist)) + 0.18f * std::sin(0.033f * lx);
        float a1 = curvyWave(phase) * (1.86f * ampMult * lvlFactor);
        float a2 = std::sin(lx * 0.14f) * (0.30f + 0.18f * pDist);

        float k1 = kicker(lx, L * 0.33f, 26.0f, 58.0f, 4.1f + 0.12f * m_level);
        float k2 = kicker(lx, L * 0.73f, 30.0f, 64.0f, 4.4f + 0.15f * m_level);

        float crevasse = 0.0f;
        if (lx > L * 0.46f && lx < L * 0.58f) {
            float ct = (lx - L * 0.46f) / (L * 0.12f);
            crevasse = -smoothWindow(ct) * (2.2f * pDist);
        }

        return (a1 + a2 + rough * 1.10f + k1 + k2 + crevasse) * ramp;
    } else if (m_biomeId == "mountain") {
        // Craggy mountainous ascents, alpine ravines and rock springboards
        float phase = lx * (0.066f * (1.0f + 0.10f * pDist)) + 0.18f * std::sin(0.034f * lx);
        float m1 = curvyWave(phase) * (1.88f * ampMult * lvlFactor);
        float m2 = std::sin(lx * 0.14f) * (0.30f + 0.20f * pDist);

        float k1 = kicker(lx, L * 0.33f, 26.0f, 58.0f, 4.1f + 0.12f * m_level);
        float k2 = kicker(lx, L * 0.73f, 30.0f, 64.0f, 4.4f + 0.15f * m_level);

        float gorge = 0.0f;
        if (lx > L * 0.46f && lx < L * 0.58f) {
            float gt = (lx - L * 0.46f) / (L * 0.12f);
            gorge = -smoothWindow(gt) * (2.4f * pDist);
        }

        return (m1 + m2 + rough * 1.20f + k1 + k2 + gorge) * ramp;
    } else if (m_biomeId == "moon") {
        // Wide deep impact craters and massive low-gravity zero-G launch kickers
        float phase = lx * (0.055f * (1.0f + 0.10f * pDist)) + 0.18f * std::sin(0.028f * lx);
        float c1 = curvyWave(phase) * (2.40f * ampMult * lvlFactor);
        float c2 = std::cos(lx * 0.11f) * (0.38f + 0.22f * pDist);

        float k1 = kicker(lx, L * 0.33f, 28.0f, 62.0f, 4.4f + 0.15f * m_level);
        float k2 = kicker(lx, L * 0.73f, 32.0f, 68.0f, 4.8f + 0.20f * m_level);

        float crater = 0.0f;
        if (lx > 22.0f && lx < 65.0f) {
            float ct = (lx - 22.0f) / 43.0f;
            crater = -smoothWindow(ct) * 1.5f;
        } else if (lx > L * 0.44f && lx < L * 0.58f) {
            float ct = (lx - L * 0.44f) / (L * 0.14f);
            crater = -smoothWindow(ct) * (2.8f * pDist);
        }

        return (c1 + c2 + rough * 0.75f + k1 + k2 + crater) * ramp;
    } else if (m_biomeId == "volcano") {
        // Dense basalt ridges, stepped thermal shelves and caldera rims
        float phase = lx * (0.066f * (1.0f + 0.10f * pDist)) + 0.18f * std::sin(0.035f * lx);
        float v1 = curvyWave(phase) * (1.88f * ampMult * lvlFactor);
        float v2 = std::sin(lx * 0.15f) * (0.30f + 0.20f * pDist);

        float k1 = kicker(lx, L * 0.33f, 26.0f, 58.0f, 4.1f + 0.12f * m_level);
        float k2 = kicker(lx, L * 0.73f, 30.0f, 64.0f, 4.4f + 0.15f * m_level);

        float caldera = 0.0f;
        if (lx > L * 0.46f && lx < L * 0.58f) {
            float ct = (lx - L * 0.46f) / (L * 0.12f);
            caldera = -smoothWindow(ct) * (2.4f * pDist);
        }

        return (v1 + v2 + rough * 1.15f + k1 + k2 + caldera) * ramp;
    }

    // Default Countryside: High-variety signature Hill Climb course with non-uniform curvy rolling hills & roughness
    float phase = lx * (0.065f * (1.0f + 0.10f * pDist)) + 0.18f * std::sin(0.032f * lx);
    float baseHills = curvyWave(phase) * (1.88f * ampMult * lvlFactor);
    float rhythmBumps = std::sin(lx * 0.13f) * (0.30f + 0.18f * pDist);

    float k1 = kicker(lx, L * 0.33f, 26.0f, 58.0f, 4.1f + 0.12f * m_level);
    float k2 = kicker(lx, L * 0.73f, 30.0f, 64.0f, 4.4f + 0.15f * m_level);

    float valleyDip = 0.0f;
    if (lx > L * 0.46f && lx < L * 0.58f) {
        float vt = (lx - L * 0.46f) / (L * 0.12f);
        valleyDip = -smoothWindow(vt) * (2.2f * pDist);
    }

    return (baseHills + rhythmBumps + rough + k1 + k2 + valleyDip) * ramp;
}

float Terrain::getHeight(float x) const {
    float base = getBaseHeight(x);
    if (m_modifiedMinIdx <= m_modifiedMaxIdx) {
        float deform = getDeformation(x).compression;
        return base - deform;
    }
    return base;
}

TerrainDeformation Terrain::getDeformation(float x) const {
    if (m_deformGrid.empty() || m_modifiedMinIdx > m_modifiedMaxIdx) return TerrainDeformation{};
    float fx = (x - DEFORM_MIN_X) / DEFORM_DX;
    int idx = static_cast<int>(fx);
    if (idx < m_modifiedMinIdx - 1 || idx > m_modifiedMaxIdx + 1) return TerrainDeformation{};
    if (idx < 0) return m_deformGrid.front();
    if (idx + 1 >= static_cast<int>(m_deformGrid.size())) {
        return (idx < static_cast<int>(m_deformGrid.size())) ? m_deformGrid[idx] : TerrainDeformation{};
    }
    float t = fx - static_cast<float>(idx);
    const auto& d0 = m_deformGrid[idx];
    const auto& d1 = m_deformGrid[idx + 1];
    TerrainDeformation res;
    res.compression = d0.compression * (1.0f - t) + d1.compression * t;
    res.grassFlattening = d0.grassFlattening * (1.0f - t) + d1.grassFlattening * t;
    res.grassTear = d0.grassTear * (1.0f - t) + d1.grassTear * t;
    res.age = std::min(d0.age, d1.age);
    return res;
}

void Terrain::applyWheelDeformation(float wheelX, float wheelRadius, float normalForce, float slipSpeed, float dt) {
    if (m_deformGrid.empty()) return;
    float footprint = std::max(0.18f, wheelRadius * 0.90f);
    float startX = wheelX - footprint;
    float endX = wheelX + footprint;
    int idxStart = std::max(0, static_cast<int>((startX - DEFORM_MIN_X) / DEFORM_DX));
    int idxEnd = std::min(static_cast<int>(m_deformGrid.size()) - 1, static_cast<int>((endX - DEFORM_MIN_X) / DEFORM_DX));

    m_modifiedMinIdx = std::min(m_modifiedMinIdx, idxStart);
    m_modifiedMaxIdx = std::max(m_modifiedMaxIdx, idxEnd);

    // Biome-specific deformation limits and compaction rates
    float maxComp = 0.045f;
    float compRate = 0.08f;
    if (m_biomeId == "desert") {
        maxComp = 0.060f; // Soft deep sand dunes
        compRate = 0.14f;
    } else if (m_biomeId == "arctic") {
        maxComp = 0.020f; // Packed snow / ice
        compRate = 0.04f;
    } else if (m_biomeId == "mountain") {
        maxComp = 0.028f; // Rocky soil
        compRate = 0.06f;
    } else if (m_biomeId == "moon") {
        maxComp = 0.040f; // Fine lunar regolith
        compRate = 0.09f;
    } else if (m_biomeId == "volcano") {
        maxComp = 0.050f; // Volcanic ash
        compRate = 0.12f;
    }

    float absSlip = std::abs(slipSpeed);
    float loadFactor = std::clamp(normalForce / 3500.0f, 0.4f, 2.5f);

    for (int i = idxStart; i <= idxEnd; ++i) {
        float px = DEFORM_MIN_X + i * DEFORM_DX;
        float dist = std::abs(px - wheelX);
        if (dist > footprint) continue;

        float normDist = dist / footprint;
        float w = 1.0f - normDist * normDist;
        auto& pt = m_deformGrid[i];

        // 1. Physical compression into terrain
        float compDelta = compRate * loadFactor * dt * w;
        pt.compression = std::min(maxComp, pt.compression + compDelta);

        // 2. Grass flattening under tire tread (bends blades while preserving lush green color)
        float flatDelta = 6.0f * loadFactor * dt * w;
        pt.grassFlattening = std::min(1.0f, pt.grassFlattening + flatDelta);

        // 3. Grass tearing / localized soil exposure occurs ONLY under genuine tire slip (burnout or skid)
        // Clean rolling or static vehicle weight never uproots or tears turf
        float slipThreshold = 1.2f; // m/s
        float excessSlip = std::max(0.0f, absSlip - slipThreshold);
        if (excessSlip > 0.05f) {
            float tearDelta = excessSlip * 0.35f * dt * w;
            pt.grassTear = std::min(1.0f, pt.grassTear + tearDelta);
        }

        pt.age = 0.0f;
    }
}

void Terrain::updateDeformation(float dt, float activeMinX, float activeMaxX) {
    if (m_deformGrid.empty() || m_modifiedMinIdx > m_modifiedMaxIdx) return;
    int idxStart = std::max(m_modifiedMinIdx, static_cast<int>((activeMinX - DEFORM_MIN_X) / DEFORM_DX));
    int idxEnd = std::min(m_modifiedMaxIdx, static_cast<int>((activeMaxX - DEFORM_MIN_X) / DEFORM_DX));
    if (idxStart > idxEnd) return;

    for (int i = idxStart; i <= idxEnd; ++i) {
        auto& pt = m_deformGrid[i];
        if (pt.grassFlattening <= 0.0f && pt.compression <= 0.0f && pt.grassTear <= 0.0f) continue;
        pt.age += dt;

        // Subtle gradual elastic restoration over time
        if (pt.age > 6.0f && pt.grassFlattening > 0.0f) {
            pt.grassFlattening = std::max(0.0f, pt.grassFlattening - 0.08f * dt);
        }
        if (pt.age > 10.0f && pt.compression > 0.0f) {
            pt.compression = std::max(0.0f, pt.compression - 0.005f * dt);
        }
        if (pt.age > 12.0f && pt.grassTear > 0.0f) {
            pt.grassTear = std::max(0.0f, pt.grassTear - 0.05f * dt);
        }
    }
}

float Terrain::getSlope(float x) const {
    const float eps = 0.05f;
    float y1 = getHeight(x - eps);
    float y2 = getHeight(x + eps);
    return (y2 - y1) / (2.0f * eps);
}

Vec2 Terrain::getNormal(float x) const {
    float m = getSlope(x);
    Vec2 n(-m, 1.0f);
    return n.normalized();
}

void Terrain::collectItem(size_t index) {
    if (index < m_items.size()) {
        m_items[index].collected = true;
    }
}

void Terrain::spawnItems() {
    m_items.clear();
    m_milestones.clear();

    float maxTrackX = m_levelSpec.lengthMeters + 120.0f;

    // 1. Spawn Checkpoint Gates & Milestones for current level
    for (const auto& cp : m_activeCheckpoints) {
        MilestoneSign sign;
        sign.distanceMeters = static_cast<int>(cp.distance);
        sign.position = {cp.distance, getHeight(cp.distance)};
        sign.isCheckpoint = true;
        sign.checkpointName = cp.name;
        sign.checkpointIndex = cp.index;
        sign.isFinishLine = cp.isFinishLine;
        m_milestones.push_back(sign);
    }

    // Intermediate 100m distance markers
    for (int dist = 100; dist <= static_cast<int>(maxTrackX); dist += 100) {
        bool nearCp = false;
        for (const auto& cp : m_activeCheckpoints) {
            if (std::abs(cp.distance - static_cast<float>(dist)) < 25.0f) {
                nearCp = true;
                break;
            }
        }
        if (!nearCp) {
            MilestoneSign sign;
            sign.distanceMeters = dist;
            float sx = static_cast<float>(dist);
            sign.position = {sx, getHeight(sx)};
            sign.isCheckpoint = false;
            m_milestones.push_back(sign);
        }
    }

    // 2. Progressive Tactical Fuel Spawning at Large Distances (Tuned to Fuel Exhaustion Distance)
    // Distance between canisters calibrated so careless driving exhausts fuel near or before the canister.
    // Progresses across levels: L1: ~220m-250m up to L5: ~400m-470m.
    float baseStartInterval = 220.0f + 45.0f * (m_level - 1); // Level 1: 220m, Level 5: 400m
    float maxLevelInterval = 250.0f + 55.0f * (m_level - 1);  // Level 1: 250m, Level 5: 470m
    float fuelTargetX = baseStartInterval;
    float prevFuelX = 0.0f;

    while (fuelTargetX < maxTrackX) {
        // Find best stable location (closest crest or flat saddle where |slope| <= 0.35)
        float bestX = fuelTargetX;
        float minSlopeMag = 999.0f;

        // Primary search window +/- 14m around target
        for (float searchX = fuelTargetX - 14.0f; searchX <= fuelTargetX + 14.0f; searchX += 0.25f) {
            if (searchX < 25.0f) continue;
            float slopeMag = std::abs(getSlope(searchX));
            if (slopeMag < minSlopeMag) {
                minSlopeMag = slopeMag;
                bestX = searchX;
            }
        }

        // If minSlopeMag is still > 0.22, search anywhere in reachable window
        if (minSlopeMag > 0.22f) {
            float minX = std::max(25.0f, prevFuelX + 65.0f);
            float maxX = (prevFuelX > 0.0f) ? (prevFuelX + maxLevelInterval) : 120.0f;
            for (float sx = minX; sx <= maxX; sx += 0.25f) {
                float sm = std::abs(getSlope(sx));
                if (sm < minSlopeMag) {
                    minSlopeMag = sm;
                    bestX = sx;
                }
            }
        }

        // Safety clamp on max interval to strictly ensure reachable spacing
        if (prevFuelX > 0.0f && (bestX - prevFuelX) > maxLevelInterval) {
            bestX = prevFuelX + maxLevelInterval;
        }

        WorldItem fuel;
        fuel.position = {bestX, getHeight(bestX) + 1.25f};
        fuel.type = ItemType::FUEL_CANISTER;
        fuel.value = 100;
        fuel.collected = false;
        m_items.push_back(fuel);

        prevFuelX = bestX;
        // Progressive tactical interval expanding across checkpoints
        float diff = getDifficultyFactor(bestX);
        float nextInterval = baseStartInterval + (maxLevelInterval - baseStartInterval) * (diff * diff);
        nextInterval = std::clamp(nextInterval, baseStartInterval, maxLevelInterval);
        fuelTargetX = bestX + nextInterval;
    }

    // 3. Collectible Ballistic Coin Arcs and Trails
    for (float x = 32.0f; x < maxTrackX; x += 14.0f) {
        // Skip spawning near fuel canisters to prevent overlap
        bool nearFuel = false;
        for (const auto& it : m_items) {
            if (it.type == ItemType::FUEL_CANISTER && std::abs(it.position.x - x) < 6.0f) {
                nearFuel = true;
                break;
            }
        }
        if (nearFuel) continue;

        float y = getHeight(x);
        float slope = getSlope(x);
        float nextSlope = getSlope(x + 6.0f);

        // Detecting a jump kicker or crest: spawn 7-coin parabolic rainbow arc (peak gold coin at 2.8m above lip)
        if (slope > 0.15f && nextSlope < slope) {
            for (int k = 0; k < 7; ++k) {
                float coinX = x + k * 2.8f;
                float coinBaseY = getHeight(coinX);
                float arcFrac = static_cast<float>(k) / 6.0f;
                float arcH = std::sin(arcFrac * 3.14159265f) * 2.4f + 1.25f;

                WorldItem arcCoin;
                arcCoin.position = {coinX, coinBaseY + arcH};
                arcCoin.type = (k == 3) ? ItemType::COIN_GOLD : (k == 2 || k == 4) ? ItemType::COIN_SILVER : ItemType::COIN_BRONZE;
                arcCoin.value = (arcCoin.type == ItemType::COIN_GOLD) ? 100 : (arcCoin.type == ItemType::COIN_SILVER) ? 25 : 5;
                arcCoin.collected = false;
                m_items.push_back(arcCoin);
            }
            x += 22.0f;
            continue;
        }

        // Standard ground-follow coins
        WorldItem coin;
        coin.position = {x, y + 1.15f};
        coin.type = (static_cast<int>(x) % 60 == 0) ? ItemType::COIN_GOLD :
                    (static_cast<int>(x) % 20 == 0) ? ItemType::COIN_SILVER : ItemType::COIN_BRONZE;
        coin.value = (coin.type == ItemType::COIN_GOLD) ? 100 : (coin.type == ItemType::COIN_SILVER) ? 25 : 5;
        coin.collected = false;
        m_items.push_back(coin);
    }
}

} // namespace Physics
