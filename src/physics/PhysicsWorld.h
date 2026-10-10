#pragma once

#include "Vehicle.h"
#include "Terrain.h"
#include "ProfileManager.h"
#include <functional>
#include <string>

namespace Physics {

/**
 * @brief Coordinates the 120Hz Physics Simulation, Stunts, Collisions, and Pickups.
 */
class PhysicsWorld {
public:
    PhysicsWorld();

    void reset(const std::string& biomeId = "countryside",
               VehicleType vehicleType = VehicleType::OFFROADER,
               DriverType driverType = DriverType::BILL);
    void step(float dt, const Core::ProfileManager& profile);

    Vehicle& vehicle() { return m_vehicle; }
    const Vehicle& vehicle() const { return m_vehicle; }

    Terrain& terrain() { return m_terrain; }
    const Terrain& terrain() const { return m_terrain; }

    // Run telemetry
    float distanceReached() const { return m_distance; }
    int coinsCollected() const { return m_coinsCollected; }
    int flipsCompleted() const { return m_flips; }
    float totalAirTime() const { return m_totalAirTime; }

    bool isGameOver() const { return m_gameOver; }
    const std::string& gameOverReason() const { return m_gameOverReason; }

    int currentCheckpoint() const { return m_lastClearedCheckpoint; }

    // Event callbacks for UI
    void setOnStunt(std::function<void(const std::string& title, int coins)> cb) { m_onStunt = cb; }
    void setOnItemCollected(std::function<void(ItemType type, int value)> cb) { m_onItem = cb; }
    void setOnCheckpoint(std::function<void(const CheckpointInfo& cp)> cb) { m_onCheckpoint = cb; }

private:
    Vehicle m_vehicle;
    Terrain m_terrain;

    float m_distance;
    int m_coinsCollected;
    int m_flips;
    float m_totalAirTime;
    int m_lastClearedCheckpoint;

    float m_airTimer;
    float m_cumulativeRotation;

    bool m_gameOver;
    std::string m_gameOverReason;

    std::function<void(const std::string&, int)> m_onStunt;
    std::function<void(ItemType, int)> m_onItem;
    std::function<void(const CheckpointInfo&)> m_onCheckpoint;

    void checkItemCollisions();
    void checkCheckpoints();
    void updateStunts(float dt);
};

} // namespace Physics
