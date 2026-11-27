#include "GameStateManager.h"
#include "Sprite.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include "AccountManager.h"
#include "VehicleConfig.h"
#include "TerrainConfig.h"
#include <QApplication>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Core {

using namespace UI;

static const std::vector<std::string> s_gpCircuit = {
    "countryside", "desert", "arctic", "mountain", "moon", "volcano", "neon", "super_countryside", "seasons"
};

GameStateManager::GameStateManager()
    : m_currentState(StateType::MAIN_MENU)
    , m_width(UI::Theme::VIRTUAL_WIDTH)
    , m_height(UI::Theme::VIRTUAL_HEIGHT)
    , m_gameTime(0.0f)
    , m_transitionTimer(0.0f)
    , m_smokeTimer(0.0f)
    , m_smokeTimer2(0.0f)
    , m_selectedBiome("countryside")
    , m_camera(m_width, m_height)
    , m_camera2(m_width, 266)
    , m_fbP1(m_width, 266)
    , m_fbP2(m_width, 266)
    , m_btnOnlineRematch(0, 0, 220, 44, "REMATCH", Theme::GREEN_GAS)
    , m_btnOnlineLobby(0, 0, 220, 44, "RETURN TO LOBBY", 0xFF37474F)
{
    m_camera2.setCompactMode(true);
    setDimensions(m_width, m_height);
    setupUiCallbacks();
    setupNetworkCallbacks();

    // Sync volume configuration from profile
    m_audio.setMasterVolume(m_profile.masterVolume());
    m_audio.setMusicVolume(m_profile.musicVolume());
    m_audio.setSfxVolume(m_profile.sfxVolume());
}

void GameStateManager::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_camera.setScreenSize(width, height);
    m_camera2.setScreenSize(width, 266);
    m_fbP1.resize(width, 266);
    m_fbP2.resize(width, 266);

    m_mainMenu.setDimensions(width, height);
    m_garage.setDimensions(width, height);
    m_stageSelect.setDimensions(width, height);
    m_hud.setDimensions(width, height);
    m_pauseOverlay.setDimensions(width, height);
    m_btnOnlineRematch.setPosition(width / 2 - 232, height / 2 + 36);
    m_btnOnlineLobby.setPosition(width / 2 + 12, height / 2 + 36);
    m_gameOverScreen.setDimensions(width, height);
    m_accountScreen.setDimensions(width, height);
    m_achievementsScreen.setDimensions(width, height);
    m_lobbyScreen.setDimensions(width, height);
    m_challengeScreen.setDimensions(width, height);
    m_evaluatorOverlay.setDimensions(width, height);

    int miniW = 440;
    int miniH = 26;
    m_minimapRadar.setDimensions((width - miniW) / 2, 10, miniW, miniH);
}

