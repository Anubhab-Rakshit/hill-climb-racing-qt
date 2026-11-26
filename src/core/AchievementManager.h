#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Core {

enum class AchievementStat {
    TOTAL_DISTANCE,
    BEST_RUN_DISTANCE,
    TOTAL_COINS,
    TOTAL_FLIPS,
    TOTAL_AIR_TIME,
    CHECKPOINTS_CLEARED,
    STAGES_COMPLETED,
    MOON_COMPLETIONS,
    VEHICLES_UNLOCKED,
    UPGRADES_PURCHASED,
    GRAND_PRIX_TROPHIES
};

struct AchievementDefinition {
    std::string id;
    std::string title;
    std::string description;
    AchievementStat stat;
    double target;
    int coinReward;
    int xpReward;
    uint32_t color;
};

struct AchievementView {
    AchievementDefinition definition;
    double current;
    double target;
    bool unlocked;
};

class AchievementManager {
public:
    AchievementManager();

    const std::vector<AchievementDefinition>& definitions() const { return m_definitions; }
    std::vector<AchievementView> snapshot() const;
    int unlockedCount() const;
    double currentValue(const AchievementDefinition& def) const;

    void setOnUnlock(std::function<void(const AchievementDefinition&)> cb) { m_onUnlock = std::move(cb); }

    void addTo(AchievementStat stat, double amount, bool shouldEvaluate = true);
    void recordMax(AchievementStat stat, double value, bool shouldEvaluate = true);
    void set(AchievementStat stat, double value, bool shouldEvaluate = true);
    std::vector<AchievementDefinition> evaluate();
    void syncProfileCounts(int vehiclesUnlocked, int upgradesPurchased, int grandPrixTrophies);
    void resetForTesting();

private:
    static const char* statKey(AchievementStat stat);

    std::vector<AchievementDefinition> m_definitions;
    std::function<void(const AchievementDefinition&)> m_onUnlock;
};

} // namespace Core
