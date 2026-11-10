#include "LobbyScreen.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include "VehicleConfig.h"
#include "TerrainConfig.h"
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

namespace UI {

static const std::vector<std::string> s_lobbyTracks = {
    "countryside", "desert", "arctic", "mountain", "moon", "volcano"
};

LobbyScreen::LobbyScreen()
    : m_btnHostMode(0, 0, 380, 56, "CREATE HOST ROOM (LAN/WIFI)", Theme::GREEN_GAS)
    , m_btnJoinMode(0, 0, 380, 56, "JOIN REMOTE ROOM BY IP", Theme::CYAN_UPGRADE)
    , m_btnBack(20, 16, 110, 36, "< MENU", 0xFF37474F)
    , m_btnPrevTrack(0, 0, 38, 34, "<", 0xFF263238)
    , m_btnNextTrack(0, 0, 38, 34, ">", 0xFF263238)
    , m_btnHostPrevVeh(0, 0, 38, 34, "<", 0xFF263238)
    , m_btnHostNextVeh(0, 0, 38, 34, ">", 0xFF263238)
    , m_btnStartRace(0, 0, 280, 50, "LAUNCH RACE", Theme::GREEN_GAS)
    , m_btnLocalhostPreset(0, 0, 150, 32, "USE 127.0.0.1", 0xFF2A3A4E)
    , m_btnConnect(0, 0, 140, 38, "CONNECT", Theme::CYAN_UPGRADE)
    , m_btnToggleReady(0, 0, 200, 46, "SET READY", Theme::GOLD)
    , m_btnPrevVehicle(0, 0, 38, 34, "<", 0xFF263238)
    , m_btnNextVehicle(0, 0, 38, 34, ">", 0xFF263238)
{
    m_btnBack.setKeyHint("ESC");
    m_btnStartRace.setKeyHint("ENTER");

    m_btnHostMode.setOnClick([this]() {
        setMode(LobbyMode::HOST_LOBBY);
        if (m_onHostCreate) {
            m_onHostCreate();
        }
    });

    m_btnJoinMode.setOnClick([this]() {
        setMode(LobbyMode::JOIN_LOBBY);
    });

    m_btnBack.setOnClick([this]() {
        if (m_mode != LobbyMode::CHOOSE_ROLE) {
            if (m_onDisconnect) {
                m_onDisconnect();
            }
            setMode(LobbyMode::CHOOSE_ROLE);
        } else if (m_onBack) {
            m_onBack();
        }
    });

    m_btnPrevTrack.setOnClick([this]() { cycleTrack(-1); });
    m_btnNextTrack.setOnClick([this]() { cycleTrack(1); });

    m_btnHostPrevVeh.setOnClick([this]() { cycleVehicle(-1); });
    m_btnHostNextVeh.setOnClick([this]() { cycleVehicle(1); });

    m_btnStartRace.setOnClick([this]() {
        if (m_onHostStart) {
            m_onHostStart(m_selectedBiome);
        }
    });

    m_btnLocalhostPreset.setOnClick([this]() {
        m_hostIpInput = "127.0.0.1";
        m_statusMessage = "Set to localhost (127.0.0.1)";
    });

    m_btnConnect.setOnClick([this]() {
        if (m_onClientConnect) {
            m_statusMessage = "Connecting to " + m_hostIpInput + ":" + std::to_string(m_hostPort) + "...";
            m_onClientConnect(m_hostIpInput, static_cast<quint16>(m_hostPort),
                              m_playerName, m_selectedVehicle, m_selectedDriver);
        }
    });

    m_btnToggleReady.setOnClick([this]() {
        m_isLocalReady = !m_isLocalReady;
        m_btnToggleReady.setText(m_isLocalReady ? "READY (WAITING...)" : "SET READY");
        m_btnToggleReady.setBaseColor(m_isLocalReady ? Theme::GREEN_GAS : Theme::GOLD);
        if (m_onClientReady) {
            m_onClientReady(m_isLocalReady, m_selectedVehicle, m_selectedDriver);
        }
    });

    m_btnPrevVehicle.setOnClick([this]() {
        cycleVehicle(-1);
        if (m_onClientReady && m_isLocalReady) {
            m_onClientReady(m_isLocalReady, m_selectedVehicle, m_selectedDriver);
        }
    });

    m_btnNextVehicle.setOnClick([this]() {
        cycleVehicle(1);
        if (m_onClientReady && m_isLocalReady) {
            m_onClientReady(m_isLocalReady, m_selectedVehicle, m_selectedDriver);
        }
    });

    setDimensions(m_width, m_height);
}

void LobbyScreen::setPlayerName(const std::string& name) {
    if (!name.empty()) {
        m_playerName = name;
    }
}

void LobbyScreen::setMode(LobbyMode mode) {
    m_mode = mode;
    m_statusMessage = "";
    m_isLocalReady = false;
    m_btnToggleReady.setText("SET READY");
    m_btnToggleReady.setBaseColor(Theme::GOLD);
    m_activeInputField = 0;
}

void LobbyScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(24, 18);

