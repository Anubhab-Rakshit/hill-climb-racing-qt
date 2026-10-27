#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>

namespace Core {

struct UserAccount {
    std::string username;
    std::string password;
    int level = 1;
    int xp = 0;
    int coins = 1000;
    std::set<std::string> unlockedVehicles = {"offroader"};
    std::set<std::string> unlockedStages = {"countryside"};
    std::map<std::string, float> records;
    int grandPrixTrophies = 0;
};

/**
 * @brief Manages user account authentication, unique names, leveling, and persistence.
 */
class AccountManager {
public:
    static AccountManager& instance();

    void load();
    void save() const;

    bool isUsernameTaken(const std::string& username) const;
    std::vector<std::string> generateSuggestions(const std::string& baseName) const;

    bool registerAccount(const std::string& username, const std::string& password,
                         std::string& outError, std::vector<std::string>& outSuggestions);

    bool loginAccount(const std::string& username, const std::string& password,
                      std::string& outError);

    void logout();

    bool isLoggedIn() const { return m_loggedIn; }
    UserAccount* activeAccount();
    const UserAccount* activeAccount() const;

    void addXP(int amount);
    void addCoins(int amount);
    bool spendCoins(int amount);

    int getXPForNextLevel(int currentLevel) const;
    float getLevelProgress(int level, int xp) const;

    void unlockVehicle(const std::string& vehicleId);
    bool isVehicleUnlocked(const std::string& vehicleId) const;

    void unlockStage(const std::string& stageId);
    bool isStageUnlocked(const std::string& stageId) const;

    void recordDistance(const std::string& stageId, float dist);
    float getRecordDistance(const std::string& stageId) const;

private:
    AccountManager();
    ~AccountManager() = default;

    std::map<std::string, UserAccount> m_accounts;
    std::string m_activeUsername;
    bool m_loggedIn = false;
    UserAccount m_guestAccount;
};

} // namespace Core
