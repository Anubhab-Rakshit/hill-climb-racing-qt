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
    void setOnHostCreate(std::function<void()> cb) { m_onHostCreate = cb; }
    void setOnHostStart(std::function<void(const std::string& biomeId)> cb) { m_onHostStart = cb; }
    void setOnHostTrackChanged(std::function<void(const std::string& biomeId)> cb) { m_onHostTrackChanged = cb; }
    void setOnHostVehicleChanged(std::function<void(int vehicle, int driver)> cb) { m_onHostVehicleChanged = cb; }
    void setOnClientConnect(std::function<void(const std::string& ip, quint16 port, const std::string& name, int vehicle, int driver)> cb) {
        m_onClientConnect = cb;
    }
    void setOnClientReady(std::function<void(bool isReady, int vehicle, int driver)> cb) {
        m_onClientReady = cb;
    }
    void setOnDisconnect(std::function<void()> cb) { m_onDisconnect = cb; }
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
    void setSelectedBiome(const std::string& biome) { m_selectedBiome = biome; }

    int getSelectedVehicle() const { return m_selectedVehicle; }
    void setSelectedVehicle(int v) { m_selectedVehicle = v; }

    int getSelectedDriver() const { return m_selectedDriver; }
    void setSelectedDriver(int d) { m_selectedDriver = d; }

    void setPlayerName(const std::string& name);
    const std::string& getPlayerName() const { return m_playerName; }

    void setStatusMessage(const std::string& msg) { m_statusMessage = msg; }

private:
    int m_width = 960;
    int m_height = 540;
    float m_animTime = 0.0f;

    LobbyMode m_mode = LobbyMode::CHOOSE_ROLE;

    std::string m_hostIpInput = "127.0.0.1";
    int m_hostPort = 7777;
    std::string m_playerName = "Racer";
    std::string m_selectedBiome = "countryside";
    int m_selectedVehicle = 0;
    int m_selectedDriver = 0;
    bool m_isLocalReady = false;
    std::string m_statusMessage;

    int m_activeInputField = 0; // 0 = IP, 1 = Name

    // Navigation & Roles
    RetroButton m_btnHostMode;
    RetroButton m_btnJoinMode;
    RetroButton m_btnBack;

    // Host Controls
    RetroButton m_btnPrevTrack;
    RetroButton m_btnNextTrack;
    RetroButton m_btnHostPrevVeh;
    RetroButton m_btnHostNextVeh;
    RetroButton m_btnStartRace;

    // Join Controls
    RetroButton m_btnLocalhostPreset;
    RetroButton m_btnConnect;
    RetroButton m_btnToggleReady;
    RetroButton m_btnPrevVehicle;
    RetroButton m_btnNextVehicle;

    std::function<void()> m_onBack;
    std::function<void()> m_onHostCreate;
    std::function<void(const std::string&)> m_onHostStart;
    std::function<void(const std::string&)> m_onHostTrackChanged;
    std::function<void(int, int)> m_onHostVehicleChanged;
    std::function<void(const std::string&, quint16, const std::string&, int, int)> m_onClientConnect;
    std::function<void(bool, int, int)> m_onClientReady;
    std::function<void()> m_onDisconnect;

    void cycleTrack(int direction);
    void cycleVehicle(int direction);
};

} // namespace UI
