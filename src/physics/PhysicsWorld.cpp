#define _USE_MATH_DEFINES
#include "PhysicsWorld.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace Physics {

PhysicsWorld::PhysicsWorld()
    : m_isMultiplayer(false)
    , m_distance(0.0f)
    , m_coinsCollected(0)
    , m_flips(0)
    , m_totalAirTime(0.0f)
    , m_airTimer(0.0f)
    , m_wheelieTimer(0.0f)
    , m_cumulativeRotation(0.0f)
    , m_gameOver(false)
    , m_gameOverReason("")
    , m_distance2(0.0f)
    , m_flips2(0)
    , m_totalAirTime2(0.0f)
    , m_airTimer2(0.0f)
    , m_wheelieTimer2(0.0f)
    , m_cumulativeRotation2(0.0f)
    , m_gameOver2(false)
    , m_gameOverReason2("")
    , m_winner(0)
    , m_mpStatusMessage("")
    , m_lastClearedCheckpoint(0)
    , m_lastClearedCheckpoint2(0)
{
}

void PhysicsWorld::reset(const std::string& biomeId,
                         VehicleType vehicleType,
                         DriverType driverType,
                         bool isMultiplayer,
                         VehicleType vehicleType2,
                         DriverType driverType2) {
    m_isMultiplayer = isMultiplayer;
    m_terrain.setBiome(biomeId);
    m_terrain.resetDeformation();

    m_itemsCollectedP1.assign(m_terrain.getItems().size(), false);
    m_itemsCollectedP2.assign(m_terrain.getItems().size(), false);

    // Both players start from the exact same start line (x = 5.0m) with fair initial placement
    const float startX = 5.0f;
    float groundY = m_terrain.getHeight(startX);

    // Player 1 Vehicle
    m_vehicle.setVehicleType(vehicleType);
    m_vehicle.setDriverType(driverType);
    float restY1 = groundY + m_vehicle.suspRestLength() + m_vehicle.wheelRadius() - m_vehicle.rearMountOffset().y;
    m_vehicle.reset(startX, restY1);

    m_distance = 0.0f;
    m_coinsCollected = 0;
    m_flips = 0;
    m_totalAirTime = 0.0f;
    m_lastClearedCheckpoint = 0;
    m_airTimer = 0.0f;
    m_wheelieTimer = 0.0f;
    m_cumulativeRotation = 0.0f;
    m_gameOver = false;
    m_gameOverReason = "";
    m_deformAccum = 0.0f;

    // Player 2 Vehicle (Multiplayer) - Identical start line and synchronized resting physics
    if (m_isMultiplayer) {
        m_vehicle2.setVehicleType(vehicleType2);
        m_vehicle2.setDriverType(driverType2);
        float restY2 = groundY + m_vehicle2.suspRestLength() + m_vehicle2.wheelRadius() - m_vehicle2.rearMountOffset().y;
        m_vehicle2.reset(startX, restY2);

        m_distance2 = 0.0f;
        m_flips2 = 0;
        m_totalAirTime2 = 0.0f;
        m_lastClearedCheckpoint2 = 0;
        m_airTimer2 = 0.0f;
        m_wheelieTimer2 = 0.0f;
        m_cumulativeRotation2 = 0.0f;
        m_gameOver2 = false;
        m_gameOverReason2 = "";
        m_winner = 0;
        m_mpStatusMessage = "RACE STARTED!";
    }
}

