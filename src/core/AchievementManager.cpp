#include "AchievementManager.h"
#include "AccountManager.h"
#include <algorithm>

namespace Core {

AchievementManager::AchievementManager()
    : m_definitions{
          {"first_run", "FIRST BLOOD", "Complete a 100m run", AchievementStat::BEST_RUN_DISTANCE, 100.0, 500, 100, 0xFFFFD54F},
          {"long_haul", "CROSS-COUNTRY", "Drive 5,000m across all runs", AchievementStat::TOTAL_DISTANCE, 5000.0, 2000, 400, 0xFF4FC3F7},
          {"coin_hoarder", "COIN HOARDER", "Collect 1,000 lifetime coins", AchievementStat::TOTAL_COINS, 1000.0, 1500, 300, 0xFFFFB300},
          {"flip_rookie", "FLIP ROOKIE", "Land 10 aerial flips", AchievementStat::TOTAL_FLIPS, 10.0, 1000, 250, 0xFF00E5FF},
          {"airborne", "BIG AIR", "Spend 10s airborne", AchievementStat::TOTAL_AIR_TIME, 10.0, 1000, 250, 0xFF69F0AE},
          {"checkpoint_sweep", "CHECKPOINT SWEEP", "Clear 25 checkpoints", AchievementStat::CHECKPOINTS_CLEARED, 25.0, 1500, 350, 0xFF7C4DFF},
          {"stage_solver", "TRAILBLAZER", "Complete your first stage", AchievementStat::STAGES_COMPLETED, 1.0, 2000, 400, 0xFF76FF03},
          {"world_tour", "WORLD TOUR", "Complete all six stages", AchievementStat::STAGES_COMPLETED, 6.0, 8000, 1200, 0xFF18FFFF},
          {"lunar_legend", "LUNAR LEGEND", "Complete The Moon", AchievementStat::MOON_COMPLETIONS, 1.0, 4000, 700, 0xFFE0E0E0},
          {"garage_collector", "FLEET COMMANDER", "Unlock five vehicles", AchievementStat::VEHICLES_UNLOCKED, 5.0, 5000, 800, 0xFFFF7043},
          {"tuner", "MASTER TUNER", "Purchase ten upgrades", AchievementStat::UPGRADES_PURCHASED, 10.0, 4000, 700, 0xFFB388FF},
          {"gp_champion", "GRAND PRIX CHAMPION", "Win a Grand Prix circuit", AchievementStat::GRAND_PRIX_TROPHIES, 1.0, 10000, 1500, 0xFFFFCA28}
     }
{
}

const char* AchievementManager::statKey(AchievementStat stat) {
    switch (stat) {
        case AchievementStat::TOTAL_DISTANCE: return "ach_total_distance";
        case AchievementStat::BEST_RUN_DISTANCE: return "ach_best_run_distance";
        case AchievementStat::TOTAL_COINS: return "ach_total_coins";
        case AchievementStat::TOTAL_FLIPS: return "ach_total_flips";
        case AchievementStat::TOTAL_AIR_TIME: return "ach_total_air_time";
        case AchievementStat::CHECKPOINTS_CLEARED: return "ach_checkpoints_cleared";
        case AchievementStat::STAGES_COMPLETED: return "ach_stages_completed";
        case AchievementStat::MOON_COMPLETIONS: return "ach_moon_completions";
        case AchievementStat::VEHICLES_UNLOCKED: return "ach_vehicles_unlocked";
        case AchievementStat::UPGRADES_PURCHASED: return "ach_upgrades_purchased";
        case AchievementStat::GRAND_PRIX_TROPHIES: return "ach_grand_prix_trophies";
    }
    return "ach_unknown";
}

double AchievementManager::currentValue(const AchievementDefinition& def) const {
    return AccountManager::instance().getStat(statKey(def.stat));
}

std::vector<AchievementView> AchievementManager::snapshot() const {
    std::vector<AchievementView> views;
    views.reserve(m_definitions.size());
    for (const auto& def : m_definitions) {
        views.push_back({def, currentValue(def), def.target,
                         AccountManager::instance().hasAchievement(def.id)});
    }
    return views;
}

int AchievementManager::unlockedCount() const {
    int count = 0;
    for (const auto& def : m_definitions) {
        if (AccountManager::instance().hasAchievement(def.id)) ++count;
    }
    return count;
}

void AchievementManager::addTo(AchievementStat stat, double amount, bool shouldEvaluate) {
    AccountManager::instance().addToStat(statKey(stat), amount);
    if (shouldEvaluate) evaluate();
}

void AchievementManager::recordMax(AchievementStat stat, double value, bool shouldEvaluate) {
    AccountManager::instance().recordMaxStat(statKey(stat), value);
    if (shouldEvaluate) evaluate();
}

void AchievementManager::set(AchievementStat stat, double value, bool shouldEvaluate) {
    AccountManager::instance().setStat(statKey(stat), value);
    if (shouldEvaluate) evaluate();
}

std::vector<AchievementDefinition> AchievementManager::evaluate() {
    std::vector<AchievementDefinition> unlocked;
    for (const auto& def : m_definitions) {
        if (AccountManager::instance().hasAchievement(def.id)) continue;
        if (currentValue(def) + 1e-9 < def.target) continue;

        AccountManager::instance().unlockAchievement(def.id);
        AccountManager::instance().addCoins(def.coinReward);
        AccountManager::instance().addXP(def.xpReward);
        unlocked.push_back(def);
        if (m_onUnlock) m_onUnlock(def);
    }
    return unlocked;
}

void AchievementManager::syncProfileCounts(int vehiclesUnlocked, int upgradesPurchased, int grandPrixTrophies) {
    set(AchievementStat::VEHICLES_UNLOCKED, vehiclesUnlocked, false);
    set(AchievementStat::UPGRADES_PURCHASED, upgradesPurchased, false);
    set(AchievementStat::GRAND_PRIX_TROPHIES, grandPrixTrophies, false);
    evaluate();
}

void AchievementManager::resetForTesting() {
    AccountManager::instance().resetAchievementsForTesting();
}

} // namespace Core