    int cx = m_width / 2;
    int cy = m_height / 2;

    // Role selection buttons
    m_btnHostMode.setPosition(cx - 190, cy - 40);
    m_btnJoinMode.setPosition(cx - 190, cy + 35);

    // Host screen layout
    int hostPanX = 50;
    int hostPanY = 75;
    m_btnPrevTrack.setPosition(hostPanX + 20, hostPanY + 95);
    m_btnNextTrack.setPosition(hostPanX + 280, hostPanY + 95);

    m_btnHostPrevVeh.setPosition(hostPanX + 20, hostPanY + 185);
    m_btnHostNextVeh.setPosition(hostPanX + 280, hostPanY + 185);

    m_btnStartRace.setPosition(cx + 80, m_height - 75);

    // Join screen layout
    int joinPanX = cx - 290;
    int joinPanY = 75;
    m_btnLocalhostPreset.setPosition(joinPanX + 40, joinPanY + 100);
    m_btnConnect.setPosition(joinPanX + 390, joinPanY + 54);

    m_btnPrevVehicle.setPosition(joinPanX + 40, joinPanY + 185);
    m_btnNextVehicle.setPosition(joinPanX + 390, joinPanY + 185);
    m_btnToggleReady.setPosition(cx - 100, joinPanY + 250);
}

void LobbyScreen::setButtonSound(std::function<void()> cb) {
    m_btnHostMode.setOnSound(cb);
    m_btnJoinMode.setOnSound(cb);
    m_btnBack.setOnSound(cb);
    m_btnPrevTrack.setOnSound(cb);
    m_btnNextTrack.setOnSound(cb);
    m_btnHostPrevVeh.setOnSound(cb);
    m_btnHostNextVeh.setOnSound(cb);
    m_btnStartRace.setOnSound(cb);
    m_btnLocalhostPreset.setOnSound(cb);
    m_btnConnect.setOnSound(cb);
    m_btnToggleReady.setOnSound(cb);
    m_btnPrevVehicle.setOnSound(cb);
    m_btnNextVehicle.setOnSound(cb);
}

void LobbyScreen::cycleTrack(int direction) {
    auto it = std::find(s_lobbyTracks.begin(), s_lobbyTracks.end(), m_selectedBiome);
    int idx = (it != s_lobbyTracks.end()) ? static_cast<int>(std::distance(s_lobbyTracks.begin(), it)) : 0;
    idx = (idx + direction + static_cast<int>(s_lobbyTracks.size())) % static_cast<int>(s_lobbyTracks.size());
    m_selectedBiome = s_lobbyTracks[idx];
    if (m_mode == LobbyMode::HOST_LOBBY && m_onHostTrackChanged) {
        m_onHostTrackChanged(m_selectedBiome);
    }
}

