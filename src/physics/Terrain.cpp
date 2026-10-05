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
        m_friction = 0.65f;
        m_gravity = 9.81f;
    } else if (biomeId == "moon") {
        m_friction = 0.80f;
        m_gravity = 1.62f;
    } else if (biomeId == "mountain") {
        m_friction = 0.88f;
        m_gravity = 9.81f;
    } else { // Countryside
        m_friction = 0.92f;
        m_gravity = 9.81f;
    }
    reset();
}

void Terrain::reset() {
    for (auto& item : m_items) {
        item.collected = false;
    }
}

float Terrain::getHeight(float x) const {
    // Starting runway is flat
    if (x < 15.0f) {
        return 0.0f;
    }

    float lx = x - 15.0f;

    if (m_biomeId == "desert") {
        // Sharp undulating sand ridges
        float h1 = std::sin(lx * 0.025f) * 14.0f;
        float h2 = std::sin(lx * 0.08f) * 4.5f;
        float h3 = std::cos(lx * 0.20f) * 1.2f;
        return h1 + h2 + h3;
    } else if (m_biomeId == "moon") {
        // Wide craters and gentle mounds
        float h1 = std::sin(lx * 0.015f) * 18.0f;
        float h2 = std::cos(lx * 0.045f) * 6.0f;
        return h1 + h2;
    } else if (m_biomeId == "mountain") {
        // Steep craggy hills
        float h1 = std::sin(lx * 0.035f) * 22.0f;
        float h2 = std::sin(lx * 0.12f) * 5.0f;
        float h3 = std::sin(lx * 0.35f) * 1.5f;
        return h1 + h2 + h3;
    }

    // Default Countryside: Smooth rolling hills
    float h1 = std::sin(lx * 0.020f) * 12.0f;
    float h2 = std::sin(lx * 0.065f) * 4.0f;
    float h3 = std::sin(lx * 0.18f) * 1.0f;
    return h1 + h2 + h3;
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

    // Spawn 5000 meters worth of coins and fuel canisters
    for (float x = 40.0f; x < 5000.0f; x += 18.0f) {
        float y = getHeight(x);

        // Every 90-110m place a fuel canister
        if (static_cast<int>(x) % 95 < 20) {
            WorldItem fuel;
            fuel.position = {x, y + 1.8f};
            fuel.type = ItemType::FUEL_CANISTER;
            fuel.value = 100; // Refills 100% fuel
            fuel.collected = false;
            m_items.push_back(fuel);
            x += 15.0f; // Gap after canister
            continue;
        }

        // Spawn Coin Arc over hill crests
        float slope = getSlope(x);
        float coinH = (std::abs(slope) > 0.3f) ? 2.5f : 1.2f;

        WorldItem coin;
        coin.position = {x, y + coinH};
        coin.type = (static_cast<int>(x) % 50 == 0) ? ItemType::COIN_GOLD :
                    (static_cast<int>(x) % 25 == 0) ? ItemType::COIN_SILVER : ItemType::COIN_BRONZE;
        coin.value = (coin.type == ItemType::COIN_GOLD) ? 100 : (coin.type == ItemType::COIN_SILVER) ? 25 : 5;
        coin.collected = false;
        m_items.push_back(coin);
    }
}

} // namespace Physics
