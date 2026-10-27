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

    // Role Selection Buttons
    RetroButton m_btnHostMode;
    RetroButton m_btnJoinMode;
    RetroButton m_btnBack;

    // Host Controls
    RetroButton m_btnPrevTrack;
    RetroButton m_btnNextTrack;
    RetroButton m_btnToggleGP;
    RetroButton m_btnStartRace;

    // Join Controls
    RetroButton m_btnConnect;
    RetroButton m_btnToggleReady;
    RetroButton m_btnPrevVehicle;
    RetroButton m_btnNextVehicle;

    std::function<void()> m_onBack;
    std::function<void(const std::string&, bool)> m_onStartRace;

    void cycleTrack(int direction);
    void cycleVehicle(int direction);
};

} // namespace UI
