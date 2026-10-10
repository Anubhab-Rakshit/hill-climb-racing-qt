#pragma once

#include "GameState.h"
#include "ProfileManager.h"
#include "PhysicsWorld.h"
#include "Camera.h"
#include "ScanlineRasterizer.h"
#include "ParticleSystem.h"
#include "HUD.h"
#include "MainMenuScreen.h"
#include "GarageScreen.h"
#include "StageSelectScreen.h"
#include "PauseOverlay.h"
#include "GameOverScreen.h"
#include "SoundManager.h"

namespace Core {

enum class StateType {
    MAIN_MENU,
    STAGE_SELECT,
    GARAGE,
    GAMEPLAY,
    PAUSED,
    GAME_OVER
};

/**
 * @brief Manages screen transitions, coordinates rendering, and bridges UI with Physics.
 */
class GameStateManager {
public:
    GameStateManager();

    void setDimensions(int width, int height);

    void changeState(StateType newState);
    StateType currentState() const { return m_currentState; }
    void startRace(const std::string& biomeId);

    void handleInput(const InputController::State& input);
    void fixedUpdate(float dt);
    void render(Graphics::Framebuffer& fb, float alpha);

    ProfileManager& profile() { return m_profile; }
    Physics::PhysicsWorld& physics() { return m_physics; }
    UI::GarageScreen& garageScreen() { return m_garage; }
    const UI::GarageScreen& garageScreen() const { return m_garage; }
    UI::StageSelectScreen& stageSelectScreen() { return m_stageSelect; }
    const UI::StageSelectScreen& stageSelectScreen() const { return m_stageSelect; }
    Graphics::Camera& camera() { return m_camera; }
    const Graphics::Camera& camera() const { return m_camera; }

private:
    StateType m_currentState;
    int m_width;
    int m_height;
    float m_gameTime;
    float m_transitionTimer;
    std::string m_selectedBiome;

    // Subsystems
    ProfileManager m_profile;
    Physics::PhysicsWorld m_physics;
    Graphics::Camera m_camera;
    Graphics::ScanlineRasterizer m_terrainRasterizer;
    Graphics::ParticleSystem m_particleSystem;
    Audio::SoundManager m_audio;

    // UI Screens
    UI::MainMenuScreen m_mainMenu;
    UI::GarageScreen m_garage;
    UI::StageSelectScreen m_stageSelect;
    UI::HUD m_hud;
    UI::PauseOverlay m_pauseOverlay;
    UI::GameOverScreen m_gameOverScreen;

    void setupUiCallbacks();
};

} // namespace Core
