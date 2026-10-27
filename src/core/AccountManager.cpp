#include "AccountManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <algorithm>
#include <random>

namespace Core {

AccountManager& AccountManager::instance() {
    static AccountManager s_mgr;
    return s_mgr;
}

AccountManager::AccountManager() {
    m_guestAccount.username = "GuestRacer";
    m_guestAccount.password = "";
    m_guestAccount.level = 1;
    m_guestAccount.xp = 0;
    m_guestAccount.coins = 2500;
    m_guestAccount.unlockedVehicles = {"offroader", "rallycar"};
    m_guestAccount.unlockedStages = {"countryside"};

    load();
}

void AccountManager::load() {
    QFile file("accounts.json");
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonObject accountsObj = root["accounts"].toObject();

    m_accounts.clear();
    for (auto it = accountsObj.begin(); it != accountsObj.end(); ++it) {
        QJsonObject accJson = it.value().toObject();
        UserAccount acc;
        acc.username = it.key().toStdString();
        acc.password = accJson["password"].toString().toStdString();
        acc.level = accJson["level"].toInt(1);
        acc.xp = accJson["xp"].toInt(0);
        acc.coins = accJson["coins"].toInt(1000);
        acc.grandPrixTrophies = accJson["trophies"].toInt(0);

        QJsonArray vehArr = accJson["unlockedVehicles"].toArray();
        for (const auto& v : vehArr) {
            acc.unlockedVehicles.insert(v.toString().toStdString());
        }
        if (acc.unlockedVehicles.empty()) {
            acc.unlockedVehicles.insert("offroader");
        }

        QJsonArray stgArr = accJson["unlockedStages"].toArray();
        for (const auto& s : stgArr) {
            acc.unlockedStages.insert(s.toString().toStdString());
        }
        if (acc.unlockedStages.empty()) {
            acc.unlockedStages.insert("countryside");
        }

        QJsonObject recObj = accJson["records"].toObject();
        for (auto rit = recObj.begin(); rit != recObj.end(); ++rit) {
            acc.records[rit.key().toStdString()] = static_cast<float>(rit.value().toDouble(0.0));
        }

        m_accounts[acc.username] = acc;
    }

    std::string lastUser = root["lastUser"].toString().toStdString();
    if (!lastUser.empty() && m_accounts.find(lastUser) != m_accounts.end()) {
        m_activeUsername = lastUser;
        m_loggedIn = true;
    }
}

void AccountManager::save() const {
    QJsonObject root;
    QJsonObject accountsObj;

    for (const auto& pair : m_accounts) {
        const UserAccount& acc = pair.second;
        QJsonObject accJson;
        accJson["password"] = QString::fromStdString(acc.password);
        accJson["level"] = acc.level;
        accJson["xp"] = acc.xp;
        accJson["coins"] = acc.coins;
        accJson["trophies"] = acc.grandPrixTrophies;

        QJsonArray vehArr;
        for (const auto& v : acc.unlockedVehicles) {
            vehArr.append(QString::fromStdString(v));
        }
        accJson["unlockedVehicles"] = vehArr;

        QJsonArray stgArr;
        for (const auto& s : acc.unlockedStages) {
            stgArr.append(QString::fromStdString(s));
        }
        accJson["unlockedStages"] = stgArr;

        QJsonObject recObj;
        for (const auto& r : acc.records) {
            recObj[QString::fromStdString(r.first)] = r.second;
        }
        accJson["records"] = recObj;

        accountsObj[QString::fromStdString(acc.username)] = accJson;
    }

    root["accounts"] = accountsObj;
    root["lastUser"] = QString::fromStdString(m_activeUsername);

    QFile file("accounts.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

bool AccountManager::isUsernameTaken(const std::string& username) const {
    for (const auto& pair : m_accounts) {
        if (QString::compare(QString::fromStdString(pair.first),
                             QString::fromStdString(username),
                             Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> AccountManager::generateSuggestions(const std::string& baseName) const {
    std::vector<std::string> suggestions;
    std::vector<std::string> suffixes = {
        "_Pro", "_Turbo", "_99", "_Racer", "_GT", "_Apex", "_Nitro", "_007"
    };

    for (const auto& suf : suffixes) {
        std::string cand = baseName + suf;
        if (!isUsernameTaken(cand)) {
            suggestions.push_back(cand);
            if (suggestions.size() >= 4) break;
        }
    }

    if (suggestions.size() < 4) {
        for (int i = 1; i <= 20; ++i) {
            std::string cand = baseName + "_" + std::to_string(i);
            if (!isUsernameTaken(cand)) {
                suggestions.push_back(cand);
                if (suggestions.size() >= 4) break;
            }
        }
    }

    return suggestions;
}

bool AccountManager::registerAccount(const std::string& username, const std::string& password,
                                     std::string& outError, std::vector<std::string>& outSuggestions) {
    outSuggestions.clear();
    std::string trimmed = username;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());

    if (trimmed.length() < 3) {
        outError = "USERNAME MUST BE AT LEAST 3 CHARACTERS!";
        return false;
    }
    if (trimmed.length() > 16) {
        outError = "USERNAME CANNOT EXCEED 16 CHARACTERS!";
        return false;
    }

    if (isUsernameTaken(trimmed)) {
        outError = "USERNAME ALREADY TAKEN!";
        outSuggestions = generateSuggestions(trimmed);
        return false;
    }

    UserAccount acc;
    acc.username = trimmed;
    acc.password = password;
    acc.level = 1;
    acc.xp = 0;
    acc.coins = 2000; // Starter bonus
    acc.unlockedVehicles = {"offroader"};
    acc.unlockedStages = {"countryside"};

    m_accounts[trimmed] = acc;
    m_activeUsername = trimmed;
    m_loggedIn = true;
    save();

    outError = "";
    return true;
}

bool AccountManager::loginAccount(const std::string& username, const std::string& password,
                                  std::string& outError) {
    for (auto& pair : m_accounts) {
        if (QString::compare(QString::fromStdString(pair.first),
                             QString::fromStdString(username),
                             Qt::CaseInsensitive) == 0) {
            if (!pair.second.password.empty() && pair.second.password != password) {
                outError = "INCORRECT PASSWORD!";
                return false;
            }
            m_activeUsername = pair.second.username;
            m_loggedIn = true;
            save();
            outError = "";
            return true;
        }
    }

    outError = "ACCOUNT NOT FOUND!";
    return false;
}

void AccountManager::logout() {
    m_loggedIn = false;
    m_activeUsername = "";
    save();
}

UserAccount* AccountManager::activeAccount() {
    if (m_loggedIn) {
        auto it = m_accounts.find(m_activeUsername);
        if (it != m_accounts.end()) return &it->second;
    }
    return &m_guestAccount;
}

const UserAccount* AccountManager::activeAccount() const {
    if (m_loggedIn) {
        auto it = m_accounts.find(m_activeUsername);
        if (it != m_accounts.end()) return &it->second;
    }
    return &m_guestAccount;
}

int AccountManager::getXPForNextLevel(int currentLevel) const {
    return currentLevel * 600;
}

float AccountManager::getLevelProgress(int level, int xp) const {
    int req = getXPForNextLevel(level);
    if (req <= 0) return 0.0f;
    return std::clamp(static_cast<float>(xp) / req, 0.0f, 1.0f);
}

void AccountManager::addXP(int amount) {
    if (amount <= 0) return;
    UserAccount* acc = activeAccount();
    acc->xp += amount;

    int req = getXPForNextLevel(acc->level);
    while (acc->xp >= req) {
        acc->xp -= req;
        acc->level++;
        acc->coins += acc->level * 500; // Level up coin prize!
        req = getXPForNextLevel(acc->level);
    }
    save();
}

void AccountManager::addCoins(int amount) {
    if (amount <= 0) return;
    UserAccount* acc = activeAccount();
    acc->coins += amount;
    save();
}

bool AccountManager::spendCoins(int amount) {
    if (amount <= 0) return true;
    UserAccount* acc = activeAccount();
    if (acc->coins >= amount) {
        acc->coins -= amount;
        save();
        return true;
    }
    return false;
}

void AccountManager::unlockVehicle(const std::string& vehicleId) {
    activeAccount()->unlockedVehicles.insert(vehicleId);
    save();
}

bool AccountManager::isVehicleUnlocked(const std::string& vehicleId) const {
    const auto& s = activeAccount()->unlockedVehicles;
    return s.find(vehicleId) != s.end();
}

void AccountManager::unlockStage(const std::string& stageId) {
    activeAccount()->unlockedStages.insert(stageId);
    save();
}

bool AccountManager::isStageUnlocked(const std::string& stageId) const {
    const auto& s = activeAccount()->unlockedStages;
    return s.find(stageId) != s.end();
}

void AccountManager::recordDistance(const std::string& stageId, float dist) {
    UserAccount* acc = activeAccount();
    if (dist > acc->records[stageId]) {
        acc->records[stageId] = dist;
        save();
    }
}

float AccountManager::getRecordDistance(const std::string& stageId) const {
    const auto& recs = activeAccount()->records;
    auto it = recs.find(stageId);
    if (it != recs.end()) return it->second;
    return 0.0f;
}

} // namespace Core