void GameStateManager::setupUiCallbacks() {
    auto playClick = [this]() {
        m_audio.playButtonClick();
    };

    m_mainMenu.setButtonSound(playClick);
    m_garage.setButtonSound(playClick);
    m_stageSelect.setButtonSound(playClick);
    m_hud.setButtonSound(playClick);
    m_pauseOverlay.setButtonSound(playClick);
    m_btnOnlineRematch.setOnSound(playClick);
    m_btnOnlineLobby.setOnSound(playClick);
    m_gameOverScreen.setButtonSound(playClick);
    m_accountScreen.setButtonSound(playClick);
    m_achievementsScreen.setButtonSound(playClick);
    m_lobbyScreen.setButtonSound(playClick);
    m_challengeScreen.setButtonSound(playClick);

    // 1. Main Menu
    m_mainMenu.setOnStart([this]() {
        m_isLocalMultiplayerFlow = false;
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnMultiplayer([this]() {
        startMultiplayerRace();
    });
    m_mainMenu.setOnOnlineMultiplayer([this]() {
        m_isLocalMultiplayerFlow = false;
        changeState(StateType::NETWORK_LOBBY);
    });
    m_mainMenu.setOnGrandPrix([this]() {
        startGrandPrixChallenge();
    });
    m_mainMenu.setOnGarage([this]() {
        m_isLocalMultiplayerFlow = false;
        m_garage.setMultiplayerMode(false);
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_mainMenu.setOnStages([this]() {
        m_isLocalMultiplayerFlow = false;
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnAccount([this]() {
        changeState(StateType::ACCOUNT_PROFILE);
    });
    m_mainMenu.setOnAchievements([this]() {
        syncAchievementProfileStats();
        changeState(StateType::ACHIEVEMENTS);
    });
    m_mainMenu.setOnEvaluator([this]() {
        startEvaluatorMode();
    });
    m_mainMenu.setOnQuit([]() {
        QApplication::quit();
    });

    // 2. Account Screen
    m_accountScreen.setOnClose([this]() {
        changeState(StateType::MAIN_MENU);
    });
    m_accountScreen.setOnLoggedIn([this](const std::string& user) {
        (void)user;
        syncAchievementProfileStats();
        changeState(StateType::MAIN_MENU);
    });

    m_achievementsScreen.setOnClose([this]() {
        changeState(StateType::MAIN_MENU);
    });

    // 3. Lobby Screen
    m_lobbyScreen.setOnBack([this]() {
        m_server.stopServer();
        m_client.disconnectFromHost();
        changeState(StateType::MAIN_MENU);
    });

    m_lobbyScreen.setOnHostRoom([this]() {
        m_server.startServer(Network::DEFAULT_PORT);
        m_server.setHostPlayerInfo(AccountManager::instance().activeAccount()->username,
                                   m_lobbyScreen.getSelectedVehicle(),
                                   static_cast<int>(m_profile.selectedDriver()));
        m_server.setTrack(m_lobbyScreen.getSelectedBiome());
    });

    m_lobbyScreen.setOnTrackChanged([this](const std::string& trackId) {
        m_server.setTrack(trackId);
    });

    m_lobbyScreen.setOnVehicleChanged([this](int vehType) {
        if (m_lobbyScreen.getMode() == UI::LobbyMode::HOST_LOBBY) {
            m_server.setHostPlayerInfo(AccountManager::instance().activeAccount()->username,
                                       vehType,
                                       static_cast<int>(m_profile.selectedDriver()));
        } else if (m_lobbyScreen.getMode() == UI::LobbyMode::JOIN_LOBBY) {
            if (m_client.isConnected()) {
                m_client.sendReady(m_lobbyScreen.isLocalReady(), vehType);
            }
        }
    });

    m_lobbyScreen.setOnConnect([this](const std::string& hostIp, int vehType) {
        QString hostStr = QString::fromStdString(hostIp).trimmed();
        quint16 port = Network::DEFAULT_PORT;
        if (hostStr.contains(':')) {
            QStringList parts = hostStr.split(':');
            hostStr = parts[0].trimmed();
            if (parts.size() > 1) {
                bool ok = false;
                int p = parts[1].trimmed().toInt(&ok);
                if (ok && p > 0 && p <= 65535) {
                    port = static_cast<quint16>(p);
                }
            }
        }
        std::string myName = AccountManager::instance().activeAccount()->username;
        m_client.connectToHost(hostStr, port, myName, vehType, static_cast<int>(m_profile.selectedDriver()));
    });

    m_lobbyScreen.setOnDisconnect([this]() {
        m_client.disconnectFromHost();
    });

    m_lobbyScreen.setOnToggleReady([this](bool isReady, int vehType) {
        if (m_client.isConnected()) {
            m_client.sendReady(isReady, vehType);
        }
    });

    m_lobbyScreen.setOnStartRace([this](const std::string& biomeId, bool isGP) {
        m_selectedBiome = biomeId;
        m_isGrandPrix = isGP;
        if (m_lobbyScreen.getMode() == UI::LobbyMode::HOST_LOBBY) {
            m_server.setHostPlayerInfo(AccountManager::instance().activeAccount()->username,
                                       m_lobbyScreen.getSelectedVehicle(),
                                       static_cast<int>(m_profile.selectedDriver()));
            m_server.setTrack(biomeId);
            m_server.broadcastStartRace();
            startNetworkMultiplayer(true, biomeId);
        }
    });

    // 4. Challenge Screen (Grand Prix)
    m_challengeScreen.setOnNextRound([this]() {
        advanceGrandPrixRound();
    });
    m_challengeScreen.setOnExitToMenu([this]() {
        m_isGrandPrix = false;
        changeState(StateType::MAIN_MENU);
    });

    // 5. Stage Select
    m_stageSelect.setOnBack([this]() {
        m_isLocalMultiplayerFlow = false;
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::MAIN_MENU);
    });
    m_stageSelect.setOnSelectStage([this](const std::string& stageId) {
        if (m_isLocalMultiplayerFlow) {
            startLocalMultiplayerRace(stageId);
        } else {
            startRace(stageId);
        }
    });

    // 6. Garage Screen
    m_garage.setOnBack([this]() {
        m_isLocalMultiplayerFlow = false;
        m_garage.setMultiplayerMode(false);
        changeState(StateType::MAIN_MENU);
    });
    m_garage.setOnDrive([this]() {
        m_stageSelect.setMultiplayerMode(m_isLocalMultiplayerFlow);
        changeState(StateType::STAGE_SELECT);
    });
    m_garage.setOnUpgradePurchased([this]() {
        syncAchievementProfileStats();
    });
    m_garage.setOnVehicleUnlocked([this]() {
        syncAchievementProfileStats();
    });

    // 7. HUD
    m_hud.setOnPauseClicked([this]() {
        if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::ONLINE_RACE) {
            changeState(StateType::PAUSED);
        }
    });

    // 8. Pause Overlay
    m_pauseOverlay.setOnResume([this]() {
        if (m_isNetworkHost || m_isNetworkClient) {
            changeState(StateType::ONLINE_RACE);
        } else {
            changeState(StateType::GAMEPLAY);
        }
    });
    m_pauseOverlay.setOnRestart([this]() {
        if (m_isNetworkHost || m_isNetworkClient) {
            startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
        } else if (m_isLocalMultiplayerFlow) {
            startLocalMultiplayerRace(m_selectedBiome);
        } else {
            startRace(m_selectedBiome);
        }
    });
    m_pauseOverlay.setOnGarage([this]() {
        m_garage.setMultiplayerMode(m_isLocalMultiplayerFlow);
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_pauseOverlay.setOnMenu([this]() {
        m_isLocalMultiplayerFlow = false;
        m_garage.setMultiplayerMode(false);
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::MAIN_MENU);
    });

    // 9. Game Over Screen
    m_gameOverScreen.setOnRetry([this]() {
        if (m_isNetworkHost || m_isNetworkClient) {
            startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
        } else if (m_isLocalMultiplayerFlow) {
            startLocalMultiplayerRace(m_selectedBiome);
        } else {
            startRace(m_selectedBiome);
        }
    });
    m_gameOverScreen.setOnGarage([this]() {
        m_garage.setMultiplayerMode(m_isLocalMultiplayerFlow);
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_gameOverScreen.setOnMenu([this]() {
        m_isLocalMultiplayerFlow = false;
        m_garage.setMultiplayerMode(false);
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::MAIN_MENU);
    });

    m_btnOnlineRematch.setOnClick([this]() {
        if (m_isNetworkHost) {
            if (m_isGrandPrix) {
                m_gpRound = 1;
                m_gpStandings.clear();
                std::string myName = AccountManager::instance().activeAccount()->username;
                m_gpStandings.push_back({myName, Theme::GOLD, 0, 0, 0.0f});
                m_gpStandings.push_back({"PPO-Apex [AI]", 0xFF00E5FF, 0, 0, 0.0f});
                m_gpStandings.push_back({"TD3-Turbo [AI]", 0xFFFF3D00, 0, 0, 0.0f});
                m_gpStandings.push_back({"TRPO-Speed [AI]", 0xFF76FF03, 0, 0, 0.0f});
            }
            m_server.broadcastRematch();
            startNetworkMultiplayer(true, m_selectedBiome);
        } else if (m_isNetworkClient) {
            m_client.sendReturnToLobby();
            changeState(StateType::NETWORK_LOBBY);
        }
    });
    m_btnOnlineLobby.setOnClick([this]() {
        if (m_isNetworkHost) {
            m_server.broadcastReturnToLobby();
            m_server.stopServer();
        } else if (m_isNetworkClient) {
            m_client.sendReturnToLobby();
        }
        m_networkOpponents.clear();
        m_isNetworkHost = false;
        m_isNetworkClient = false;
        m_onlineResultShown = false;
        changeState(StateType::NETWORK_LOBBY);
    });

    // 10. Physics World Events
    m_physics.setOnItemCollected([this](Physics::ItemType type, int value, int itemIndex) {
        Physics::Vec2 carScr = m_camera.worldToScreen(m_physics.vehicle().chassisPos());
        if (type == Physics::ItemType::COIN_BRONZE || 
            type == Physics::ItemType::COIN_SILVER || 
            type == Physics::ItemType::COIN_GOLD) {
            m_profile.addCoins(value);
            AccountManager::instance().addCoins(value);
            m_achievements.addTo(AchievementStat::TOTAL_COINS, value);
            m_audio.playCoinPickup();
            uint32_t col = (type == Physics::ItemType::COIN_GOLD) ? UI::Theme::GOLD :
                           (type == Physics::ItemType::COIN_SILVER) ? 0xFFCFD8DC : 0xFFCD7F32;
            m_hud.addFloatingText("+" + std::to_string(value), carScr.x, carScr.y - 30.0f, col);
        } else if (type == Physics::ItemType::FUEL_CANISTER) {
            m_audio.playFuelPickup();
            m_hud.triggerFuelFlash();
            m_hud.addFloatingText("+FUEL", carScr.x, carScr.y - 35.0f, UI::Theme::GREEN_GAS);
        }

        if (m_isNetworkHost) {
            m_server.sendItemCollected(itemIndex, 0);
        } else if (m_isNetworkClient) {
            m_client.sendItemCollected(itemIndex);
        }
    });

    m_physics.setOnLocalItemCollected([this](Physics::ItemType type, int value, bool isP1) {
        const auto& veh = isP1 ? m_physics.vehicle() : m_physics.vehicle2();
        const auto& cam = isP1 ? m_camera : m_camera2;
        if (isP1) {
            m_profile.addCoins(value);
            AccountManager::instance().addCoins(value);
            m_achievements.addTo(AchievementStat::TOTAL_COINS, value);
        }
        Physics::Vec2 scr = cam.worldToScreen(veh.chassisPos());
        if (type == Physics::ItemType::FUEL_CANISTER) {
            m_audio.playFuelPickup();
        } else {
            m_audio.playCoinPickup();
        }
        std::string label = (isP1 ? "P1 +" : "P2 +") +
                            (type == Physics::ItemType::FUEL_CANISTER ? std::string("FUEL")
                                                                      : std::to_string(value));
        uint32_t col = type == Physics::ItemType::FUEL_CANISTER ? UI::Theme::GREEN_GAS : UI::Theme::GOLD;
        if (isP1) {
            m_hud.addFloatingText(label, scr.x, scr.y - 30.0f, col);
        }
    });

    m_physics.setOnStunt([this](const std::string& name, int rewardCoins) {
        m_hud.triggerStunt(name, rewardCoins);
        m_profile.addCoins(rewardCoins);
        AccountManager::instance().addCoins(rewardCoins);
        AccountManager::instance().addXP(100);
        m_audio.playStuntCheer();
    });

    m_physics.setOnCheckpoint([this](const Physics::CheckpointInfo& cp) {
        m_hud.triggerStunt(cp.name, cp.coinReward);
        m_achievements.addTo(AchievementStat::CHECKPOINTS_CLEARED, 1);
        m_profile.recordStageCheckpoint(m_selectedBiome, cp.index);
        m_profile.addCoins(cp.coinReward);
        AccountManager::instance().addCoins(cp.coinReward);
        AccountManager::instance().addXP(250);
        m_audio.playStuntCheer();
        if (cp.isFinishLine) {
            m_profile.setStageCompleted(m_selectedBiome, true);
            m_achievements.addTo(AchievementStat::STAGES_COMPLETED, 1);
            if (m_selectedBiome == "moon") {
                m_achievements.addTo(AchievementStat::MOON_COMPLETIONS, 1);
            }
        }
    });

    m_achievements.setOnUnlock([this](const AchievementDefinition& def) {
        pushAchievementToast(def);
        m_gameOverScreen.addUnlockedAchievement(def.title);
    });

    m_devOverlay.setOnClose([this]() {
        m_showDevOverlay = false;
    });

    syncAchievementProfileStats();
}

void GameStateManager::syncAchievementProfileStats() {
    int vehicles = 0;
    for (int i = 0; i < static_cast<int>(Physics::VehicleType::COUNT); ++i) {
        if (m_profile.isVehicleUnlocked(static_cast<Physics::VehicleType>(i))) ++vehicles;
    }

    int upgrades = 0;
    for (int v = 0; v < static_cast<int>(Physics::VehicleType::COUNT); ++v) {
        for (int u = 0; u < ProfileManager::UPGRADE_COUNT; ++u) {
            upgrades += m_profile.getUpgradeLevel(static_cast<Physics::VehicleType>(v),
                                                  static_cast<ProfileManager::UpgradeType>(u));
        }
    }

    m_achievements.syncProfileCounts(vehicles, upgrades,
                                     AccountManager::instance().activeAccount()->grandPrixTrophies);
}

void GameStateManager::pushAchievementToast(const AchievementDefinition& def) {
    m_achievementToastTitle = def.title;
    m_achievementToastTimer = 3.2f;
    m_audio.playStuntCheer();
}

void GameStateManager::setupNetworkCallbacks() {
    auto updateOpponentTelemetry = [this](const Network::RemotePlayerState& st) {
        auto it = m_networkOpponents.find(st.id);
        if (it == m_networkOpponents.end()) {
            Network::RemotePlayerState copy = st;
            copy.posX = st.posX;
            copy.posY = st.posY;
            copy.chassisAngle = st.chassisAngle;
            copy.targetX = st.posX;
            copy.targetY = st.posY;
            copy.targetAngle = st.chassisAngle;
            m_networkOpponents[st.id] = copy;
        } else {
            it->second.name = st.name;
            it->second.color = st.color;
            it->second.vehicleType = st.vehicleType;
            it->second.driverType = st.driverType;
            it->second.targetX = st.posX;
            it->second.targetY = st.posY;
            it->second.velX = st.velX;
            it->second.velY = st.velY;
            it->second.targetAngle = st.chassisAngle;
            it->second.chassisAngularVel = st.chassisAngularVel;
            it->second.rearWheelAngle = st.rearWheelAngle;
            it->second.frontWheelAngle = st.frontWheelAngle;
            it->second.rearSuspLen = st.rearSuspLen;
            it->second.frontSuspLen = st.frontSuspLen;
            it->second.driverHeadAngle = st.driverHeadAngle;
            it->second.speedKmh = st.speedKmh;
            it->second.fuel = st.fuel;
            it->second.distance = st.distance;
            it->second.isAirborne = st.isAirborne;
            it->second.isCrashed = st.isCrashed;
            it->second.isFinished = st.isFinished;
            it->second.finishTime = st.finishTime;

            float dx = it->second.targetX - it->second.posX;
            float dy = it->second.targetY - it->second.posY;
            if (dx * dx + dy * dy > 225.0f) {
                it->second.posX = it->second.targetX;
                it->second.posY = it->second.targetY;
                it->second.chassisAngle = it->second.targetAngle;
            }
        }
    };

    // --- SERVER CALLBACKS ---
    m_server.setOnTelemetryReceived([this, updateOpponentTelemetry](const Network::RemotePlayerState& st) {
        updateOpponentTelemetry(st);
        checkNetworkRaceFinish();
    });

    m_server.setOnPlayerDisconnected([this](int id) {
        m_networkOpponents.erase(id);
    });

    m_server.setOnItemCollected([this](int itemIdx, int pId) {
        (void)pId;
        auto& items = m_physics.terrain().getItems();
        if (itemIdx >= 0 && itemIdx < static_cast<int>(items.size())) {
            items[itemIdx].collected = true;
        }
    });

    m_server.setOnPlayerCrashed([this](int pId, float dist) {
        auto it = m_networkOpponents.find(pId);
        if (it != m_networkOpponents.end()) {
            it->second.isCrashed = true;
            it->second.distance = std::max(it->second.distance, dist);
        }
        checkNetworkRaceFinish();
    });

    m_server.setOnPlayerFinished([this](int pId, float dist, float time) {
        auto it = m_networkOpponents.find(pId);
        if (it != m_networkOpponents.end()) {
            it->second.isFinished = true;
            it->second.finishTime = time;
            it->second.distance = std::max(it->second.distance, dist);
        }
        checkNetworkRaceFinish();
    });

    m_server.setOnReturnToLobby([this]() {
        m_networkOpponents.clear();
        m_onlineResultShown = false;
        changeState(StateType::NETWORK_LOBBY);
    });

    // --- CLIENT CALLBACKS ---
    m_client.setOnConnected([this]() {
        m_lobbyScreen.setStatusMessage("Connected! Waiting for host to launch...");
    });

    m_client.setOnRejected([this](const std::string& reason, const std::vector<std::string>& suggestions) {
        (void)suggestions;
        m_lobbyScreen.setStatusMessage("Connection failed: " + reason);
        m_lobbyScreen.setLocalReady(false);
    });

    m_client.setOnDisconnected([this]() {
        m_lobbyScreen.setStatusMessage("Disconnected from host.");
        m_lobbyScreen.setLocalReady(false);
    });

    m_client.setOnLobbyUpdated([this](const std::string& biomeId, const std::vector<Network::PlayerLobbyInfo>& players) {
        m_lobbyScreen.setLobbyTrack(biomeId);
        m_lobbyScreen.setLobbyPlayers(players);
    });

    m_client.setOnRaceStart([this](const std::string& biomeId) {
        startNetworkMultiplayer(false, biomeId);
    });

    m_client.setOnRemoteTelemetry([this, updateOpponentTelemetry](const Network::RemotePlayerState& st) {
        updateOpponentTelemetry(st);
        checkNetworkRaceFinish();
    });

    m_client.setOnItemCollected([this](int itemIdx, int pId) {
        (void)pId;
        auto& items = m_physics.terrain().getItems();
        if (itemIdx >= 0 && itemIdx < static_cast<int>(items.size())) {
            items[itemIdx].collected = true;
        }
    });

    m_client.setOnGrandPrixNext([this](const std::string& nextBiome, int round) {
        m_gpRound = round;
        startNetworkMultiplayer(false, nextBiome);
    });

    m_client.setOnRematch([this](const std::string& biomeId) {
        m_isGrandPrix = false;
        startNetworkMultiplayer(false, biomeId);
    });

    m_client.setOnReturnToLobby([this]() {
        m_networkOpponents.clear();
        m_onlineResultShown = false;
        m_isNetworkClient = false;
        changeState(StateType::NETWORK_LOBBY);
    });

    m_client.setOnRemotePlayerCrashed([this](int pId, float dist) {
        auto it = m_networkOpponents.find(pId);
        if (it != m_networkOpponents.end()) {
            it->second.isCrashed = true;
            it->second.distance = std::max(it->second.distance, dist);
        }
        checkNetworkRaceFinish();
    });

    m_client.setOnRemotePlayerFinished([this](int pId, float dist, float time) {
        auto it = m_networkOpponents.find(pId);
        if (it != m_networkOpponents.end()) {
            it->second.isFinished = true;
            it->second.finishTime = time;
            it->second.distance = std::max(it->second.distance, dist);
        }
        checkNetworkRaceFinish();
    });
}

void GameStateManager::changeState(StateType newState) {
    m_currentState = newState;
    m_transitionTimer = 0.12f;

    if (m_currentState == StateType::MAIN_MENU || m_currentState == StateType::GARAGE ||
        m_currentState == StateType::STAGE_SELECT || m_currentState == StateType::ACCOUNT_PROFILE ||
        m_currentState == StateType::ACHIEVEMENTS || m_currentState == StateType::NETWORK_LOBBY ||
        m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        m_audio.setTargetMusic(Audio::MusicTrack::MENU);
    } else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE ||
               m_currentState == StateType::ONLINE_RACE) {
        m_audio.setTargetMusic(Audio::MusicTrack::RACE);
    } else if (m_currentState == StateType::GAME_OVER) {
        m_audio.stopAllMusic();
    }
}

void GameStateManager::startRace(const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_isNetworkHost = false;
    m_isNetworkClient = false;
    m_isLocalMultiplayerFlow = false;
    m_networkOpponents.clear();
    m_onlineResultShown = false;
    m_garage.setMultiplayerMode(false);
    m_stageSelect.setMultiplayerMode(false);

    m_camera.setCompactMode(false);
    m_camera.setScreenSize(m_width, m_height);
    m_physics.reset(biomeId, m_profile.selectedVehicle(), m_profile.selectedDriver(), false);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_gameOverScreen.clearUnlockedAchievements();
    m_minimapRadar.setCourseLength(1750.0f);
    syncAchievementProfileStats();

    // Initialize dynamic in-run bounties & personal best ghost tracker
    initRunBounties();
    m_inCrashSequence = false;
    m_crashSequenceTimer = 0.0f;
    m_hud.setNitro(100.0f);
    m_hud.setBoosting(false);
    m_hud.setBoostVirtualPressed(false);
    m_hud.setBountyInfo("", "", 0);
    m_hud.setSurfaceInfo("", 0xFFFFFFFF);
    m_hud.setRolloverWarning(false);
    if (m_selectedBiome != m_ghostBiome) {
        m_bestRunGhost.clear();
        m_ghostRecordDist = 0.0f;
        m_ghostBiome = m_selectedBiome;
    }
    m_currentRunGhost.clear();
    m_ghostSampleTimer = 0.0f;

    changeState(StateType::GAMEPLAY);
}

void GameStateManager::startMultiplayerRace() {
    m_isLocalMultiplayerFlow = true;
    m_isNetworkHost = false;
    m_isNetworkClient = false;
    m_networkOpponents.clear();
    m_garage.setMultiplayerMode(true);
    m_garage.setViewedVehicle(m_profile.selectedVehicle());
    m_stageSelect.setMultiplayerMode(true);
    changeState(StateType::GARAGE);
}

void GameStateManager::startLocalMultiplayerRace(const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_isLocalMultiplayerFlow = true;
    m_isNetworkHost = false;
    m_isNetworkClient = false;
    m_networkOpponents.clear();

    m_camera.setCompactMode(true);
    m_camera.setScreenSize(m_width, 266);
    m_camera2.setCompactMode(true);
    m_camera2.setScreenSize(m_width, 266);
    m_physics.reset(biomeId, m_profile.selectedVehicle(), m_profile.selectedDriver(), true,
                    m_garage.p2Vehicle(), m_garage.p2Driver());
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_camera2.reset(m_physics.vehicle2().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_smokeTimer = 0.0f;
    m_smokeTimer2 = 0.0f;
    m_gameOverScreen.clearUnlockedAchievements();
    m_minimapRadar.setCourseLength(1750.0f);
    syncAchievementProfileStats();
    m_inCrashSequence = false;
    m_crashSequenceTimer = 0.0f;
    m_hud.setNitro(100.0f);
    m_hud.setBoosting(false);
    m_hud.setBoostVirtualPressed(false);
    m_hud.setBountyInfo("", "", 0);
    m_hud.setSurfaceInfo("", 0xFFFFFFFF);
    m_hud.setRolloverWarning(false);
    m_currentRunGhost.clear();
    m_ghostSampleTimer = 0.0f;
    changeState(StateType::MULTIPLAYER_RACE);
}

void GameStateManager::startNetworkMultiplayer(bool isHost, const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_isNetworkHost = isHost;
    m_isNetworkClient = !isHost;
    m_isLocalMultiplayerFlow = false;
    m_networkOpponents.clear();
    m_onlineResultShown = false;
    m_localOutcomeBroadcast = false;
    m_localFinishTime = 0.0f;
    m_garage.setMultiplayerMode(false);
    m_stageSelect.setMultiplayerMode(false);

    int vehIdx = m_lobbyScreen.getSelectedVehicle();
    if (vehIdx >= 0 && vehIdx < static_cast<int>(Physics::VehicleType::COUNT)) {
        m_profile.setSelectedVehicle(static_cast<Physics::VehicleType>(vehIdx));
    }

    float startX = 5.0f;
    if (!isHost) {
        int myId = m_client.getLocalPlayerId();
        if (myId > 0) {
            startX = 5.0f - myId * 3.5f;
        }
    }

    m_camera.setCompactMode(false);
    m_camera.setScreenSize(m_width, m_height);
    m_physics.reset(biomeId, m_profile.selectedVehicle(), m_profile.selectedDriver(), false,
                    Physics::VehicleType::PICKUP_TRUCK, Physics::DriverType::SARAH, startX);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_smokeTimer = 0.0f;
    m_netTelemetryTimer = 0.0f;
    m_gameOverScreen.clearUnlockedAchievements();
    m_minimapRadar.setCourseLength(1750.0f);
    syncAchievementProfileStats();
    m_inCrashSequence = false;
    m_crashSequenceTimer = 0.0f;
    m_hud.setNitro(100.0f);
    m_hud.setBoosting(false);
    m_hud.setBoostVirtualPressed(false);
    m_hud.setBountyInfo("", "", 0);
    m_hud.setSurfaceInfo("", 0xFFFFFFFF);
    m_hud.setRolloverWarning(false);
    m_currentRunGhost.clear();
    m_ghostSampleTimer = 0.0f;
    changeState(StateType::ONLINE_RACE);
}

void GameStateManager::startGrandPrixChallenge() {
    m_isGrandPrix = true;
    m_gpRound = 1;

    m_gpStandings.clear();
    std::string myName = AccountManager::instance().activeAccount()->username;
    m_gpStandings.push_back({myName, Theme::GOLD, 0, 0, 0.0f});
    m_gpStandings.push_back({"PPO-Apex [AI]", 0xFF00E5FF, 0, 0, 0.0f});
    m_gpStandings.push_back({"TD3-Turbo [AI]", 0xFFFF3D00, 0, 0, 0.0f});
    m_gpStandings.push_back({"TRPO-Speed [AI]", 0xFF76FF03, 0, 0, 0.0f});

    startRace(s_gpCircuit[0]);
}

void GameStateManager::advanceGrandPrixRound() {
    m_gpRound++;
    if (m_gpRound <= static_cast<int>(s_gpCircuit.size())) {
        std::string nextBiome = s_gpCircuit[m_gpRound - 1];
        if (m_isNetworkHost) {
            m_server.broadcastNextRound(nextBiome, m_gpRound);
            startNetworkMultiplayer(true, nextBiome);
        } else {
            startRace(nextBiome);
        }
    } else {
        // Championship Complete!
        AccountManager::instance().activeAccount()->grandPrixTrophies++;
        AccountManager::instance().addCoins(15000);
        AccountManager::instance().addXP(1000);
        AccountManager::instance().save();
        syncAchievementProfileStats();
        changeState(StateType::MAIN_MENU);
    }
}

void GameStateManager::startEvaluatorMode() {
    m_isEvaluatorMode = true;
    m_profile.setEvaluatorMode(true);
    startRace("countryside");
}

void GameStateManager::onKeyTyped(int key, const std::string& text) {
    if (key == Qt::Key_F1) {
        m_showDevOverlay = !m_showDevOverlay;
        return;
    }
    if (key == Qt::Key_F2) {
        static int s_trkIdx = 0;
        Audio::MusicTrack trks[] = {
            Audio::MusicTrack::POKEMON_CHIPTUNE,
            Audio::MusicTrack::SOOTHING_PASTORAL,
            Audio::MusicTrack::EDGY_SYNTHWAVE,
            Audio::MusicTrack::ENERGETIC_ARCADE
        };
        s_trkIdx = (s_trkIdx + 1) % 4;
        m_audio.setTargetMusic(trks[s_trkIdx]);
        return;
    }
    if (key == Qt::Key_B) {
        Core::LocalizationManager::instance().toggleLanguage();
        return;
    }

    if (m_currentState == StateType::ACCOUNT_PROFILE) {
        m_accountScreen.onKeyPress(key, text);
        return;
    }
    if (m_currentState == StateType::NETWORK_LOBBY) {
        m_lobbyScreen.onKeyPress(key, text);
        return;
    }

    // Evaluator hotkeys
    if ((m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE) && m_isEvaluatorMode) {
        if (key == Qt::Key_F) {
            m_evaluatorOverlay.toggleInfiniteFuel();
        } else if (key == Qt::Key_Z) {
            m_evaluatorOverlay.toggleSlowMotion();
        } else if (key == Qt::Key_V) {
            int nextVeh = (static_cast<int>(m_profile.selectedVehicle()) + 1) % static_cast<int>(Physics::VehicleType::COUNT);
            m_profile.setSelectedVehicle(static_cast<Physics::VehicleType>(nextVeh));
            m_physics.vehicle().setVehicleType(static_cast<Physics::VehicleType>(nextVeh));
        } else if (key >= Qt::Key_1 && key <= Qt::Key_9) {
            int stageIdx = key - Qt::Key_1;
            if (stageIdx < static_cast<int>(s_gpCircuit.size())) {
                startRace(s_gpCircuit[stageIdx]);
            }
        }
    }
}

void GameStateManager::handleInput(const InputController::State& input) {
    int mx = input.mouseX;
    int my = input.mouseY;

    if (m_showDevOverlay) {
        if (input.mouseMoved) m_devOverlay.onMouseMove(mx, my);
        if (input.mouseJustPressed) {
            m_devOverlay.onMouseDown(mx, my, m_physics, m_audio);
        }
        if (input.mouseJustReleased) m_devOverlay.onMouseUp(mx, my);
        return;
    }

    if (m_currentState == StateType::MAIN_MENU) {
        if (input.multiplayer) {
            startMultiplayerRace();
            return;
        }
        if (input.onlineMultiplayer) {
            m_isLocalMultiplayerFlow = false;
            changeState(StateType::NETWORK_LOBBY);
            return;
        }
        if (input.enter) {
            startRace(m_selectedBiome);
            return;
        }
        if (input.mouseMoved) m_mainMenu.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_mainMenu.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_mainMenu.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::ACCOUNT_PROFILE) {
        if (input.mouseMoved) m_accountScreen.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_accountScreen.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_accountScreen.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::ACHIEVEMENTS) {
        if (input.pause || input.enter) {
            changeState(StateType::MAIN_MENU);
            return;
        }
        if (input.mouseMoved) m_achievementsScreen.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_achievementsScreen.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_achievementsScreen.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::NETWORK_LOBBY) {
        if (input.mouseMoved) m_lobbyScreen.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_lobbyScreen.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_lobbyScreen.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        if (input.mouseMoved) m_challengeScreen.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_challengeScreen.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_challengeScreen.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::STAGE_SELECT) {
        if (input.mouseMoved) m_stageSelect.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_stageSelect.onMouseDown(mx, my, m_profile);
        if (input.mouseJustReleased) m_stageSelect.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::GARAGE) {
        if (input.mouseMoved) m_garage.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_garage.onMouseDown(mx, my, m_profile);
        if (input.mouseJustReleased) m_garage.onMouseUp(mx, my, m_profile);
    }
    else if (m_currentState == StateType::GAMEPLAY) {
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }
        if (input.restart) {
            startRace(m_selectedBiome);
            return;
        }

        if (input.mouseMoved) m_hud.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_hud.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_hud.onMouseUp(mx, my);

        bool gasActive = input.gas || (input.mouseDown && m_hud.isGasPressed());
        bool brakeActive = input.brake || (input.mouseDown && m_hud.isBrakePressed());
        bool reverseActive = input.reverse || (input.mouseDown && m_hud.isReversePressed());
        bool boostActive = input.p1.boost || (input.mouseDown && m_hud.isBoostPressed());

        m_hud.setGasVirtualPressed(gasActive);
        m_hud.setBrakeVirtualPressed(brakeActive);
        m_hud.setReverseVirtualPressed(reverseActive);
        m_hud.setBoostVirtualPressed(boostActive);

        m_physics.vehicle().applyInput(gasActive ? 1.0f : 0.0f,
                                       brakeActive ? 1.0f : 0.0f,
                                       reverseActive ? 1.0f : 0.0f,
                                       input.p1.tiltLeft ? 1.0f : 0.0f,
                                       input.p1.tiltRight ? 1.0f : 0.0f,
                                       boostActive ? 1.0f : 0.0f);
    }
    else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }
        if (input.restart) {
            startLocalMultiplayerRace(m_selectedBiome);
            return;
        }

        const auto& p1 = input.p1;
        const auto& p2 = input.p2;
        m_physics.vehicle().applyInput(p1.gas ? 1.0f : 0.0f, p1.brake ? 1.0f : 0.0f,
                                       p1.reverse ? 1.0f : 0.0f, p1.tiltLeft ? 1.0f : 0.0f,
                                       p1.tiltRight ? 1.0f : 0.0f, p1.boost ? 1.0f : 0.0f);
        m_physics.vehicle2().applyInput(p2.gas ? 1.0f : 0.0f, p2.brake ? 1.0f : 0.0f,
                                        p2.reverse ? 1.0f : 0.0f, p2.tiltLeft ? 1.0f : 0.0f,
                                        p2.tiltRight ? 1.0f : 0.0f, p2.boost ? 1.0f : 0.0f);
    }
    else if (m_currentState == StateType::ONLINE_RACE) {
        if (m_onlineResultShown) {
            if (input.mouseMoved) {
                m_btnOnlineRematch.onMouseMove(mx, my);
                m_btnOnlineLobby.onMouseMove(mx, my);
            }
            if (input.mouseJustPressed) {
                m_btnOnlineRematch.onMouseDown(mx, my);
                m_btnOnlineLobby.onMouseDown(mx, my);
            }
            if (input.mouseJustReleased) {
                m_btnOnlineRematch.onMouseUp(mx, my);
                m_btnOnlineLobby.onMouseUp(mx, my);
            }
            if (input.enter) m_btnOnlineRematch.triggerClick();
            if (input.pause) m_btnOnlineLobby.triggerClick();
            return;
        }
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }
        if (input.restart) {
            startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
            return;
        }

        if (input.mouseMoved) m_hud.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_hud.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_hud.onMouseUp(mx, my);

        bool gasActive = input.p1.gas || (input.mouseDown && m_hud.isGasPressed());
        bool brakeActive = input.p1.brake || (input.mouseDown && m_hud.isBrakePressed());
        bool reverseActive = input.p1.reverse || (input.mouseDown && m_hud.isReversePressed());
        bool boostActive = input.p1.boost || (input.mouseDown && m_hud.isBoostPressed());

        m_hud.setGasVirtualPressed(gasActive);
        m_hud.setBrakeVirtualPressed(brakeActive);
        m_hud.setReverseVirtualPressed(reverseActive);
        m_hud.setBoostVirtualPressed(boostActive);

        m_physics.vehicle().applyInput(gasActive ? 1.0f : 0.0f,
                                       brakeActive ? 1.0f : 0.0f,
                                       reverseActive ? 1.0f : 0.0f,
                                       input.p1.tiltLeft ? 1.0f : 0.0f,
                                       input.p1.tiltRight ? 1.0f : 0.0f,
                                       boostActive ? 1.0f : 0.0f);
    }
    else if (m_currentState == StateType::PAUSED) {
        if (input.pause || input.enter) {
            if (m_isNetworkHost || m_isNetworkClient) {
                changeState(StateType::ONLINE_RACE);
            } else if (m_isLocalMultiplayerFlow) {
                changeState(StateType::MULTIPLAYER_RACE);
            } else {
                changeState(StateType::GAMEPLAY);
            }
            return;
        }
        if (input.mouseMoved) m_pauseOverlay.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_pauseOverlay.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_pauseOverlay.onMouseUp(mx, my);
    }
    else if (m_currentState == StateType::GAME_OVER) {
        if (input.restart || input.enter) {
            if (m_isNetworkHost || m_isNetworkClient) {
                startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
            } else if (m_isLocalMultiplayerFlow) {
                startLocalMultiplayerRace(m_selectedBiome);
            } else {
                startRace(m_selectedBiome);
            }
            return;
        }
        if (input.mouseMoved) m_gameOverScreen.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_gameOverScreen.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_gameOverScreen.onMouseUp(mx, my);
    }
}

void GameStateManager::fixedUpdate(float dt) {
    if (m_transitionTimer > 0.0f) {
        m_transitionTimer = std::max(0.0f, m_transitionTimer - dt);
    }
    if (m_achievementToastTimer > 0.0f) {
        m_achievementToastTimer = std::max(0.0f, m_achievementToastTimer - dt);
    }

    bool isPaused = (m_currentState == StateType::PAUSED);
    if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE ||
        m_currentState == StateType::ONLINE_RACE || m_currentState == StateType::PAUSED ||
        m_currentState == StateType::GAME_OVER) {
        m_terrainRasterizer.dayNightCycle().update(dt, isPaused);
    }

    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.update(dt);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.update(dt);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.update(dt);
    } else if (m_currentState == StateType::ACCOUNT_PROFILE) {
        m_accountScreen.update(dt);
    } else if (m_currentState == StateType::ACHIEVEMENTS) {
        m_achievementsScreen.update(dt);
    } else if (m_currentState == StateType::NETWORK_LOBBY) {
        m_lobbyScreen.update(dt);
    } else if (m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        m_challengeScreen.update(dt);
    } else if (m_currentState == StateType::GAMEPLAY) {
        float effectiveDt = dt;
        if (m_isEvaluatorMode && m_evaluatorOverlay.isSlowMotion()) {
            effectiveDt *= 0.5f;
        }

        // True 4x Slow Motion during dramatic crash tumble
        if (m_inCrashSequence) {
            effectiveDt *= 0.25f;
        }

        m_gameTime += effectiveDt;

        if (m_isEvaluatorMode && m_evaluatorOverlay.isInfiniteFuel()) {
            m_physics.vehicle().addFuel(100.0f);
        }

        m_physics.step(effectiveDt, m_profile);

        const auto& car = m_physics.vehicle();
        m_camera.update(effectiveDt, car.chassisPos(), car.chassisVel().x);

        // Exhaust smoke / Rocket nitro boost flames
        m_smokeTimer -= effectiveDt;
        if (car.fuel() > 0.0f && m_smokeTimer <= 0.0f) {
            float cosA = std::cos(car.chassisAngle());
            float sinA = std::sin(car.chassisAngle());
            Physics::Vec2 pLocal(car.rearMountOffset().x * 0.95f, car.rearMountOffset().y * 0.4f);
            Physics::Vec2 pWorld = car.chassisPos() + Physics::Vec2(pLocal.x * cosA - pLocal.y * sinA, pLocal.x * sinA + pLocal.y * cosA);

            if (car.isBoosting()) {
                m_smokeTimer = 0.02f;
                m_particleSystem.emitNitroFlame(pWorld, Physics::Vec2(cosA, sinA), car.chassisVel());
                m_camera.addTrauma(0.04f);
            } else {
                m_smokeTimer = (car.getSpeedKmh() > 5.0f) ? 0.05f : 0.10f;
                m_particleSystem.emitSmoke(pWorld, car.chassisVel());
            }
        }

        // Wheel spray
        const std::string& currentBiome = m_physics.terrain().getBiome();
        if (car.isRearOnGround()) {
            m_particleSystem.emitWheelSpray(car.rearWheelPos(), car.rearWheelRadius(),
                                            car.rearContactTangent(), car.rearSlipSpeed(),
                                            car.rearWheelAngularVel(), car.rearNormalForce(), currentBiome);
        }
        if (car.isFrontOnGround()) {
            m_particleSystem.emitWheelSpray(car.frontWheelPos(), car.frontWheelRadius(),
                                            car.frontContactTangent(), car.frontSlipSpeed(),
                                            car.frontWheelAngularVel(), car.frontNormalForce(), currentBiome);
        }

        m_particleSystem.update(effectiveDt);

        // Update Surface info and Rollover Hazard
        const auto& surf = m_physics.terrain().getSurfaceAt(car.chassisPos().x);
        const auto& mat = Physics::SurfaceMaterialRegistry::get(surf.type);
        m_hud.setSurfaceInfo(Physics::SurfaceMaterialRegistry::name(surf.type), mat.topColor);
        float cosPitch = std::cos(car.chassisAngle());
        bool rolloverRisk = (cosPitch < 0.25f && car.chassisVel().length() > 3.0f && !m_physics.isGameOver());
        m_hud.setRolloverWarning(rolloverRisk);

        // Update Bounties
        updateRunBounties(effectiveDt);

        // Sample Ghost Frame every 0.08s
        m_ghostSampleTimer += effectiveDt;
        if (m_ghostSampleTimer >= 0.08f) {
            m_ghostSampleTimer = 0.0f;
            GhostFrame gf;
            gf.x = car.chassisPos().x;
            gf.y = car.chassisPos().y;
            gf.angle = car.chassisAngle();
            gf.speedKmh = car.getSpeedKmh();
            gf.rearSuspLen = car.rearSuspensionCompression();
            gf.frontSuspLen = car.frontSuspensionCompression();
            gf.rearWheelAngle = car.rearWheelAngle();
            gf.frontWheelAngle = car.frontWheelAngle();
            m_currentRunGhost.push_back(gf);
        }

        // Update HUD Telemetry (Speedometer, Tachometer, Fuel Bar, Distance, Coins)
        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setFuel(car.fuel());
        m_hud.setNitro(car.nitro());
        m_hud.setBoosting(car.isBoosting());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setCoins(m_profile.coins());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.update(effectiveDt);

        // Dramatic Crash / Driver Down Slow-Mo Handling
        if (m_physics.isGameOver() && !m_inCrashSequence) {
            m_inCrashSequence = true;
            m_crashSequenceTimer = 1.25f;
            m_crashBannerText = (m_physics.gameOverReason() == "DRIVER DOWN!" ? "DRIVER DOWN!" : (m_physics.gameOverReason() == "OUT OF FUEL!" ? "OUT OF FUEL!" : "WRECKED!"));
            m_crashBannerSubtext = (m_physics.gameOverReason() == "DRIVER DOWN!" ? "FATAL IMPACT - REPAIR VEHICLE" : (m_physics.gameOverReason() == "OUT OF FUEL!" ? "TANK DRIED OUT" : "TOTAL LOSS"));
            m_camera.addTrauma(1.0f);
            m_particleSystem.emitCrashExplosion(car.chassisPos(), 32);
            if (m_physics.gameOverReason() == "DRIVER DOWN!") {
                m_audio.playNeckCrunch();
            } else {
                m_audio.playCrash();
            }
        }

        if (m_inCrashSequence) {
            m_crashSequenceTimer -= dt;
            m_camera.addTrauma(0.15f);
            if (m_crashSequenceTimer <= 0.0f) {
                m_inCrashSequence = false;

                // Save Ghost Run if new distance record
                float dist = m_physics.distanceReached();
                if (dist > m_ghostRecordDist) {
                    m_bestRunGhost = m_currentRunGhost;
                    m_ghostRecordDist = dist;
                }

                int coinsEarned = m_physics.coinsCollected();
                bool isNewRec = dist > m_profile.getRecordDistance(m_selectedBiome);

                m_profile.recordDistance(m_selectedBiome, dist);
                AccountManager::instance().recordDistance(m_selectedBiome, dist);
                AccountManager::instance().addXP(static_cast<int>(dist));

                m_achievements.recordMax(AchievementStat::BEST_RUN_DISTANCE, dist, false);
                m_achievements.addTo(AchievementStat::TOTAL_DISTANCE, dist, false);
                m_achievements.addTo(AchievementStat::TOTAL_FLIPS, m_physics.flipsCompleted(), false);
                m_achievements.addTo(AchievementStat::TOTAL_AIR_TIME, m_physics.totalAirTime(), false);
                m_achievements.evaluate();

                if (m_isGrandPrix) {
                    // Award points for this round
                    int pts = (dist > 1500.0f) ? 10 : (dist > 1000.0f) ? 8 : (dist > 500.0f) ? 6 : 4;
                    m_gpStandings[0].points += pts;
                    m_gpStandings[0].bestDistance = std::max(m_gpStandings[0].bestDistance, dist);
                    if (pts == 10) m_gpStandings[0].wins++;

                    // Simulate AI competitor scores
                    m_gpStandings[1].points += 8;
                    m_gpStandings[1].bestDistance = 1420.0f;
                    m_gpStandings[2].points += 6;
                    m_gpStandings[2].bestDistance = 1250.0f;
                    m_gpStandings[3].points += 4;
                    m_gpStandings[3].bestDistance = 980.0f;

                    bool isLast = (m_gpRound >= static_cast<int>(s_gpCircuit.size()));
                    std::string nextTrk = isLast ? "FINALE" : s_gpCircuit[m_gpRound];
                    m_challengeScreen.setStandings(m_gpStandings, m_gpRound, static_cast<int>(s_gpCircuit.size()),
                                                   m_selectedBiome, nextTrk, isLast);
                    changeState(StateType::GRAND_PRIX_CHALLENGE);
                    return;
                }

                m_gameOverScreen.setStats(dist, coinsEarned, m_physics.flipsCompleted(), m_physics.totalAirTime(), isNewRec);
                changeState(StateType::GAME_OVER);
            }
        }
    }
    else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        m_gameTime += dt;
        m_physics.step(dt, m_profile);

        const auto& car1 = m_physics.vehicle();
        const auto& car2 = m_physics.vehicle2();
        m_camera.update(dt, car1.chassisPos(), car1.chassisVel().x);
        m_camera2.update(dt, car2.chassisPos(), car2.chassisVel().x);

        auto emitVehicleParticles = [this, dt](const Physics::Vehicle& car, float& smokeTimer, Graphics::Camera& cam) {
            smokeTimer -= dt;
            if (car.fuel() > 0.0f && smokeTimer <= 0.0f) {
                float cosA = std::cos(car.chassisAngle());
                float sinA = std::sin(car.chassisAngle());
                Physics::Vec2 pLocal(car.rearMountOffset().x * 0.95f, car.rearMountOffset().y * 0.4f);
                Physics::Vec2 pWorld = car.chassisPos() + Physics::Vec2(pLocal.x * cosA - pLocal.y * sinA,
                                                                       pLocal.x * sinA + pLocal.y * cosA);
                if (car.isBoosting()) {
                    smokeTimer = 0.02f;
                    m_particleSystem.emitNitroFlame(pWorld, Physics::Vec2(cosA, sinA), car.chassisVel());
                    cam.addTrauma(0.04f);
                } else {
                    smokeTimer = (car.getSpeedKmh() > 5.0f) ? 0.05f : 0.10f;
                    m_particleSystem.emitSmoke(pWorld, car.chassisVel());
                }
            }
            const std::string& biome = m_physics.terrain().getBiome();
            if (car.isRearOnGround()) {
                m_particleSystem.emitWheelSpray(car.rearWheelPos(), car.rearWheelRadius(),
                                                car.rearContactTangent(), car.rearSlipSpeed(),
                                                car.rearWheelAngularVel(), car.rearNormalForce(), biome);
            }
            if (car.isFrontOnGround()) {
                m_particleSystem.emitWheelSpray(car.frontWheelPos(), car.frontWheelRadius(),
                                                car.frontContactTangent(), car.frontSlipSpeed(),
                                                car.frontWheelAngularVel(), car.frontNormalForce(), biome);
            }
        };

        emitVehicleParticles(car1, m_smokeTimer, m_camera);
        emitVehicleParticles(car2, m_smokeTimer2, m_camera2);
        m_particleSystem.update(dt);

        m_hud.setSpeed(car1.getSpeedKmh());
        m_hud.setRpm(car1.getEngineRpm());
        m_hud.setFuel(car1.fuel());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setCoins(m_profile.coins());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.update(dt);

        if (m_physics.winner() != 0) {
            float dist = m_physics.winner() == 2 ? m_physics.distanceReached2() : m_physics.distanceReached();
            m_gameOverScreen.setCrashReason(m_physics.mpStatusMessage());
            m_gameOverScreen.setStats(dist, m_physics.coinsCollected(), m_physics.flipsCompleted(),
                                      m_physics.totalAirTime(), dist > m_profile.getRecordDistance(m_selectedBiome));
            m_camera.addTrauma(0.7f);
            m_camera2.addTrauma(0.7f);
            m_audio.playStuntCheer();
            changeState(StateType::GAME_OVER);
        }
    }
    else if (m_currentState == StateType::ONLINE_RACE) {
        m_gameTime += dt;
        m_physics.step(dt, m_profile);

        const auto& car = m_physics.vehicle();
        m_camera.update(dt, car.chassisPos(), car.chassisVel().x);

        float lerpFactor = std::clamp(dt * 18.0f, 0.0f, 1.0f);
        for (auto& pair : m_networkOpponents) {
            auto& opp = pair.second;
            opp.posX += (opp.targetX - opp.posX) * lerpFactor;
            opp.posY += (opp.targetY - opp.posY) * lerpFactor;

            float dAngle = opp.targetAngle - opp.chassisAngle;
            while (dAngle > 3.14159f) dAngle -= 6.28318f;
            while (dAngle < -3.14159f) dAngle += 6.28318f;
            opp.chassisAngle += dAngle * lerpFactor;
        }

        m_smokeTimer -= dt;
        if (car.fuel() > 0.0f && m_smokeTimer <= 0.0f) {
            float cosA = std::cos(car.chassisAngle());
            float sinA = std::sin(car.chassisAngle());
            Physics::Vec2 pLocal(car.rearMountOffset().x * 0.95f, car.rearMountOffset().y * 0.4f);
            Physics::Vec2 pWorld = car.chassisPos() + Physics::Vec2(pLocal.x * cosA - pLocal.y * sinA,
                                                                   pLocal.x * sinA + pLocal.y * cosA);
            if (car.isBoosting()) {
                m_smokeTimer = 0.02f;
                m_particleSystem.emitNitroFlame(pWorld, Physics::Vec2(cosA, sinA), car.chassisVel());
                m_camera.addTrauma(0.04f);
            } else {
                m_smokeTimer = (car.getSpeedKmh() > 5.0f) ? 0.05f : 0.10f;
                m_particleSystem.emitSmoke(pWorld, car.chassisVel());
            }
        }

        const std::string& currentBiome = m_physics.terrain().getBiome();
        if (car.isRearOnGround()) {
            m_particleSystem.emitWheelSpray(car.rearWheelPos(), car.rearWheelRadius(),
                                            car.rearContactTangent(), car.rearSlipSpeed(),
                                            car.rearWheelAngularVel(), car.rearNormalForce(), currentBiome);
        }
        if (car.isFrontOnGround()) {
            m_particleSystem.emitWheelSpray(car.frontWheelPos(), car.frontWheelRadius(),
                                            car.frontContactTangent(), car.frontSlipSpeed(),
                                            car.frontWheelAngularVel(), car.frontNormalForce(), currentBiome);
        }

        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setFuel(car.fuel());
        m_hud.setNitro(car.nitro());
        m_hud.setBoosting(car.isBoosting());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setCoins(m_profile.coins());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.update(dt);

        float dist = m_physics.distanceReached();
        bool crashed = m_physics.isGameOver();
        bool finished = dist >= 1500.0f;
        if (finished && m_localFinishTime <= 0.0f) {
            m_localFinishTime = m_gameTime;
        }

        m_netTelemetryTimer += dt;
        if (m_netTelemetryTimer >= 0.033f) {
            m_netTelemetryTimer = 0.0f;

            Network::RemotePlayerState myState;
            myState.id = m_isNetworkHost ? 0 : m_client.getLocalPlayerId();
            myState.name = AccountManager::instance().activeAccount()->username;
            myState.color = m_isNetworkHost ? 0xFFFF3D00 : m_client.getAssignedColor();
            myState.vehicleType = static_cast<int>(m_profile.selectedVehicle());
            myState.driverType = static_cast<int>(m_profile.selectedDriver());

            myState.posX = car.chassisPos().x;
            myState.posY = car.chassisPos().y;
            myState.velX = car.chassisVel().x;
            myState.velY = car.chassisVel().y;
            myState.chassisAngle = car.chassisAngle();
            myState.chassisAngularVel = car.chassisAngularVel();
            myState.rearWheelAngle = car.rearWheelAngle();
            myState.frontWheelAngle = car.frontWheelAngle();
            myState.rearSuspLen = car.rearSuspensionCompression();
            myState.frontSuspLen = car.frontSuspensionCompression();
            myState.driverHeadAngle = car.driverHeadAngle();
            myState.speedKmh = car.getSpeedKmh();
            myState.fuel = car.fuel();
            myState.distance = dist;
            myState.isAirborne = car.isAirborne();
            myState.isCrashed = crashed;
            myState.isFinished = finished;
            myState.finishTime = m_localFinishTime;

            if (m_isNetworkHost) {
                m_server.updateHostTelemetry(myState);
            } else if (m_isNetworkClient) {
                m_client.sendTelemetry(myState);
            }
        }

        m_particleSystem.update(dt);

        if ((crashed || finished) && !m_localOutcomeBroadcast) {
            broadcastLocalRaceOutcome(crashed, dist, m_localFinishTime);
            if (m_isGrandPrix) {
                int pts = (dist >= 1500.0f) ? 10 : (dist > 1000.0f) ? 8 : (dist > 500.0f) ? 6 : 4;
                m_gpStandings[0].points += pts;
                m_gpStandings[0].bestDistance = std::max(m_gpStandings[0].bestDistance, dist);

                bool isLast = (m_gpRound >= static_cast<int>(s_gpCircuit.size()));
                std::string nextTrk = isLast ? "FINALE" : s_gpCircuit[m_gpRound];
                m_challengeScreen.setStandings(m_gpStandings, m_gpRound, static_cast<int>(s_gpCircuit.size()),
                                               m_selectedBiome, nextTrk, isLast);
                changeState(StateType::GRAND_PRIX_CHALLENGE);
                return;
            }
        }

        checkNetworkRaceFinish();
    }

    m_audio.update(dt, m_physics.vehicle(), m_physics.terrain(), m_currentState);
}

