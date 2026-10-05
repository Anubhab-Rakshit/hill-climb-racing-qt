#include "ProfileManager.h"
#include <QSettings>
#include <algorithm>

namespace Core {

static const int BASE_UPGRADE_COST[ProfileManager::UPGRADE_COUNT] = {
    800,  // Engine
    600,  // Suspension
    700,  // Tires
    1200  // 4WD
};

ProfileManager::ProfileManager()
    : m_coins(2500) // Starting seed coins so player can immediately test garage upgrades!
{
    for (int i = 0; i < UPGRADE_COUNT; ++i) {
        m_upgradeLevels[i] = 1;
    }
    m_stageUnlocked["countryside"] = true;
    m_stageUnlocked["desert"] = true;
    m_stageUnlocked["moon"] = false;
    m_stageUnlocked["mountain"] = false;

    m_stageRecords["countryside"] = 450.0f;
    m_stageRecords["desert"] = 320.0f;
    m_stageRecords["moon"] = 0.0f;
    m_stageRecords["mountain"] = 0.0f;

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

int ProfileManager::getUpgradeLevel(UpgradeType type) const {
    if (type >= 0 && type < UPGRADE_COUNT) {
        return m_upgradeLevels[type];
    }
    return 1;
}

int ProfileManager::getUpgradeCost(UpgradeType type) const {
    if (type < 0 || type >= UPGRADE_COUNT) return 999999;
    int lvl = m_upgradeLevels[type];
    if (lvl >= 20) return 0; // Maxed out
    // Exponential progression curve: Base * 1.25^(lvl - 1)
    float cost = BASE_UPGRADE_COST[type] * std::pow(1.24f, static_cast<float>(lvl - 1));
    return static_cast<int>(cost / 50.0f) * 50; // Round to neat 50s
}

bool ProfileManager::canAffordUpgrade(UpgradeType type) const {
    if (type < 0 || type >= UPGRADE_COUNT) return false;
    if (m_upgradeLevels[type] >= 20) return false;
    return m_coins >= getUpgradeCost(type);
}

bool ProfileManager::purchaseUpgrade(UpgradeType type) {
    if (canAffordUpgrade(type)) {
        spendCoins(getUpgradeCost(type));
        m_upgradeLevels[type]++;
        save();
        return true;
    }
    return false;
}

float ProfileManager::getEngineTorqueMultiplier() const {
    // Level 1 = 1.0x, Level 20 = ~3.2x
    return 1.0f + 0.12f * (m_upgradeLevels[UPGRADE_ENGINE] - 1);
}

float ProfileManager::getSuspensionStiffnessMultiplier() const {
    // Level 1 = 1.0x, Level 20 = ~2.9x
    return 1.0f + 0.10f * (m_upgradeLevels[UPGRADE_SUSPENSION] - 1);
}

float ProfileManager::getTireGripMultiplier() const {
    // Level 1 = 1.0x, Level 20 = ~1.95x
    return 1.0f + 0.05f * (m_upgradeLevels[UPGRADE_TIRES] - 1);
}

float ProfileManager::get4wdTorqueSplit() const {
    // Level 1 = 0% front wheel drive, Level 20 = 50% front wheel drive (full AWD)
    float frac = static_cast<float>(m_upgradeLevels[UPGRADE_4WD] - 1) / 19.0f;
    return 0.50f * frac;
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

void ProfileManager::load() {
    QSettings settings("HillClimbQtTeam", "HillClimbRacingQt");
    if (settings.contains("coins")) {
        m_coins = settings.value("coins", m_coins).toInt();
        m_upgradeLevels[UPGRADE_ENGINE] = settings.value("up_engine", 1).toInt();
        m_upgradeLevels[UPGRADE_SUSPENSION] = settings.value("up_suspension", 1).toInt();
        m_upgradeLevels[UPGRADE_TIRES] = settings.value("up_tires", 1).toInt();
        m_upgradeLevels[UPGRADE_4WD] = settings.value("up_4wd", 1).toInt();

        m_stageRecords["countryside"] = settings.value("rec_countryside", 450.0f).toFloat();
        m_stageRecords["desert"] = settings.value("rec_desert", 320.0f).toFloat();
        m_stageRecords["moon"] = settings.value("rec_moon", 0.0f).toFloat();
        m_stageRecords["mountain"] = settings.value("rec_mountain", 0.0f).toFloat();

        m_stageUnlocked["moon"] = settings.value("unlocked_moon", false).toBool();
        m_stageUnlocked["mountain"] = settings.value("unlocked_mountain", false).toBool();
    }
}

void ProfileManager::save() const {
    QSettings settings("HillClimbQtTeam", "HillClimbRacingQt");
    settings.setValue("coins", m_coins);
    settings.setValue("up_engine", m_upgradeLevels[UPGRADE_ENGINE]);
    settings.setValue("up_suspension", m_upgradeLevels[UPGRADE_SUSPENSION]);
    settings.setValue("up_tires", m_upgradeLevels[UPGRADE_TIRES]);
    settings.setValue("up_4wd", m_upgradeLevels[UPGRADE_4WD]);

    settings.setValue("rec_countryside", m_stageRecords.at("countryside"));
    settings.setValue("rec_desert", m_stageRecords.at("desert"));
    settings.setValue("rec_moon", m_stageRecords.at("moon"));
    settings.setValue("rec_mountain", m_stageRecords.at("mountain"));

    settings.setValue("unlocked_moon", m_stageUnlocked.at("moon"));
    settings.setValue("unlocked_mountain", m_stageUnlocked.at("mountain"));
}

} // namespace Core
