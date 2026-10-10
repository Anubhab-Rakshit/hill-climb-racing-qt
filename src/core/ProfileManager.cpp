#include "ProfileManager.h"
#include "../physics/TerrainConfig.h"
#include <QSettings>
#include <algorithm>
#include <cmath>

namespace Core {

static const int BASE_UPGRADE_COST[ProfileManager::UPGRADE_COUNT] = {
    800,  // Engine
    600,  // Suspension
    700,  // Tires
    1200, // 4WD
    500,  // Brakes
    1000, // Transmission
    900,  // Chassis
    850   // Downforce
};

static const char* UPGRADE_KEYS[ProfileManager::UPGRADE_COUNT] = {
    "engine",
    "suspension",
    "tires",
    "4wd",
    "brakes",
    "transmission",
    "chassis",
    "downforce"
};

ProfileManager::ProfileManager()
    : m_coins(5000) // Starting seed coins so player can test vehicle & garage upgrades immediately
    , m_selectedVehicle(Physics::VehicleType::OFFROADER)
    , m_selectedDriver(Physics::DriverType::BILL)
{
    int numVehicles = static_cast<int>(Physics::VehicleType::COUNT);
    for (int v = 0; v < numVehicles; ++v) {
        m_vehicleUnlocked[v] = (v == static_cast<int>(Physics::VehicleType::OFFROADER));
        for (int u = 0; u < UPGRADE_COUNT; ++u) {
            m_upgradeLevels[v][u] = 1;
        }
    }

    const std::vector<std::string> allStageIds = {"countryside", "desert", "arctic", "mountain", "moon", "volcano"};
    for (const auto& sId : allStageIds) {
        m_stageUnlocked[sId] = (sId == "countryside");
        m_stageRecords[sId] = (sId == "countryside") ? 450.0f : 0.0f;
        m_stageCheckpoints[sId] = 0;
        m_stageCompleted[sId] = false;
    }

    load();
}

void ProfileManager::addCoins(int amount) {
    if (amount > 0) {
        m_coins += amount;
        save();
    }
}

bool ProfileManager::spendCoins(int amount) {
    if (amount > 0 && m_coins >= amount) {
        m_coins -= amount;
        save();
        return true;
    }
    return false;
}

void ProfileManager::setSelectedVehicle(Physics::VehicleType type) {
    if (type >= Physics::VehicleType::OFFROADER && type < Physics::VehicleType::COUNT) {
        if (isVehicleUnlocked(type)) {
            m_selectedVehicle = type;
            save();
        }
    }
}

bool ProfileManager::isVehicleUnlocked(Physics::VehicleType type) const {
    int v = static_cast<int>(type);
    if (v >= 0 && v < static_cast<int>(Physics::VehicleType::COUNT)) {
        return m_vehicleUnlocked[v];
    }
    return false;
}

void ProfileManager::unlockVehicle(Physics::VehicleType type) {
    int v = static_cast<int>(type);
    if (v >= 0 && v < static_cast<int>(Physics::VehicleType::COUNT)) {
        m_vehicleUnlocked[v] = true;
        save();
    }
}

int ProfileManager::getVehicleUnlockCost(Physics::VehicleType type) const {
    const auto& cfg = Physics::VehicleRegistry::getConfig(type);
    return cfg.unlockCost;
}

bool ProfileManager::unlockVehicleWithCoins(Physics::VehicleType type) {
    int cost = getVehicleUnlockCost(type);
    if (!isVehicleUnlocked(type) && spendCoins(cost)) {
        unlockVehicle(type);
        m_selectedVehicle = type;
        return true;
    }
    return false;
}

void ProfileManager::setSelectedDriver(Physics::DriverType type) {
    if (type >= Physics::DriverType::BILL && type < Physics::DriverType::COUNT) {
        m_selectedDriver = type;
        save();
    }
}

int ProfileManager::getUpgradeLevel(UpgradeType type) const {
    return getUpgradeLevel(m_selectedVehicle, type);
}

int ProfileManager::getUpgradeLevel(Physics::VehicleType vehicle, UpgradeType type) const {
    int v = static_cast<int>(vehicle);
    int u = static_cast<int>(type);
    if (v >= 0 && v < static_cast<int>(Physics::VehicleType::COUNT) && u >= 0 && u < UPGRADE_COUNT) {
        return m_upgradeLevels[v][u];
    }
    return 1;
}

int ProfileManager::getUpgradeCost(UpgradeType type) const {
    return getUpgradeCost(m_selectedVehicle, type);
}

int ProfileManager::getUpgradeCost(Physics::VehicleType vehicle, UpgradeType type) const {
    int lvl = getUpgradeLevel(vehicle, type);
    if (lvl >= MAX_UPGRADE_LEVEL) return 0;
    int u = static_cast<int>(type);
    if (u < 0 || u >= UPGRADE_COUNT) return 999999;
    float cost = BASE_UPGRADE_COST[u] * std::pow(1.23f, static_cast<float>(lvl - 1));
    return static_cast<int>(cost / 50.0f) * 50;
}

bool ProfileManager::canAffordUpgrade(UpgradeType type) const {
    return canAffordUpgrade(m_selectedVehicle, type);
}

bool ProfileManager::canAffordUpgrade(Physics::VehicleType vehicle, UpgradeType type) const {
    int lvl = getUpgradeLevel(vehicle, type);
    if (lvl >= MAX_UPGRADE_LEVEL) return false;
    return m_coins >= getUpgradeCost(vehicle, type);
}

bool ProfileManager::purchaseUpgrade(UpgradeType type) {
    return purchaseUpgrade(m_selectedVehicle, type);
}

bool ProfileManager::purchaseUpgrade(Physics::VehicleType vehicle, UpgradeType type) {
    if (canAffordUpgrade(vehicle, type)) {
        int cost = getUpgradeCost(vehicle, type);
        spendCoins(cost);
        int v = static_cast<int>(vehicle);
        int u = static_cast<int>(type);
        if (v >= 0 && v < static_cast<int>(Physics::VehicleType::COUNT) && u >= 0 && u < UPGRADE_COUNT) {
            m_upgradeLevels[v][u]++;
            save();
            return true;
        }
    }
    return false;
}

float ProfileManager::getEngineTorqueMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_ENGINE);
    return 1.0f + 0.12f * (lvl - 1);
}

