#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "NetworkTypes.h"
#include "GameServer.h"
#include "GameClient.h"
#include <string>
#include <vector>
#include <functional>

namespace UI {

enum class LobbyMode {
    CHOOSE_ROLE,
    HOST_LOBBY,
    JOIN_LOBBY
};

/**
 * @brief Interactive Remote Multiplayer Room & Lobby Screen.
 * Allows hosting on LAN/WAN or joining via IP with real-time roster sync.
 */
class LobbyScreen {
public:
    LobbyScreen();

    void setDimensions(int width, int height);

    void setOnBack(std::function<void()> cb) { m_onBack = cb; }
    void setOnStartRace(std::function<void(const std::string& biomeId, bool isGrandPrix)> cb) {
        m_onStartRace = cb;
    }
    void setButtonSound(std::function<void()> cb);

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);
    void onKeyPress(int key, const std::string& text);

    void update(float dt);
    void render(Graphics::Framebuffer& fb, Network::GameServer* server, Network::GameClient* client);

    LobbyMode getMode() const { return m_mode; }
    void setMode(LobbyMode mode);

    const std::string& getSelectedBiome() const { return m_selectedBiome; }
    bool isGrandPrix() const { return m_isGrandPrix; }

    // Multi-Device Room Callbacks & State
    void setOnHostRoom(std::function<void()> cb) { m_onHostRoom = cb; }
    void setOnConnect(std::function<void(const std::string& hostIp, int vehicleType)> cb) { m_onConnect = cb; }
    void setOnDisconnect(std::function<void()> cb) { m_onDisconnect = cb; }
    void setOnToggleReady(std::function<void(bool isReady, int vehicleType)> cb) { m_onToggleReady = cb; }
    void setOnTrackChanged(std::function<void(const std::string& trackId)> cb) { m_onTrackChanged = cb; }
    void setOnVehicleChanged(std::function<void(int vehicleType)> cb) { m_onVehicleChanged = cb; }

    void setLobbyPlayers(const std::vector<Network::PlayerLobbyInfo>& players) { m_clientLobbyPlayers = players; }
    void setLobbyTrack(const std::string& trackId) { m_selectedBiome = trackId; }
    void setStatusMessage(const std::string& msg) { m_statusMessage = msg; }
    int getSelectedVehicle() const { return m_selectedVehicle; }
    void setSelectedVehicle(int v) { m_selectedVehicle = v; }
    bool isLocalReady() const { return m_isLocalReady; }
    void setLocalReady(bool r);

    const std::string& getHostIpInput() const { return m_hostIpInput; }
    void setHostIpInput(const std::string& ip) { m_hostIpInput = ip; }

private:
    int m_width = 960;
    int m_height = 540;
    float m_animTime = 0.0f;

    LobbyMode m_mode = LobbyMode::CHOOSE_ROLE;

    std::string m_hostIpInput = "127.0.0.1";
    int m_hostPort = 7777;
    std::string m_selectedBiome = "countryside";
    int m_selectedVehicle = 0;
    bool m_isGrandPrix = false;
    bool m_isLocalReady = false;
    std::string m_statusMessage;

    std::vector<Network::PlayerLobbyInfo> m_clientLobbyPlayers;

    // Role Selection Buttons
    RetroButton m_btnHostMode;
    RetroButton m_btnJoinMode;
    RetroButton m_btnBack;

    // Host Controls
    RetroButton m_btnPrevTrack;
    RetroButton m_btnNextTrack;
    RetroButton m_btnHostPrevVehicle;
    RetroButton m_btnHostNextVehicle;
    RetroButton m_btnToggleGP;
    RetroButton m_btnStartRace;

    // Join Controls
    RetroButton m_btnConnect;
    RetroButton m_btnToggleReady;
    RetroButton m_btnPrevVehicle;
    RetroButton m_btnNextVehicle;

    std::function<void()> m_onBack;
    std::function<void(const std::string&, bool)> m_onStartRace;
    std::function<void()> m_onHostRoom;
    std::function<void(const std::string&, int)> m_onConnect;
    std::function<void()> m_onDisconnect;
    std::function<void(bool, int)> m_onToggleReady;
    std::function<void(const std::string&)> m_onTrackChanged;
    std::function<void(int)> m_onVehicleChanged;

    void cycleTrack(int direction);
    void cycleVehicle(int direction);
};

} // namespace UI