void GameStateManager::broadcastLocalRaceOutcome(bool crashed, float distance, float time) {
    if (m_localOutcomeBroadcast) return;
    m_localOutcomeBroadcast = true;

    int localId = m_isNetworkHost ? 0 : m_client.getLocalPlayerId();
    if (crashed) {
        if (m_isNetworkHost) {
            m_server.broadcastPlayerCrashed(localId, distance);
        } else if (m_isNetworkClient) {
            m_client.sendPlayerCrashed(distance);
        }
    } else {
        if (m_isNetworkHost) {
            m_server.broadcastPlayerFinished(localId, distance, time);
        } else if (m_isNetworkClient) {
            m_client.sendPlayerFinished(distance, time);
        }
    }
}

void GameStateManager::checkNetworkRaceFinish() {
    if (m_currentState != StateType::ONLINE_RACE || m_isGrandPrix || m_onlineResultShown) return;

    float dist = m_physics.distanceReached();
    bool localDone = m_physics.isGameOver() || dist >= 1500.0f;
    if (!localDone) return;

    for (const auto& pair : m_networkOpponents) {
        const auto& opp = pair.second;
        if (!(opp.isCrashed || opp.isFinished || opp.distance >= 1500.0f)) return;
    }

    float bestOppDist = 0.0f;
    std::string bestOppName = "RIVAL";
    for (const auto& pair : m_networkOpponents) {
        const auto& opp = pair.second;
        if (opp.distance > bestOppDist) {
            bestOppDist = opp.distance;
            bestOppName = opp.name;
        }
    }

    if (dist > bestOppDist + 1.0f) {
        m_onlineResultTitle = "VICTORY";
    } else if (bestOppDist > dist + 1.0f) {
        m_onlineResultTitle = "DEFEAT";
    } else {
        m_onlineResultTitle = "DRAW";
    }

    std::string localStatus = m_physics.isGameOver() ? "CRASHED" : "FINISHED";
    m_onlineResultDetail = "YOU " + std::to_string(static_cast<int>(dist)) + "m (" + localStatus + ")  |  " +
                           bestOppName + " " + std::to_string(static_cast<int>(bestOppDist)) + "m";
    m_onlineResultShown = true;
    m_btnOnlineRematch.setText(m_isNetworkHost ? "REMATCH" : "ASK HOST");
    m_btnOnlineRematch.setEnabled(m_isNetworkHost);
}

