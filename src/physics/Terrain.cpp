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
    m_recipe = TrackRecipeRegistry::get(biomeId);
    m_friction = m_spec.friction;
    m_gravity = m_spec.gravity;
    m_airDensity = m_spec.airDensity;
    m_hasSurfaceOverride = false;

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
    m_recipe = TrackRecipe{};
    m_recipe.biomeId = "test_ramp";
    m_recipe.length = 5000.0f;
    m_friction = 1.35f;
    m_gravity = 9.81f;
    m_airDensity = 1.0f;
    m_hasSurfaceOverride = false;
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

void Terrain::setTestEnvironment(float gravity, float airDensity) {
    m_gravity = gravity;
    m_airDensity = airDensity;
}

void Terrain::setTestSurface(SurfaceType type) {
    m_surfaceOverride = type;
    m_hasSurfaceOverride = true;
    m_friction = SurfaceMaterialRegistry::get(type).friction;
}

void Terrain::clearTestSurface() {
    m_hasSurfaceOverride = false;
    m_friction = m_spec.friction;
}

float Terrain::getFrictionAt(float x) const {
    return getSurfaceAt(x).friction;
}

SurfaceProperties Terrain::getSurfaceAt(float x) const {
    if (m_hasSurfaceOverride) {
        return propertiesFromMaterial(SurfaceMaterialRegistry::get(m_surfaceOverride));
    }

    const auto& zones = m_recipe.surfaceZones;
    if (zones.empty()) {
        SurfaceProperties fallback;
        fallback.friction = m_friction;
        return fallback;
    }

    size_t index = 0;
    while (index + 1 < zones.size() && x >= zones[index].endX) {
        ++index;
    }

    const SurfaceZone& zone = zones[index];
    SurfaceProperties props = propertiesFromMaterial(SurfaceMaterialRegistry::get(zone.material));

    if (index > 0 && zone.blendWidth > 0.0f && x < zone.startX + zone.blendWidth) {
        const SurfaceProperties previous =
            propertiesFromMaterial(SurfaceMaterialRegistry::get(zones[index - 1].material));
        const float t = std::clamp((x - zone.startX) / zone.blendWidth, 0.0f, 1.0f);
        props = blendSurfaceProperties(previous, props, t);
    }

    return props;
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
    return TerrainGenerator::heightAt(m_recipe, x);
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

    const SurfaceProperties surface = getSurfaceAt(wheelX);
    const float maxComp = surface.maxCompression;
    const float compRate = surface.deformationRate;

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
    // In early game: friendly ~85m spacing on gentle crests
    // In mid/late game: progressive spacing up to ~200m+ near steeper summits
    float fuelTargetX = 85.0f;
    float prevFuelX = 0.0f;
    while (fuelTargetX < 5000.0f) {
        float diff = getDifficultyFactor(fuelTargetX);
        // Fuel must remain reachable: never exceed the 165 m safe interval.
        float maxAllowedSpacing = std::min(165.0f, 110.0f + std::min(diff, 2.5f) * 22.0f); // 110m -> 165m

        // Find best stable location (closest crest or flat saddle where |slope| <= 0.38)
        float bestX = fuelTargetX;
        float minSlopeMag = 999.0f;

        // Primary search window around target
        for (float searchX = fuelTargetX - 20.0f; searchX <= fuelTargetX + 20.0f; searchX += 0.25f) {
            if (searchX < 25.0f) continue;
            float slopeMag = std::abs(getSlope(searchX));
            if (slopeMag < minSlopeMag) {
                minSlopeMag = slopeMag;
                bestX = searchX;
            }
        }

        // Secondary search across the whole reachable interval if the local crest is steep.
        if (minSlopeMag > 0.30f) {
            float minX = std::max(25.0f, prevFuelX + 40.0f);
            float maxX = (prevFuelX > 0.0f) ? (prevFuelX + maxAllowedSpacing - 8.0f) : 120.0f;
            for (float sx = minX; sx <= maxX; sx += 0.25f) {
                float sm = std::abs(getSlope(sx));
                if (sm < minSlopeMag) {
                    minSlopeMag = sm;
                    bestX = sx;
                }
            }
        }

        // Safety clamp on max interval to ensure reachable spacing
        if (prevFuelX > 0.0f && (bestX - prevFuelX) > maxAllowedSpacing) {
            bestX = prevFuelX + maxAllowedSpacing;
        }

        WorldItem fuel;
        fuel.position = {bestX, getHeight(bestX) + 1.25f};
        fuel.type = ItemType::FUEL_CANISTER;
        fuel.value = 100;
        fuel.collected = false;
        m_items.push_back(fuel);

        prevFuelX = bestX;
        // Progressive interval: grows from 85 m toward the safe maximum, leaving room for the local search.
        float nextInterval = std::min(maxAllowedSpacing - 22.0f, 85.0f + std::min(diff, 2.5f) * 46.0f);
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
