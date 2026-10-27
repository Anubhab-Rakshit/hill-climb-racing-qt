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
#include "LevelCompleteScreen.h"
#include "SoundManager.h"

namespace Core {

/**
 * @brief Manages screen transitions, coordinates rendering, and bridges UI with Physics.
 */
class GameStateManager {
public:
    GameStateManager();

    void setDimensions(int width, int height);

    void changeState(StateType newState);
    StateType currentState() const { return m_currentState; }
    void startRace(const std::string& biomeId, int level = 1);

    void handleInput(const InputController::State& input);
    void fixedUpdate(float dt);
    void render(Graphics::Framebuffer& fb, float alpha);

    ProfileManager& profile() { return m_profile; }
    Physics::PhysicsWorld& physics() { return m_physics; }
    UI::GarageScreen& garageScreen() { return m_garage; }
    const UI::GarageScreen& garageScreen() const { return m_garage; }
    UI::StageSelectScreen& stageSelectScreen() { return m_stageSelect; }
    const UI::StageSelectScreen& stageSelectScreen() const { return m_stageSelect; }
    UI::LevelCompleteScreen& levelCompleteScreen() { return m_levelCompleteScreen; }
    const UI::LevelCompleteScreen& levelCompleteScreen() const { return m_levelCompleteScreen; }
    Graphics::Camera& camera() { return m_camera; }
    const Graphics::Camera& camera() const { return m_camera; }
    Graphics::ScanlineRasterizer& terrainRasterizer() { return m_terrainRasterizer; }
    Graphics::ParticleSystem& particleSystem() { return m_particleSystem; }
    UI::HUD& hud() { return m_hud; }
    float gameTime() const { return m_gameTime; }
    int currentLevel() const { return m_currentLevel; }
    const std::string& selectedBiome() const { return m_selectedBiome; }

private:
    StateType m_currentState;
    int m_width;
    int m_height;
    float m_gameTime;
    float m_levelTime;
    float m_transitionTimer;
    float m_smokeTimer;
    std::string m_selectedBiome;
    int m_currentLevel;
    int m_levelCoinsEarned;

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
    UI::LevelCompleteScreen m_levelCompleteScreen;

    void setupUiCallbacks();
};

} // namespace Core
