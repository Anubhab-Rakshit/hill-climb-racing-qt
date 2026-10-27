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
{
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
    m_gameOverScreen.setDimensions(width, height);
    m_accountScreen.setDimensions(width, height);
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
    m_gameOverScreen.setButtonSound(playClick);
    m_accountScreen.setButtonSound(playClick);
    m_lobbyScreen.setButtonSound(playClick);
    m_challengeScreen.setButtonSound(playClick);

    // 1. Main Menu
    m_mainMenu.setOnStart([this]() {
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnMultiplayer([this]() {
        changeState(StateType::NETWORK_LOBBY);
    });
    m_mainMenu.setOnGrandPrix([this]() {
        startGrandPrixChallenge();
    });
    m_mainMenu.setOnGarage([this]() {
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_mainMenu.setOnStages([this]() {
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnAccount([this]() {
        changeState(StateType::ACCOUNT_PROFILE);
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
        changeState(StateType::MAIN_MENU);
    });

    // 3. Lobby Screen
    m_lobbyScreen.setOnBack([this]() {
        m_server.stopServer();
        m_client.disconnectFromHost();
        changeState(StateType::MAIN_MENU);
    });
    m_lobbyScreen.setOnStartRace([this](const std::string& biomeId, bool isGP) {
        m_selectedBiome = biomeId;
        m_isGrandPrix = isGP;
        if (m_lobbyScreen.getMode() == UI::LobbyMode::HOST_LOBBY) {
            m_server.startServer(Network::DEFAULT_PORT);
            m_server.setHostPlayerInfo(AccountManager::instance().activeAccount()->username,
                                       static_cast<int>(m_profile.selectedVehicle()),
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
        changeState(StateType::MAIN_MENU);
    });
    m_stageSelect.setOnSelectStage([this](const std::string& stageId) {
        startRace(stageId);
    });

    // 6. Garage Screen
    m_garage.setOnBack([this]() {
        changeState(StateType::MAIN_MENU);
    });
    m_garage.setOnDrive([this]() {
        changeState(StateType::STAGE_SELECT);
    });

    // 7. HUD
    m_hud.setOnPauseClicked([this]() {
        if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE) {
            changeState(StateType::PAUSED);
        }
    });

    // 8. Pause Overlay
    m_pauseOverlay.setOnResume([this]() {
        if (m_isNetworkHost || m_isNetworkClient) {
            changeState(StateType::MULTIPLAYER_RACE);
        } else {
            changeState(StateType::GAMEPLAY);
        }
    });
    m_pauseOverlay.setOnRestart([this]() {
        if (m_isNetworkHost || m_isNetworkClient) {
            startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
        } else {
            startRace(m_selectedBiome);
        }
    });
    m_pauseOverlay.setOnGarage([this]() {
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_pauseOverlay.setOnMenu([this]() {
        changeState(StateType::MAIN_MENU);
    });

    // 9. Game Over Screen
    m_gameOverScreen.setOnRetry([this]() {
        if (m_isNetworkHost || m_isNetworkClient) {
            startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
        } else {
            startRace(m_selectedBiome);
        }
    });
    m_gameOverScreen.setOnGarage([this]() {
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_gameOverScreen.setOnMenu([this]() {
        changeState(StateType::MAIN_MENU);
    });

    // 10. Physics World Events
    m_physics.setOnItemCollected([this](Physics::ItemType type, int value) {
        Physics::Vec2 carScr = m_camera.worldToScreen(m_physics.vehicle().chassisPos());
        if (type == Physics::ItemType::COIN_BRONZE || 
            type == Physics::ItemType::COIN_SILVER || 
            type == Physics::ItemType::COIN_GOLD) {
            m_profile.addCoins(value);
            AccountManager::instance().addCoins(value);
            m_audio.playCoinPickup();
            uint32_t col = (type == Physics::ItemType::COIN_GOLD) ? UI::Theme::GOLD :
                           (type == Physics::ItemType::COIN_SILVER) ? 0xFFCFD8DC : 0xFFCD7F32;
            m_hud.addFloatingText("+" + std::to_string(value), carScr.x, carScr.y - 30.0f, col);
        } else if (type == Physics::ItemType::FUEL_CANISTER) {
            m_audio.playFuelPickup();
            m_hud.triggerFuelFlash();
            m_hud.addFloatingText("+FUEL", carScr.x, carScr.y - 35.0f, UI::Theme::GREEN_GAS);
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
        m_profile.recordStageCheckpoint(m_selectedBiome, cp.index);
        m_profile.addCoins(cp.coinReward);
        AccountManager::instance().addCoins(cp.coinReward);
        AccountManager::instance().addXP(250);
        m_audio.playStuntCheer();
        if (cp.isFinishLine) {
            m_profile.setStageCompleted(m_selectedBiome, true);
        }
    });

    m_devOverlay.setOnClose([this]() {
        m_showDevOverlay = false;
    });
}

void GameStateManager::setupNetworkCallbacks() {
    // --- SERVER CALLBACKS ---
    m_server.setOnTelemetryReceived([this](const Network::RemotePlayerState& st) {
        m_networkOpponents[st.id] = st;
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

    // --- CLIENT CALLBACKS ---
    m_client.setOnRaceStart([this](const std::string& biomeId) {
        startNetworkMultiplayer(false, biomeId);
    });

    m_client.setOnRemoteTelemetry([this](const Network::RemotePlayerState& st) {
        m_networkOpponents[st.id] = st;
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
}

void GameStateManager::changeState(StateType newState) {
    m_currentState = newState;
    m_transitionTimer = 0.12f;

    if (m_currentState == StateType::MAIN_MENU || m_currentState == StateType::GARAGE ||
        m_currentState == StateType::STAGE_SELECT || m_currentState == StateType::ACCOUNT_PROFILE ||
        m_currentState == StateType::NETWORK_LOBBY || m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        m_audio.setTargetMusic(Audio::MusicTrack::MENU);
    } else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE) {
        m_audio.setTargetMusic(Audio::MusicTrack::RACE);
    } else if (m_currentState == StateType::GAME_OVER) {
        m_audio.stopAllMusic();
    }
}

void GameStateManager::startRace(const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_isNetworkHost = false;
    m_isNetworkClient = false;
    m_networkOpponents.clear();

    m_camera.setScreenSize(m_width, m_height);
    m_physics.reset(biomeId, m_profile.selectedVehicle(), m_profile.selectedDriver(), false);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_minimapRadar.setCourseLength(1750.0f);
    changeState(StateType::GAMEPLAY);
}

void GameStateManager::startMultiplayerRace() {
    startNetworkMultiplayer(true, "countryside");
}

void GameStateManager::startNetworkMultiplayer(bool isHost, const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_isNetworkHost = isHost;
    m_isNetworkClient = !isHost;
    m_networkOpponents.clear();

    // Fullscreen single camera per player
    m_camera.setScreenSize(m_width, m_height);
    m_physics.reset(biomeId, m_profile.selectedVehicle(), m_profile.selectedDriver(), false);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_smokeTimer = 0.0f;
    m_netTelemetryTimer = 0.0f;
    m_minimapRadar.setCourseLength(1750.0f);
    changeState(StateType::MULTIPLAYER_RACE);
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
    else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::MULTIPLAYER_RACE) {
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }
        if (input.restart) {
            if (m_isNetworkHost || m_isNetworkClient) {
                startNetworkMultiplayer(m_isNetworkHost, m_selectedBiome);
            } else {
                startRace(m_selectedBiome);
            }
            return;
        }

        if (input.mouseMoved) m_hud.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_hud.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_hud.onMouseUp(mx, my);

        bool gasActive = input.gas || (input.mouseDown && m_hud.isGasPressed());
        bool brakeActive = input.brake || (input.mouseDown && m_hud.isBrakePressed());
        bool reverseActive = input.reverse || (input.mouseDown && m_hud.isReversePressed());

        // Evaluator Turbo Boost holding 'T'
        if (m_isEvaluatorMode) {
            // Evaluator turbo multiplier
        }

        m_hud.setGasVirtualPressed(gasActive);
        m_hud.setBrakeVirtualPressed(brakeActive);
        m_hud.setReverseVirtualPressed(reverseActive);

        m_physics.vehicle().applyInput(gasActive ? 1.0f : 0.0f,
                                       brakeActive ? 1.0f : 0.0f,
                                       reverseActive ? 1.0f : 0.0f);
    }
    else if (m_currentState == StateType::PAUSED) {
        if (input.pause || input.enter) {
            if (m_isNetworkHost || m_isNetworkClient) {
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
        m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
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
    } else if (m_currentState == StateType::NETWORK_LOBBY) {
        m_lobbyScreen.update(dt);
    } else if (m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        m_challengeScreen.update(dt);
    } else if (m_currentState == StateType::GAMEPLAY) {
        float effectiveDt = dt;
        if (m_isEvaluatorMode && m_evaluatorOverlay.isSlowMotion()) {
            effectiveDt *= 0.5f;
        }

        m_gameTime += effectiveDt;

        if (m_isEvaluatorMode && m_evaluatorOverlay.isInfiniteFuel()) {
            m_physics.vehicle().addFuel(100.0f);
        }

        m_physics.step(effectiveDt, m_profile);

        const auto& car = m_physics.vehicle();
        m_camera.update(effectiveDt, car.chassisPos(), car.chassisVel().x);

        // Exhaust smoke
        m_smokeTimer -= effectiveDt;
        if (car.fuel() > 0.0f && m_smokeTimer <= 0.0f) {
            m_smokeTimer = (car.getSpeedKmh() > 5.0f) ? 0.05f : 0.10f;
            float cosA = std::cos(car.chassisAngle());
            float sinA = std::sin(car.chassisAngle());
            Physics::Vec2 pLocal(car.rearMountOffset().x * 0.95f, car.rearMountOffset().y * 0.4f);
            Physics::Vec2 pWorld = car.chassisPos() + Physics::Vec2(pLocal.x * cosA - pLocal.y * sinA, pLocal.x * sinA + pLocal.y * cosA);
            m_particleSystem.emitSmoke(pWorld, car.chassisVel());
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

        // Update HUD Telemetry (Speedometer, Tachometer, Fuel Bar, Distance, Coins)
        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setFuel(car.fuel());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setCoins(m_profile.coins());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.update(effectiveDt);

        if (m_physics.isGameOver()) {
            float dist = m_physics.distanceReached();
            int coinsEarned = m_physics.coinsCollected();
            bool isNewRec = dist > m_profile.getRecordDistance(m_selectedBiome);

            m_profile.recordDistance(m_selectedBiome, dist);
            AccountManager::instance().recordDistance(m_selectedBiome, dist);
            AccountManager::instance().addXP(static_cast<int>(dist));

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
            m_camera.addTrauma(0.9f);
            if (m_physics.gameOverReason() == "DRIVER DOWN!") {
                m_audio.playNeckCrunch();
            } else {
                m_audio.playCrash();
            }
            changeState(StateType::GAME_OVER);
        }
    }
    else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        m_gameTime += dt;
        m_physics.step(dt, m_profile);

        const auto& car = m_physics.vehicle();
        m_camera.update(dt, car.chassisPos(), car.chassisVel().x);

        // Update interpolation for smooth opponent rendering
        m_client.updateInterpolation(dt);

        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setFuel(car.fuel());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setCoins(m_profile.coins());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.update(dt);

        // Broadcast local telemetry at 30 Hz
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
            myState.distance = m_physics.distanceReached();
            myState.isAirborne = car.isAirborne();
            myState.isCrashed = m_physics.isGameOver();
            myState.isFinished = (myState.distance >= 1500.0f);

            if (m_isNetworkHost) {
                m_server.updateHostTelemetry(myState);
            } else if (m_isNetworkClient) {
                m_client.sendTelemetry(myState);
            }
        }

        m_particleSystem.update(dt);
        checkNetworkRaceFinish();
    }

    m_audio.update(dt, m_physics.vehicle(), m_physics.terrain(), m_currentState);
}

void GameStateManager::checkNetworkRaceFinish() {
    float dist = m_physics.distanceReached();
    if (dist >= 1500.0f || m_physics.isGameOver()) {
        if (m_isGrandPrix) {
            int pts = (dist >= 1500.0f) ? 10 : (dist > 1000.0f) ? 8 : (dist > 500.0f) ? 6 : 4;
            m_gpStandings[0].points += pts;
            m_gpStandings[0].bestDistance = std::max(m_gpStandings[0].bestDistance, dist);

            bool isLast = (m_gpRound >= static_cast<int>(s_gpCircuit.size()));
            std::string nextTrk = isLast ? "FINALE" : s_gpCircuit[m_gpRound];
            m_challengeScreen.setStandings(m_gpStandings, m_gpRound, static_cast<int>(s_gpCircuit.size()),
                                           m_selectedBiome, nextTrk, isLast);
            changeState(StateType::GRAND_PRIX_CHALLENGE);
        }
    }
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
    renderNetworkRace(fb, alpha);
}

void GameStateManager::render(Graphics::Framebuffer& fb, float alpha) {
    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.render(fb, m_profile);
    } else if (m_currentState == StateType::ACCOUNT_PROFILE) {
        m_accountScreen.render(fb);
    } else if (m_currentState == StateType::NETWORK_LOBBY) {
        m_lobbyScreen.render(fb, &m_server, &m_client);
    } else if (m_currentState == StateType::GRAND_PRIX_CHALLENGE) {
        m_challengeScreen.render(fb);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.render(fb, m_profile);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.render(fb, m_profile);
    } else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        renderNetworkRace(fb, alpha);
    } else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
        m_camera.setRenderAlpha(alpha);

        // 1. World Terrain & Parallax
        m_terrainRasterizer.render(fb, m_camera, m_physics.terrain(), m_gameTime);

        // 2. Particles
        m_particleSystem.render(fb, m_camera);

        // 3. Vehicle
        Graphics::SpriteRenderer::renderVehicle(fb, m_camera, m_physics.vehicle(), alpha);

        // 4. Contracted Minimap / Radar Tracker
        std::vector<UI::RadarRacer> racers;
        std::string myName = AccountManager::instance().activeAccount()->username;
        racers.push_back({myName, m_physics.distanceReached(), Theme::GOLD, true, m_physics.isGameOver()});

        if (m_isGrandPrix) {
            // Draw benchmark competitor pins on minimap
            for (size_t i = 1; i < m_gpStandings.size(); ++i) {
                float aiDist = std::min(m_gpStandings[i].bestDistance, m_physics.distanceReached() + (static_cast<float>(i) * 45.0f - 70.0f));
                racers.push_back({m_gpStandings[i].name, aiDist, m_gpStandings[i].color, false, false});
            }
        }
        m_minimapRadar.render(fb, racers);

        // 5. Telemetry HUD
        m_hud.render(fb);

        // 6. Evaluator Mode Toolbar
        if (m_isEvaluatorMode) {
            const auto& cfg = Physics::VehicleRegistry::getConfig(m_profile.selectedVehicle());
            const auto& bSpec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
            m_evaluatorOverlay.render(fb, cfg.name, bSpec.name);
        }

        // 7. Overlays
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
}

} // namespace Core