void LobbyScreen::cycleVehicle(int direction) {
    int totalVehicles = static_cast<int>(Physics::VehicleType::COUNT);
    m_selectedVehicle = (m_selectedVehicle + direction + totalVehicles) % totalVehicles;
    if (m_mode == LobbyMode::HOST_LOBBY && m_onHostVehicleChanged) {
        m_onHostVehicleChanged(m_selectedVehicle, m_selectedDriver);
    } else if (m_mode == LobbyMode::JOIN_LOBBY && m_onClientReady) {
        m_onClientReady(m_isLocalReady, m_selectedVehicle, m_selectedDriver);
    }
}

void LobbyScreen::update(float dt) {
    m_animTime += dt;
}

void LobbyScreen::onMouseMove(int px, int py) {
    m_btnBack.onMouseMove(px, py);

    if (m_mode == LobbyMode::CHOOSE_ROLE) {
        m_btnHostMode.onMouseMove(px, py);
        m_btnJoinMode.onMouseMove(px, py);
    } else if (m_mode == LobbyMode::HOST_LOBBY) {
        m_btnPrevTrack.onMouseMove(px, py);
        m_btnNextTrack.onMouseMove(px, py);
        m_btnHostPrevVeh.onMouseMove(px, py);
        m_btnHostNextVeh.onMouseMove(px, py);
        m_btnStartRace.onMouseMove(px, py);
    } else if (m_mode == LobbyMode::JOIN_LOBBY) {
        m_btnLocalhostPreset.onMouseMove(px, py);
        m_btnConnect.onMouseMove(px, py);
        m_btnPrevVehicle.onMouseMove(px, py);
        m_btnNextVehicle.onMouseMove(px, py);
        m_btnToggleReady.onMouseMove(px, py);
    }
}

bool LobbyScreen::onMouseDown(int px, int py) {
    if (m_btnBack.onMouseDown(px, py)) return true;

    if (m_mode == LobbyMode::CHOOSE_ROLE) {
        if (m_btnHostMode.onMouseDown(px, py)) return true;
        if (m_btnJoinMode.onMouseDown(px, py)) return true;
    } else if (m_mode == LobbyMode::HOST_LOBBY) {
        if (m_btnPrevTrack.onMouseDown(px, py)) return true;
        if (m_btnNextTrack.onMouseDown(px, py)) return true;
        if (m_btnHostPrevVeh.onMouseDown(px, py)) return true;
        if (m_btnHostNextVeh.onMouseDown(px, py)) return true;
        if (m_btnStartRace.onMouseDown(px, py)) return true;
    } else if (m_mode == LobbyMode::JOIN_LOBBY) {
        if (m_btnLocalhostPreset.onMouseDown(px, py)) return true;
        if (m_btnConnect.onMouseDown(px, py)) return true;
        if (m_btnPrevVehicle.onMouseDown(px, py)) return true;
        if (m_btnNextVehicle.onMouseDown(px, py)) return true;
        if (m_btnToggleReady.onMouseDown(px, py)) return true;

        // Check clicking on input fields
        int cx = m_width / 2;
        int panX = cx - 290;
        int panY = 75;
        // IP box
        if (px >= panX + 40 && px <= panX + 370 && py >= panY + 54 && py <= panY + 90) {
            m_activeInputField = 0;
            return true;
        }
    }
    return false;
}

