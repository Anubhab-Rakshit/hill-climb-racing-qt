#pragma once

#include "PhysicsTypes.h"
#include "Terrain.h"

#include <Box2D/Box2D.h>

namespace Physics {

struct Box2DTerrainOptions {
    float startX = -20.0f;
    float endX = 1200.0f;
    float sampleSpacing = 0.5f;
    float surfaceOffset = 0.0f;
    float friction = 0.9f;
};

/**
 * @brief Fixed-step Box2D 2.2.1 world used by the authentic HCR physics port.
 *
 * The existing Vehicle/PhysicsWorld solver remains active during the migration.
 * This class provides the deterministic 60Hz world and terrain edge-chain
 * foundation that the Box2D vehicle backend will use.
 */
class Box2DPhysics {
public:
    explicit Box2DPhysics(float gravityY = -9.81f);
    ~Box2DPhysics();

    Box2DPhysics(const Box2DPhysics&) = delete;
    Box2DPhysics& operator=(const Box2DPhysics&) = delete;

    void setGravity(float gravityX, float gravityY);
    void step(float dt);
    void resetAccumulator();

    void createTerrainCollider(const Terrain& terrain,
                               const Box2DTerrainOptions& options = Box2DTerrainOptions{});
    void createSolidTerrainCollider(const Terrain& terrain,
                                    const Box2DTerrainOptions& options = Box2DTerrainOptions{},
                                    float depth = 2.0f);
    void clearTerrain();
    bool hasTerrain() const { return m_terrainBody != nullptr; }
    int terrainVertexCount() const { return m_terrainVertexCount; }

    b2Body* createDynamicCircle(Vec2 position, float radius, float density = 1.0f,
                                float friction = 0.8f, float angle = 0.0f);
    b2Body* createDynamicBox(Vec2 position, Vec2 halfExtents, float density = 1.0f,
                             float friction = 0.8f, float angle = 0.0f);

    b2World& world() { return *m_world; }
    const b2World& world() const { return *m_world; }

    float fixedDt() const { return kFixedDt; }
    int stepCount() const { return m_stepCount; }
    float accumulator() const { return m_accumulator; }

    static constexpr float kFixedDt = 1.0f / 60.0f;
    static constexpr int kVelocityIterations = 8;
    static constexpr int kPositionIterations = 3;
    static constexpr int kMaxSubSteps = 8;

private:
    b2World* m_world = nullptr;
    b2Body* m_terrainBody = nullptr;
    int m_terrainVertexCount = 0;
    int m_stepCount = 0;
    float m_accumulator = 0.0f;
};

} // namespace Physics
