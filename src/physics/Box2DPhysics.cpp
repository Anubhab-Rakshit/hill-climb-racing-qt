#include "Box2DPhysics.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Physics {

Box2DPhysics::Box2DPhysics(float gravityY) {
    m_world = new b2World(b2Vec2(0.0f, gravityY));
    m_world->SetAllowSleeping(true);
    m_world->SetContinuousPhysics(true);
}

Box2DPhysics::~Box2DPhysics() {
    delete m_world;
}

void Box2DPhysics::setGravity(float gravityX, float gravityY) {
    m_world->SetGravity(b2Vec2(gravityX, gravityY));
}

void Box2DPhysics::resetAccumulator() {
    m_accumulator = 0.0f;
}

void Box2DPhysics::step(float dt) {
    if (!(dt > 0.0f)) {
        return;
    }

    m_accumulator += dt;

    int subSteps = 0;
    while (m_accumulator >= kFixedDt && subSteps < kMaxSubSteps) {
        m_world->Step(kFixedDt, kVelocityIterations, kPositionIterations);
        m_accumulator -= kFixedDt;
        ++m_stepCount;
        ++subSteps;
    }

    if (m_accumulator >= kFixedDt) {
        m_accumulator = 0.0f;
    }
}

void Box2DPhysics::createTerrainCollider(const Terrain& terrain,
                                         const Box2DTerrainOptions& options) {
    clearTerrain();

    if (!(options.sampleSpacing > 0.0f) || !(options.endX > options.startX)) {
        return;
    }

    const float span = options.endX - options.startX;
    const int sampleCount = static_cast<int>(std::floor(span / options.sampleSpacing)) + 1;
    if (sampleCount < 2) {
        return;
    }

    std::vector<b2Vec2> vertices;
    vertices.reserve(static_cast<size_t>(sampleCount) + 1);

    for (int i = 0; i < sampleCount; ++i) {
        const float x = options.startX + static_cast<float>(i) * options.sampleSpacing;
        vertices.emplace_back(x, terrain.getHeight(x) + options.surfaceOffset);
    }

    const float lastX = options.startX +
                        static_cast<float>(sampleCount - 1) * options.sampleSpacing;
    if (lastX < options.endX - options.sampleSpacing * 0.5f) {
        vertices.emplace_back(options.endX,
                              terrain.getHeight(options.endX) + options.surfaceOffset);
    }

    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(0.0f, 0.0f);
    m_terrainBody = m_world->CreateBody(&bodyDef);

    b2ChainShape chain;
    chain.CreateChain(vertices.data(), static_cast<int32>(vertices.size()));

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &chain;
    fixtureDef.friction = options.friction;
    fixtureDef.restitution = 0.0f;
    m_terrainBody->CreateFixture(&fixtureDef);

    m_terrainVertexCount = static_cast<int>(vertices.size());
}


void Box2DPhysics::createSolidTerrainCollider(const Terrain& terrain,
                                               const Box2DTerrainOptions& options,
                                               float depth) {
    clearTerrain();

    if (!(options.sampleSpacing > 0.0f) || !(options.endX > options.startX) || !(depth > 0.0f)) {
        return;
    }

    const float span = options.endX - options.startX;
    const int sampleCount = static_cast<int>(std::floor(span / options.sampleSpacing)) + 1;
    if (sampleCount < 2) {
        return;
    }

    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(0.0f, 0.0f);
    m_terrainBody = m_world->CreateBody(&bodyDef);

    auto addStrip = [&](float x0, float x1) {
        const float y0 = terrain.getHeight(x0) + options.surfaceOffset;
        const float y1 = terrain.getHeight(x1) + options.surfaceOffset;

        b2Vec2 vertices[4];
        vertices[0].Set(x0, y0);
        vertices[1].Set(x0, y0 - depth);
        vertices[2].Set(x1, y1 - depth);
        vertices[3].Set(x1, y1);

        b2PolygonShape shape;
        shape.Set(vertices, 4);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.friction = options.useMaterialFriction
                                 ? terrain.getSurfaceAt((x0 + x1) * 0.5f).friction
                                 : options.friction;
        fixtureDef.restitution = 0.0f;
        m_terrainBody->CreateFixture(&fixtureDef);
    };

    for (int i = 0; i < sampleCount - 1; ++i) {
        const float x0 = options.startX + static_cast<float>(i) * options.sampleSpacing;
        const float x1 = x0 + options.sampleSpacing;
        addStrip(x0, x1);
    }

    const float lastX = options.startX + static_cast<float>(sampleCount - 1) * options.sampleSpacing;
    if (lastX < options.endX - options.sampleSpacing * 0.5f) {
        addStrip(lastX, options.endX);
    }

    m_terrainVertexCount = sampleCount;
}

void Box2DPhysics::clearTerrain() {
    if (m_terrainBody != nullptr) {
        m_world->DestroyBody(m_terrainBody);
        m_terrainBody = nullptr;
    }
    m_terrainVertexCount = 0;
}

b2Body* Box2DPhysics::createDynamicCircle(Vec2 position, float radius, float density,
                                          float friction, float angle) {
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(position.x, position.y);
    bodyDef.angle = angle;
    bodyDef.allowSleep = true;
    b2Body* body = m_world->CreateBody(&bodyDef);

    b2CircleShape shape;
    shape.m_radius = radius;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = density;
    fixtureDef.friction = friction;
    fixtureDef.restitution = 0.05f;
    body->CreateFixture(&fixtureDef);

    return body;
}

b2Body* Box2DPhysics::createDynamicBox(Vec2 position, Vec2 halfExtents, float density,
                                       float friction, float angle) {
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(position.x, position.y);
    bodyDef.angle = angle;
    bodyDef.allowSleep = true;
    b2Body* body = m_world->CreateBody(&bodyDef);

    b2PolygonShape shape;
    shape.SetAsBox(halfExtents.x, halfExtents.y);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = density;
    fixtureDef.friction = friction;
    fixtureDef.restitution = 0.05f;
    body->CreateFixture(&fixtureDef);

    return body;
}

} // namespace Physics