float ProfileManager::getSuspensionStiffnessMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_SUSPENSION);
    return 1.0f + 0.10f * (lvl - 1);
}

float ProfileManager::getTireGripMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_TIRES);
    return 1.0f + 0.06f * (lvl - 1);
}

float ProfileManager::get4wdTorqueSplit(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_4WD);
    float frac = static_cast<float>(lvl - 1) / static_cast<float>(MAX_UPGRADE_LEVEL - 1);
    return 0.30f * frac;
}

float ProfileManager::getBrakingMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_BRAKES);
    return 1.0f + 0.14f * (lvl - 1);
}

float ProfileManager::getTransmissionMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_TRANSMISSION);
    return 1.0f + 0.05f * (lvl - 1);
}

float ProfileManager::getChassisMassMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_CHASSIS);
    // Upgraded chassis reduces dead weight up to 25%
    return std::max(0.75f, 1.0f - 0.018f * (lvl - 1));
}

float ProfileManager::getDownforceMultiplier(Physics::VehicleType vehicle) const {
    int lvl = getUpgradeLevel(vehicle, UPGRADE_DOWNFORCE);
    return 1.0f + 0.12f * (lvl - 1);
}

float ProfileManager::getRecordDistance(const std::string& stageId) const {
    auto it = m_stageRecords.find(stageId);
    if (it != m_stageRecords.end()) return it->second;
    return 0.0f;
}

void ProfileManager::recordDistance(const std::string& stageId, float distance) {
    if (distance > m_stageRecords[stageId]) {
        m_stageRecords[stageId] = distance;
        save();
    }
}

bool ProfileManager::isStageUnlocked(const std::string& stageId) const {
    auto it = m_stageUnlocked.find(stageId);
    if (it != m_stageUnlocked.end()) return it->second;
    return false;
}

void ProfileManager::unlockStage(const std::string& stageId) {
    m_stageUnlocked[stageId] = true;
    save();
}

int ProfileManager::getStageUnlockCost(const std::string& stageId) const {
    return Physics::BiomeRegistry::getBiome(stageId).unlockCostCoins;
}

bool ProfileManager::unlockStageWithCoins(const std::string& stageId) {
    int cost = getStageUnlockCost(stageId);
    if (!isStageUnlocked(stageId) && spendCoins(cost)) {
        unlockStage(stageId);
        return true;
    }
    return false;
}

int ProfileManager::getStageCheckpointProgress(const std::string& stageId) const {
    auto it = m_stageCheckpoints.find(stageId);
    if (it != m_stageCheckpoints.end()) return it->second;
    return 0;
}

void ProfileManager::recordStageCheckpoint(const std::string& stageId, int checkpointIndex) {
    if (checkpointIndex > m_stageCheckpoints[stageId]) {
        m_stageCheckpoints[stageId] = checkpointIndex;
        if (checkpointIndex >= 5) {
            setStageCompleted(stageId, true);
        } else {
            save();
        }
    }
}

bool ProfileManager::isStageCompleted(const std::string& stageId) const {
    auto it = m_stageCompleted.find(stageId);
    if (it != m_stageCompleted.end()) return it->second;
    return false;
}

void ProfileManager::setStageCompleted(const std::string& stageId, bool completed) {
    m_stageCompleted[stageId] = completed;
    if (completed) {
        m_stageCheckpoints[stageId] = 5;
        const auto& spec = Physics::BiomeRegistry::getBiome(stageId);
        if (!spec.nextBiomeId.empty()) {
            m_stageUnlocked[spec.nextBiomeId] = true;
        }
    }
    save();
}

