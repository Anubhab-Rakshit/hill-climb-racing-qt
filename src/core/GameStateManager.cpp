#include "GameStateManager.h"
#include "Sprite.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include <QApplication>
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

    m_mainMenu.setDimensions(width, height);
    m_garage.setDimensions(width, height);
    m_stageSelect.setDimensions(width, height);
    m_hud.setDimensions(width, height);
    m_pauseOverlay.setDimensions(width, height);
    m_gameOverScreen.setDimensions(width, height);
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

    // 1. Main Menu
    m_mainMenu.setOnStart([this]() {
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnMultiplayer([this]() {
        startMultiplayerRace();
    });
    m_mainMenu.setOnGarage([this]() {
        m_garage.setViewedVehicle(m_profile.selectedVehicle());
        changeState(StateType::GARAGE);
    });
    m_mainMenu.setOnStages([this]() {
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnQuit([]() {
        QApplication::quit();
    });

    // 2. Garage
    m_garage.setOnBack([this]() {
        changeState(StateType::MAIN_MENU);
    });
    m_garage.setOnDrive([this]() {
        changeState(StateType::STAGE_SELECT);
    });

    // 3. Stage Select
    m_stageSelect.setOnBack([this]() {
        changeState(StateType::MAIN_MENU);
    });
    m_stageSelect.setOnSelectStage([this](const std::string& stageId) {
        startRace(stageId);
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
            startMultiplayerRace();
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

    // 6. Game Over Screen
    m_gameOverScreen.setOnRetry([this]() {
        if (m_physics.isMultiplayer()) {
            startMultiplayerRace();
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

    // 7. Physics Event Callbacks
    m_physics.setOnStunt([this](const std::string& title, int bonusCoins) {
        m_hud.triggerStunt(title, bonusCoins);
        m_profile.addCoins(bonusCoins);
        m_audio.playStuntCheer();
    });

    m_physics.setOnItemCollected([this](Physics::ItemType type, int val) {
        Physics::Vec2 carPos = m_physics.vehicle().chassisPos();
        Physics::Vec2 scr = m_camera.worldToScreen(carPos);
        if (type == Physics::ItemType::FUEL_CANISTER) {
            m_audio.playFuelPickup();
            m_hud.triggerStunt("+100% FUEL!", 0, UI::Theme::GREEN_GAS);
            m_hud.addFloatingText("+100% FUEL!", scr.x, scr.y - 25.0f, UI::Theme::GREEN_GAS);
            m_hud.triggerFuelFlash();
            m_particleSystem.emitSparkles(carPos, UI::Theme::GREEN_GAS);
        } else {
            m_audio.playCoinPickup();
            uint32_t sparkCol = (type == Physics::ItemType::COIN_GOLD) ? UI::Theme::GOLD :
                                (type == Physics::ItemType::COIN_SILVER) ? 0xFFCFD8DC : 0xFFCD7F32;
            m_particleSystem.emitSparkles(carPos, sparkCol);
            m_hud.addFloatingText("+$" + std::to_string(val), scr.x, scr.y - 25.0f, sparkCol);
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

void GameStateManager::startMultiplayerRace() {
    m_selectedBiome = "countryside";
    m_camera.setScreenSize(m_width, 266);
    m_camera2.setScreenSize(m_width, 266);
    m_physics.reset("countryside",
                    Physics::VehicleType::MONSTER_TRUCK, Physics::DriverType::BILL,
                    true,
                    Physics::VehicleType::PICKUP_TRUCK, Physics::DriverType::SARAH);
    m_camera.reset(m_physics.vehicle().chassisPos());
    m_camera2.reset(m_physics.vehicle2().chassisPos());
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    m_smokeTimer = 0.0f;
    m_smokeTimer2 = 0.0f;
    changeState(StateType::MULTIPLAYER_RACE);
}

void GameStateManager::handleInput(const InputController::State& input) {
    int mx = input.mouseX;
    int my = input.mouseY;

    if (m_currentState == StateType::MAIN_MENU) {
        if (input.multiplayer) {
            startMultiplayerRace();
            return;
        }
        if (input.enter) {
            startRace(m_selectedBiome);
            return;
        }
        if (input.mouseMoved) m_mainMenu.onMouseMove(mx, my);
        if (input.mouseJustPressed) m_mainMenu.onMouseDown(mx, my);
        if (input.mouseJustReleased) m_mainMenu.onMouseUp(mx, my);
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
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }
        if (input.restart) {
            startMultiplayerRace();
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
        m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
        m_terrainRasterizer.dayNightCycle().update(dt, isPaused);
    }

    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.update(dt);
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
    }

    m_audio.update(dt, m_physics.vehicle(), m_physics.terrain(), m_currentState);
}

void GameStateManager::renderCompactHud(Graphics::Framebuffer& fb, const Physics::Vehicle& veh, float dist,
                                        const std::string& pLabel, uint32_t tagCol, const std::string& ctrlHint) {
    int W = fb.width();

    // 1. Racer Identity Pill (Top-Left)
    int pillW = 210;
    int pillH = 26;
    UI::UIComponents::drawArcadePanel(fb, 12, 10, pillW, pillH, 0xDD090F1A, UI::Theme::CARD_BORDER, tagCol, 2, true, true);
    fb.fillCircle(24, 23, 5, tagCol);
    Graphics::RasterFont::drawString(fb, 36, 17, pLabel, UI::Theme::TEXT_WHITE, 1);

    // Controls hint
    Graphics::RasterFont::drawString(fb, 14, 40, ctrlHint, 0xBBFFFFFF, 1);

    // 2. Speedometer
    int speedKmh = static_cast<int>(veh.getSpeedKmh());
    std::string speedStr = std::to_string(speedKmh) + " KM/H";
    int spdW = Graphics::RasterFont::getTextWidth(speedStr, 2);
    int spdX = W / 2 - spdW / 2;
    Graphics::RasterFont::drawString(fb, spdX, 12, speedStr, UI::Theme::TEXT_WHITE, 2);

    // 3. Distance Odometer (Top-Right)
    std::string distStr = UI::UIComponents::formatNumber(static_cast<int>(dist)) + "m";
    int distW = Graphics::RasterFont::getTextWidth(distStr, 2);
    int distBoxW = std::max(120, distW + 28);
    int distBoxX = W - distBoxW - 14;
    UI::UIComponents::drawArcadePanel(fb, distBoxX, 10, distBoxW, 26, 0xDD090F1A, UI::Theme::CARD_BORDER, UI::Theme::GOLD, 2, true, true);
    Graphics::RasterFont::drawString(fb, distBoxX + 14, 15, distStr, UI::Theme::GOLD, 2);

    // 4. Fuel Bar (Right below Distance)
    int fuelBarW = 120;
    int fuelBarH = 10;
    int fuelBarX = W - fuelBarW - 14;
    int fuelBarY = 40;
    fb.fillRect(fuelBarX - 1, fuelBarY - 1, fuelBarW + 2, fuelBarH + 2, 0xFF1B2433);
    fb.fillRect(fuelBarX, fuelBarY, fuelBarW, fuelBarH, 0xFF0E1520);

    float fuelFrac = std::clamp(veh.fuel() / 100.0f, 0.0f, 1.0f);
    int fillW = static_cast<int>(fuelBarW * fuelFrac);
    uint32_t fuelCol = (veh.fuel() > 50.0f) ? UI::Theme::GREEN_GAS :
                       (veh.fuel() > 25.0f) ? UI::Theme::GOLD : UI::Theme::RED_BRAKE;

    if (veh.isLowFuel()) {
        float blink = std::sin(m_gameTime * 12.0f);
        if (blink > 0.0f) fuelCol = 0xFFFF1744;
    }
    if (fillW > 0) {
        fb.fillRect(fuelBarX, fuelBarY, fillW, fuelBarH, fuelCol);
    }
    Graphics::RasterFont::drawString(fb, fuelBarX - 34, fuelBarY + 1, "FUEL", UI::Theme::TEXT_MUTED, 1);
}

void GameStateManager::renderMultiplayer(Graphics::Framebuffer& fb, float alpha) {
    m_camera.setRenderAlpha(alpha);
    m_camera2.setRenderAlpha(alpha);

    // --- TOP VIEWPORT: PLAYER 1 (MONSTER TRUCK) ---
    m_terrainRasterizer.render(m_fbP1, m_camera, m_physics.terrain(), m_gameTime);
    m_particleSystem.render(m_fbP1, m_camera);
    Graphics::SpriteRenderer::renderVehicle(m_fbP1, m_camera, m_physics.vehicle(), alpha);
    Graphics::SpriteRenderer::renderVehicle(m_fbP1, m_camera, m_physics.vehicle2(), alpha);

    // Draw P2 tag in Viewport 1 if nearby
    Physics::Vec2 p2Scr = m_camera.worldToScreen(m_physics.vehicle2().chassisPos());
    if (p2Scr.x >= 20 && p2Scr.x <= m_fbP1.width() - 20 && p2Scr.y >= 20 && p2Scr.y <= m_fbP1.height() - 20) {
        Graphics::RasterFont::drawStringCentered(m_fbP1, static_cast<int>(p2Scr.x), static_cast<int>(p2Scr.y - 34), "[P2]", 0xFF00E5FF, 1);
    }
    renderCompactHud(m_fbP1, m_physics.vehicle(), m_physics.distanceReached(), "P1 - MONSTER TRUCK", 0xFFFF3D00, "WASD: W=GAS S=BRK A/D=TILT");

    // --- BOTTOM VIEWPORT: PLAYER 2 (PICKUP TRUCK) ---
    m_terrainRasterizer.render(m_fbP2, m_camera2, m_physics.terrain(), m_gameTime);
    m_particleSystem.render(m_fbP2, m_camera2);
    Graphics::SpriteRenderer::renderVehicle(m_fbP2, m_camera2, m_physics.vehicle(), alpha);
    Graphics::SpriteRenderer::renderVehicle(m_fbP2, m_camera2, m_physics.vehicle2(), alpha);

    // Draw P1 tag in Viewport 2 if nearby
    Physics::Vec2 p1Scr = m_camera2.worldToScreen(m_physics.vehicle().chassisPos());
    if (p1Scr.x >= 20 && p1Scr.x <= m_fbP2.width() - 20 && p1Scr.y >= 20 && p1Scr.y <= m_fbP2.height() - 20) {
        Graphics::RasterFont::drawStringCentered(m_fbP2, static_cast<int>(p1Scr.x), static_cast<int>(p1Scr.y - 34), "[P1]", 0xFFFF3D00, 1);
    }
    renderCompactHud(m_fbP2, m_physics.vehicle2(), m_physics.distanceReached2(), "P2 - PICKUP TRUCK", 0xFF00E5FF, "ARROWS: UP=GAS DN=BRK L/R=TILT");

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

void GameStateManager::render(Graphics::Framebuffer& fb, float alpha) {
    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.render(fb, m_profile);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.render(fb, m_profile);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.render(fb, m_profile);
    } else if (m_currentState == StateType::MULTIPLAYER_RACE) {
        renderMultiplayer(fb, alpha);
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