void LobbyScreen::onMouseUp(int px, int py) {
    m_btnBack.onMouseUp(px, py);

    if (m_mode == LobbyMode::CHOOSE_ROLE) {
        m_btnHostMode.onMouseUp(px, py);
        m_btnJoinMode.onMouseUp(px, py);
    } else if (m_mode == LobbyMode::HOST_LOBBY) {
        m_btnPrevTrack.onMouseUp(px, py);
        m_btnNextTrack.onMouseUp(px, py);
        m_btnHostPrevVeh.onMouseUp(px, py);
        m_btnHostNextVeh.onMouseUp(px, py);
        m_btnStartRace.onMouseUp(px, py);
    } else if (m_mode == LobbyMode::JOIN_LOBBY) {
        m_btnLocalhostPreset.onMouseUp(px, py);
        m_btnConnect.onMouseUp(px, py);
        m_btnPrevVehicle.onMouseUp(px, py);
        m_btnNextVehicle.onMouseUp(px, py);
        m_btnToggleReady.onMouseUp(px, py);
    }
}

void LobbyScreen::onKeyPress(int key, const std::string& text) {
    if (key == Qt::Key_Escape) {
        m_btnBack.triggerClick();
        return;
    }

    if (m_mode == LobbyMode::HOST_LOBBY) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            m_btnStartRace.triggerClick();
            return;
        }
    }

    if (m_mode == LobbyMode::JOIN_LOBBY) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            m_btnConnect.triggerClick();
            return;
        }

        if (key == Qt::Key_Backspace) {
            if (!m_hostIpInput.empty()) {
                m_hostIpInput.pop_back();
            }
            return;
        }

        if (!text.empty()) {
            char ch = text[0];
            if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == ':') {
                if (m_hostIpInput.length() < 24) {
                    m_hostIpInput += ch;
                }
            }
        }
    }
}