void GameStateManager::renderOverheadTag(Graphics::Framebuffer& fb, const Graphics::Camera& cam,
                                        const Physics::Vec2& worldPos, const std::string& name,
                                        uint32_t color, float distDiff) {
    Physics::Vec2 scr = cam.worldToScreen({worldPos.x, worldPos.y + 0.95f});
    int sx = static_cast<int>(scr.x);
    int sy = static_cast<int>(scr.y);

    if (sx < 20 || sx > m_width - 20 || sy < 20 || sy > m_height - 20) return;

    // Overhead Racer Badge
    std::string tag = "[" + name + "]";
    int tagW = Graphics::RasterFont::getTextWidth(tag, 1) + 12;
    int tagH = 18;
    int bx = sx - tagW / 2;
    int by = sy - tagH - 4;

    UIComponents::drawArcadePanel(fb, bx, by, tagW, tagH, 0xEE090F1A, Theme::CARD_BORDER, color, 1, false, true);
    Graphics::RasterFont::drawStringCentered(fb, sx, by + 4, tag, color, 1);

    // Distance differential (e.g. +14m or -8m)
    std::string diffStr = (distDiff >= 0.0f ? "+" : "") + std::to_string(static_cast<int>(distDiff)) + "m";
    uint32_t diffCol = (distDiff >= 0.0f) ? Theme::GOLD : Theme::CYAN_UPGRADE;
    Graphics::RasterFont::drawStringCentered(fb, sx, by + 19, diffStr, diffCol, 1);
}

