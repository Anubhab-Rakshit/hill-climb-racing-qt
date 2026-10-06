#pragma once

#include "PhysicsTypes.h"
#include <string>
#include <vector>

namespace Physics {

enum class ItemType {
    COIN_BRONZE,
    COIN_SILVER,
    COIN_GOLD,
    FUEL_CANISTER
};

struct WorldItem {
    Vec2 position;
    ItemType type;
    int value = 0;
    bool collected = false;
};

struct MilestoneSign {
    Vec2 position;
    int distanceMeters = 0;
};

/**
 * @brief Continuous Procedural Terrain and Item Spawner.
 * Provides height, slope, normal vectors, and manages collectible items.
 */
class Terrain {
public:
    Terrain();

    void setBiome(const std::string& biomeId);
    const std::string& getBiome() const { return m_biomeId; }

    float getHeight(float x) const;
    float getSlope(float x) const;
    Vec2 getNormal(float x) const;

    float getFriction() const { return m_friction; }
    float getGravity() const { return m_gravity; }

    const std::vector<WorldItem>& getItems() const { return m_items; }
    std::vector<WorldItem>& getItems() { return m_items; }

    const std::vector<MilestoneSign>& getMilestones() const { return m_milestones; }

    void collectItem(size_t index);
    void reset();

private:
    std::string m_biomeId;
    float m_friction;
    float m_gravity;
    std::vector<WorldItem> m_items;
    std::vector<MilestoneSign> m_milestones;

    void spawnItems();
};

} // namespace Physics