void PhysicsWorld::step(float dt, const Core::ProfileManager& profile) {
    if (!m_isMultiplayer) {
        // --- SINGLE PLAYER SIMULATION ---
        if (m_gameOver) {
            m_vehicle.applyInput(0.0f, 0.0f);
            m_vehicle.step(dt, m_terrain, profile);
            return;
        }

        m_vehicle.step(dt, m_terrain, profile);

        if (m_vehicle.isRearOnGround()) {
            m_terrain.applyWheelDeformation(m_vehicle.rearWheelPos().x, m_vehicle.rearWheelRadius(),
                                            m_vehicle.rearNormalForce(), m_vehicle.rearSlipSpeed(), dt);
        }
        if (m_vehicle.isFrontOnGround()) {
            m_terrain.applyWheelDeformation(m_vehicle.frontWheelPos().x, m_vehicle.frontWheelRadius(),
                                            m_vehicle.frontNormalForce(), m_vehicle.frontSlipSpeed(), dt);
        }

        m_deformAccum += dt;
        if (m_deformAccum >= 0.08f) {
            m_terrain.updateDeformation(m_deformAccum, m_vehicle.chassisPos().x - 60.0f, m_vehicle.chassisPos().x + 60.0f);
            m_deformAccum = 0.0f;
        }

        float currentX = m_vehicle.chassisPos().x;
        if (currentX > m_distance) {
            m_distance = currentX;
        }

        checkItemCollisions();
        checkCheckpoints();
        updateStunts(dt);

        if (m_vehicle.isDriverDown()) {
            m_gameOver = true;
            m_gameOverReason = "DRIVER DOWN!";
        } else if (m_vehicle.isOutOfFuel() && m_vehicle.getSpeedKmh() < 0.5f) {
            m_gameOver = true;
            m_gameOverReason = "OUT OF FUEL!";
        }
    } else {
        // --- MULTIPLAYER HEAD-TO-HEAD RACE SIMULATION ---
        // Step Vehicle 1
        if (m_gameOver) {
            m_vehicle.applyInput(0.0f, 0.0f);
            m_vehicle.step(dt, m_terrain, profile);
        } else {
            m_vehicle.step(dt, m_terrain, profile);
        }

        // Step Vehicle 2 (both step on identical terrain state for perfect physical parity)
        if (m_gameOver2) {
            m_vehicle2.applyInput(0.0f, 0.0f);
            m_vehicle2.step(dt, m_terrain, profile);
        } else {
            m_vehicle2.step(dt, m_terrain, profile);
        }

        // Apply wheel deformations for both vehicles after both have evaluated contact forces
        if (!m_gameOver) {
            if (m_vehicle.isRearOnGround()) {
                m_terrain.applyWheelDeformation(m_vehicle.rearWheelPos().x, m_vehicle.rearWheelRadius(),
                                                m_vehicle.rearNormalForce(), m_vehicle.rearSlipSpeed(), dt);
            }
            if (m_vehicle.isFrontOnGround()) {
                m_terrain.applyWheelDeformation(m_vehicle.frontWheelPos().x, m_vehicle.frontWheelRadius(),
                                                m_vehicle.frontNormalForce(), m_vehicle.frontSlipSpeed(), dt);
            }
        }
        if (!m_gameOver2) {
            if (m_vehicle2.isRearOnGround()) {
                m_terrain.applyWheelDeformation(m_vehicle2.rearWheelPos().x, m_vehicle2.rearWheelRadius(),
                                                m_vehicle2.rearNormalForce(), m_vehicle2.rearSlipSpeed(), dt);
            }
            if (m_vehicle2.isFrontOnGround()) {
                m_terrain.applyWheelDeformation(m_vehicle2.frontWheelPos().x, m_vehicle2.frontWheelRadius(),
                                                m_vehicle2.frontNormalForce(), m_vehicle2.frontSlipSpeed(), dt);
            }
        }

        // Deformations
        m_deformAccum += dt;
        if (m_deformAccum >= 0.08f) {
            float minX = std::min(m_vehicle.chassisPos().x, m_vehicle2.chassisPos().x) - 40.0f;
            float maxX = std::max(m_vehicle.chassisPos().x, m_vehicle2.chassisPos().x) + 40.0f;
            m_terrain.updateDeformation(m_deformAccum, minX, maxX);
            m_deformAccum = 0.0f;
        }

        // Track distances
        if (m_vehicle.chassisPos().x > m_distance) m_distance = m_vehicle.chassisPos().x;
        if (m_vehicle2.chassisPos().x > m_distance2) m_distance2 = m_vehicle2.chassisPos().x;

        // Pickup collisions for both vehicles
        checkItemCollisionsForVehicle(m_vehicle, true);
        checkItemCollisionsForVehicle(m_vehicle2, false);

        // Checkpoints for both vehicles
        checkCheckpoints();

        // Stunt evaluation for both
        updateStuntsForVehicle(m_vehicle, dt, m_airTimer, m_wheelieTimer, m_totalAirTime, m_cumulativeRotation, m_flips, true);
        updateStuntsForVehicle(m_vehicle2, dt, m_airTimer2, m_wheelieTimer2, m_totalAirTime2, m_cumulativeRotation2, m_flips2, false);

        // Check crash and fuel conditions
        if (!m_gameOver) {
            if (m_vehicle.isDriverDown()) {
                m_gameOver = true;
                m_gameOverReason = "P1 CRASHED!";
            } else if (m_vehicle.isOutOfFuel() && m_vehicle.getSpeedKmh() < 0.5f) {
                m_gameOver = true;
                m_gameOverReason = "P1 OUT OF FUEL!";
            }
        }

        if (!m_gameOver2) {
            if (m_vehicle2.isDriverDown()) {
                m_gameOver2 = true;
                m_gameOverReason2 = "P2 CRASHED!";
            } else if (m_vehicle2.isOutOfFuel() && m_vehicle2.getSpeedKmh() < 0.5f) {
                m_gameOver2 = true;
                m_gameOverReason2 = "P2 OUT OF FUEL!";
            }
        }

        // Evaluate race winner rules
        updateMultiplayerRules();
    }
}