void GameStateManager::renderNetworkRace(Graphics::Framebuffer& fb, float alpha) {
    m_camera.setRenderAlpha(alpha);

    // 1. World Terrain & Parallax
    m_terrainRasterizer.render(fb, m_camera, m_physics.terrain(), m_gameTime);

    // 2. Particles
    m_particleSystem.render(fb, m_camera);

    // 3. Local Player Vehicle
    Graphics::SpriteRenderer::renderVehicle(fb, m_camera, m_physics.vehicle(), alpha);

    // 4. Remote Opponent Vehicles
    for (const auto& pair : m_networkOpponents) {
        const auto& opp = pair.second;
        Graphics::SpriteRenderer::renderRemoteVehicle(fb, m_camera, opp);

        float distDiff = opp.distance - m_physics.distanceReached();
        renderOverheadTag(fb, m_camera, {opp.posX, opp.posY}, opp.name, opp.color, distDiff);
    }

    // 5. Contracted Minimap / Radar Tracker at the top!
    std::vector<UI::RadarRacer> racers;
    std::string myName = AccountManager::instance().activeAccount()->username;
    racers.push_back({myName, m_physics.distanceReached(), Theme::GOLD, true, m_physics.isGameOver()});

    for (const auto& pair : m_networkOpponents) {
        racers.push_back({pair.second.name, pair.second.distance, pair.second.color, false, pair.second.isCrashed});
    }
    m_minimapRadar.render(fb, racers);

    // 6. HUD Telemetry
    m_hud.render(fb);
}

