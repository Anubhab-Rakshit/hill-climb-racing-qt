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
#include "LobbyScreen.h"
#include "MinimapRadar.h"
#include "GameServer.h"
#include "GameClient.h"

namespace Core {

/**
 * @brief Manages screen transitions, coordinates rendering, and bridges UI with Physics.
 * Supports Single Player and Split-Screen 2-Truck Multiplayer Racing.
 */
class GameStateManager {
public:
    GameStateManager();

    void setDimensions(int width, int height);

    void changeState(StateType newState);
    StateType currentState() const { return m_currentState; }
    void startRace(const std::string& biomeId);
    void startMultiplayerRace(const std::string& biomeId = "countryside");
    void startOnlineLobby();
    void startNetworkMultiplayer(bool isHost, const std::string& biomeId);

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
    Graphics::ScanlineRasterizer& terrainRasterizer() { return m_terrainRasterizer; }
    Graphics::ParticleSystem& particleSystem() { return m_particleSystem; }
    UI::HUD& hud() { return m_hud; }
    UI::LobbyScreen& lobbyScreen() { return m_lobbyScreen; }
    Network::GameServer& server() { return m_server; }
    Network::GameClient& client() { return m_client; }
    float gameTime() const { return m_gameTime; }

private:
    StateType m_currentState;
    int m_width;
    int m_height;
    float m_gameTime;
    float m_transitionTimer;
    float m_smokeTimer;
    float m_smokeTimer2 = 0.0f;
    std::string m_selectedBiome;

    // Subsystems
    ProfileManager m_profile;
    Physics::PhysicsWorld m_physics;
    Graphics::Camera m_camera;
    Graphics::Camera m_camera2;
    Graphics::ScanlineRasterizer m_terrainRasterizer;
    Graphics::ParticleSystem m_particleSystem;
    Audio::SoundManager m_audio;

    // Multiplayer Split-Screen Viewport Buffers (960 x 266 each)
    Graphics::Framebuffer m_fbP1;
    Graphics::Framebuffer m_fbP2;

    bool m_isMultiplayerFlow = false;
    std::string m_selectedMpBiome = "countryside";
    Physics::VehicleType m_mpVehicleP1 = Physics::VehicleType::OFFROADER;
    Physics::VehicleType m_mpVehicleP2 = Physics::VehicleType::OFFROADER;
    Physics::DriverType m_mpDriverP1 = Physics::DriverType::BILL;
    Physics::DriverType m_mpDriverP2 = Physics::DriverType::SARAH;

    // Multiplayer Instrumentation & Dynamic Gauges
    UI::AnalogGauge m_mpSpeedoP1;
    UI::AnalogGauge m_mpSpeedoP2;
    UI::AnalogGauge m_mpTachoP1;
    UI::AnalogGauge m_mpTachoP2;
    UI::FuelBar m_mpFuelBarP1;
    UI::FuelBar m_mpFuelBarP2;

    // UI Screens
    UI::MainMenuScreen m_mainMenu;
    UI::GarageScreen m_garage;
    UI::StageSelectScreen m_stageSelect;
    UI::HUD m_hud;
    UI::PauseOverlay m_pauseOverlay;
    UI::GameOverScreen m_gameOverScreen;
    UI::LobbyScreen m_lobbyScreen;
    UI::MinimapRadar m_minimapRadar;

    // Online Networking
    Network::GameServer m_server;
    Network::GameClient m_client;
    bool m_isOnlineHost = false;
    bool m_isOnlineClient = false;
    float m_onlineTelemetryTimer = 0.0f;
    std::map<int, Network::RemotePlayerState> m_onlineOpponents;
    int m_onlineWinnerId = -1;
    std::string m_onlineWinnerName;
    uint32_t m_onlineWinnerColor = 0xFFFFD700;
    std::string m_onlineWinReason;
    bool m_onlineRaceFinished = false;

    void setupUiCallbacks();
    void renderMultiplayer(Graphics::Framebuffer& fb, float alpha);
    void renderOnlineRace(Graphics::Framebuffer& fb, float alpha);
    void renderOverheadTag(Graphics::Framebuffer& fb, const Graphics::Camera& cam,
                           const Physics::Vec2& worldPos, const std::string& name,
                           uint32_t color, float distDiff);
    void checkOnlineRaceRules();
    void renderCompactHud(Graphics::Framebuffer& fb, const Physics::Vehicle& veh, float dist,
                          const std::string& pLabel, uint32_t tagCol, const std::string& ctrlHint,
                          UI::AnalogGauge& speedo, UI::AnalogGauge& tacho, UI::FuelBar& fuelBar);
};

} // namespace Core