void ProfileManager::load() {
    QSettings settings("HillClimbQtTeam", "HillClimbRacingQt");
    if (settings.contains("coins")) {
        m_coins = settings.value("coins", m_coins).toInt();
        int selV = settings.value("selected_vehicle", 0).toInt();
        if (selV >= 0 && selV < static_cast<int>(Physics::VehicleType::COUNT)) {
            m_selectedVehicle = static_cast<Physics::VehicleType>(selV);
        }
        int selD = settings.value("selected_driver", 0).toInt();
        if (selD >= 0 && selD < static_cast<int>(Physics::DriverType::COUNT)) {
            m_selectedDriver = static_cast<Physics::DriverType>(selD);
        }

        int numVehicles = static_cast<int>(Physics::VehicleType::COUNT);
        for (int v = 0; v < numVehicles; ++v) {
            auto vType = static_cast<Physics::VehicleType>(v);
            const auto& cfg = Physics::VehicleRegistry::getConfig(vType);
            QString unlockKey = QString::fromStdString("unlocked_veh_" + cfg.id);
            if (v == 0) {
                m_vehicleUnlocked[v] = true;
            } else if (settings.contains(unlockKey)) {
                m_vehicleUnlocked[v] = settings.value(unlockKey).toBool();
            }

            for (int u = 0; u < UPGRADE_COUNT; ++u) {
                QString upKey = QString::fromStdString("up_" + cfg.id + "_" + UPGRADE_KEYS[u]);
                if (settings.contains(upKey)) {
                    m_upgradeLevels[v][u] = settings.value(upKey, 1).toInt();
                } else if (v == 0) {
                    // Backward compatibility with previous version keys
                    QString oldKey = QString::fromStdString(std::string("up_") + UPGRADE_KEYS[u]);
                    if (settings.contains(oldKey)) {
                        m_upgradeLevels[v][u] = settings.value(oldKey, 1).toInt();
                    }
                }
            }
        }

        const std::vector<std::string> allStageIds = {"countryside", "desert", "arctic", "mountain", "moon", "volcano"};
        for (const auto& sId : allStageIds) {
            QString recKey = QString::fromStdString("rec_" + sId);
            if (settings.contains(recKey)) {
                m_stageRecords[sId] = settings.value(recKey, m_stageRecords[sId]).toFloat();
            }
            QString unlockKey = QString::fromStdString("unlocked_" + sId);
            if (settings.contains(unlockKey)) {
                m_stageUnlocked[sId] = settings.value(unlockKey, m_stageUnlocked[sId]).toBool();
            }
            QString cpKey = QString::fromStdString("cp_" + sId);
            if (settings.contains(cpKey)) {
                m_stageCheckpoints[sId] = settings.value(cpKey, 0).toInt();
            }
            QString compKey = QString::fromStdString("completed_" + sId);
            if (settings.contains(compKey)) {
                m_stageCompleted[sId] = settings.value(compKey, false).toBool();
            }
        }
    }
}

void ProfileManager::save() const {
    QSettings settings("HillClimbQtTeam", "HillClimbRacingQt");
    settings.setValue("coins", m_coins);
    settings.setValue("selected_vehicle", static_cast<int>(m_selectedVehicle));
    settings.setValue("selected_driver", static_cast<int>(m_selectedDriver));

    int numVehicles = static_cast<int>(Physics::VehicleType::COUNT);
    for (int v = 0; v < numVehicles; ++v) {
        auto vType = static_cast<Physics::VehicleType>(v);
        const auto& cfg = Physics::VehicleRegistry::getConfig(vType);
        settings.setValue(QString::fromStdString("unlocked_veh_" + cfg.id), m_vehicleUnlocked[v]);

        for (int u = 0; u < UPGRADE_COUNT; ++u) {
            settings.setValue(QString::fromStdString("up_" + cfg.id + "_" + UPGRADE_KEYS[u]), m_upgradeLevels[v][u]);
        }
    }

    // Save legacy keys for offroader for backwards compatibility
    settings.setValue("up_engine", m_upgradeLevels[0][UPGRADE_ENGINE]);
    settings.setValue("up_suspension", m_upgradeLevels[0][UPGRADE_SUSPENSION]);
    settings.setValue("up_tires", m_upgradeLevels[0][UPGRADE_TIRES]);
    settings.setValue("up_4wd", m_upgradeLevels[0][UPGRADE_4WD]);

    for (const auto& pair : m_stageRecords) {
        settings.setValue(QString::fromStdString("rec_" + pair.first), pair.second);
    }
    for (const auto& pair : m_stageUnlocked) {
        settings.setValue(QString::fromStdString("unlocked_" + pair.first), pair.second);
    }
    for (const auto& pair : m_stageCheckpoints) {
        settings.setValue(QString::fromStdString("cp_" + pair.first), pair.second);
    }
    for (const auto& pair : m_stageCompleted) {
        settings.setValue(QString::fromStdString("completed_" + pair.first), pair.second);
    }
}

} // namespace Core