void GameStateManager::renderCompactHud(Graphics::Framebuffer& fb, const Physics::Vehicle& veh, float dist,
                                        const std::string& pLabel, uint32_t tagCol, const std::string& ctrlHint) {
    int W = fb.width();

    // 1. Racer Identity Pill
    int pillW = 210;
    int pillH = 26;
    UI::UIComponents::drawArcadePanel(fb, 12, 10, pillW, pillH, 0xDD090F1A, UI::Theme::CARD_BORDER, tagCol, 2, true, true);
    fb.fillCircle(24, 23, 5, tagCol);
    Graphics::RasterFont::drawString(fb, 36, 17, pLabel, UI::Theme::TEXT_WHITE, 1);

    Graphics::RasterFont::drawString(fb, 14, 40, ctrlHint, 0xBBFFFFFF, 1);
    std::string nosStr = "NOS " + std::to_string(static_cast<int>(veh.nitro())) + "%" + (veh.isBoosting() ? "  BOOST!" : "");
    Graphics::RasterFont::drawString(fb, 14, 63, nosStr, veh.isBoosting() ? 0xFF00E5FF : 0xBBFFFFFF, 1);

    // 2. Speedometer
    int speedKmh = static_cast<int>(veh.getSpeedKmh());
    std::string speedStr = std::to_string(speedKmh) + " KM/H";
    int spdW = Graphics::RasterFont::getTextWidth(speedStr, 2);
    int spdX = W / 2 - spdW / 2;
    Graphics::RasterFont::drawString(fb, spdX, 12, speedStr, UI::Theme::TEXT_WHITE, 2);

    // 3. Distance Odometer
    std::string distStr = UI::UIComponents::formatNumber(static_cast<int>(dist)) + "m";
    int distW = Graphics::RasterFont::getTextWidth(distStr, 2);
    int distBoxW = std::max(120, distW + 28);
    int distBoxX = W - distBoxW - 14;
    UI::UIComponents::drawArcadePanel(fb, distBoxX, 10, distBoxW, 26, 0xDD090F1A, UI::Theme::CARD_BORDER, UI::Theme::GOLD, 2, true, true);
    Graphics::RasterFont::drawString(fb, distBoxX + 14, 15, distStr, UI::Theme::GOLD, 2);
}

