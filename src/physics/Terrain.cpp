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
    resetDeformation();
    spawnItems();
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
}

float Terrain::getDifficultyFactor(float x) const {
    float lx = std::max(0.0f, x - 20.0f);
    return std::clamp(lx / 1750.0f, 0.0f, 1.0f);
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
    // Starting runway is flat for smooth launch and acceleration
    if (x < 20.0f) {
        return 0.0f;
    }

    float lx = x - 20.0f;
    // Cosine smoothstep entry ramp: C1 continuous, zero slope at x=20
    float ramp = (lx < 20.0f) ? (0.5f - 0.5f * std::cos(lx * 3.14159265f / 20.0f)) : 1.0f;

    // Dynamic difficulty factor: 0.0 at start -> 1.0 at final checkpoint (1750m)
    float diff = getDifficultyFactor(x);
    float ampMult = 1.0f + 0.65f * diff;

    if (m_biomeId == "desert") {
        // Asymmetric windward sand dunes with slip-face drops
        float d1 = std::sin(lx * 0.015f) * (8.5f * ampMult);
        float d2 = std::sin(lx * 0.038f + 0.8f) * (2.8f + 1.6f * diff);
        float slip = std::sin(lx * 0.085f) * (0.8f + 0.4f * diff);
        // Dune bowl feature at 650m - 1100m
        float bowl = 0.0f;
        if (lx > 650.0f && lx < 1100.0f) {
            float bt = (lx - 650.0f) / 450.0f;
            bowl = -std::sin(bt * 3.14159f) * (4.5f * diff);
        }
        return (d1 + d2 + slip + bowl) * ramp;
    } else if (m_biomeId == "arctic") {
        // Glacial ice plateaus and slippery crevasses
        float a1 = std::sin(lx * 0.016f) * (9.0f * ampMult);
        float a2 = std::sin(lx * 0.040f) * (3.0f + 1.6f * diff);
        float iceRipples = std::sin(lx * 0.090f) * (0.7f + 0.3f * diff);
        // Deep glacial crevasse with ramp recovery at 800m - 1200m
        float crevasse = 0.0f;
        if (lx > 800.0f && lx < 1200.0f) {
            float ct = (lx - 800.0f) / 400.0f;
            crevasse = -std::sin(ct * 3.14159f) * (5.0f * diff);
        }
        return (a1 + a2 + iceRipples + crevasse) * ramp;
    } else if (m_biomeId == "mountain") {
        // Craggy mountainous ascents, jagged rocky ledges and alpine peaks
        float m1 = std::sin(lx * 0.017f) * (9.5f * ampMult);
        float m2 = std::sin(lx * 0.042f) * (3.2f + 1.6f * diff);
        float crag = std::sin(lx * 0.095f) * (0.8f + 0.4f * diff);
        // Steep summit ascent at 950m - 1450m
        float summit = 0.0f;
        if (lx > 950.0f && lx < 1450.0f) {
            float st = (lx - 950.0f) / 500.0f;
            summit = std::sin(st * 3.14159f) * (6.0f * diff);
        }
        return (m1 + m2 + crag + summit) * ramp;
    } else if (m_biomeId == "moon") {
        // Wide deep impact craters and massive parabolic low-gravity launch mounds
        float c1 = std::sin(lx * 0.013f) * (10.0f * ampMult);
        float c2 = std::cos(lx * 0.028f) * (3.8f + 1.8f * diff);
        float c3 = std::sin(lx * 0.065f) * (1.1f + 0.5f * diff);
        float crater = 0.0f;
        if (lx > 700.0f && lx < 1150.0f) {
            float ct = (lx - 700.0f) / 450.0f;
            crater = -std::sin(ct * 3.14159f) * (5.5f * diff);
        }
        return (c1 + c2 + c3 + crater) * ramp;
    } else if (m_biomeId == "volcano") {
        // Basalt ridges, thermal vent steps and caldera rims
        float v1 = std::sin(lx * 0.018f) * (10.0f * ampMult);
        float v2 = std::sin(lx * 0.044f) * (3.2f + 1.6f * diff);
        float basalt = std::sin(lx * 0.095f) * (0.8f + 0.4f * diff);
        // Caldera wall ascent at 850m - 1350m
        float caldera = 0.0f;
        if (lx > 850.0f && lx < 1350.0f) {
            float vt = (lx - 850.0f) / 500.0f;
            caldera = std::sin(vt * 3.14159f) * (6.5f * diff);
        }
        return (v1 + v2 + basalt + caldera) * ramp;
    }

    // Default Countryside: High-variety signature Hill Climb course
    float baseHills = std::sin(lx * 0.015f) * (8.5f * ampMult);
    float rhythmBumps = std::sin(lx * 0.042f) * (2.2f + 1.2f * diff);
    float microRelief = std::sin(lx * 0.090f) * 0.5f;

    // Feature 1: Ski Jump Launch Ramp (lx ~ 65 - 125m)
    float jumpRamp = 0.0f;
    if (lx > 65.0f && lx < 125.0f) {
        float jt = (lx - 65.0f) / 60.0f;
        jumpRamp = std::sin(jt * 3.14159f) * 3.5f;
    }

    // Feature 2: Technical Moguls & Washboards (lx ~ 550 - 850m)
    float moguls = 0.0f;
    if (lx > 550.0f && lx < 850.0f) {
        float mt = (lx - 550.0f) / 300.0f;
        float envelope = std::sin(mt * 3.14159f);
        moguls = std::sin(lx * 0.18f) * (0.6f + 0.4f * diff) * envelope;
    }

    // Feature 3: Mountain Wall Ascent (lx ~ 950 - 1300m)
    float mountainWall = 0.0f;
    if (lx > 950.0f && lx < 1300.0f) {
        float mt = (lx - 950.0f) / 350.0f;
        mountainWall = std::sin(mt * 3.14159f) * (6.0f + 2.5f * diff);
    }

    // Feature 4: Canyon Dip (lx ~ 1350 - 1680m)
    float canyon = 0.0f;
    if (lx > 1350.0f && lx < 1680.0f) {
        float ct = (lx - 1350.0f) / 330.0f;
        canyon = -std::sin(ct * 3.14159f) * (5.0f * diff);
    }

    return (baseHills + rhythmBumps + microRelief + jumpRamp + moguls + mountainWall + canyon) * ramp;
}