void PhysicsWorld::updateMultiplayerRules() {
    const float FINISH_DISTANCE = 1500.0f;

    // Check finish line crossing
    if (m_distance >= FINISH_DISTANCE && m_winner == 0) {
        m_winner = 1;
        m_mpStatusMessage = "PLAYER 1 WINS AT FINISH LINE!";
        return;
    }
    if (m_distance2 >= FINISH_DISTANCE && m_winner == 0) {
        m_winner = 2;
        m_mpStatusMessage = "PLAYER 2 WINS AT FINISH LINE!";
        return;
    }

    // Both stopped
    if (m_gameOver && m_gameOver2) {
        if (m_winner == 0) {
            if (m_distance > m_distance2 + 1.0f) {
                m_winner = 1;
                m_mpStatusMessage = "PLAYER 1 WINS ON DISTANCE!";
            } else if (m_distance2 > m_distance + 1.0f) {
                m_winner = 2;
                m_mpStatusMessage = "PLAYER 2 WINS ON DISTANCE!";
            } else {
                m_winner = 3;
                m_mpStatusMessage = "DEAD HEAT DRAW!";
            }
        }
        return;
    }

    // P1 eliminated, P2 advancing
    if (m_gameOver && !m_gameOver2) {
        if (m_distance2 > m_distance) {
            m_winner = 2;
            m_mpStatusMessage = "PLAYER 2 OVERTAKES & WINS!";
        } else {
            int toBeat = static_cast<int>(m_distance - m_distance2);
            m_mpStatusMessage = "P1 STOPPED | P2 NEEDS +" + std::to_string(toBeat) + "m";
        }
        return;
    }

    // P2 eliminated, P1 advancing
    if (!m_gameOver && m_gameOver2) {
        if (m_distance > m_distance2) {
            m_winner = 1;
            m_mpStatusMessage = "PLAYER 1 OVERTAKES & WINS!";
        } else {
            int toBeat = static_cast<int>(m_distance2 - m_distance);
            m_mpStatusMessage = "P2 STOPPED | P1 NEEDS +" + std::to_string(toBeat) + "m";
        }
        return;
    }

    // Both active: display relative leader
    float delta = m_distance - m_distance2;
    if (std::abs(delta) < 2.5f) {
        m_mpStatusMessage = "NECK AND NECK!";
    } else if (delta > 0.0f) {
        m_mpStatusMessage = "P1 LEADS (+" + std::to_string(static_cast<int>(delta)) + "m)";
    } else {
        m_mpStatusMessage = "P2 LEADS (+" + std::to_string(static_cast<int>(-delta)) + "m)";
    }
}

void PhysicsWorld::checkItemCollisions() {
    checkItemCollisionsForVehicle(m_vehicle, true);
}