void GameStateManager::renderMultiplayer(Graphics::Framebuffer& fb, float alpha) {
    if (m_isLocalMultiplayerFlow) {
        renderLocalMultiplayerRace(fb, alpha);
    } else {
        renderNetworkRace(fb, alpha);
    }
}

void GameStateManager::renderLocalMultiplayerRace(Graphics::Framebuffer& fb, float alpha) {
    m_camera.setRenderAlpha(alpha);
    m_camera2.setRenderAlpha(alpha);

    m_terrainRasterizer.render(m_fbP1, m_camera, m_physics.terrain(), m_gameTime, &m_physics.itemsCollectedP1());
    m_particleSystem.render(m_fbP1, m_camera);
    Graphics::SpriteRenderer::renderVehicle(m_fbP1, m_camera, m_physics.vehicle2(), alpha, 0.55f);
    Graphics::SpriteRenderer::renderVehicle(m_fbP1, m_camera, m_physics.vehicle(), alpha, 1.0f);

    Physics::Vec2 p2Scr = m_camera.worldToScreen(m_physics.vehicle2().chassisPos());
    float relDistP2 = m_physics.vehicle2().chassisPos().x - m_physics.vehicle().chassisPos().x;
    if (p2Scr.x >= 20 && p2Scr.x <= m_fbP1.width() - 20 && p2Scr.y >= 15 && p2Scr.y <= m_fbP1.height() - 15) {
        Graphics::RasterFont::drawStringCentered(m_fbP1, static_cast<int>(p2Scr.x),
                                                 static_cast<int>(p2Scr.y - 34), "[P2]", 0xFF00E5FF, 1);
    } else {
        int indY = m_fbP1.height() / 2;
        std::string indStr = relDistP2 >= 0.0f
                                 ? "P2 +" + std::to_string(static_cast<int>(relDistP2)) + "m >"
                                 : "< P2 " + std::to_string(relDistP2) + "m";
        int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
        int ix = relDistP2 >= 0.0f ? m_fbP1.width() - tw - 18 : 10;
        UIComponents::drawArcadePanel(m_fbP1, ix, indY - 10, tw + 14, 20, 0xDD090F1A,
                                      Theme::CARD_BORDER, 0xFF00E5FF, 0, false, true);
        Graphics::RasterFont::drawString(m_fbP1, ix + 7, indY - 5, indStr, 0xFF00E5FF, 1);
    }
    std::string vehName1 = m_physics.vehicle().config().name;
    renderCompactHud(m_fbP1, m_physics.vehicle(), m_physics.distanceReached(),
                     "P1 - " + vehName1, 0xFFFF3D00, "P1: [W/D] GAS  [S/A] BRK/REV  [SHIFT] NOS");

    m_terrainRasterizer.render(m_fbP2, m_camera2, m_physics.terrain(), m_gameTime, &m_physics.itemsCollectedP2());
    m_particleSystem.render(m_fbP2, m_camera2);
    Graphics::SpriteRenderer::renderVehicle(m_fbP2, m_camera2, m_physics.vehicle(), alpha, 0.55f);
    Graphics::SpriteRenderer::renderVehicle(m_fbP2, m_camera2, m_physics.vehicle2(), alpha, 1.0f);

    Physics::Vec2 p1Scr = m_camera2.worldToScreen(m_physics.vehicle().chassisPos());
    float relDistP1 = m_physics.vehicle().chassisPos().x - m_physics.vehicle2().chassisPos().x;
    if (p1Scr.x >= 20 && p1Scr.x <= m_fbP2.width() - 20 && p1Scr.y >= 15 && p1Scr.y <= m_fbP2.height() - 15) {
        Graphics::RasterFont::drawStringCentered(m_fbP2, static_cast<int>(p1Scr.x),
                                                 static_cast<int>(p1Scr.y - 34), "[P1]", 0xFFFF3D00, 1);
    } else {
        int indY = m_fbP2.height() / 2;
        std::string indStr = relDistP1 >= 0.0f
                                 ? "P1 +" + std::to_string(static_cast<int>(relDistP1)) + "m >"
                                 : "< P1 " + std::to_string(relDistP1) + "m";
        int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
        int ix = relDistP1 >= 0.0f ? m_fbP2.width() - tw - 18 : 10;
        UIComponents::drawArcadePanel(m_fbP2, ix, indY - 10, tw + 14, 20, 0xDD090F1A,
                                      Theme::CARD_BORDER, 0xFFFF3D00, 0, false, true);
        Graphics::RasterFont::drawString(m_fbP2, ix + 7, indY - 5, indStr, 0xFFFF3D00, 1);
    }
    std::string vehName2 = m_physics.vehicle2().config().name;
    renderCompactHud(m_fbP2, m_physics.vehicle2(), m_physics.distanceReached2(),
                     "P2 - " + vehName2, 0xFF00E5FF, "P2: [UP/DN] GAS  [LT/RT] BRK/REV  [0] NOS");

    fb.blit(0, 0, m_fbP1);
    fb.blit(0, 274, m_fbP2);

    int cx = m_width / 2;
    fb.fillRect(0, 266, m_width, 8, 0xFF0A0F18);
    fb.drawLine(0, 266, m_width, 266, 0xFFFF3D00);
    fb.drawLine(0, 273, m_width, 273, 0xFF00E5FF);
    fb.fillCircle(cx, 270, 14, 0xFF0E1826);
    fb.drawCircle(cx, 270, 14, Theme::GOLD);
    Graphics::RasterFont::drawStringCentered(fb, cx, 266, "VS", Theme::GOLD, 1);

    const std::string& status = m_physics.mpStatusMessage();
    if (!status.empty()) {
        int sw = Graphics::RasterFont::getTextWidth(status, 1) + 20;
        int sx = cx - sw / 2;
        UIComponents::drawArcadePanel(fb, sx, 248, sw, 18, 0xF00A111D, Theme::CARD_BORDER,
                                      Theme::GOLD, 0, false, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 252, status, Theme::GOLD, 1);
    }
}

void GameStateManager::renderOnlineResultOverlay(Graphics::Framebuffer& fb) {
    fb.fillRect(0, 0, m_width, m_height, 0xB4060A12);

    int panelW = std::min(660, m_width - 60);
    int panelH = 210;
    int px = (m_width - panelW) / 2;
    int py = m_height / 2 - 150;
    UIComponents::drawArcadePanel(fb, px, py, panelW, panelH, 0xF0090F1A,
                                  Theme::CARD_BORDER, Theme::GOLD, 3);

    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, py + 24, "* RACE RESULT *", Theme::GOLD, 1);
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, py + 56, m_onlineResultTitle,
                                             m_onlineResultTitle == "VICTORY" ? Theme::GREEN_GAS
                                                                              : Theme::TEXT_WHITE, 3);
    Graphics::RasterFont::drawStringCentered(fb, m_width / 2, py + 112, m_onlineResultDetail,
                                             Theme::TEXT_WHITE, 1);

    m_btnOnlineRematch.render(fb, 2);
    m_btnOnlineLobby.render(fb, 2);
}

void GameStateManager::render(Graphics::Framebuffer& fb, float alpha) {
    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.render(fb, m_profile);
    } else if (m_currentState == StateType::ACCOUNT_PROFILE) {
        m_accountScreen.render(fb);
    } else if (m_currentState == StateType::ACHIEVEMENTS) {
        m_achievementsScreen.render(fb);
    } else if (m_currentState == StateType::NETWORK_LOBBY) {
        m_lobbyScreen.render(fb, &m_server, &m_client);
    } else if (m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        m_challengeScreen.render(fb);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.render(fb, m_profile);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.render(fb, m_profile);
    } else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        renderLocalMultiplayerRace(fb, alpha);
    } else if (m_currentState == StateType::ONLINE_RACE) {
        renderNetworkRace(fb, alpha);
        if (m_onlineResultShown) {
            renderOnlineResultOverlay(fb);
        }
    } else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
        if (m_isLocalMultiplayerFlow && (m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER)) {
            renderLocalMultiplayerRace(fb, alpha);
        } else {
            m_camera.setRenderAlpha(alpha);

            m_terrainRasterizer.render(fb, m_camera, m_physics.terrain(), m_gameTime);
            m_particleSystem.render(fb, m_camera);

            // Render Personal Best Ghost Racer
            renderGhostVehicle(fb, m_camera);

            Graphics::SpriteRenderer::renderVehicle(fb, m_camera, m_physics.vehicle(), alpha);

            // Speed Warp Streaks when boosting at velocity
            if (m_physics.vehicle().isBoosting() && m_physics.vehicle().getSpeedKmh() > 40.0f) {
                renderSpeedStreaks(fb, std::clamp(m_physics.vehicle().getSpeedKmh() / 110.0f, 0.4f, 1.0f));
            }

            // Dramatic Crash Slow-Mo Sequence Overlay
            if (m_inCrashSequence) {
                renderCrashSequenceOverlay(fb);
            }

            std::vector<UI::RadarRacer> racers;
            std::string myName = AccountManager::instance().activeAccount()->username;
            racers.push_back({myName, m_physics.distanceReached(), Theme::GOLD, true, m_physics.isGameOver()});

            // If ghost exists, track ghost on radar
            if (!m_bestRunGhost.empty()) {
                float sampleIndexF = m_gameTime / 0.08f;
                size_t gIdx = static_cast<size_t>(sampleIndexF);
                if (gIdx < m_bestRunGhost.size()) {
                    racers.push_back({"RECORD", m_bestRunGhost[gIdx].x, 0xFF00E5FF, false, false});
                }
            }

            if (m_isGrandPrix) {
                for (size_t i = 1; i < m_gpStandings.size(); ++i) {
                    float aiDist = std::min(m_gpStandings[i].bestDistance, m_physics.distanceReached() + (static_cast<float>(i) * 45.0f - 70.0f));
                    racers.push_back({m_gpStandings[i].name, aiDist, m_gpStandings[i].color, false, false});
                }
            }
            m_minimapRadar.render(fb, racers);
            m_hud.render(fb);

            if (m_isEvaluatorMode) {
                const auto& cfg = Physics::VehicleRegistry::getConfig(m_profile.selectedVehicle());
                const auto& bSpec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
                m_evaluatorOverlay.render(fb, cfg.name, bSpec.name);
            }
        }

        if (m_currentState == StateType::PAUSED) {
            m_pauseOverlay.render(fb);
        } else if (m_currentState == StateType::GAME_OVER) {
            m_gameOverScreen.render(fb);
        }
    }

    // 8. Developer Mode Console Overlay (Renders over any state when active)
    if (m_showDevOverlay) {
        m_devOverlay.render(fb, m_physics, m_audio);
    }

    // Smooth screen fade curtain
    if (m_transitionTimer > 0.0f) {
        float frac = m_transitionTimer / 0.12f;
        uint32_t a = static_cast<uint32_t>(frac * 160.0f);
        fb.fillRect(0, 0, m_width, m_height, (a << 24) | 0x060A12);
    }

    if (m_achievementToastTimer > 0.0f && !m_achievementToastTitle.empty()) {
        int toastW = std::min(460, m_width - 40);
        int toastH = 62;
        int toastX = (m_width - toastW) / 2;
        int toastY = 62;
        float fade = std::min(1.0f, m_achievementToastTimer / 0.6f);
        uint32_t bg = (static_cast<uint32_t>(fade * 0xE8) << 24) | 0x11263A;
        UI::UIComponents::drawArcadePanel(fb, toastX, toastY, toastW, toastH,
                                          bg, Theme::GOLD, Theme::GOLD, 3);
        Graphics::RasterFont::drawStringCentered(fb, m_width / 2, toastY + 8,
                                                 "* ACHIEVEMENT UNLOCKED *", Theme::GOLD);
        Graphics::RasterFont::drawStringCentered(fb, m_width / 2, toastY + 28,
                                                 m_achievementToastTitle, Theme::TEXT_WHITE, 2);
    }
}

