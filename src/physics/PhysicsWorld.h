#pragma once

#include "Vehicle.h"
#include "Terrain.h"
#include "ProfileManager.h"
#include <functional>
#include <string>

namespace Physics {

/**
 * @brief Coordinates the 120Hz Physics Simulation, Stunts, Collisions, and Pickups.
 * Supports both Single Player and Head-to-Head 2-Truck Multiplayer Racing.
 */
class PhysicsWorld {
public:
    PhysicsWorld();

    void reset(const std::string& biomeId = "countryside",
               VehicleType vehicleType = VehicleType::OFFROADER,
               DriverType driverType = DriverType::BILL,
               bool isMultiplayer = false,
               VehicleType vehicleType2 = VehicleType::PICKUP_TRUCK,
               DriverType driverType2 = DriverType::SARAH,
               float customStartX = 5.0f);

    void step(float dt, const Core::ProfileManager& profile);

    Vehicle& vehicle() { return m_vehicle; }
    const Vehicle& vehicle() const { return m_vehicle; }

    Vehicle& vehicle2() { return m_vehicle2; }
    const Vehicle& vehicle2() const { return m_vehicle2; }

    Terrain& terrain() { return m_terrain; }
    const Terrain& terrain() const { return m_terrain; }

    bool isMultiplayer() const { return m_isMultiplayer; }

    // Run telemetry P1
    float distanceReached() const { return m_distance; }
    int coinsCollected() const { return m_coinsCollected; }
    int flipsCompleted() const { return m_flips; }
    float totalAirTime() const { return m_totalAirTime; }
    bool isGameOver() const { return m_gameOver; }
    const std::string& gameOverReason() const { return m_gameOverReason; }

    // Run telemetry P2 (Multiplayer)
    float distanceReached2() const { return m_distance2; }
    int flipsCompleted2() const { return m_flips2; }
    float totalAirTime2() const { return m_totalAirTime2; }
    bool isGameOver2() const { return m_gameOver2; }
    const std::string& gameOverReason2() const { return m_gameOverReason2; }

    // Multiplayer Race Outcome: 0 = In Progress, 1 = P1 Wins, 2 = P2 Wins, 3 = Draw
    int winner() const { return m_winner; }
    const std::string& mpStatusMessage() const { return m_mpStatusMessage; }

    int currentCheckpoint() const { return m_lastClearedCheckpoint; }

    // Runtime Developer Mode Tuning Parameters
    void setCustomGravity(float g);
    float customGravity() const { return m_customGravity; }
    void setSuspStiffnessMultiplier(float m);
    float suspStiffnessMultiplier() const { return m_suspStiffnessMult; }
    void setSuspDampingMultiplier(float m);
    float suspDampingMultiplier() const { return m_suspDampingMult; }
    void setCustomTorqueMultiplier(float m);
    float customTorqueMultiplier() const { return m_customTorqueMult; }
    void setCustomFrictionMultiplier(float m);
    float customFrictionMultiplier() const { return m_customFrictionMult; }
    void setCustomAirGyroMultiplier(float m);
    float customAirGyroMultiplier() const { return m_customAirGyroMult; }
    void setFuelBurnMultiplier(float m);
    float fuelBurnMultiplier() const { return m_fuelBurnMult; }
    void setInfiniteFuel(bool inf);
    bool isInfiniteFuel() const { return m_infiniteFuel; }

    // Multiplayer Trophy Competition Configuration
    void setFinishDistance(float d) { m_finishDistance = d; }
    float finishDistance() const { return m_finishDistance; }
    void setRubberbanding(bool enabled) { m_rubberbandingEnabled = enabled; }
    bool isRubberbanding() const { return m_rubberbandingEnabled; }

    // Event callbacks for UI
    void setOnStunt(std::function<void(const std::string& title, int coins)> cb) { m_onStunt = cb; }
    void setOnItemCollected(std::function<void(ItemType type, int value, int itemIndex)> cb) { m_onItem = cb; }
    void setOnCheckpoint(std::function<void(const CheckpointInfo& cp)> cb) { m_onCheckpoint = cb; }

private:
    Vehicle m_vehicle;
    Vehicle m_vehicle2;
    Terrain m_terrain;

    bool m_isMultiplayer = false;

    // Player 1 state
    float m_distance;
    int m_coinsCollected;
    int m_flips;
    float m_totalAirTime;
    float m_airTimer;
    float m_wheelieTimer = 0.0f;
    float m_cumulativeRotation;
    bool m_gameOver;
    std::string m_gameOverReason;

    // Player 2 state
    float m_distance2;
    int m_flips2;
    float m_totalAirTime2;
    float m_airTimer2;
    float m_wheelieTimer2 = 0.0f;
    float m_cumulativeRotation2;
    bool m_gameOver2;
    std::string m_gameOverReason2;

    int m_winner = 0;
    std::string m_mpStatusMessage;

    int m_lastClearedCheckpoint;
    float m_deformAccum = 0.0f;

    std::function<void(const std::string&, int)> m_onStunt;
    std::function<void(ItemType, int, int)> m_onItem;
    std::function<void(const CheckpointInfo&)> m_onCheckpoint;

    // Live Tuning Variables (Dev Mode)
    float m_customGravity = -9.81f;
    float m_suspStiffnessMult = 1.0f;
    float m_suspDampingMult = 1.0f;
    float m_customTorqueMult = 1.0f;
    float m_customFrictionMult = 1.0f;
    float m_customAirGyroMult = 1.0f;
    float m_fuelBurnMult = 1.0f;
    bool m_infiniteFuel = false;
    float m_finishDistance = 1500.0f;
    bool m_rubberbandingEnabled = false;

    void checkItemCollisions();
    void checkItemCollisionsForVehicle(Vehicle& veh, bool isP1);
    void checkCheckpoints();
    void updateStunts(float dt);
    void updateStuntsForVehicle(Vehicle& veh, float dt, float& airTimer, float& wheelieTimer,
                                float& totalAirTime, float& cumulativeRotation, int& flips,
                                bool isP1);
    void updateMultiplayerRules();
};

} // namespace Physics