void PhysicsWorld::checkItemCollisionsForVehicle(Vehicle& veh, bool isP1) {
    Vec2 carPos = veh.chassisPos();
    auto& items = m_terrain.getItems();
    auto& collectedList = isP1 ? m_itemsCollectedP1 : m_itemsCollectedP2;

    if (collectedList.size() != items.size()) {
        collectedList.assign(items.size(), false);
    }

    for (size_t i = 0; i < items.size(); ++i) {
        if (collectedList[i]) continue;
        if (std::abs(items[i].position.x - carPos.x) > 2.8f) continue;

        float dx = items[i].position.x - carPos.x;
        float dy = items[i].position.y - carPos.y;
        float distSq = dx * dx + dy * dy;

        // Collection radius: 2.2 meters
        if (distSq < 2.2f * 2.2f) {
            collectedList[i] = true;
            if (!m_isMultiplayer) {
                items[i].collected = true;
            }

            if (items[i].type == ItemType::FUEL_CANISTER) {
                veh.addFuel(45.0f); // Calibrated +45% fuel recharge (not an easy 100% full-pass)
            } else {
                if (isP1) {
                    m_coinsCollected += items[i].value;
                }
            }
            if (m_onItem) {
                m_onItem(items[i].type, items[i].value, isP1);
            }
        }
    }
}

void PhysicsWorld::checkCheckpoints() {
    checkCheckpointsForVehicle(m_vehicle, m_lastClearedCheckpoint, true);
    if (m_isMultiplayer) {
        checkCheckpointsForVehicle(m_vehicle2, m_lastClearedCheckpoint2, false);
    }
}

void PhysicsWorld::checkCheckpointsForVehicle(Vehicle& veh, int& lastCp, bool isP1) {
    const auto& checkpoints = m_terrain.getCheckpoints();
    float carX = veh.chassisPos().x;

    for (const auto& cp : checkpoints) {
        if (carX >= cp.distance && lastCp < cp.index) {
            lastCp = cp.index;
            if (isP1) {
                m_coinsCollected += cp.coinReward;
                if (m_onCheckpoint) {
                    m_onCheckpoint(cp);
                }
            }
        }
    }
}

void PhysicsWorld::updateStunts(float dt) {
    updateStuntsForVehicle(m_vehicle, dt, m_airTimer, m_wheelieTimer, m_totalAirTime, m_cumulativeRotation, m_flips, true);
}

void PhysicsWorld::updateStuntsForVehicle(Vehicle& veh, float dt, float& airTimer, float& wheelieTimer,
                                         float& totalAirTime, float& cumulativeRotation, int& flips,
                                         bool isP1) {
    bool isAirborne = veh.isAirborne();

    if (isAirborne) {
        wheelieTimer = 0.0f;
        airTimer += dt;
        totalAirTime += dt;

        float dTheta = veh.chassisAngularVel() * dt;
        cumulativeRotation += dTheta;

        // Check for 360-degree Backflip
        if (cumulativeRotation >= 2.0f * static_cast<float>(M_PI)) {
            flips++;
            cumulativeRotation -= 2.0f * static_cast<float>(M_PI);
            veh.addFuel(15.0f); // Bonus fuel reward for daring stunts!
            if (isP1) {
                m_coinsCollected += 1000;
                if (m_onStunt) {
                    m_onStunt("BACKFLIP! +15% FUEL", 1000);
                }
            }
        }
        // Check for 360-degree Frontflip
        else if (cumulativeRotation <= -2.0f * static_cast<float>(M_PI)) {
            flips++;
            cumulativeRotation += 2.0f * static_cast<float>(M_PI);
            veh.addFuel(20.0f); // Bonus fuel reward for frontflip!
            if (isP1) {
                m_coinsCollected += 1500;
                if (m_onStunt) {
                    m_onStunt("FRONTFLIP! +20% FUEL", 1500);
                }
            }
        }
    } else {
        // Air Time landing bonus
        if (airTimer >= 1.2f && isP1) {
            int airBonus = static_cast<int>(airTimer * 180.0f);
            m_coinsCollected += airBonus;
            if (m_onStunt) {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "AIR TIME: %.1fs", airTimer);
                m_onStunt(buf, airBonus);
            }
        }
        airTimer = 0.0f;
        cumulativeRotation = 0.0f;

        // Wheelie Stunt Detection: driving on rear wheel only at speed
        if (veh.isRearOnGround() && !veh.isFrontOnGround() && veh.getSpeedKmh() > 18.0f) {
            wheelieTimer += dt;
            if (wheelieTimer >= 1.2f) {
                veh.addFuel(8.0f);
                if (isP1) {
                    m_coinsCollected += 400;
                    if (m_onStunt) {
                        m_onStunt("WHEELIE! +8% FUEL", 400);
                    }
                }
                wheelieTimer = 0.0f;
            }
        } else {
            wheelieTimer = 0.0f;
        }
    }
}

} // namespace Physics