void GameStateManager::initRunBounties() {
    m_bounties.clear();
    m_currentBountyIdx = 0;
    m_bounties.push_back({"AIRTIME PRO", "Stay airborne for 2.0s", 2.0f, 0.0f, 300, false});
    m_bounties.push_back({"SPEED DEMON", "Hit 75 KM/H", 75.0f, 0.0f, 350, false});
    m_bounties.push_back({"STUNT MASTER", "Perform 1 Flip", 1.0f, 0.0f, 500, false});
    m_bounties.push_back({"NITRO ROCKET", "Boost for 2.5s", 2.5f, 0.0f, 400, false});
    m_bounties.push_back({"WHEELIE KING", "Hold a 1.2s Wheelie", 1.2f, 0.0f, 300, false});
    m_bounties.push_back({"ENDURANCE", "Reach 600m", 600.0f, 0.0f, 600, false});
    m_bounties.push_back({"HIGH VELOCITY", "Hit 95 KM/H", 95.0f, 0.0f, 500, false});
    m_bounties.push_back({"MARATHON", "Reach 1200m", 1200.0f, 0.0f, 1000, false});
}

void GameStateManager::updateRunBounties(float dt) {
    if (m_currentBountyIdx >= m_bounties.size()) {
        m_hud.setBountyInfo("ALL CLEAR!", "DONE", 0);
        return;
    }

    auto& b = m_bounties[m_currentBountyIdx];
    const auto& car = m_physics.vehicle();
    char progBuf[32];

    switch (m_currentBountyIdx) {
        case 0: // Airtime 2.0s
            if (car.isAirborne()) b.current += dt;
            else if (car.isRearOnGround() || car.isFrontOnGround()) b.current = 0.0f;
            std::snprintf(progBuf, sizeof(progBuf), "%.1fs / %.1fs", b.current, b.target);
            break;
        case 1: // Speed 75 km/h
            b.current = std::max(b.current, car.getSpeedKmh());
            std::snprintf(progBuf, sizeof(progBuf), "%d / %d KM/H", static_cast<int>(b.current), static_cast<int>(b.target));
            break;
        case 2: // Stunt flips
            b.current = static_cast<float>(m_physics.flipsCompleted());
            std::snprintf(progBuf, sizeof(progBuf), "%d / %d", static_cast<int>(b.current), static_cast<int>(b.target));
            break;
        case 3: // Nitro Boost 2.5s
            if (car.isBoosting()) b.current += dt;
            std::snprintf(progBuf, sizeof(progBuf), "%.1fs / %.1fs", b.current, b.target);
            break;
        case 4: // Wheelie 1.2s
            if (car.isRearOnGround() && !car.isFrontOnGround() && car.getSpeedKmh() > 18.0f) b.current += dt;
            else b.current = 0.0f;
            std::snprintf(progBuf, sizeof(progBuf), "%.1fs / %.1fs", b.current, b.target);
            break;
        case 5: // Distance 600m
            b.current = m_physics.distanceReached();
            std::snprintf(progBuf, sizeof(progBuf), "%dm / %dm", static_cast<int>(b.current), static_cast<int>(b.target));
            break;
        case 6: // Speed 95 km/h
            b.current = std::max(b.current, car.getSpeedKmh());
            std::snprintf(progBuf, sizeof(progBuf), "%d / %d KM/H", static_cast<int>(b.current), static_cast<int>(b.target));
            break;
        case 7: // Distance 1200m
            b.current = m_physics.distanceReached();
            std::snprintf(progBuf, sizeof(progBuf), "%dm / %dm", static_cast<int>(b.current), static_cast<int>(b.target));
            break;
        default:
            std::snprintf(progBuf, sizeof(progBuf), "ACTIVE");
            break;
    }

    m_hud.setBountyInfo(b.title, progBuf, b.reward);

    if (b.current >= b.target && !b.completed) {
        b.completed = true;
        m_profile.addCoins(b.reward);
        AccountManager::instance().addCoins(b.reward);
        m_physics.addCoins(b.reward);
        m_hud.triggerStunt("BOUNTY: " + b.title, b.reward, Theme::GOLD);
        m_particleSystem.emitSparkles(car.chassisPos(), Theme::GOLD);
        m_currentBountyIdx++;
    }
}

void GameStateManager::renderSpeedStreaks(Graphics::Framebuffer& fb, float intensity) {
    int W = fb.width();
    int H = fb.height();
    int numStreaks = static_cast<int>(intensity * 16.0f);
    for (int i = 0; i < numStreaks; ++i) {
        int y = (i * 39 + static_cast<int>(m_gameTime * 380.0f)) % H;
        int len = 45 + (i * 21) % 95;
        int x = (i % 2 == 0) ? (i * 29) % (W / 4) : W - len - (i * 27) % (W / 4);
        uint32_t col = (i % 3 == 0) ? 0x6600E5FF : 0x44FFFFFF;
        fb.drawLine(x, y, x + len, y, col);
    }
}

void GameStateManager::renderCrashSequenceOverlay(Graphics::Framebuffer& fb) {
    if (!m_inCrashSequence) return;
    int W = fb.width();
    int H = fb.height();

    // Red pulsating cinematic vignette around screen edges
    float pulse = 0.5f + 0.5f * std::sin(m_crashSequenceTimer * 16.0f);
    uint32_t vigA = static_cast<uint32_t>(pulse * 90.0f + 50.0f);
    uint32_t vigCol = (vigA << 24) | 0x00D50000;

    int borderThickness = 12;
    fb.fillRect(0, 0, W, borderThickness, vigCol);
    fb.fillRect(0, H - borderThickness, W, borderThickness, vigCol);
    fb.fillRect(0, 0, borderThickness, H, vigCol);
    fb.fillRect(W - borderThickness, 0, borderThickness, H, vigCol);

    // Large high-impact center banner
    int bannerW = std::min(560, W - 40);
    int bannerH = 74;
    int bx = (W - bannerW) / 2;
    int by = H / 2 - 45;

    UI::UIComponents::drawArcadePanel(fb, bx, by, bannerW, bannerH, 0xF2090F1A, UI::Theme::CARD_BORDER, 0xFFFF3D00, 3, false, true);
    Graphics::RasterFont::drawStringCentered(fb, W / 2, by + 12, m_crashBannerText, 0xFFFF3D00, 3);
    Graphics::RasterFont::drawStringCentered(fb, W / 2, by + 46, m_crashBannerSubtext, UI::Theme::TEXT_WHITE, 1);
}

void GameStateManager::renderGhostVehicle(Graphics::Framebuffer& fb, const Graphics::Camera& cam) {
    if (m_bestRunGhost.empty()) return;

    float sampleIndexF = m_gameTime / 0.08f;
    size_t idx0 = static_cast<size_t>(sampleIndexF);
    if (idx0 >= m_bestRunGhost.size()) return;

    size_t idx1 = std::min(idx0 + 1, m_bestRunGhost.size() - 1);
    float frac = sampleIndexF - static_cast<float>(idx0);

    const auto& f0 = m_bestRunGhost[idx0];
    const auto& f1 = m_bestRunGhost[idx1];

    float gX = f0.x + (f1.x - f0.x) * frac;
    float gY = f0.y + (f1.y - f0.y) * frac;
    float gAngle = f0.angle + (f1.angle - f0.angle) * frac;
    float gRearSusp = f0.rearSuspLen + (f1.rearSuspLen - f0.rearSuspLen) * frac;
    float gFrontSusp = f0.frontSuspLen + (f1.frontSuspLen - f0.frontSuspLen) * frac;
    float gRearWheel = f0.rearWheelAngle + (f1.rearWheelAngle - f0.rearWheelAngle) * frac;
    float gFrontWheel = f0.frontWheelAngle + (f1.frontWheelAngle - f0.frontWheelAngle) * frac;

    Network::RemotePlayerState ghostState;
    ghostState.id = -1;
    ghostState.name = "BEST RECORD";
    ghostState.color = 0x8800E5FF; // Ethereal glowing cyan!
    ghostState.vehicleType = static_cast<int>(m_profile.selectedVehicle());
    ghostState.driverType = static_cast<int>(m_profile.selectedDriver());
    ghostState.posX = gX;
    ghostState.posY = gY;
    ghostState.chassisAngle = gAngle;
    ghostState.rearSuspLen = gRearSusp;
    ghostState.frontSuspLen = gFrontSusp;
    ghostState.rearWheelAngle = gRearWheel;
    ghostState.frontWheelAngle = gFrontWheel;

    Graphics::SpriteRenderer::renderRemoteVehicle(fb, cam, ghostState);

    float distDiff = gX - m_physics.distanceReached();
    renderOverheadTag(fb, cam, {gX, gY}, "PERSONAL BEST", 0xFF00E5FF, distDiff);
}

} // namespace Core
