#define _USE_MATH_DEFINES
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

void Terrain::setBiome(const std::string& biomeId) {
    m_biomeId = biomeId;
    m_spec = BiomeRegistry::getBiome(biomeId);
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
    // Continuous progressive scaling: 0.0 at start, 1.0 at 1000m, 2.0 at 2000m, 3.0 at 3000m
    return lx / 1000.0f;
}

const CheckpointInfo* Terrain::getNextCheckpoint(float currentDist) const {
    for (const auto& cp : m_spec.checkpoints) {
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

    // Dynamic difficulty factor scaling: increases hill slopes, valleys, and obstacle frequency
    float diff = getDifficultyFactor(x);
    float ampMult = 1.0f + 0.85f * std::min(diff, 3.0f);

    // Smooth C1-continuous Hann envelope: w(0)=0, w'(0)=0, w(1)=0, w'(1)=0 (zero slope kinks at boundaries)
    auto smoothWindow = [](float t) -> float {
        float s = std::sin(t * 3.14159265f);
        return s * s;
    };

    if (m_biomeId == "desert") {
        // Asymmetric windward sand dunes with slip-face drops and varied crests
        float pD = lx * 0.014f + 0.28f * std::sin(lx * 0.0041f);
        float d1 = std::sin(pD) * (8.5f * ampMult);
        float d2 = std::sin(lx * 0.036f) * (2.8f + 1.8f * std::min(diff, 2.5f));
        float slip = std::sin(lx * 0.080f) * (0.8f + 0.5f * std::min(diff, 2.5f));
        // Dune bowl feature at 650m - 1100m
        float bowl = 0.0f;
        if (lx > 650.0f && lx < 1100.0f) {
            float bt = (lx - 650.0f) / 450.0f;
            bowl = -smoothWindow(bt) * (5.0f * std::min(diff, 2.0f));
        }
        // Sand cliff at 1600m - 2200m
        float sandCliff = 0.0f;
        if (lx > 1600.0f && lx < 2200.0f) {
            float st = (lx - 1600.0f) / 600.0f;
            sandCliff = smoothWindow(st) * 7.5f;
        }
        return (d1 + d2 + slip + bowl + sandCliff) * ramp;
    } else if (m_biomeId == "arctic") {
        // Glacial ice plateaus and slippery crevasses
        float pA = lx * 0.015f + 0.25f * std::sin(lx * 0.0039f);
        float a1 = std::sin(pA) * (9.0f * ampMult);
        float a2 = std::sin(lx * 0.038f) * (3.0f + 1.8f * std::min(diff, 2.5f));
        float iceRipples = std::sin(lx * 0.082f) * (0.7f + 0.4f * std::min(diff, 2.5f));
        // Deep glacial crevasse with ramp recovery at 800m - 1200m
        float crevasse = 0.0f;
        if (lx > 800.0f && lx < 1200.0f) {
            float ct = (lx - 800.0f) / 400.0f;
            crevasse = -smoothWindow(ct) * (5.5f * std::min(diff, 2.0f));
        }
        float iceWall = 0.0f;
        if (lx > 1750.0f && lx < 2350.0f) {
            float it = (lx - 1750.0f) / 600.0f;
            iceWall = smoothWindow(it) * 8.0f;
        }
        return (a1 + a2 + iceRipples + crevasse + iceWall) * ramp;
    } else if (m_biomeId == "mountain") {
        // Craggy mountainous ascents, jagged rocky ledges and alpine peaks
        float pM = lx * 0.0155f + 0.28f * std::sin(lx * 0.0042f);
        float m1 = std::sin(pM) * (9.5f * ampMult);
        float m2 = std::sin(lx * 0.040f) * (3.2f + 1.8f * std::min(diff, 2.5f));
        float crag = std::sin(lx * 0.088f) * (0.8f + 0.4f * std::min(diff, 2.5f));
        // Steep summit ascent at 950m - 1450m
        float summit = 0.0f;
        if (lx > 950.0f && lx < 1450.0f) {
            float st = (lx - 950.0f) / 500.0f;
            summit = smoothWindow(st) * (6.5f * std::min(diff, 2.0f));
        }
        float ridgeWall = 0.0f;
        if (lx > 1800.0f && lx < 2500.0f) {
            float rt = (lx - 1800.0f) / 700.0f;
            ridgeWall = smoothWindow(rt) * 9.0f;
        }
        return (m1 + m2 + crag + summit + ridgeWall) * ramp;
    } else if (m_biomeId == "moon") {
        // Wide deep impact craters and massive parabolic low-gravity launch mounds
        float c1 = std::sin(lx * 0.013f) * (8.5f * (1.0f + 0.65f * std::min(diff, 2.5f)));
        float c2 = std::cos(lx * 0.028f) * (3.8f + 1.6f * std::min(diff, 2.5f));
        float c3 = std::sin(lx * 0.065f) * (1.2f + 0.6f * std::min(diff, 2.5f));
        float crater = 0.0f;
        if (lx > 700.0f && lx < 1150.0f) {
            float ct = (lx - 700.0f) / 450.0f;
            crater = -smoothWindow(ct) * (5.5f * std::min(diff, 2.0f));
        }
        float superCrater = 0.0f;
        if (lx > 1700.0f && lx < 2300.0f) {
            float st = (lx - 1700.0f) / 600.0f;
            superCrater = -smoothWindow(st) * 7.5f;
        }
        return (c1 + c2 + c3 + crater + superCrater) * ramp;
    } else if (m_biomeId == "volcano") {
        // Basalt ridges, thermal vent steps and caldera rims
        float pV = lx * 0.0155f + 0.25f * std::sin(lx * 0.0039f);
        float v1 = std::sin(pV) * (9.1f * ampMult);
        float v2 = std::sin(lx * 0.040f) * (2.9f + 1.6f * std::min(diff, 2.5f));
        float basalt = std::sin(lx * 0.088f) * (0.8f + 0.4f * std::min(diff, 2.5f));
        // Caldera wall ascent at 850m - 1350m
        float caldera = 0.0f;
        if (lx > 850.0f && lx < 1350.0f) {
            float vt = (lx - 850.0f) / 500.0f;
            caldera = smoothWindow(vt) * (6.2f * std::min(diff, 2.0f));
        }
        float magmaPeak = 0.0f;
        if (lx > 1850.0f && lx < 2550.0f) {
            float mt = (lx - 1850.0f) / 700.0f;
            magmaPeak = smoothWindow(mt) * 8.0f;
        }
        return (v1 + v2 + basalt + caldera + magmaPeak) * ramp;
    }

    // Default Countryside: Signature progressive Hill Climb course with steep slopes and technical humps
    float basePhase = (lx < 125.0f) ? (lx * 0.019f) : (lx * 0.019f + 0.28f * std::sin((lx - 125.0f) * 0.0050f));
    float baseHills = std::sin(basePhase) * (9.2f * ampMult);
    float rhythmBumps = std::sin(lx * 0.044f) * (2.4f + 1.1f * std::min(diff, 2.0f));
    float steepRollers = std::sin(lx * 0.080f) * (0.8f + 0.4f * std::min(diff, 2.0f));
    float microRelief = std::sin(lx * 0.110f) * 0.30f;

    // Feature 1: Ski Jump Launch Ramp & Steep Crest (lx ~ 65 - 130m)
    float jumpRamp = 0.0f;
    if (lx > 65.0f && lx < 130.0f) {
        float jt = (lx - 65.0f) / 65.0f;
        jumpRamp = smoothWindow(jt) * 3.6f;
    }

    // Feature 2: Technical Moguls & Washboards (lx ~ 550 - 850m)
    float moguls = 0.0f;
    if (lx > 550.0f && lx < 850.0f) {
        float mt = (lx - 550.0f) / 300.0f;
        float envelope = smoothWindow(mt);
        moguls = std::sin(lx * 0.16f) * (0.50f + 0.35f * std::min(diff, 2.0f)) * envelope;
    }

    // Feature 3: Mountain Wall Ascent (lx ~ 950 - 1300m) - Steep technical climb
    float mountainWall = 0.0f;
    if (lx > 950.0f && lx < 1300.0f) {
        float mt = (lx - 950.0f) / 350.0f;
        mountainWall = smoothWindow(mt) * (5.0f + 2.0f * std::min(diff, 2.0f));
    }

    // Feature 4: Canyon Dip (lx ~ 1350 - 1680m) - Rapid drop with uphill exit
    float canyon = 0.0f;
    if (lx > 1350.0f && lx < 1680.0f) {
        float ct = (lx - 1350.0f) / 330.0f;
        canyon = -smoothWindow(ct) * (4.2f + 1.6f * std::min(diff, 2.0f));
    }

    // Feature 5: The Alpine Ascent & Technical Crests (lx ~ 1700 - 2250m)
    float alpine = 0.0f;
    if (lx > 1700.0f && lx < 2250.0f) {
        float at = (lx - 1700.0f) / 550.0f;
        float env = smoothWindow(at);
        alpine = (smoothWindow(at) * 6.5f + std::sin(lx * 0.065f) * 2.4f) * env;
    }

    // Feature 6: The Great Wall / Ridge Climb (lx ~ 2300 - 2950m) - Extreme climb
    float monsterRidge = 0.0f;
    if (lx > 2300.0f && lx < 2950.0f) {
        float rt = (lx - 2300.0f) / 650.0f;
        monsterRidge = smoothWindow(rt) * 7.5f;
    }

    // Feature 7: High-Altitude Mogul Field & Rolling Peaks (lx > 2950m)
    float extremeField = 0.0f;
    if (lx > 2950.0f) {
        extremeField = std::sin(lx * 0.035f) * 4.5f + std::sin(lx * 0.095f) * 1.8f;
    }

    return (baseHills + rhythmBumps + steepRollers + microRelief + jumpRamp + moguls + mountainWall + canyon + alpine + monsterRidge + extremeField) * ramp;
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

        // 2. Grass flattening under tire tread
        float flatDelta = 6.0f * loadFactor * dt * w;
        pt.grassFlattening = std::min(1.0f, pt.grassFlattening + flatDelta);

        // 3. Grass tearing under tire slip
        float slipThreshold = 1.2f;
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

        // Gradual elastic restoration over time
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

    // 1. Spawn Checkpoint Gates & Milestones
    for (const auto& cp : m_spec.checkpoints) {
        MilestoneSign sign;
        sign.distanceMeters = static_cast<int>(cp.distance);
        sign.position = {cp.distance, getHeight(cp.distance)};
        sign.isCheckpoint = true;
        sign.checkpointName = cp.name;
        sign.checkpointIndex = cp.index;
        sign.isFinishLine = cp.isFinishLine;
        m_milestones.push_back(sign);
    }

    // Intermediate distance markers
    const int intermediateDists[] = {100, 400, 700, 1100, 1500, 2000, 2500, 3000};
    for (int dist : intermediateDists) {
        bool nearCp = false;
        for (const auto& cp : m_spec.checkpoints) {
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

    // 2. Progressive Fuel Spawning with Reachability Validation
    // Calibrated spacing: ~90m at start, scaling up to ~150m near steep summits (safe <= 165m)
    float fuelTargetX = 90.0f;
    float prevFuelX = 0.0f;
    while (fuelTargetX < 5000.0f) {
        float diff = getDifficultyFactor(fuelTargetX);
        float maxAllowedSpacing = 95.0f + std::min(diff, 2.0f) * 28.0f; // 95m at start -> ~151m

        float searchMin = (prevFuelX > 0.0f) ? std::max(prevFuelX + 55.0f, fuelTargetX - 16.0f) : 25.0f;
        float searchMax = (prevFuelX > 0.0f) ? (prevFuelX + maxAllowedSpacing - 2.0f) : 115.0f;
        if (searchMin >= searchMax) {
            searchMin = (prevFuelX > 0.0f) ? (prevFuelX + 50.0f) : 25.0f;
        }

        float bestX = (searchMin + searchMax) * 0.5f;
        float minSlopeMag = 999.0f;

        // Search for crest or trough (minimum slope) within safe spacing
        for (float sx = searchMin; sx <= searchMax; sx += 0.25f) {
            float sm = std::abs(getSlope(sx));
            if (sm < minSlopeMag) {
                minSlopeMag = sm;
                bestX = sx;
            }
        }

        // Broaden search backwards if still on slope > 0.30
        if (minSlopeMag > 0.30f && prevFuelX > 0.0f) {
            for (float sx = prevFuelX + 50.0f; sx <= searchMax; sx += 0.25f) {
                float sm = std::abs(getSlope(sx));
                if (sm < minSlopeMag) {
                    minSlopeMag = sm;
                    bestX = sx;
                }
            }
        }

        WorldItem fuel;
        fuel.position = {bestX, getHeight(bestX) + 1.25f};
        fuel.type = ItemType::FUEL_CANISTER;
        fuel.value = 45;
        fuel.collected = false;
        m_items.push_back(fuel);

        prevFuelX = bestX;
        // Progressive interval: grows from 90m at start to 150m
        float nextInterval = 90.0f + std::min(diff, 2.0f) * 28.0f;
        fuelTargetX = bestX + nextInterval;
    }

    // 3. Collectible Coin Arcs and Trails
    for (float x = 32.0f; x < 5000.0f; x += 15.0f) {
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

        // Detecting a jump crest: spawn 5-coin rainbow arc over the crest
        if (slope > 0.15f && nextSlope < slope) {
            for (int k = 0; k < 5; ++k) {
                float coinX = x + k * 3.0f;
                float coinBaseY = getHeight(coinX);
                float arcH = std::sin((static_cast<float>(k) / 4.0f) * 3.14159f) * 1.4f + 1.15f;

                WorldItem arcCoin;
                arcCoin.position = {coinX, coinBaseY + arcH};
                arcCoin.type = (k == 2) ? ItemType::COIN_GOLD : ItemType::COIN_SILVER;
                arcCoin.value = (arcCoin.type == ItemType::COIN_GOLD) ? 100 : 25;
                arcCoin.collected = false;
                m_items.push_back(arcCoin);
            }
            x += 18.0f;
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
