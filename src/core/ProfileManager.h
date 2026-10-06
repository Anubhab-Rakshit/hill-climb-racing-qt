#pragma once

#include <string>
#include <map>
#include <cmath>

namespace Core {

/**
 * @brief Manages player progression, upgrades, coin economy, and high-score persistence.
 */
class ProfileManager {
public:
    enum UpgradeType {
        UPGRADE_ENGINE = 0,
        UPGRADE_SUSPENSION,
        UPGRADE_TIRES,
        UPGRADE_4WD,
        UPGRADE_COUNT
    };

    ProfileManager();

    // Economy
    int coins() const { return m_coins; }
    void addCoins(int amount);
    bool spendCoins(int amount);

    // Upgrades
    int getUpgradeLevel(UpgradeType type) const;
    int getUpgradeCost(UpgradeType type) const;
    bool canAffordUpgrade(UpgradeType type) const;
    bool purchaseUpgrade(UpgradeType type);

    // Physical multipliers derived from upgrades
    float getEngineTorqueMultiplier() const;
    float getSuspensionStiffnessMultiplier() const;
    float getTireGripMultiplier() const;
    float get4wdTorqueSplit() const; // 0.0 = 100% RWD, 0.5 = 50/50 AWD

    // High Scores & Progression
    float getRecordDistance(const std::string& stageId) const;
    void recordDistance(const std::string& stageId, float distance);
    bool isStageUnlocked(const std::string& stageId) const;
    void unlockStage(const std::string& stageId);
    int getStageUnlockCost(const std::string& stageId) const;
    bool unlockStageWithCoins(const std::string& stageId);

    // Serialization
    void load();
    void save() const;

private:
    int m_coins;
    int m_upgradeLevels[UPGRADE_COUNT];
    std::map<std::string, float> m_stageRecords;
    std::map<std::string, bool> m_stageUnlocked;
};

} // namespace Core
