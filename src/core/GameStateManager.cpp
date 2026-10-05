#include "GameStateManager.h"
#include "Sprite.h"
#include "UITheme.h"
#include <QApplication>

namespace Core {

GameStateManager::GameStateManager()
    : m_currentState(StateType::MAIN_MENU)
    , m_width(UI::Theme::VIRTUAL_WIDTH)
    , m_height(UI::Theme::VIRTUAL_HEIGHT)
    , m_gameTime(0.0f)
    , m_selectedBiome("countryside")
    , m_camera(m_width, m_height)
{
    setDimensions(m_width, m_height);
    setupUiCallbacks();
}

void GameStateManager::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_camera.setScreenSize(width, height);
    m_mainMenu.setDimensions(width, height);
    m_garage.setDimensions(width, height);
    m_stageSelect.setDimensions(width, height);
    m_hud.setDimensions(width, height);
    m_pauseOverlay.setDimensions(width, height);
    m_gameOverScreen.setDimensions(width, height);
}

void GameStateManager::setupUiCallbacks() {
    // 1. Main Menu
    m_mainMenu.setOnStart([this]() {
        changeState(StateType::STAGE_SELECT);
    });
    m_mainMenu.setOnGarage([this]() {
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
        if (m_currentState == StateType::GAMEPLAY) {
            changeState(StateType::PAUSED);
        }
    });

    // 5. Pause Overlay
    m_pauseOverlay.setOnResume([this]() {
        changeState(StateType::GAMEPLAY);
    });
    m_pauseOverlay.setOnRestart([this]() {
        startRace(m_selectedBiome);
    });
    m_pauseOverlay.setOnGarage([this]() {
        changeState(StateType::GARAGE);
    });
    m_pauseOverlay.setOnMenu([this]() {
        changeState(StateType::MAIN_MENU);
    });

    // 6. Game Over Screen
    m_gameOverScreen.setOnRetry([this]() {
        startRace(m_selectedBiome);
    });
    m_gameOverScreen.setOnGarage([this]() {
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
        if (type == Physics::ItemType::FUEL_CANISTER) {
            m_audio.playFuelPickup();
            m_hud.triggerStunt("+100 FUEL!", 0, UI::Theme::GREEN_GAS);
        } else {
            m_audio.playCoinPickup();
            m_hud.triggerStunt("+$" + std::to_string(val), 0, UI::Theme::GOLD);
        }
    });
}

void GameStateManager::changeState(StateType newState) {
    m_currentState = newState;
}

void GameStateManager::startRace(const std::string& biomeId) {
    m_selectedBiome = biomeId;
    m_physics.reset(biomeId);
    m_particleSystem.clear();
    m_gameTime = 0.0f;
    changeState(StateType::GAMEPLAY);
}

void GameStateManager::handleInput(const InputController::State& input) {
    int mx = input.mouseX;
    int my = input.mouseY;

    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.onMouseMove(mx, my);
        if (input.mouseDown) m_mainMenu.onMouseDown(mx, my);
        else m_mainMenu.onMouseUp(mx, my);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.onMouseMove(mx, my);
        if (input.mouseDown) m_garage.onMouseDown(mx, my, m_profile);
        else m_garage.onMouseUp(mx, my);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.onMouseMove(mx, my);
        if (input.mouseDown) m_stageSelect.onMouseDown(mx, my);
        else m_stageSelect.onMouseUp(mx, my);
    } else if (m_currentState == StateType::GAMEPLAY) {
        if (input.pause) {
            changeState(StateType::PAUSED);
            return;
        }

        m_hud.onMouseMove(mx, my);
        if (input.mouseDown) m_hud.onMouseDown(mx, my);
        else m_hud.onMouseUp(mx, my);

        // Map inputs: either keyboard or touch pedal
        float gas = (input.gas || m_hud.isGasPressed()) ? 1.0f : 0.0f;
        float brake = (input.brake || m_hud.isBrakePressed()) ? 1.0f : 0.0f;

        m_hud.setGasVirtualPressed(input.gas);
        m_hud.setBrakeVirtualPressed(input.brake);

        m_physics.vehicle().applyInput(gas, brake);
    } else if (m_currentState == StateType::PAUSED) {
        if (input.pause) {
            changeState(StateType::GAMEPLAY);
            return;
        }
        m_pauseOverlay.onMouseMove(mx, my);
        if (input.mouseDown) m_pauseOverlay.onMouseDown(mx, my);
        else m_pauseOverlay.onMouseUp(mx, my);
    } else if (m_currentState == StateType::GAME_OVER) {
        m_gameOverScreen.onMouseMove(mx, my);
        if (input.mouseDown) m_gameOverScreen.onMouseDown(mx, my);
        else m_gameOverScreen.onMouseUp(mx, my);
    }
}

void GameStateManager::fixedUpdate(float dt) {
    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.update(dt);
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.update(dt);
    } else if (m_currentState == StateType::GAMEPLAY) {
        m_gameTime += dt;

        // Step physics simulation
        m_physics.step(dt, m_profile);

        // Update camera tracking
        const auto& car = m_physics.vehicle();
        m_camera.update(dt, car.chassisPos(), car.chassisVel().x);

        // Particles
        if (car.fuel() > 0.0f) {
            m_particleSystem.emitSmoke(car.chassisPos() + Physics::Vec2(-1.2f, -0.1f), car.chassisVel());
        }
        if (car.isRearOnGround() && std::abs(car.chassisVel().x) > 2.0f) {
            m_particleSystem.emitDirt(car.rearWheelPos(), {-1.0f, 0.4f});
        }
        m_particleSystem.update(dt);

        // Update HUD Telemetry
        m_hud.setFuel(car.fuel());
        m_hud.setSpeed(car.getSpeedKmh());
        m_hud.setRpm(car.getEngineRpm());
        m_hud.setDistance(m_physics.distanceReached());
        m_hud.setRecord(m_profile.getRecordDistance(m_selectedBiome));
        m_hud.setCoins(m_profile.coins() + m_physics.coinsCollected());
        m_hud.update(dt);

        m_audio.updateEngineRpm(car.getEngineRpm());

        // Check Game Over Transition
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
    }
}

void GameStateManager::render(Graphics::Framebuffer& fb, float /*alpha*/) {
    if (m_currentState == StateType::MAIN_MENU) {
        m_mainMenu.render(fb, m_profile.coins());
    } else if (m_currentState == StateType::GARAGE) {
        m_garage.render(fb, m_profile);
    } else if (m_currentState == StateType::STAGE_SELECT) {
        m_stageSelect.render(fb, m_profile);
    } else if (m_currentState == StateType::GAMEPLAY || m_currentState == StateType::PAUSED || m_currentState == StateType::GAME_OVER) {
        // 1. Render World (Terrain + Parallax)
        m_terrainRasterizer.render(fb, m_camera, m_physics.terrain(), m_gameTime);

        // 2. Render Particles (Smoke & Dirt)
        m_particleSystem.render(fb, m_camera);

        // 3. Render Vehicle (Chassis, Rotated Wheels, Driver)
        Graphics::SpriteRenderer::renderVehicle(fb, m_camera, m_physics.vehicle());

        // 4. Render Telemetry HUD
        m_hud.render(fb);

        // 5. Overlays
        if (m_currentState == StateType::PAUSED) {
            m_pauseOverlay.render(fb);
        } else if (m_currentState == StateType::GAME_OVER) {
            m_gameOverScreen.render(fb);
        }
    }
}

} // namespace Core
