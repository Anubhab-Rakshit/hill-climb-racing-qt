#include "Terrain.h"
#include <cmath>
#include <algorithm>

namespace Physics {

Terrain::Terrain()
    : m_biomeId("countryside")
    , m_friction(0.92f)
    , m_gravity(9.81f)
{
    spawnItems();
}

void Terrain::setBiome(const std::string& biomeId) {
    m_biomeId = biomeId;
    if (biomeId == "desert") {
        m_friction = 1.05f;
        m_gravity = 9.81f;
    } else if (biomeId == "moon") {
        m_friction = 1.15f;
        m_gravity = 1.62f;
    } else if (biomeId == "mountain") {
        m_friction = 1.20f;
        m_gravity = 9.81f;
    } else { // Countryside
        m_friction = 1.25f;
        m_gravity = 9.81f;
    }
    spawnItems();
}

void Terrain::reset() {
    for (auto& item : m_items) {
        item.collected = false;
    }
}

float Terrain::getHeight(float x) const {
    // Starting runway is flat for smooth launch and acceleration
    if (x < 22.0f) {
        return 0.0f;
    }

    float lx = x - 22.0f;
    float ramp = std::clamp(lx / 20.0f, 0.0f, 1.0f); // Smooth entry transition

    if (m_biomeId == "desert") {
        // Asymmetric windward sand dunes with sharp slip-face drops
        float d1 = std::sin(lx * 0.020f) * 14.0f;
        float d2 = std::sin(lx * 0.055f + 0.8f) * 5.0f;
        float slip = std::sin(lx * 0.12f) * 1.5f;
        return (d1 + d2 + slip) * ramp;
    } else if (m_biomeId == "moon") {
        // Wide deep impact craters and massive parabolic low-gravity launch mounds
        float c1 = std::sin(lx * 0.014f) * 18.0f;
        float c2 = std::cos(lx * 0.032f) * 7.5f;
        float crater = (std::sin(lx * 0.07f) < -0.4f) ? -3.5f : 0.0f;
        return (c1 + c2 + crater) * ramp;
    } else if (m_biomeId == "mountain") {
        // Craggy mountainous ascents, jagged rocky ledges and alpine peaks
        float m1 = std::sin(lx * 0.025f) * 22.0f;
        float m2 = std::sin(lx * 0.075f) * 6.0f;
        float crag = std::sin(lx * 0.22f) * 1.8f;
        return (m1 + m2 + crag) * ramp;
    }

    // Default Countryside: High-variety signature Hill Climb course
    // 1. Rolling Introduction Hills (0 - 65m)
    // 2. The Big Ski Jump Launch Ramp (65 - 125m)
    // 3. Moguls & Washboards (130 - 195m)
    // 4. Steep Country Mountain Ascent (200 - 310m)
    // 5. Rollercoaster Canyon Dips (315 - 420m)
    // 6. Endless Wilderness (420m+)
    float baseHills = std::sin(lx * 0.016f) * 10.5f;
    float rhythmBumps = std::sin(lx * 0.055f) * 3.0f;
    float microRelief = std::sin(lx * 0.12f) * 0.8f;

    // Feature 1: Signature Ski Jump Ramp (lx ~ 65 - 125m)
    float jumpRamp = 0.0f;
    if (lx > 65.0f && lx < 125.0f) {
        float jt = (lx - 65.0f) / 60.0f;
        jumpRamp = std::sin(jt * 3.14159f) * 6.5f;
    }

    // Feature 2: Moguls & Washboards (lx ~ 130 - 195m)
    float moguls = 0.0f;
    if (lx > 130.0f && lx < 195.0f) {
        moguls = std::sin(lx * 0.65f) * 1.5f;
    }

    // Feature 3: Mountain Wall Ascent (lx ~ 200 - 310m)
    float mountainWall = 0.0f;
    if (lx > 200.0f && lx < 310.0f) {
        float mt = (lx - 200.0f) / 110.0f;
        mountainWall = std::sin(mt * 3.14159f) * 12.0f;
    }

    // Feature 4: Canyon Dip (lx ~ 315 - 420m)
    float canyon = 0.0f;
    if (lx > 315.0f && lx < 420.0f) {
        float ct = (lx - 315.0f) / 105.0f;
        canyon = -std::sin(ct * 3.14159f) * 8.0f;
    }

    return (baseHills + rhythmBumps + microRelief + jumpRamp + moguls + mountainWall + canyon) * ramp;
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

    // 1. Spawn Distance Milestone Markers along course
    const int milestoneDistances[] = {50, 100, 200, 300, 500, 750, 1000, 1500, 2000, 3000, 5000};
    for (int dist : milestoneDistances) {
        MilestoneSign sign;
        sign.distanceMeters = dist;
        float sx = static_cast<float>(dist);
        sign.position = {sx, getHeight(sx)};
        m_milestones.push_back(sign);
    }

    // 2. Spawn Collectibles: Fuel Jerry Cans & Sweeping Parabolic Coin Arcs
    for (float x = 35.0f; x < 5000.0f; x += 16.0f) {
        float y = getHeight(x);

        // Every 85-105m place a vital Fuel Canister
        if (static_cast<int>(x) % 95 < 16) {
            WorldItem fuel;
            fuel.position = {x, y + 1.6f};
            fuel.type = ItemType::FUEL_CANISTER;
            fuel.value = 100;
            fuel.collected = false;
            m_items.push_back(fuel);
            x += 12.0f; // Clear spacing after fuel
            continue;
        }

        // Parabolic Coin Arc over crests and jumps
        float slope = getSlope(x);
        float nextSlope = getSlope(x + 6.0f);

        // Detecting a jump crest (slope turns from uphill to downhill)
        if (slope > 0.15f && nextSlope < slope) {
            // Spawn 5-coin rainbow arc across the crest
            for (int k = 0; k < 5; ++k) {
                float coinX = x + k * 3.0f;
                float coinBaseY = getHeight(coinX);
                // Parabolic trajectory offset
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

        // Normal ground follow coin
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
