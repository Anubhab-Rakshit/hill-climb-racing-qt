#pragma once

#include "PhysicsTypes.h"
#include "TerrainConfig.h"
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
    bool isCheckpoint = false;
    std::string checkpointName;
    int checkpointIndex = 0;
    bool isFinishLine = false;
};

struct TerrainDeformation {
    float compression = 0.0f;     // physical surface depression [0.0, 0.06] meters
    float grassFlattening = 0.0f; // [0.0, 1.0] grass bent/trampled flat
    float grassTear = 0.0f;       // [0.0, 1.0] grass torn away, exposing dark fertile soil/loam
    float age = 0.0f;             // seconds since wheel contact
};

/**
 * @brief Continuous Procedural Terrain, Dynamic Difficulty, and Validated Item Generator.
 * Provides height, slope, normal vectors, dynamic terrain surface deformation, progressive fuel placement, and checkpoint gates.
 */
class Terrain {
public:
    Terrain();

    void setBiome(const std::string& biomeId);
    const std::string& getBiome() const { return m_biomeId; }
    const BiomeSpec& getBiomeSpec() const { return m_spec; }

    float getBaseHeight(float x) const;
    float getHeight(float x) const;
    float getSlope(float x) const;
    Vec2 getNormal(float x) const;

    // Dynamic Terrain Deformation API
    TerrainDeformation getDeformation(float x) const;
    void applyWheelDeformation(float wheelX, float wheelRadius, float normalForce, float slipSpeed, float dt);
    void updateDeformation(float dt, float activeMinX, float activeMaxX);
    void resetDeformation();

    float getFriction() const { return m_friction; }
    float getGravity() const { return m_gravity; }

    const std::vector<WorldItem>& getItems() const { return m_items; }
    std::vector<WorldItem>& getItems() { return m_items; }

    const std::vector<MilestoneSign>& getMilestones() const { return m_milestones; }
    const std::vector<CheckpointInfo>& getCheckpoints() const { return m_spec.checkpoints; }
    const CheckpointInfo* getNextCheckpoint(float currentDist) const;

    void collectItem(size_t index);
    void reset();

    float getDifficultyFactor(float x) const;

private:
    std::string m_biomeId;
    BiomeSpec m_spec;
    float m_friction;
    float m_gravity;
    std::vector<WorldItem> m_items;
    std::vector<MilestoneSign> m_milestones;

    static constexpr float DEFORM_MIN_X = -50.0f;
    static constexpr float DEFORM_DX = 0.05f;
    static constexpr size_t DEFORM_SAMPLES = 72000;
    std::vector<TerrainDeformation> m_deformGrid;

    void spawnItems();
};

} // namespace Physics