void LobbyScreen::render(Graphics::Framebuffer& fb, Network::GameServer* server, Network::GameClient* client) {
    // Backdrop gradient
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF070C15, 0xFF141F30);

    m_btnBack.render(fb, 2);

    int cx = m_width / 2;

    if (m_mode == LobbyMode::CHOOSE_ROLE) {
        // Header
        UIComponents::drawArcadePanel(fb, cx - 250, 40, 500, 44, 0xEE0B121C, Theme::CARD_BORDER, Theme::GOLD, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 52, "ONLINE NETWORK MULTIPLAYER (TCP/IP)", Theme::GOLD, 2);

        Graphics::RasterFont::drawStringCentered(fb, cx, 115, "Play together across different computers on LAN, Wi-Fi, or Internet!", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawStringCentered(fb, cx, 135, "Full-screen gameplay per laptop with synchronized live opponents!", Theme::CYAN_UPGRADE, 1);

        m_btnHostMode.render(fb, 2);
        m_btnJoinMode.render(fb, 2);

        Graphics::RasterFont::drawStringCentered(fb, cx, m_height - 60, "HOST creates a room; other laptops join using the Host's IP address.", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawStringCentered(fb, cx, m_height - 40, "Press ESC to return to Main Menu.", Theme::TEXT_MUTED, 1);
    }
    else if (m_mode == LobbyMode::HOST_LOBBY) {
        // Top Header with Host IP banner
        std::string ip = server ? server->getLocalIpAddress().toStdString() : "127.0.0.1";
        std::string titleStr = "HOST ROOM ACTIVE - IP: " + ip + ":7777 (or 127.0.0.1:7777)";

        UIComponents::drawArcadePanel(fb, cx - 350, 16, 700, 42, 0xF00A1322, Theme::CARD_BORDER, Theme::GREEN_GAS, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 28, titleStr, Theme::GREEN_GAS, 2);

        int leftX = 50;
        int leftW = 390;
        int panY = 70;
        int panH = 370;

        // --- LEFT PANEL: ROOM CONFIGURATION ---
        UIComponents::drawArcadePanel(fb, leftX, panY, leftW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 2, true, true);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 16, "MATCH CONFIGURATION", Theme::GOLD, 2);

        // Track selector
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 54, "STAGE / BIOME:", Theme::TEXT_WHITE, 1);
        fb.fillRect(leftX + 68, panY + 80, 202, 34, 0xFF0E1724);
        fb.drawRect(leftX + 68, panY + 80, 202, 34, Theme::CYAN_UPGRADE);

        const auto& spec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
        Graphics::RasterFont::drawStringCentered(fb, leftX + 169, panY + 89, spec.name, Theme::TEXT_WHITE, 1);

        m_btnPrevTrack.setPosition(leftX + 20, panY + 80);
        m_btnNextTrack.setPosition(leftX + 280, panY + 80);
        m_btnPrevTrack.render(fb, 1);
        m_btnNextTrack.render(fb, 1);

        // Host Vehicle selector
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 130, "YOUR VEHICLE (HOST):", Theme::TEXT_WHITE, 1);
        fb.fillRect(leftX + 68, panY + 154, 202, 34, 0xFF0E1724);
        fb.drawRect(leftX + 68, panY + 154, 202, 34, Theme::GOLD);

        const auto& vehConfig = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(m_selectedVehicle));
        Graphics::RasterFont::drawStringCentered(fb, leftX + 169, panY + 163, vehConfig.name, Theme::TEXT_WHITE, 1);

        m_btnHostPrevVeh.setPosition(leftX + 20, panY + 154);
        m_btnHostNextVeh.setPosition(leftX + 280, panY + 154);
        m_btnHostPrevVeh.render(fb, 1);
        m_btnHostNextVeh.render(fb, 1);

        // Connection instruction
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 210, "CONNECTION GUIDE:", Theme::GOLD, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 230, "1. Ensure laptops are on same Wi-Fi / LAN.", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 250, "2. Other players enter IP: " + ip, Theme::CYAN_UPGRADE, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 270, "3. For local testing, use 127.0.0.1", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 290, "4. Click LAUNCH RACE when ready!", Theme::GREEN_GAS, 1);

        // --- RIGHT PANEL: CONNECTED RACERS ROSTER ---
        int rightX = leftX + leftW + 30;
        int rightW = m_width - rightX - 50;
        UIComponents::drawArcadePanel(fb, rightX, panY, rightW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

        int playerCount = server ? server->getPlayerCount() : 1;
        std::string rosterTitle = "ROOM ROSTER (" + std::to_string(playerCount) + " PLAYERS)";
        Graphics::RasterFont::drawString(fb, rightX + 20, panY + 16, rosterTitle, Theme::GOLD, 2);

        // 1. Host card
        fb.fillRect(rightX + 16, panY + 54, rightW - 32, 44, 0xDD121D2C);
        fb.drawRect(rightX + 16, panY + 54, rightW - 32, 44, 0xFFFF3D00);
        fb.fillCircle(rightX + 34, panY + 76, 9, 0xFFFF3D00);
        Graphics::RasterFont::drawString(fb, rightX + 52, panY + 62, "P1: YOU (HOST)", 0xFFFFFFFF, 2);
        Graphics::RasterFont::drawString(fb, rightX + 52, panY + 82, vehConfig.name, Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawString(fb, rightX + rightW - 100, panY + 68, "[HOST READY]", Theme::GREEN_GAS, 1);

        // 2. Connected client cards
        if (server) {
            int slotY = panY + 108;
            for (const auto& c : server->getClients()) {
                fb.fillRect(rightX + 16, slotY, rightW - 32, 44, 0xDD121D2C);
                fb.drawRect(rightX + 16, slotY, rightW - 32, 44, c.assignedColor);
                fb.fillCircle(rightX + 34, slotY + 22, 9, c.assignedColor);

                std::string pTag = "P" + std::to_string(c.id + 1) + ": " + c.name;
                Graphics::RasterFont::drawString(fb, rightX + 52, slotY + 8, pTag, 0xFFFFFFFF, 2);

                const auto& clientVeh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(c.vehicleType));
                Graphics::RasterFont::drawString(fb, rightX + 52, slotY + 28, clientVeh.name, Theme::TEXT_MUTED, 1);

                std::string rdyStr = c.isReady ? "[READY]" : "[WAITING...]";
                uint32_t rdyCol = c.isReady ? Theme::GREEN_GAS : Theme::GOLD;
                Graphics::RasterFont::drawString(fb, rightX + rightW - 100, slotY + 16, rdyStr, rdyCol, 1);

                slotY += 52;
            }
        }

        m_btnStartRace.render(fb, 2);
    }
    else if (m_mode == LobbyMode::JOIN_LOBBY) {
        // Join Screen
        UIComponents::drawArcadePanel(fb, cx - 320, 16, 640, 42, 0xF00A1322, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 28, "JOIN ONLINE MULTIPLAYER ROOM", Theme::CYAN_UPGRADE, 2);

        int panW = 580;
        int panH = 400;
        int panX = cx - panW / 2;
        int panY = 70;

        UIComponents::drawArcadePanel(fb, panX, panY, panW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 2, true, true);

        // Host IP Address input
        Graphics::RasterFont::drawString(fb, panX + 40, panY + 24, "HOST LAPTOP IP ADDRESS:", Theme::GOLD, 1);
        fb.fillRect(panX + 40, panY + 48, 330, 36, 0xFF0E1624);
        fb.drawRect(panX + 40, panY + 48, 330, 36, (m_activeInputField == 0) ? Theme::CYAN_UPGRADE : Theme::CARD_BORDER);

        std::string ipDisp = m_hostIpInput;
        if (m_activeInputField == 0 && static_cast<int>(m_animTime * 2.5f) % 2 == 0) {
            ipDisp += "_";
        }
        Graphics::RasterFont::drawString(fb, panX + 52, panY + 58, ipDisp, 0xFFFFFFFF, 2);

        m_btnConnect.setPosition(panX + 390, panY + 48);
        m_btnConnect.render(fb, 1);

        m_btnLocalhostPreset.setPosition(panX + 40, panY + 92);
        m_btnLocalhostPreset.render(fb, 1);

        // Vehicle Picker
        Graphics::RasterFont::drawString(fb, panX + 40, panY + 140, "CHOOSE YOUR VEHICLE:", Theme::TEXT_WHITE, 1);
        fb.fillRect(panX + 90, panY + 165, 280, 36, 0xFF0E1624);
        fb.drawRect(panX + 90, panY + 165, 280, 36, Theme::GOLD);

        const auto& veh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(m_selectedVehicle));
        Graphics::RasterFont::drawStringCentered(fb, panX + 230, panY + 175, veh.name, Theme::TEXT_WHITE, 1);

        m_btnPrevVehicle.setPosition(panX + 40, panY + 165);
        m_btnNextVehicle.setPosition(panX + 390, panY + 165);
        m_btnPrevVehicle.render(fb, 1);
        m_btnNextVehicle.render(fb, 1);

        // Ready toggle
        m_btnToggleReady.setPosition(cx - 100, panY + 225);
        m_btnToggleReady.render(fb, 2);

        // Track info received from server
        if (client && client->isConnected()) {
            const auto& trackSpec = Physics::BiomeRegistry::getBiome(client->getTrack());
            std::string trkStr = "CURRENT STAGE: " + trackSpec.name;
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 290, trkStr, Theme::GOLD, 1);
        }

        // Status banner
        if (!m_statusMessage.empty()) {
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 320, m_statusMessage, Theme::GOLD, 1);
        } else if (client && client->isConnected()) {
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 320, "CONNECTED TO HOST! WAITING FOR HOST TO LAUNCH...", Theme::GREEN_GAS, 1);
        } else {
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 320, "Enter Host IP and click CONNECT.", Theme::TEXT_MUTED, 1);
        }

        // Instructions
        Graphics::RasterFont::drawStringCentered(fb, cx, panY + 355, "Type IP with keyboard digits. Click 'USE 127.0.0.1' for local testing.", Theme::TEXT_MUTED, 1);
    }
}

} // namespace UI