float Terrain::getHeight(float x) const {
    float base = getBaseHeight(x);
    float deform = getDeformation(x).compression;
    return base - deform;
}

TerrainDeformation Terrain::getDeformation(float x) const {
    if (m_deformGrid.empty()) return TerrainDeformation{};
    float fx = (x - DEFORM_MIN_X) / DEFORM_DX;
    if (fx < 0.0f) return m_deformGrid.front();
    int idx = static_cast<int>(fx);
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
        float flatDelta = 8.0f * loadFactor * dt * w;
        pt.grassFlattening = std::min(1.0f, pt.grassFlattening + flatDelta);

        // 3. Grass tearing / soil exposure under slip, acceleration, and braking
        float tearDelta = (absSlip * 0.18f + (loadFactor - 0.5f) * 0.12f) * 3.5f * dt * w;
        pt.grassTear = std::min(1.0f, pt.grassTear + tearDelta);

        pt.age = 0.0f;
    }
}

void Terrain::updateDeformation(float dt, float activeMinX, float activeMaxX) {
    if (m_deformGrid.empty()) return;
    int idxStart = std::max(0, static_cast<int>((activeMinX - DEFORM_MIN_X) / DEFORM_DX));
    int idxEnd = std::min(static_cast<int>(m_deformGrid.size()) - 1, static_cast<int>((activeMaxX - DEFORM_MIN_X) / DEFORM_DX));

    for (int i = idxStart; i <= idxEnd; ++i) {
        auto& pt = m_deformGrid[i];
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
        // Only add if not colliding with a checkpoint
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
    float fuelTargetX = 85.0f;
    float prevFuelX = 0.0f;
    while (fuelTargetX < 5000.0f) {
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

        // If minSlopeMag is still > 0.22, search anywhere in [prevFuelX + 70, prevFuelX + 156] for a flat crest/saddle
        if (minSlopeMag > 0.22f) {
            float minX = std::max(25.0f, prevFuelX + 70.0f);
            float maxX = (prevFuelX > 0.0f) ? (prevFuelX + 156.0f) : 120.0f;
            for (float sx = minX; sx <= maxX; sx += 0.25f) {
                float sm = std::abs(getSlope(sx));
                if (sm < minSlopeMag) {
                    minSlopeMag = sm;
                    bestX = sx;
                }
            }
        }

        // Safety clamp on max interval to strictly ensure reachable spacing (<= 158m)
        if (prevFuelX > 0.0f && (bestX - prevFuelX) > 158.0f) {
            bestX = prevFuelX + 156.0f;
        }

        WorldItem fuel;
        fuel.position = {bestX, getHeight(bestX) + 1.6f};
        fuel.type = ItemType::FUEL_CANISTER;
        fuel.value = 100;
        fuel.collected = false;
        m_items.push_back(fuel);

        prevFuelX = bestX;
        // Progressive interval: grows from 85m at start to 150m near finish line
        float diff = getDifficultyFactor(bestX);
        float nextInterval = 85.0f + 65.0f * diff;
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
                float arcH = std::sin((static_cast<float>(k) / 4.0f) * 3.14159f) * 3.2f + 1.5f;

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
        coin.position = {x, y + 1.3f};
        coin.type = (static_cast<int>(x) % 60 == 0) ? ItemType::COIN_GOLD :
                    (static_cast<int>(x) % 20 == 0) ? ItemType::COIN_SILVER : ItemType::COIN_BRONZE;
        coin.value = (coin.type == ItemType::COIN_GOLD) ? 100 : (coin.type == ItemType::COIN_SILVER) ? 25 : 5;
        coin.collected = false;
        m_items.push_back(coin);
    }
}

} // namespace Physics
