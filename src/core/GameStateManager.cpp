#include "GameStateManager.h"
#include "Sprite.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include <QApplication>
#include <iostream>
#include <cmath>

namespace Core {

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
    , m_mpSpeedoP1(0, 0, 24, 0.0f, 140.0f, "SPD", "KM/H", 0.85f)
    , m_mpSpeedoP2(0, 0, 24, 0.0f, 140.0f, "SPD", "KM/H", 0.85f)
    , m_mpTachoP1(0, 0, 24, 0.0f, 8.5f, "RPM", "x1000", 0.78f)
    , m_mpTachoP2(0, 0, 24, 0.0f, 8.5f, "RPM", "x1000", 0.78f)
    , m_mpFuelBarP1(14, 40, 130, 14)
    , m_mpFuelBarP2(14, 40, 130, 14)
{
    setDimensions(m_width, m_height);
    setupUiCallbacks();

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

    // Dynamic dual analog instrument cluster positioning in 960x266 split-screens
    m_mpSpeedoP1.setCenter(width - 88, 230);
    m_mpTachoP1.setCenter(width - 32, 230);
    m_mpSpeedoP2.setCenter(width - 88, 230);
    m_mpTachoP2.setCenter(width - 32, 230);
    m_mpFuelBarP1.setPosition(14, 40);
    m_mpFuelBarP2.setPosition(14, 40);

    m_mainMenu.setDimensions(width, height);
    m_garage.setDimensions(width, height);
    m_stageSelect.setDimensions(width, height);
    m_hud.setDimensions(width, height);
    m_pauseOverlay.setDimensions(width, height);
    m_gameOverScreen.setDimensions(width, height);
    m_lobbyScreen.setDimensions(width, height);
    m_minimapRadar.setDimensions(width / 2 - 210, 10, 420, 24);
}

void GameStateManager::setupUiCallbacks() {
    auto playClick = [this]() {
        m_audio.playButtonClick();
    };

    // Button audio feedback
    m_mainMenu.setButtonSound(playClick);
    m_garage.setButtonSound(playClick);
    m_stageSelect.setButtonSound(playClick);
    m_hud.setButtonSound(playClick);
    m_pauseOverlay.setButtonSound(playClick);
    m_gameOverScreen.setButtonSound(playClick);
    m_lobbyScreen.setButtonSound(playClick);

    // 1. Main Menu
    m_mainMenu.setOnStart([this]() {
        m_isMultiplayerFlow = false;
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnMultiplayer([this]() {
        m_isMultiplayerFlow = true;
        m_garage.setMultiplayerMode(true);
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        m_garage.setP2Vehicle(m_profile.selectedVehicle());
        m_garage.setP2Driver(Physics::DriverType::SARAH);
        changeState(StateType::GARAGE);
    });
    m_mainMenu.setOnOnlineMultiplayer([this]() {
        startOnlineLobby();
    });
    m_mainMenu.setOnGarage([this]() {
        m_isMultiplayerFlow = false;
        m_garage.setMultiplayerMode(false);
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_mainMenu.setOnStages([this]() {
        m_isMultiplayerFlow = false;
        m_stageSelect.setMultiplayerMode(false);
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnQuit([]() {
        QApplication::quit();
    });

    // 1.1 Online Lobby
    m_lobbyScreen.setOnBack([this]() {
        if (m_server.isRunning()) m_server.stopServer();
        if (m_client.isConnected()) m_client.disconnectFromHost();
        changeState(StateType::MAIN_MENU);
    });
    m_lobbyScreen.setOnHostCreate([this]() {
        if (!m_server.isRunning()) {
            m_server.startServer(Network::DEFAULT_PORT);
        }
        m_server.setHostPlayerInfo(m_lobbyScreen.getPlayerName(), m_lobbyScreen.getSelectedVehicle(), m_lobbyScreen.getSelectedDriver());
        m_server.setTrack(m_lobbyScreen.getSelectedBiome());
        std::cout << "[Host] Server listening on port " << Network::DEFAULT_PORT << " IP: " << m_server.getLocalIpAddress().toStdString() << std::endl;
    });
    m_lobbyScreen.setOnHostTrackChanged([this](const std::string& biomeId) {
        if (m_server.isRunning()) {
            m_server.setTrack(biomeId);
        }
    });
    m_lobbyScreen.setOnHostVehicleChanged([this](int vehicle, int driver) {
        if (m_server.isRunning()) {
            m_server.setHostPlayerInfo(m_lobbyScreen.getPlayerName(), vehicle, driver);
        }
    });
    m_lobbyScreen.setOnHostStart([this](const std::string& biomeId) {
        m_server.setTrack(biomeId);
        m_server.broadcastStartRace();
        startNetworkMultiplayer(true, biomeId);
    });
    m_lobbyScreen.setOnClientConnect([this](const std::string& ip, quint16 port, const std::string& name, int vehicle, int driver) {
        m_client.connectToHost(QString::fromStdString(ip), port, name, vehicle, driver);
    });
    m_lobbyScreen.setOnClientReady([this](bool isReady, int vehicle, int driver) {
        m_client.sendReady(isReady, vehicle, driver);
    });
    m_lobbyScreen.setOnDisconnect([this]() {
        if (m_server.isRunning()) m_server.stopServer();
        if (m_client.isConnected()) m_client.disconnectFromHost();
    });

    // Server Network Events
    m_server.setOnPlayerConnected([this](int id, const std::string& name) {
        std::cout << "[Server] Player connected: ID " << id << " Name: " << name << std::endl;
    });
    m_server.setOnPlayerDisconnected([this](int id) {
        std::cout << "[Server] Player disconnected: ID " << id << std::endl;
        m_onlineOpponents.erase(id);
    });
    m_server.setOnTelemetryReceived([this](const Network::RemotePlayerState& st) {
        m_onlineOpponents[st.id] = st;
        checkOnlineRaceRules();
    });
    m_server.setOnPlayerCrashed([this](int playerId, float /*dist*/) {
        m_onlineOpponents[playerId].isCrashed = true;
        checkOnlineRaceRules();
    });
    m_server.setOnPlayerFinished([this](int playerId, float /*dist*/, float /*time*/) {
        m_onlineOpponents[playerId].isFinished = true;
        checkOnlineRaceRules();
    });

    // Client Network Events
    m_client.setOnConnected([this]() {
        m_lobbyScreen.setStatusMessage("CONNECTED TO HOST! SELECT VEHICLE & CLICK READY.");
    });
    m_client.setOnConnectionError([this](const std::string& err) {
        m_lobbyScreen.setStatusMessage("CONNECTION FAILED: " + err);
    });
    m_client.setOnDisconnected([this]() {
        m_lobbyScreen.setStatusMessage("DISCONNECTED FROM HOST.");
        if (m_currentState == StateType::ONLINE_RACE) {
            changeState(StateType::ONLINE_LOBBY);
        }
    });
    m_client.setOnLobbyUpdated([this](const std::string& biomeId, const std::vector<Network::PlayerLobbyInfo>& /*players*/) {
        m_lobbyScreen.setSelectedBiome(biomeId);
    });
    m_client.setOnRaceStart([this](const std::string& biomeId) {
        startNetworkMultiplayer(false, biomeId);
    });
    m_client.setOnRematch([this](const std::string& biomeId) {
        startNetworkMultiplayer(false, biomeId);
    });
    m_client.setOnReturnToLobby([this]() {
        changeState(StateType::ONLINE_LOBBY);
    });
    m_client.setOnRemoteTelemetry([this](const Network::RemotePlayerState& st) {
        m_onlineOpponents[st.id] = st;
        checkOnlineRaceRules();
    });
    m_client.setOnRemotePlayerCrashed([this](int playerId, float /*dist*/) {
        m_onlineOpponents[playerId].isCrashed = true;
        checkOnlineRaceRules();
    });
    m_client.setOnRemotePlayerFinished([this](int playerId, float /*dist*/, float /*time*/) {
        m_onlineOpponents[playerId].isFinished = true;
        checkOnlineRaceRules();
    });

    // 2. Garage
    m_garage.setOnBack([this]() {
        m_isMultiplayerFlow = false;
        changeState(StateType::MAIN_MENU);
    });
    m_garage.setOnDrive([this]() {
        if (m_isMultiplayerFlow) {
            m_mpVehicleP1 = m_profile.selectedVehicle();
            m_mpVehicleP2 = m_garage.p2Vehicle();
            m_mpDriverP1 = m_profile.selectedDriver();
            m_mpDriverP2 = m_garage.p2Driver();
            m_stageSelect.setMultiplayerMode(true);
        } else {
            m_stageSelect.setMultiplayerMode(false);
        }
        changeState(StateType::STAGE_SELECT);
    });

    // 3. Stage Select
    m_stageSelect.setOnBack([this]() {
        if (m_isMultiplayerFlow) {
            m_garage.setMultiplayerMode(true);
            changeState(StateType::GARAGE);
        } else {
            changeState(StateType::MAIN_MENU);
        }
    });
    m_stageSelect.setOnSelectStage([this](const std::string& stageId) {
        if (m_isMultiplayerFlow) {
            startMultiplayerRace(stageId);
        } else {
            startRace(stageId);
        }
    });

    // 4. HUD
    m_hud.setOnPauseClicked([this]() {
        if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE) {
            changeState(StateType::PAUSED);
        }
    });

    // 5. Pause Overlay
    m_pauseOverlay.setOnResume([this]() {
        if (m_physics.isMultiplayer()) {
            changeState(StateType::MULTIPLAYER_RACE);
        } else {
            changeState(StateType::GAMEPLAY);
        }
    });
    m_pauseOverlay.setOnRestart([this]() {
        if (m_physics.isMultiplayer()) {
            startMultiplayerRace(m_selectedMpBiome);
        } else {
            startRace(m_selectedBiome);
        }
    });
    m_pauseOverlay.setOnGarage([this]() {
        if (m_physics.isMultiplayer()) {
            m_isMultiplayerFlow = true;
            m_garage.setMultiplayerMode(true);
            m_garage.setP2Vehicle(m_mpVehicleP2);
            m_garage.setP2Driver(m_mpDriverP2);
        } else {
            m_isMultiplayerFlow = false;
            m_garage.setMultiplayerMode(false);
        }
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_pauseOverlay.setOnMenu([this]() {
        m_isMultiplayerFlow = false;
        changeState(StateType::MAIN_MENU);
    });

    // 6. Game Over Screen
    m_gameOverScreen.setOnRetry([this]() {
        if (m_physics.isMultiplayer()) {
            startMultiplayerRace(m_selectedMpBiome);
        } else {
            startRace(m_selectedBiome);
        }
    });
    m_gameOverScreen.setOnGarage([this]() {
        if (m_physics.isMultiplayer()) {
            m_isMultiplayerFlow = true;
            m_garage.setMultiplayerMode(true);
            m_garage.setP2Vehicle(m_mpVehicleP2);
            m_garage.setP2Driver(m_mpDriverP2);
        } else {
            m_isMultiplayerFlow = false;
            m_garage.setMultiplayerMode(false);
        }
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_gameOverScreen.setOnMenu([this]() {
        m_isMultiplayerFlow = false;
        changeState(StateType::MAIN_MENU);
    });

    // 7. Physics Event Callbacks
    m_physics.setOnStunt([this](const std::string& title, int bonusCoins) {
        m_hud.triggerStunt(title, bonusCoins);
        m_profile.addCoins(bonusCoins);
        m_audio.playStuntCheer();
    });

    m_physics.setOnItemCollected([this](Physics::ItemType type, int val, bool isP1) {
        Physics::Vec2 carPos = isP1 ? m_physics.vehicle().chassisPos() : m_physics.vehicle2().chassisPos();
        Physics::Vec2 scr = isP1 ? m_camera.worldToScreen(carPos) : m_camera2.worldToScreen(carPos);
        if (type == Physics::ItemType::FUEL_CANISTER) {
            m_audio.playFuelPickup();
            if (isP1) {
                m_hud.triggerStunt("+45% FUEL!", 0, UI::Theme::GREEN_GAS);
                m_hud.addFloatingText("+45% FUEL!", scr.x, scr.y - 25.0f, UI::Theme::GREEN_GAS);
                m_hud.triggerFuelFlash();
                m_mpFuelBarP1.triggerPickupFlash();
            } else {
                m_mpFuelBarP2.triggerPickupFlash();
            }
            m_particleSystem.emitSparkles(carPos, UI::Theme::GREEN_GAS);
        } else {
            m_audio.playCoinPickup();
            uint32_t sparkCol = (type == Physics::ItemType::COIN_GOLD) ? UI::Theme::GOLD :
                                (type == Physics::ItemType::COIN_SILVER) ? 0xFFCFD8DC : 0xFFCD7F32;
            m_particleSystem.emitSparkles(carPos, sparkCol);
            if (isP1) {
                m_hud.addFloatingText("+$" + std::to_string(val), scr.x, scr.y - 25.0f, sparkCol);
            }
        }
    });

    // 8. Checkpoint Crossings
    m_physics.setOnCheckpoint([this](const Physics::CheckpointInfo& cp) {
        m_profile.addCoins(cp.coinReward);
        m_profile.recordStageCheckpoint(m_selectedBiome, cp.index);
        m_audio.playStuntCheer();

        if (cp.isFinishLine) {
            std::string msg = "STAGE CLEARED! +" + std::to_string(cp.coinReward);
            m_hud.triggerStunt(msg, cp.coinReward, UI::Theme::GOLD);
            const auto& spec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
            if (!spec.nextBiomeId.empty()) {
                m_profile.unlockStage(spec.nextBiomeId);
            }
        } else {
            std::string msg = "CHECKPOINT " + std::to_string(cp.index) + ": " + cp.name + "!";
            m_hud.triggerStunt(msg, cp.coinReward, UI::Theme::GREEN_GAS);
        }

        Physics::Vec2 carPos = m_physics.vehicle().chassisPos();
        Physics::Vec2 scr = m_camera.worldToScreen(carPos);
        m_hud.addFloatingText("+$" + std::to_string(cp.coinReward), scr.x, scr.y - 35.0f, UI::Theme::GOLD);
    });
}

void GameStateManager::changeState(StateType newState) {
    if (m_currentState != newState) {
        m_transitionTimer = 0.12f;
        m_currentState = newState;
    }
}

void GameStateManager::startRace(const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_camera.setScreenSize(m_width, m_height);
    m_physics.reset(biomeId, m_profile.selectedVehicle(), m_profile.selectedDriver(), false);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    changeState(StateType::GAMEPLAY);
}

void GameStateManager::startMultiplayerRace(const std::string& biomeId) {
    m_selectedMpBiome = biomeId;
    m_camera.setScreenSize(m_width, 266);
    m_camera2.setScreenSize(m_width, 266);

    Physics::VehicleType vType1 = m_mpVehicleP1;
    Physics::VehicleType vType2 = m_mpVehicleP2;
    Physics::DriverType dTypeP1 = m_mpDriverP1;
    Physics::DriverType dTypeP2 = m_mpDriverP2;

    m_physics.reset(biomeId,
                    vType1, dTypeP1,
                    true,
                    vType2, dTypeP2);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_camera2.reset(m_physics.vehicle2().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_smokeTimer = 0.0f;
    m_smokeTimer2 = 0.0f;

    m_mpSpeedoP1.setValue(0.0f);
    m_mpSpeedoP2.setValue(0.0f);
    m_mpTachoP1.setValue(1.0f);
    m_mpTachoP2.setValue(1.0f);
    m_mpFuelBarP1.setFuel(100.0f);
    m_mpFuelBarP2.setFuel(100.0f);

    changeState(StateType::MULTIPLAYER_RACE);
}

void GameStateManager::startOnlineLobby() {
    m_lobbyScreen.setMode(UI::LobbyMode::CHOOSE_ROLE);
    m_lobbyScreen.setSelectedVehicle(static_cast<int>(m_profile.selectedVehicle()));
    m_lobbyScreen.setSelectedDriver(static_cast<int>(m_profile.selectedDriver()));
    changeState(StateType::ONLINE_LOBBY);
}

void GameStateManager::startNetworkMultiplayer(bool isHost, const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_isOnlineHost = isHost;
    m_isOnlineClient = !isHost;
    m_onlineOpponents.clear();
    m_onlineWinnerId = -1;
    m_onlineWinnerName.clear();
    m_onlineWinReason.clear();
    m_onlineRaceFinished = false;

    // Dedicated Fullscreen for each laptop/player!
    m_camera.setScreenSize(m_width, m_height);

    Physics::VehicleType chosenVeh = static_cast<Physics::VehicleType>(m_lobbyScreen.getSelectedVehicle());
    Physics::DriverType chosenDrv = static_cast<Physics::DriverType>(m_lobbyScreen.getSelectedDriver());

    m_physics.reset(biomeId, chosenVeh, chosenDrv, false);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_smokeTimer = 0.0f;
    m_onlineTelemetryTimer = 0.0f;
    m_minimapRadar.setCourseLength(1500.0f);

    changeState(StateType::ONLINE_RACE);
}

void GameStateManager::handleInput(const InputController::State& input) {
    int mx = input.mouseX;
    int my = input.mouseY;

    if (m_currentState == StateType::MAIN_MENU) {
        if (input.multiplayer) {
            m_isMultiplayerFlow = true;
            m_garage.setMultiplayerMode(true);
            m_garage.setViewedVehicle(m_profile.selectedVehicle());
            m_garage.setP2Vehicle(m_profile.selectedVehicle());
            m_garage.setP2Driver(Physics::DriverType::SARAH);
            changeState(StateType::GARAGE);
            return;
        }
        if (input.onlineMultiplayer) {
            startOnlineLobby();
            return;
        }
        if (input.enter) {
            startRace(m_selectedBiome);
            return;
        }
        if (input.mouseMoved) m_mainMenu.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_mainMenu.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_mainMenu.onMouseUp(mx, my);
    } else if (m_currentState == StateType::ONLINE_LOBBY) {
        if (input.mouseMoved) m_lobbyScreen.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_lobbyScreen.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_lobbyScreen.onMouseUp(mx, my);
        if (input.keyTyped) {
            m_lobbyScreen.onKeyPress(input.lastKey, input.lastText);
        }
    } else if (m_currentState == StateType::GARAGE) {
        if (input.mouseMoved) m_garage.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_garage.onMouseDown(mx, my, m_profile);
        if (input.mouseJustReleased) m_garage.onMouseUp(mx, my, m_profile);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        if (input.enter) {
            startRace(m_selectedBiome);
            return;
        }
        if (input.mouseMoved) m_stageSelect.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_stageSelect.onMouseDown(mx, my, m_profile);
        if (input.mouseJustReleased) m_stageSelect.onMouseUp(mx, my);
    } else if (m_currentState == StateType::GAMEPLAY) {
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

        // Map inputs: either keyboard or touch/mouse pedal hold
        bool gasActive = input.gas || (input.mouseDown && m_hud.isGasPressed());
        bool brakeActive = input.brake || (input.mouseDown && m_hud.isBrakePressed());
        bool reverseActive = input.reverse || (input.mouseDown && m_hud.isReversePressed());

        m_hud.setGasVirtualPressed(gasActive);
        m_hud.setBrakeVirtualPressed(brakeActive);
        m_hud.setReverseVirtualPressed(reverseActive);

        m_physics.vehicle().applyInput(gasActive ? 1.0f : 0.0f,
                                       brakeActive ? 1.0f : 0.0f,
                                       reverseActive ? 1.0f : 0.0f);
    } else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        if (m_physics.winner() != 0) {
            if (input.restart || input.enter) {
                startMultiplayerRace(m_selectedMpBiome);
                return;
            }
            if (input.pause) {
                m_isMultiplayerFlow = false;
                changeState(StateType::MAIN_MENU);
                return;
            }
        }
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }
        if (input.restart) {
            startMultiplayerRace(m_selectedMpBiome);
            return;
        }

        // Player 1 controls (WASD)
        m_physics.vehicle().applyInput(input.p1.gas ? 1.0f : 0.0f,
                                       input.p1.brake ? 1.0f : 0.0f,
                                       input.p1.reverse ? 1.0f : 0.0f,
                                       input.p1.tiltLeft ? 1.0f : 0.0f,
                                       input.p1.tiltRight ? 1.0f : 0.0f);

        // Player 2 controls (Arrow Keys)
        m_physics.vehicle2().applyInput(input.p2.gas ? 1.0f : 0.0f,
                                        input.p2.brake ? 1.0f : 0.0f,
                                        input.p2.reverse ? 1.0f : 0.0f,
                                        input.p2.tiltLeft ? 1.0f : 0.0f,
                                        input.p2.tiltRight ? 1.0f : 0.0f);
    } else if (m_currentState == StateType::ONLINE_RACE) {
        if (m_onlineRaceFinished) {
            if (input.restart || input.enter) {
                if (m_isOnlineHost) {
                    m_server.broadcastRematch();
                    startNetworkMultiplayer(true, m_selectedBiome);
                    return;
                }
            }
            if (input.pause) {
                if (m_isOnlineHost) {
                    m_server.broadcastReturnToLobby();
                } else if (m_isOnlineClient) {
                    m_client.sendReturnToLobby();
                }
                changeState(StateType::ONLINE_LOBBY);
                return;
            }
        }
        if (input.pause) {
            if (m_isOnlineHost) {
                m_server.broadcastReturnToLobby();
            } else if (m_isOnlineClient) {
                m_client.sendReturnToLobby();
            }
            changeState(StateType::ONLINE_LOBBY);
            return;
        }

        if (input.mouseMoved) m_hud.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_hud.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_hud.onMouseUp(mx, my);

        bool gasActive = input.gas || input.p1.gas || (input.mouseDown && m_hud.isGasPressed());
        bool brakeActive = input.brake || input.p1.brake || (input.mouseDown && m_hud.isBrakePressed());
        bool reverseActive = input.reverse || input.p1.reverse || (input.mouseDown && m_hud.isReversePressed());

        m_hud.setGasVirtualPressed(gasActive);
        m_hud.setBrakeVirtualPressed(brakeActive);
        m_hud.setReverseVirtualPressed(reverseActive);

        m_physics.vehicle().applyInput(gasActive ? 1.0f : 0.0f,
                                       brakeActive ? 1.0f : 0.0f,
                                       reverseActive ? 1.0f : 0.0f,
                                       input.p1.tiltLeft ? 1.0f : 0.0f,
                                       input.p1.tiltRight ? 1.0f : 0.0f);
    } else if (m_currentState == StateType::PAUSED) {
        if (input.pause || input.enter) {
            if (m_physics.isMultiplayer()) {
                changeState(StateType::MULTIPLAYER_RACE);
            } else {
                changeState(StateType::GAMEPLAY);
            }
            return;
        }
        if (input.restart) {
            if (m_physics.isMultiplayer()) {
                startMultiplayerRace();
            } else {
                startRace(m_selectedBiome);
            }
            return;
        }
        if (input.mouseMoved) m_pauseOverlay.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_pauseOverlay.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_pauseOverlay.onMouseUp(mx, my);
    } else if (m_currentState == StateType::GAME_OVER) {
        if (input.restart || input.enter) {
            if (m_physics.isMultiplayer()) {
                startMultiplayerRace();
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

    bool isPaused = (m_currentState == StateType::PAUSED);
    if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE ||
        m_currentState == StateType::ONLINE_RACE ||
        m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
        m_terrainRasterizer.dayNightCycle().update(dt, isPaused);
    }

    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.update(dt);
    } else if (m_currentState == StateType::ONLINE_LOBBY) {
        m_lobbyScreen.update(dt);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.update(dt);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.update(dt);
    } else if (m_currentState == StateType::GAME_OVER) {
        m_gameOverScreen.update(dt);
        m_physics.step(dt, m_profile);
        const auto& car = m_physics.vehicle();
        m_camera.update(dt, car.chassisPos(), car.chassisVel().x);
        m_particleSystem.update(dt);
    } else if (m_currentState == StateType::PAUSED) {
        m_pauseOverlay.update(dt);
    } else if (m_currentState == StateType::GAMEPLAY) {
        m_gameTime += dt;

        m_physics.step(dt, m_profile);

        const auto& car = m_physics.vehicle();
        m_camera.update(dt, car.chassisPos(), car.chassisVel().x);

        const std::string& currentBiome = m_physics.terrain().getBiome();
        bool isElectric = (car.vehicleType() == Physics::VehicleType::ELECTRIC_OFFROADER);
        bool isVacuum = (currentBiome == "moon");

        m_smokeTimer -= dt;
        if (!isElectric && !isVacuum && car.fuel() > 0.0f && m_smokeTimer <= 0.0f) {
            m_smokeTimer = (car.getSpeedKmh() > 5.0f) ? 0.045f : 0.09f;
            float cosA = std::cos(car.chassisAngle());
            float sinA = std::sin(car.chassisAngle());
            Physics::Vec2 pipeLocal(car.rearMountOffset().x * 0.95f, car.rearMountOffset().y * 0.4f);
            Physics::Vec2 pipeWorld = car.chassisPos() + Physics::Vec2(pipeLocal.x * cosA - pipeLocal.y * sinA,
                                                                       pipeLocal.x * sinA + pipeLocal.y * cosA);
            m_particleSystem.emitSmoke(pipeWorld, car.chassisVel());
        }

        if (car.isRearOnGround()) {
            m_particleSystem.emitWheelSpray(car.rearWheelPos(), car.rearWheelRadius(),
                                            car.rearContactTangent(), car.rearSlipSpeed(),
                                            car.rearWheelAngularVel(), car.rearNormalForce(),
                                            currentBiome);
        }
        if (car.isFrontOnGround()) {
            m_particleSystem.emitWheelSpray(car.frontWheelPos(), car.frontWheelRadius(),
                                            car.frontContactTangent(), car.frontSlipSpeed(),
                                            car.frontWheelAngularVel(), car.frontNormalForce(),
                                            currentBiome);
        }

        if (car.isChassisContact()) {
            m_particleSystem.emitSparkles(car.chassisPos() + Physics::Vec2(0.0f, -0.2f));
        }
        m_particleSystem.update(dt);

        // Update HUD Telemetry
        m_hud.setFuel(car.fuel());
        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.setCoins(m_profile.coins() + m_physics.coinsCollected());
        m_hud.setCheckpointProgress(m_physics.currentCheckpoint(), 5);
        const auto* nextCp = m_physics.terrain().getNextCheckpoint(m_physics.distanceReached());
        if (nextCp) {
            m_hud.setNextCheckpoint(nextCp->name, nextCp->distance);
        }
        m_hud.update(dt);

        // Single Player Game Over Check
        if (m_physics.isGameOver()) {
            float dist = m_physics.distanceReached();
            int coinsEarned = m_physics.coinsCollected();
            m_profile.addCoins(coinsEarned);

            float prevRec = m_profile.getRecordDistance(m_selectedBiome);
            bool isNewRec = (dist > prevRec);
            m_profile.recordDistance(m_selectedBiome, dist);

            m_gameOverScreen.setCrashReason(m_physics.gameOverReason());
            m_gameOverScreen.setStats(dist, coinsEarned, m_physics.flipsCompleted(), m_physics.totalAirTime(), isNewRec);

            m_camera.addTrauma(0.9f);
            m_audio.playCrash();
            changeState(StateType::GAME_OVER);
        }
    } else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        m_gameTime += dt;

        m_physics.step(dt, m_profile);

        const auto& car1 = m_physics.vehicle();
        const auto& car2 = m_physics.vehicle2();

        m_camera.update(dt, car1.chassisPos(), car1.chassisVel().x);
        m_camera2.update(dt, car2.chassisPos(), car2.chassisVel().x);

        const std::string& currentBiome = m_physics.terrain().getBiome();

        // Exhaust smoke for Vehicle 1
        m_smokeTimer -= dt;
        if (car1.fuel() > 0.0f && m_smokeTimer <= 0.0f) {
            m_smokeTimer = (car1.getSpeedKmh() > 5.0f) ? 0.045f : 0.09f;
            float cosA = std::cos(car1.chassisAngle());
            float sinA = std::sin(car1.chassisAngle());
            Physics::Vec2 pLocal(car1.rearMountOffset().x * 0.95f, car1.rearMountOffset().y * 0.4f);
            Physics::Vec2 pWorld = car1.chassisPos() + Physics::Vec2(pLocal.x * cosA - pLocal.y * sinA, pLocal.x * sinA + pLocal.y * cosA);
            m_particleSystem.emitSmoke(pWorld, car1.chassisVel());
        }

        // Exhaust smoke for Vehicle 2
        m_smokeTimer2 -= dt;
        if (car2.fuel() > 0.0f && m_smokeTimer2 <= 0.0f) {
            m_smokeTimer2 = (car2.getSpeedKmh() > 5.0f) ? 0.045f : 0.09f;
            float cosA = std::cos(car2.chassisAngle());
            float sinA = std::sin(car2.chassisAngle());
            Physics::Vec2 pLocal(car2.rearMountOffset().x * 0.95f, car2.rearMountOffset().y * 0.4f);
            Physics::Vec2 pWorld = car2.chassisPos() + Physics::Vec2(pLocal.x * cosA - pLocal.y * sinA, pLocal.x * sinA + pLocal.y * cosA);
            m_particleSystem.emitSmoke(pWorld, car2.chassisVel());
        }

        // Wheel spray for both
        if (car1.isRearOnGround()) {
            m_particleSystem.emitWheelSpray(car1.rearWheelPos(), car1.rearWheelRadius(),
                                            car1.rearContactTangent(), car1.rearSlipSpeed(),
                                            car1.rearWheelAngularVel(), car1.rearNormalForce(), currentBiome);
        }
        if (car2.isRearOnGround()) {
            m_particleSystem.emitWheelSpray(car2.rearWheelPos(), car2.rearWheelRadius(),
                                            car2.rearContactTangent(), car2.rearSlipSpeed(),
                                            car2.rearWheelAngularVel(), car2.rearNormalForce(), currentBiome);
        }

        m_particleSystem.update(dt);

        // Dynamic instrument cluster & fuel gauge simulation for split screens
        m_mpSpeedoP1.setValue(car1.getSpeedKmh());
        m_mpSpeedoP1.update(dt);
        m_mpTachoP1.setValue(car1.getEngineRpm() / 1000.0f);
        m_mpTachoP1.update(dt);
        m_mpFuelBarP1.setFuel(car1.fuel());
        m_mpFuelBarP1.update(dt);

        m_mpSpeedoP2.setValue(car2.getSpeedKmh());
        m_mpSpeedoP2.update(dt);
        m_mpTachoP2.setValue(car2.getEngineRpm() / 1000.0f);
        m_mpTachoP2.update(dt);
        m_mpFuelBarP2.setFuel(car2.fuel());
        m_mpFuelBarP2.update(dt);
    } else if (m_currentState == StateType::ONLINE_RACE) {
        m_gameTime += dt;

        m_physics.step(dt, m_profile);

        const auto& car = m_physics.vehicle();
        m_camera.update(dt, car.chassisPos(), car.chassisVel().x);

        // Interpolate remote opponent cars smoothly
        m_client.updateInterpolation(dt);

        const std::string& currentBiome = m_physics.terrain().getBiome();
        bool isElectric = (car.vehicleType() == Physics::VehicleType::ELECTRIC_OFFROADER);
        bool isVacuum = (currentBiome == "moon");

        // Smoke particles
        m_smokeTimer -= dt;
        if (!isElectric && !isVacuum && car.fuel() > 0.0f && m_smokeTimer <= 0.0f) {
            m_smokeTimer = (car.getSpeedKmh() > 5.0f) ? 0.045f : 0.09f;
            float cosA = std::cos(car.chassisAngle());
            float sinA = std::sin(car.chassisAngle());
            Physics::Vec2 pipeLocal(car.rearMountOffset().x * 0.95f, car.rearMountOffset().y * 0.4f);
            Physics::Vec2 pipeWorld = car.chassisPos() + Physics::Vec2(pipeLocal.x * cosA - pipeLocal.y * sinA,
                                                                       pipeLocal.x * sinA + pipeLocal.y * cosA);
            m_particleSystem.emitSmoke(pipeWorld, car.chassisVel());
        }

        // Wheel spray
        if (car.isRearOnGround()) {
            m_particleSystem.emitWheelSpray(car.rearWheelPos(), car.rearWheelRadius(),
                                            car.rearContactTangent(), car.rearSlipSpeed(),
                                            car.rearWheelAngularVel(), car.rearNormalForce(),
                                            currentBiome);
        }
        if (car.isFrontOnGround()) {
            m_particleSystem.emitWheelSpray(car.frontWheelPos(), car.frontWheelRadius(),
                                            car.frontContactTangent(), car.frontSlipSpeed(),
                                            car.frontWheelAngularVel(), car.frontNormalForce(),
                                            currentBiome);
        }
        if (car.isChassisContact()) {
            m_particleSystem.emitSparkles(car.chassisPos() + Physics::Vec2(0.0f, -0.2f));
        }
        m_particleSystem.update(dt);

        // Update HUD Telemetry
        m_hud.setFuel(car.fuel());
        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.setCoins(m_profile.coins() + m_physics.coinsCollected());
        m_hud.setCheckpointProgress(m_physics.currentCheckpoint(), 5);
        const auto* nextCp = m_physics.terrain().getNextCheckpoint(m_physics.distanceReached());
        if (nextCp) {
            m_hud.setNextCheckpoint(nextCp->name, nextCp->distance);
        }
        m_hud.update(dt);

        // Broadcast telemetry at ~30 Hz (every 0.033s)
        m_onlineTelemetryTimer += dt;
        if (m_onlineTelemetryTimer >= 0.033f) {
            m_onlineTelemetryTimer = 0.0f;

            Network::RemotePlayerState myState;
            myState.id = m_isOnlineHost ? 0 : m_client.getLocalPlayerId();
            myState.name = m_isOnlineHost ? "Host (You)" : m_client.getLocalPlayerName();
            myState.color = m_isOnlineHost ? 0xFFFF3D00 : m_client.getAssignedColor();
            myState.vehicleType = static_cast<int>(car.vehicleType());
            myState.driverType = static_cast<int>(car.driverType());

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
            myState.distance = m_physics.distanceReached();
            myState.isAirborne = car.isAirborne();
            myState.isCrashed = m_physics.isGameOver();
            myState.isFinished = (myState.distance >= 1500.0f);
            myState.finishTime = m_gameTime;

            if (m_isOnlineHost) {
                m_server.updateHostTelemetry(myState);
            } else if (m_isOnlineClient) {
                m_client.sendTelemetry(myState);
            }
        }

        checkOnlineRaceRules();
    }

    m_audio.update(dt, m_physics.vehicle(), m_physics.terrain(), m_currentState);
}

void GameStateManager::renderCompactHud(Graphics::Framebuffer& fb, const Physics::Vehicle& veh, float dist,
                                        const std::string& pLabel, uint32_t tagCol, const std::string& ctrlHint,
                                        UI::AnalogGauge& speedo, UI::AnalogGauge& tacho, UI::FuelBar& fuelBar) {
    int W = fb.width();
    int H = fb.height();

    // 1. Racer Identity Pill (Top-Left)
    int labelW = Graphics::RasterFont::getTextWidth(pLabel, 1);
    int pillW = std::max(220, labelW + 48);
    int pillH = 26;
    UI::UIComponents::drawArcadePanel(fb, 12, 10, pillW, pillH, 0xDD090F1A, UI::Theme::CARD_BORDER, tagCol, 2, true, true);
    fb.fillCircle(24, 23, 5, tagCol);
    Graphics::RasterFont::drawString(fb, 36, 17, pLabel, UI::Theme::TEXT_WHITE, 1);

    // 2. Dynamic Segmented LED Fuel Gauge & Live Readout (Under Identity)
    fuelBar.render(fb);
    std::string fuelPctStr = "FUEL " + std::to_string(static_cast<int>(veh.fuel())) + "%";
    uint32_t fuelTextCol = veh.isLowFuel() ? 0xFFFF1744 : (veh.fuel() > 50.0f) ? UI::Theme::GREEN_GAS : UI::Theme::GOLD;
    Graphics::RasterFont::drawString(fb, 150, 43, fuelPctStr, fuelTextCol, 1);

    // Controls hint
    Graphics::RasterFont::drawString(fb, 14, 62, ctrlHint, 0xBBFFFFFF, 1);

    // Flashing Low Fuel Alert / Out of Fuel Banner
    if (veh.isOutOfFuel()) {
        UI::UIComponents::drawArcadePanel(fb, W / 2 - 90, 40, 180, 22, 0xEE1E0508, 0xFFFF1744, 0xFFFF1744, 1, false, false);
        Graphics::RasterFont::drawStringCentered(fb, W / 2, 45, "! OUT OF FUEL !", 0xFFFF1744, 1);
    } else if (veh.isLowFuel()) {
        float blink = std::sin(m_gameTime * 14.0f);
        if (blink > 0.0f) {
            UI::UIComponents::drawArcadePanel(fb, W / 2 - 105, 40, 210, 22, 0xEE1E0508, 0xFFFF1744, 0xFFFF1744, 1, false, false);
            Graphics::RasterFont::drawStringCentered(fb, W / 2, 45, "! LOW FUEL WARNING !", 0xFFFF1744, 1);
        }
    }

    // 3. Distance Odometer & Progress (Top-Center)
    int distInt = static_cast<int>(dist);
    char distBuf[32];
    std::snprintf(distBuf, sizeof(distBuf), "%05dm", distInt);
    int distBoxW = 140;
    int distBoxH = 26;
    int distBoxX = W / 2 - distBoxW / 2;
    UI::UIComponents::drawArcadePanel(fb, distBoxX, 10, distBoxW, distBoxH, 0xDD090F1A, UI::Theme::CARD_BORDER, UI::Theme::GOLD, 2, true, true);
    Graphics::RasterFont::drawStringCentered(fb, W / 2, 16, distBuf, UI::Theme::GOLD, 2);

    // 4. Dynamic Dual Circular Analog Instrument Cluster (Bottom-Right Cockpit Binnacle)
    int binnacleW = 120;
    int binnacleH = 62;
    int binnacleX = W - binnacleW - 12;
    int binnacleY = H - binnacleH - 8;

    fb.fillRect(binnacleX, binnacleY, binnacleW, binnacleH, 0xEE090E18);
    fb.drawRect(binnacleX, binnacleY, binnacleW, binnacleH, UI::Theme::CARD_BORDER);
    fb.fillRect(binnacleX + 1, binnacleY + 1, binnacleW - 2, 1, 0x44FFFFFF);

    speedo.render(fb);
    tacho.render(fb);

    // Digital Speedometer Readout above gauges
    int speedKmh = static_cast<int>(veh.getSpeedKmh());
    std::string speedStr = std::to_string(speedKmh) + " KM/H";
    Graphics::RasterFont::drawStringCentered(fb, binnacleX + 32, binnacleY + 4, speedStr, UI::Theme::TEXT_WHITE, 1);
    int rpmVal = static_cast<int>(veh.getEngineRpm());
    std::string rpmStr = std::to_string(rpmVal) + " RPM";
    Graphics::RasterFont::drawStringCentered(fb, binnacleX + 88, binnacleY + 4, rpmStr, (rpmVal > 7000) ? 0xFFFF1744 : UI::Theme::CYAN_UPGRADE, 1);
}

void GameStateManager::renderMultiplayer(Graphics::Framebuffer& fb, float alpha) {
    m_camera.setRenderAlpha(alpha);
    m_camera2.setRenderAlpha(alpha);

    // --- TOP VIEWPORT: PLAYER 1 ---
    m_terrainRasterizer.render(m_fbP1, m_camera, m_physics.terrain(), m_gameTime, &m_physics.itemsCollectedP1());
    m_particleSystem.render(m_fbP1, m_camera);

    // 1. Render Opponent (P2) behind Player 1 with translucent competitor ghost styling (0.55 opacity)
    Graphics::SpriteRenderer::renderVehicle(m_fbP1, m_camera, m_physics.vehicle2(), alpha, 0.55f);

    // 2. Render Player 1 (primary) crisp, solid and fully opaque ON TOP
    Graphics::SpriteRenderer::renderVehicle(m_fbP1, m_camera, m_physics.vehicle(), alpha, 1.0f);

    // Draw P2 tag in Viewport 1 if nearby or radar arrow if off-screen
    Physics::Vec2 p2Scr = m_camera.worldToScreen(m_physics.vehicle2().chassisPos());
    float relDistP2 = m_physics.vehicle2().chassisPos().x - m_physics.vehicle().chassisPos().x;
    if (p2Scr.x >= 20 && p2Scr.x <= m_fbP1.width() - 20 && p2Scr.y >= 15 && p2Scr.y <= m_fbP1.height() - 15) {
        Graphics::RasterFont::drawStringCentered(m_fbP1, static_cast<int>(p2Scr.x), static_cast<int>(p2Scr.y - 34), "[P2]", 0xFF00E5FF, 1);
    } else {
        int indY = m_fbP1.height() / 2;
        if (relDistP2 > 0.0f) {
            std::string indStr = "P2 +" + std::to_string(static_cast<int>(relDistP2)) + "m >";
            int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
            UI::UIComponents::drawArcadePanel(m_fbP1, m_fbP1.width() - tw - 18, indY - 10, tw + 14, 20, 0xDD090F1A, UI::Theme::CARD_BORDER, 0xFF00E5FF, 0, false, true);
            Graphics::RasterFont::drawString(m_fbP1, m_fbP1.width() - tw - 11, indY - 5, indStr, 0xFF00E5FF, 1);
        } else {
            std::string indStr = "< P2 " + std::to_string(static_cast<int>(relDistP2)) + "m";
            int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
            UI::UIComponents::drawArcadePanel(m_fbP1, 10, indY - 10, tw + 14, 20, 0xDD090F1A, UI::Theme::CARD_BORDER, 0xFF00E5FF, 0, false, true);
            Graphics::RasterFont::drawString(m_fbP1, 17, indY - 5, indStr, 0xFF00E5FF, 1);
        }
    }
    std::string vehName1 = m_physics.vehicle().config().name;
    renderCompactHud(m_fbP1, m_physics.vehicle(), m_physics.distanceReached(), "P1 - " + vehName1, 0xFFFF3D00, "P1: [W/D] GAS  [S/A] BRK/REV", m_mpSpeedoP1, m_mpTachoP1, m_mpFuelBarP1);

    // --- BOTTOM VIEWPORT: PLAYER 2 ---
    m_terrainRasterizer.render(m_fbP2, m_camera2, m_physics.terrain(), m_gameTime, &m_physics.itemsCollectedP2());
    m_particleSystem.render(m_fbP2, m_camera2);

    // 1. Render Opponent (P1) behind Player 2 with translucent competitor ghost styling (0.55 opacity)
    Graphics::SpriteRenderer::renderVehicle(m_fbP2, m_camera2, m_physics.vehicle(), alpha, 0.55f);

    // 2. Render Player 2 (primary) crisp, solid and fully opaque ON TOP
    Graphics::SpriteRenderer::renderVehicle(m_fbP2, m_camera2, m_physics.vehicle2(), alpha, 1.0f);

    // Draw P1 tag in Viewport 2 if nearby or radar arrow if off-screen
    Physics::Vec2 p1Scr = m_camera2.worldToScreen(m_physics.vehicle().chassisPos());
    float relDistP1 = m_physics.vehicle().chassisPos().x - m_physics.vehicle2().chassisPos().x;
    if (p1Scr.x >= 20 && p1Scr.x <= m_fbP2.width() - 20 && p1Scr.y >= 15 && p1Scr.y <= m_fbP2.height() - 15) {
        Graphics::RasterFont::drawStringCentered(m_fbP2, static_cast<int>(p1Scr.x), static_cast<int>(p1Scr.y - 34), "[P1]", 0xFFFF3D00, 1);
    } else {
        int indY = m_fbP2.height() / 2;
        if (relDistP1 > 0.0f) {
            std::string indStr = "P1 +" + std::to_string(static_cast<int>(relDistP1)) + "m >";
            int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
            UI::UIComponents::drawArcadePanel(m_fbP2, m_fbP2.width() - tw - 18, indY - 10, tw + 14, 20, 0xDD090F1A, UI::Theme::CARD_BORDER, 0xFFFF3D00, 0, false, true);
            Graphics::RasterFont::drawString(m_fbP2, m_fbP2.width() - tw - 11, indY - 5, indStr, 0xFFFF3D00, 1);
        } else {
            std::string indStr = "< P1 " + std::to_string(static_cast<int>(relDistP1)) + "m";
            int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
            UI::UIComponents::drawArcadePanel(m_fbP2, 10, indY - 10, tw + 14, 20, 0xDD090F1A, UI::Theme::CARD_BORDER, 0xFFFF3D00, 0, false, true);
            Graphics::RasterFont::drawString(m_fbP2, 17, indY - 5, indStr, 0xFFFF3D00, 1);
        }
    }
    std::string vehName2 = m_physics.vehicle2().config().name;
    renderCompactHud(m_fbP2, m_physics.vehicle2(), m_physics.distanceReached2(), "P2 - " + vehName2, 0xFF00E5FF, "P2: [ARROWS] UP/RT=GAS  DN/LT=BRK/REV", m_mpSpeedoP2, m_mpTachoP2, m_mpFuelBarP2);

    // Compose split-screen viewports onto master framebuffer
    fb.blit(0, 0, m_fbP1);
    fb.blit(0, 274, m_fbP2);

    // --- CENTER DIVIDER BAR & "VS" MEDALLION ---
    int cx = m_width / 2;
    fb.fillRect(0, 266, m_width, 8, 0xFF0A0F18);
    fb.drawLine(0, 266, m_width, 266, 0xFFFF3D00); // Red P1 Accent
    fb.drawLine(0, 273, m_width, 273, 0xFF00E5FF); // Cyan P2 Accent

    // Circular VS Badge
    fb.fillCircle(cx, 270, 14, 0xFF0E1826);
    fb.drawCircle(cx, 270, 14, UI::Theme::GOLD);
    Graphics::RasterFont::drawStringCentered(fb, cx, 266, "VS", UI::Theme::GOLD, 1);

    // Live Race Leader Status Tag
    const std::string& status = m_physics.mpStatusMessage();
    int sw = Graphics::RasterFont::getTextWidth(status, 1) + 20;
    int sx = cx - sw / 2;
    int sy = 248;
    UI::UIComponents::drawArcadePanel(fb, sx, sy, sw, 18, 0xF00A111D, UI::Theme::CARD_BORDER, UI::Theme::GOLD, 0, false, true);
    Graphics::RasterFont::drawStringCentered(fb, cx, sy + 4, status, UI::Theme::GOLD, 1);

    // --- VICTORY POPUP OVERLAY ---
    if (m_physics.winner() != 0) {
        int vw = 500;
        int vh = 145;
        int vx = cx - vw / 2;
        int vy = 197;
        uint32_t winCol = (m_physics.winner() == 1) ? 0xFFFF3D00 :
                          (m_physics.winner() == 2) ? 0xFF00E5FF : UI::Theme::GOLD;

        UI::UIComponents::drawArcadePanel(fb, vx, vy, vw, vh, 0xF4080D18, UI::Theme::CARD_BORDER, winCol, 5, true, true);

        std::string winTitle = (m_physics.winner() == 1) ? "PLAYER 1 WINS!" :
                               (m_physics.winner() == 2) ? "PLAYER 2 WINS!" : "DEAD HEAT DRAW!";
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 18, winTitle, winCol, 3);
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 50, m_physics.mpStatusMessage(), UI::Theme::TEXT_WHITE, 1);

        // Score summary
        int p1d = static_cast<int>(m_physics.distanceReached());
        int p2d = static_cast<int>(m_physics.distanceReached2());
        std::string statStr = "P1: " + UI::UIComponents::formatNumber(p1d) + "m   vs   P2: " + UI::UIComponents::formatNumber(p2d) + "m";
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 75, statStr, UI::Theme::GOLD, 2);

        // Action instructions
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 112, "PRESS [R] FOR REMATCH   |   PRESS [ESC] FOR MENU", UI::Theme::TEXT_MUTED, 1);
    }
}

void GameStateManager::checkOnlineRaceRules() {
    if (m_onlineRaceFinished) return;

    const float FINISH_LINE = 1500.0f;
    float myDist = m_physics.distanceReached();

    // 1. Check if local player reached finish line
    if (myDist >= FINISH_LINE) {
        m_onlineRaceFinished = true;
        m_onlineWinnerId = m_isOnlineHost ? 0 : m_client.getLocalPlayerId();
        m_onlineWinnerName = m_isOnlineHost ? "YOU (HOST)" : m_client.getLocalPlayerName();
        m_onlineWinnerColor = m_isOnlineHost ? 0xFFFF3D00 : m_client.getAssignedColor();
        m_onlineWinReason = "CROSSES THE FINISH LINE FIRST!";
        if (m_isOnlineClient) {
            m_client.sendPlayerFinished(myDist, m_gameTime);
        }
        return;
    }

    // 2. Check if any remote opponent reached finish line
    for (const auto& pair : m_onlineOpponents) {
        const auto& opp = pair.second;
        if (opp.isFinished || opp.distance >= FINISH_LINE) {
            m_onlineRaceFinished = true;
            m_onlineWinnerId = opp.id;
            m_onlineWinnerName = opp.name;
            m_onlineWinnerColor = opp.color;
            m_onlineWinReason = "CROSSES THE FINISH LINE FIRST!";
            return;
        }
    }

    // 3. Check if all racers crashed or ran out of fuel
    bool localCrashed = m_physics.isGameOver();
    bool allOpponentsCrashed = !m_onlineOpponents.empty();
    for (const auto& pair : m_onlineOpponents) {
        if (!pair.second.isCrashed) {
            allOpponentsCrashed = false;
            break;
        }
    }

    if (localCrashed && allOpponentsCrashed) {
        m_onlineRaceFinished = true;
        float maxDist = myDist;
        int winId = m_isOnlineHost ? 0 : m_client.getLocalPlayerId();
        std::string winName = m_isOnlineHost ? "YOU (HOST)" : m_client.getLocalPlayerName();
        uint32_t winCol = m_isOnlineHost ? 0xFFFF3D00 : m_client.getAssignedColor();

        for (const auto& pair : m_onlineOpponents) {
            if (pair.second.distance > maxDist) {
                maxDist = pair.second.distance;
                winId = pair.second.id;
                winName = pair.second.name;
                winCol = pair.second.color;
            }
        }

        m_onlineWinnerId = winId;
        m_onlineWinnerName = winName;
        m_onlineWinnerColor = winCol;
        m_onlineWinReason = "GREATEST DISTANCE (" + std::to_string(static_cast<int>(maxDist)) + "m)";
    }
}

void GameStateManager::renderOverheadTag(Graphics::Framebuffer& fb, const Graphics::Camera& cam,
                                        const Physics::Vec2& worldPos, const std::string& name,
                                        uint32_t color, float distDiff) {
    Physics::Vec2 scr = cam.worldToScreen({worldPos.x, worldPos.y + 1.15f});
    int sx = static_cast<int>(scr.x);
    int sy = static_cast<int>(scr.y);

    if (sx < 20 || sx > m_width - 20 || sy < 20 || sy > m_height - 20) return;

    std::string tag = "[" + name + "]";
    int tagW = Graphics::RasterFont::getTextWidth(tag, 1) + 12;
    int tagH = 18;
    int bx = sx - tagW / 2;
    int by = sy - tagH - 4;

    UI::UIComponents::drawArcadePanel(fb, bx, by, tagW, tagH, 0xEE090F1A, UI::Theme::CARD_BORDER, color, 1, false, true);
    Graphics::RasterFont::drawStringCentered(fb, sx, by + 4, tag, color, 1);

    std::string diffStr = (distDiff >= 0.0f ? "+" : "") + std::to_string(static_cast<int>(distDiff)) + "m";
    uint32_t diffCol = (distDiff >= 0.0f) ? UI::Theme::GOLD : UI::Theme::CYAN_UPGRADE;
    Graphics::RasterFont::drawStringCentered(fb, sx, by + 19, diffStr, diffCol, 1);
}

void GameStateManager::renderOnlineRace(Graphics::Framebuffer& fb, float alpha) {
    m_camera.setRenderAlpha(alpha);

    // 1. World Terrain & Parallax
    m_terrainRasterizer.render(fb, m_camera, m_physics.terrain(), m_gameTime);

    // 2. Particles
    m_particleSystem.render(fb, m_camera);

    // 3. Remote Opponent Vehicles
    for (const auto& pair : m_onlineOpponents) {
        const auto& opp = pair.second;
        Graphics::SpriteRenderer::renderRemoteVehicle(fb, m_camera, opp, 0.95f);

        float distDiff = opp.distance - m_physics.distanceReached();
        renderOverheadTag(fb, m_camera, {opp.posX, opp.posY}, opp.name, opp.color, distDiff);

        // Off-screen indicator arrows
        Physics::Vec2 oppScr = m_camera.worldToScreen({opp.posX, opp.posY});
        if (oppScr.x < 0 || oppScr.x > m_width) {
            int indY = m_height / 2;
            if (oppScr.x > m_width) {
                std::string indStr = opp.name + " +" + std::to_string(static_cast<int>(distDiff)) + "m >";
                int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
                UI::UIComponents::drawArcadePanel(fb, m_width - tw - 20, indY - 10, tw + 16, 20, 0xDD090F1A, UI::Theme::CARD_BORDER, opp.color, 0, false, true);
                Graphics::RasterFont::drawString(fb, m_width - tw - 12, indY - 5, indStr, opp.color, 1);
            } else {
                std::string indStr = "< " + opp.name + " " + std::to_string(static_cast<int>(distDiff)) + "m";
                int tw = Graphics::RasterFont::getTextWidth(indStr, 1);
                UI::UIComponents::drawArcadePanel(fb, 10, indY - 10, tw + 16, 20, 0xDD090F1A, UI::Theme::CARD_BORDER, opp.color, 0, false, true);
                Graphics::RasterFont::drawString(fb, 18, indY - 5, indStr, opp.color, 1);
            }
        }
    }

    // 4. Local Player Vehicle
    Graphics::SpriteRenderer::renderVehicle(fb, m_camera, m_physics.vehicle(), alpha, 1.0f);

    // 5. Minimap Radar Tracker at the top
    std::vector<UI::RadarRacer> racers;
    std::string myName = m_isOnlineHost ? "YOU (HOST)" : m_client.getLocalPlayerName();
    uint32_t myCol = m_isOnlineHost ? 0xFFFF3D00 : m_client.getAssignedColor();
    racers.push_back({myName, m_physics.distanceReached(), myCol, true, m_physics.isGameOver()});

    for (const auto& pair : m_onlineOpponents) {
        racers.push_back({pair.second.name, pair.second.distance, pair.second.color, false, pair.second.isCrashed});
    }
    m_minimapRadar.render(fb, racers);

    // 6. HUD Telemetry
    m_hud.render(fb);

    // 7. Victory Overlay Banner
    if (m_onlineRaceFinished) {
        int cx = m_width / 2;
        int vw = 520;
        int vh = 160;
        int vx = cx - vw / 2;
        int vy = m_height / 2 - vh / 2;

        UI::UIComponents::drawArcadePanel(fb, vx, vy, vw, vh, 0xF4080D18, UI::Theme::CARD_BORDER, m_onlineWinnerColor, 5, true, true);

        std::string winTitle = m_onlineWinnerName + " WINS!";
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 18, winTitle, m_onlineWinnerColor, 3);
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 54, m_onlineWinReason, UI::Theme::TEXT_WHITE, 1);

        int myD = static_cast<int>(m_physics.distanceReached());
        std::string statStr = "YOUR DISTANCE: " + UI::UIComponents::formatNumber(myD) + "m";
        Graphics::RasterFont::drawStringCentered(fb, cx, vy + 82, statStr, UI::Theme::GOLD, 2);

        if (m_isOnlineHost) {
            Graphics::RasterFont::drawStringCentered(fb, cx, vy + 124, "HOST: [R/ENTER] REMATCH  |  [ESC] RETURN TO LOBBY", UI::Theme::CYAN_UPGRADE, 1);
        } else {
            Graphics::RasterFont::drawStringCentered(fb, cx, vy + 124, "CLIENT: WAITING FOR HOST  |  [ESC] LEAVE TO LOBBY", UI::Theme::TEXT_MUTED, 1);
        }
    }
}

void GameStateManager::render(Graphics::Framebuffer& fb, float alpha) {
    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.render(fb, m_profile);
    } else if (m_currentState == StateType::ONLINE_LOBBY) {
        m_lobbyScreen.render(fb, &m_server, &m_client);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.render(fb, m_profile);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.render(fb, m_profile);
    } else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        renderMultiplayer(fb, alpha);
    } else if (m_currentState == StateType::ONLINE_RACE) {
        renderOnlineRace(fb, alpha);
    } else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
        m_camera.setRenderAlpha(alpha);

        // 1. Render World (Terrain + Parallax)
        m_terrainRasterizer.render(fb, m_camera, m_physics.terrain(), m_gameTime);

        // 2. Render Particles (Smoke & Dirt & Sparks)
        m_particleSystem.render(fb, m_camera);

        // 3. Render Vehicle
        Graphics::SpriteRenderer::renderVehicle(fb, m_camera, m_physics.vehicle(), alpha);

        // 4. Render Telemetry HUD
        m_hud.render(fb);

        // 5. Overlays
        if (m_currentState == StateType::PAUSED) {
            m_pauseOverlay.render(fb);
        } else if (m_currentState == StateType::GAME_OVER) {
            m_gameOverScreen.render(fb);
        }
    }

    // 6. Smooth Screen Transition Fade Curtain
    if (m_transitionTimer > 0.0f) {
        float frac = m_transitionTimer / 0.12f;
        uint32_t a = static_cast<uint32_t>(frac * 160.0f);
        fb.fillRect(0, 0, m_width, m_height, (a << 24) | 0x060A12);
    }
}

} // namespace Core
