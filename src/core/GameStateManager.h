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
#include "AccountScreen.h"
#include "LobbyScreen.h"
#include "ChallengeScreen.h"
#include "EvaluatorOverlay.h"
#include "MinimapRadar.h"
#include "DeveloperOverlay.h"
#include "LocalizationManager.h"
#include "GameServer.h"
#include "GameClient.h"

namespace Core {

/**
 * @brief Manages screen transitions, coordinates rendering, network multiplayer, accounts, and challenge circuits.
 */
class GameStateManager {
public:
    GameStateManager();

    void setDimensions(int width, int height);

    void changeState(StateType newState);
    StateType currentState() const { return m_currentState; }
    void startRace(const std::string& biomeId);
    void startMultiplayerRace();
    void startNetworkMultiplayer(bool isHost, const std::string& biomeId);
    void startGrandPrixChallenge();
    void advanceGrandPrixRound();
    void startEvaluatorMode();

    void handleInput(const InputController::State& input);
    void fixedUpdate(float dt);
    void render(Graphics::Framebuffer& fb, float alpha);

    void onKeyTyped(int key, const std::string& text);

    ProfileManager& profile() { return m_profile; }
    Physics::PhysicsWorld& physics() { return m_physics; }
    UI::GarageScreen& garageScreen() { return m_garage; }
    const UI::GarageScreen& garageScreen() const { return m_garage; }
    UI::StageSelectScreen& stageSelectScreen() { return m_stageSelect; }
    const UI::StageSelectScreen& stageSelectScreen() const { return m_stageSelect; }
    Graphics::Camera& camera() { return m_camera; }
    const Graphics::Camera& camera() const { return m_camera; }
    Graphics::ScanlineRasterizer& terrainRasterizer() { return m_terrainRasterizer; }
    Graphics::ParticleSystem& particleSystem() { return m_particleSystem; }
    UI::HUD& hud() { return m_hud; }
    float gameTime() const { return m_gameTime; }

private:
    StateType m_currentState;
    int m_width;
    int m_height;
    float m_gameTime;
    float m_transitionTimer;
    float m_smokeTimer;
    float m_smokeTimer2 = 0.0f;
    float m_netTelemetryTimer = 0.0f;
    std::string m_selectedBiome;

    // Subsystems
    ProfileManager m_profile;
    Physics::PhysicsWorld m_physics;
    Graphics::Camera m_camera;
    Graphics::Camera m_camera2;
    Graphics::ScanlineRasterizer m_terrainRasterizer;
    Graphics::ParticleSystem m_particleSystem;
    Audio::SoundManager m_audio;

    // Multiplayer Split-Screen Viewport Buffers
    Graphics::Framebuffer m_fbP1;
    Graphics::Framebuffer m_fbP2;

    // Networking
    Network::GameServer m_server;
    Network::GameClient m_client;
    bool m_isNetworkHost = false;
    bool m_isNetworkClient = false;
    std::map<int, Network::RemotePlayerState> m_networkOpponents;

    // Grand Prix Circuit
    bool m_isGrandPrix = false;
    int m_gpRound = 1;
    std::vector<UI::GrandPrixRacerScore> m_gpStandings;

    // Evaluator Sandbox
    bool m_isEvaluatorMode = false;

    // UI Screens
    UI::MainMenuScreen m_mainMenu;
    UI::GarageScreen m_garage;
    UI::StageSelectScreen m_stageSelect;
    UI::HUD m_hud;
    UI::PauseOverlay m_pauseOverlay;
    UI::GameOverScreen m_gameOverScreen;
    UI::AccountScreen m_accountScreen;
    UI::LobbyScreen m_lobbyScreen;
    UI::ChallengeScreen m_challengeScreen;
    UI::EvaluatorOverlay m_evaluatorOverlay;
    UI::MinimapRadar m_minimapRadar;
    UI::DeveloperOverlay m_devOverlay;
    bool m_showDevOverlay = false;

    void setupUiCallbacks();
    void setupNetworkCallbacks();
    void renderMultiplayer(Graphics::Framebuffer& fb, float alpha);
    void renderNetworkRace(Graphics::Framebuffer& fb, float alpha);
    void renderCompactHud(Graphics::Framebuffer& fb, const Physics::Vehicle& veh, float dist,
                          const std::string& pLabel, uint32_t tagCol, const std::string& ctrlHint);
    void renderOverheadTag(Graphics::Framebuffer& fb, const Graphics::Camera& cam, const Physics::Vec2& worldPos,
                           const std::string& name, uint32_t color, float distDiff);
    void checkNetworkRaceFinish();
};

} // namespace Core
