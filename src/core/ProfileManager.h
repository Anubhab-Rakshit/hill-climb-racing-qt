#pragma once

#include <string>
#include <map>
#include <vector>
#include <cmath>
#include "VehicleConfig.h"

namespace Core {

/**
 * @brief Manages player progression, multi-vehicle upgrades, coin economy, and persistence.
 */
class ProfileManager {
public:
    static constexpr int MAX_UPGRADE_LEVEL = 15;

    enum UpgradeType {
        UPGRADE_ENGINE = 0,
        UPGRADE_SUSPENSION,
        UPGRADE_TIRES,
        UPGRADE_4WD,
        UPGRADE_BRAKES,
        UPGRADE_TRANSMISSION,
        UPGRADE_CHASSIS,
        UPGRADE_DOWNFORCE,
        UPGRADE_COUNT
    };

    ProfileManager();

    // Economy
    int coins() const { return m_coins; }
    void addCoins(int amount);
    bool spendCoins(int amount);

    // Vehicle Selection & Unlocking
    Physics::VehicleType selectedVehicle() const { return m_selectedVehicle; }
    void setSelectedVehicle(Physics::VehicleType type);
    bool isVehicleUnlocked(Physics::VehicleType type) const;
    void unlockVehicle(Physics::VehicleType type);
    bool unlockVehicleWithCoins(Physics::VehicleType type);
    int getVehicleUnlockCost(Physics::VehicleType type) const;

    // Driver Selection
    Physics::DriverType selectedDriver() const { return m_selectedDriver; }
    void setSelectedDriver(Physics::DriverType type);

    // Upgrades for Selected Vehicle (backward compatibility)
    int getUpgradeLevel(UpgradeType type) const;
    int getUpgradeCost(UpgradeType type) const;
    bool canAffordUpgrade(UpgradeType type) const;
    bool purchaseUpgrade(UpgradeType type);

    // Upgrades for Specific Vehicle
    int getUpgradeLevel(Physics::VehicleType vehicle, UpgradeType type) const;
    int getUpgradeCost(Physics::VehicleType vehicle, UpgradeType type) const;
    bool canAffordUpgrade(Physics::VehicleType vehicle, UpgradeType type) const;
    bool purchaseUpgrade(Physics::VehicleType vehicle, UpgradeType type);

    // Physical Multipliers (Derived from upgrades)
    float getEngineTorqueMultiplier() const { return getEngineTorqueMultiplier(m_selectedVehicle); }
    float getEngineTorqueMultiplier(Physics::VehicleType vehicle) const;

    float getSuspensionStiffnessMultiplier() const { return getSuspensionStiffnessMultiplier(m_selectedVehicle); }
    float getSuspensionStiffnessMultiplier(Physics::VehicleType vehicle) const;

    float getTireGripMultiplier() const { return getTireGripMultiplier(m_selectedVehicle); }
    float getTireGripMultiplier(Physics::VehicleType vehicle) const;

    float get4wdTorqueSplit() const { return get4wdTorqueSplit(m_selectedVehicle); }
    float get4wdTorqueSplit(Physics::VehicleType vehicle) const;

    float getBrakingMultiplier() const { return getBrakingMultiplier(m_selectedVehicle); }
    float getBrakingMultiplier(Physics::VehicleType vehicle) const;

    float getTransmissionMultiplier() const { return getTransmissionMultiplier(m_selectedVehicle); }
    float getTransmissionMultiplier(Physics::VehicleType vehicle) const;

    float getChassisMassMultiplier() const { return getChassisMassMultiplier(m_selectedVehicle); }
    float getChassisMassMultiplier(Physics::VehicleType vehicle) const;

    float getDownforceMultiplier() const { return getDownforceMultiplier(m_selectedVehicle); }
    float getDownforceMultiplier(Physics::VehicleType vehicle) const;

    // High Scores & Stage Progression
    float getRecordDistance(const std::string& stageId) const;
    void recordDistance(const std::string& stageId, float distance);
    bool isStageUnlocked(const std::string& stageId) const;
    void unlockStage(const std::string& stageId);
    int getStageUnlockCost(const std::string& stageId) const;
    bool unlockStageWithCoins(const std::string& stageId);

    // Checkpoint Progression
    int getStageCheckpointProgress(const std::string& stageId) const;
    void recordStageCheckpoint(const std::string& stageId, int checkpointIndex);
    bool isStageCompleted(const std::string& stageId) const;
    void setStageCompleted(const std::string& stageId, bool completed = true);

    // Serialization
    void load();
    void save() const;

private:
    int m_coins;
    Physics::VehicleType m_selectedVehicle;
    Physics::DriverType m_selectedDriver;

    // Upgrade levels per vehicle: [VehicleType][UpgradeType]
    int m_upgradeLevels[static_cast<int>(Physics::VehicleType::COUNT)][UPGRADE_COUNT];
    bool m_vehicleUnlocked[static_cast<int>(Physics::VehicleType::COUNT)];

    std::map<std::string, float> m_stageRecords;
    std::map<std::string, bool> m_stageUnlocked;
    std::map<std::string, int> m_stageCheckpoints;
    std::map<std::string, bool> m_stageCompleted;
};

} // namespace Core
