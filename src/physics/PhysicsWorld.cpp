#include "PhysicsWorld.h"
#include <cmath>
#include <algorithm>

namespace Physics {

PhysicsWorld::PhysicsWorld()
    : m_distance(0.0f)
    , m_coinsCollected(0)
    , m_flips(0)
    , m_totalAirTime(0.0f)
    , m_airTimer(0.0f)
    , m_cumulativeRotation(0.0f)
    , m_gameOver(false)
    , m_gameOverReason("")
{
}

void PhysicsWorld::reset(const std::string& biomeId) {
    m_terrain.setBiome(biomeId);
    float startX = 5.0f;
    float groundY = m_terrain.getHeight(startX);
    m_vehicle.reset(startX, groundY + 1.20f);

    m_distance = 0.0f;
    m_coinsCollected = 0;
    m_flips = 0;
    m_totalAirTime = 0.0f;
    m_airTimer = 0.0f;
    m_cumulativeRotation = 0.0f;
    m_gameOver = false;
    m_gameOverReason = "";
}

void PhysicsWorld::step(float dt, const Core::ProfileManager& profile) {
    if (m_gameOver) return;

    // Step multi-body vehicle
    m_vehicle.step(dt, m_terrain, profile);

    // Update forward distance reached
    float currentX = m_vehicle.chassisPos().x;
    if (currentX > m_distance) {
        m_distance = currentX;
    }

    // Check item pickup collisions
    checkItemCollisions();

    // Check stunt flips and airtime
    updateStunts(dt);

    // Check game over triggers
    if (m_vehicle.isDriverDown()) {
        m_gameOver = true;
        m_gameOverReason = "DRIVER DOWN!";
    } else if (m_vehicle.isOutOfFuel() && m_vehicle.getSpeedKmh() < 0.5f) {
        m_gameOver = true;
        m_gameOverReason = "OUT OF FUEL!";
    }
}

void PhysicsWorld::checkItemCollisions() {
    Vec2 carPos = m_vehicle.chassisPos();
    auto& items = m_terrain.getItems();

    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].collected) continue;

        float dx = items[i].position.x - carPos.x;
        float dy = items[i].position.y - carPos.y;
        float distSq = dx * dx + dy * dy;

        // Collection radius: 2.2 meters
        if (distSq < 2.2f * 2.2f) {
            items[i].collected = true;

            if (items[i].type == ItemType::FUEL_CANISTER) {
                m_vehicle.addFuel(100.0f);
                if (m_onStunt) {
                    m_onStunt("+100 FUEL!", 0);
                }
            } else {
                m_coinsCollected += items[i].value;
                if (m_onItem) {
                    m_onItem(items[i].type, items[i].value);
                }
            }
        }
    }
}

void PhysicsWorld::updateStunts(float dt) {
    bool isAirborne = m_vehicle.isAirborne();

    if (isAirborne) {
        m_airTimer += dt;
        m_totalAirTime += dt;

        float dTheta = m_vehicle.chassisAngularVel() * dt;
        m_cumulativeRotation += dTheta;

        // Check for 360-degree Backflip
        if (m_cumulativeRotation >= 2.0f * static_cast<float>(M_PI)) {
            m_flips++;
            m_cumulativeRotation -= 2.0f * static_cast<float>(M_PI);
            m_coinsCollected += 1000;
            if (m_onStunt) {
                m_onStunt("BACKFLIP!", 1000);
            }
        }
        // Check for 360-degree Frontflip
        else if (m_cumulativeRotation <= -2.0f * static_cast<float>(M_PI)) {
            m_flips++;
            m_cumulativeRotation += 2.0f * static_cast<float>(M_PI);
            m_coinsCollected += 1000;
            if (m_onStunt) {
                m_onStunt("FRONTFLIP!", 1000);
            }
        }
    } else {
        // Landing event
        if (m_airTimer >= 1.2f) {
            int airBonus = static_cast<int>(m_airTimer * 150.0f);
            m_coinsCollected += airBonus;
            if (m_onStunt) {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "AIR TIME: %.1fs", m_airTimer);
                m_onStunt(buf, airBonus);
            }
        }
        m_airTimer = 0.0f;
        m_cumulativeRotation = 0.0f;
    }
}

} // namespace Physics
