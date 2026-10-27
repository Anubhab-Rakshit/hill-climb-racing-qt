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
    "countryside", "desert", "arctic", "mountain", "moon", "volcano", "neon", "super_countryside", "seasons"
};

LobbyScreen::LobbyScreen()
    : m_btnHostMode(0, 0, 360, 60, "CREATE HOST ROOM (LAN/WIFI)", Theme::GREEN_GAS)
    , m_btnJoinMode(0, 0, 360, 60, "JOIN REMOTE ROOM BY IP", Theme::CYAN_UPGRADE)
    , m_btnBack(20, 16, 110, 36, "< MENU", 0xFF37474F)
    , m_btnPrevTrack(0, 0, 42, 34, "<", 0xFF263238)
    , m_btnNextTrack(0, 0, 42, 34, ">", 0xFF263238)
    , m_btnHostPrevVehicle(0, 0, 42, 34, "<", 0xFF263238)
    , m_btnHostNextVehicle(0, 0, 42, 34, ">", 0xFF263238)
    , m_btnToggleGP(0, 0, 340, 36, "MODE: SINGLE RACE", 0xFF2A3A4E)
    , m_btnStartRace(0, 0, 260, 48, "LAUNCH RACE", Theme::GREEN_GAS)
    , m_btnConnect(0, 0, 130, 36, "CONNECT", Theme::CYAN_UPGRADE)
    , m_btnToggleReady(0, 0, 220, 42, "SET READY", Theme::GOLD)
    , m_btnPrevVehicle(0, 0, 42, 34, "<", 0xFF263238)
    , m_btnNextVehicle(0, 0, 42, 34, ">", 0xFF263238)
{
    m_btnBack.setKeyHint("ESC");
    m_btnStartRace.setKeyHint("ENTER");

    m_btnHostMode.setOnClick([this]() {
        setMode(LobbyMode::HOST_LOBBY);
        if (m_onHostRoom) {
            m_onHostRoom();
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

    // Track cycling (Host)
    m_btnPrevTrack.setOnClick([this]() { cycleTrack(-1); });
    m_btnNextTrack.setOnClick([this]() { cycleTrack(1); });

    // Host Vehicle cycling
    m_btnHostPrevVehicle.setOnClick([this]() { cycleVehicle(-1); });
    m_btnHostNextVehicle.setOnClick([this]() { cycleVehicle(1); });

    m_btnToggleGP.setOnClick([this]() {
        m_isGrandPrix = !m_isGrandPrix;
        m_btnToggleGP.setText(m_isGrandPrix ? "MODE: GRAND PRIX CIRCUIT" : "MODE: SINGLE RACE");
        m_btnToggleGP.setBaseColor(m_isGrandPrix ? Theme::GOLD : 0xFF2A3A4E);
    });

    m_btnStartRace.setOnClick([this]() {
        if (m_onStartRace) {
            m_onStartRace(m_selectedBiome, m_isGrandPrix);
        }
    });

    // Client Connect button
    m_btnConnect.setOnClick([this]() {
        if (m_btnConnect.text() == "DISCONNECT") {
            if (m_onDisconnect) {
                m_onDisconnect();
            }
            setLocalReady(false);
            m_btnConnect.setText("CONNECT");
            m_btnConnect.setBaseColor(Theme::CYAN_UPGRADE);
            m_statusMessage = "Disconnected from room.";
        } else {
            if (m_onConnect) {
                m_statusMessage = "Connecting to " + m_hostIpInput + "...";
                m_onConnect(m_hostIpInput, m_selectedVehicle);
            }
        }
    });

    // Client Ready toggle
    m_btnToggleReady.setOnClick([this]() {
        m_isLocalReady = !m_isLocalReady;
        setLocalReady(m_isLocalReady);
        if (m_onToggleReady) {
            m_onToggleReady(m_isLocalReady, m_selectedVehicle);
        }
    });

    // Client Vehicle cycling
    m_btnPrevVehicle.setOnClick([this]() { cycleVehicle(-1); });
    m_btnNextVehicle.setOnClick([this]() { cycleVehicle(1); });

    setDimensions(m_width, m_height);
}

void LobbyScreen::setMode(LobbyMode mode) {
    m_mode = mode;
    m_statusMessage = "";
    m_clientLobbyPlayers.clear();
    setLocalReady(false);
    m_btnConnect.setText("CONNECT");
    m_btnConnect.setBaseColor(Theme::CYAN_UPGRADE);
}

void LobbyScreen::setLocalReady(bool r) {
    m_isLocalReady = r;
    m_btnToggleReady.setText(m_isLocalReady ? "READY (WAITING...)" : "SET READY");
    m_btnToggleReady.setBaseColor(m_isLocalReady ? Theme::GREEN_GAS : Theme::GOLD);
}

void LobbyScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    m_btnBack.setPosition(24, 18);

    int cx = m_width / 2;
    int cy = m_height / 2;

    // Role selection
    m_btnHostMode.setPosition(cx - 180, cy - 45);
    m_btnJoinMode.setPosition(cx - 180, cy + 35);

    int leftX = 40;
    int leftW = 400;
    int panY = 70;

    // Host screen layout
    m_btnPrevTrack.setPosition(leftX + 20, panY + 75);
    m_btnNextTrack.setPosition(leftX + 338, panY + 75);

    m_btnHostPrevVehicle.setPosition(leftX + 20, panY + 145);
    m_btnHostNextVehicle.setPosition(leftX + 338, panY + 145);

    m_btnToggleGP.setPosition(leftX + 20, panY + 205);
    m_btnStartRace.setPosition(cx + 80, m_height - 65);

    // Join screen layout
    m_btnConnect.setPosition(leftX + 250, panY + 75);

    m_btnPrevVehicle.setPosition(leftX + 20, panY + 160);
    m_btnNextVehicle.setPosition(leftX + 338, panY + 160);

    m_btnToggleReady.setPosition(leftX + 80, panY + 225);
}

void LobbyScreen::setButtonSound(std::function<void()> cb) {
    m_btnHostMode.setOnSound(cb);
    m_btnJoinMode.setOnSound(cb);
    m_btnBack.setOnSound(cb);
    m_btnPrevTrack.setOnSound(cb);
    m_btnNextTrack.setOnSound(cb);
    m_btnHostPrevVehicle.setOnSound(cb);
    m_btnHostNextVehicle.setOnSound(cb);
    m_btnToggleGP.setOnSound(cb);
    m_btnStartRace.setOnSound(cb);
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

    if (m_onTrackChanged) {
        m_onTrackChanged(m_selectedBiome);
    }
}

void LobbyScreen::cycleVehicle(int direction) {
    int totalVehicles = static_cast<int>(Physics::VehicleType::COUNT);
    m_selectedVehicle = (m_selectedVehicle + direction + totalVehicles) % totalVehicles;

    if (m_onVehicleChanged) {
        m_onVehicleChanged(m_selectedVehicle);
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
        m_btnHostPrevVehicle.onMouseMove(px, py);
        m_btnHostNextVehicle.onMouseMove(px, py);
        m_btnToggleGP.onMouseMove(px, py);
        m_btnStartRace.onMouseMove(px, py);
    } else if (m_mode == LobbyMode::JOIN_LOBBY) {
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
        if (m_btnHostPrevVehicle.onMouseDown(px, py)) return true;
        if (m_btnHostNextVehicle.onMouseDown(px, py)) return true;
        if (m_btnToggleGP.onMouseDown(px, py)) return true;
        if (m_btnStartRace.onMouseDown(px, py)) return true;
    } else if (m_mode == LobbyMode::JOIN_LOBBY) {
        if (m_btnConnect.onMouseDown(px, py)) return true;
        if (m_btnPrevVehicle.onMouseDown(px, py)) return true;
        if (m_btnNextVehicle.onMouseDown(px, py)) return true;
        if (m_btnToggleReady.onMouseDown(px, py)) return true;
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
        m_btnHostPrevVehicle.onMouseUp(px, py);
        m_btnHostNextVehicle.onMouseUp(px, py);
        m_btnToggleGP.onMouseUp(px, py);
        m_btnStartRace.onMouseUp(px, py);
    } else if (m_mode == LobbyMode::JOIN_LOBBY) {
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

    if (m_mode == LobbyMode::JOIN_LOBBY) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            m_btnConnect.triggerClick();
            return;
        }
        if (key == Qt::Key_Backspace && !m_hostIpInput.empty()) {
            m_hostIpInput.pop_back();
            return;
        }
        if (!text.empty() && (std::isdigit(text[0]) || text[0] == '.' || text[0] == ':')) {
            if (m_hostIpInput.length() < 24) {
                m_hostIpInput += text[0];
            }
        }
    }
}

void LobbyScreen::render(Graphics::Framebuffer& fb, Network::GameServer* server, Network::GameClient* client) {
    // Backdrop
    fb.fillVerticalGradient(0, 0, m_width, m_height, 0xFF070C15, 0xFF141F30);

    m_btnBack.render(fb, 2);

    int cx = m_width / 2;

    if (m_mode == LobbyMode::CHOOSE_ROLE) {
        // Header
        UIComponents::drawArcadePanel(fb, cx - 240, 40, 480, 44, 0xEE0B121C, Theme::CARD_BORDER, Theme::GOLD, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 52, "REMOTE NETWORK MULTIPLAYER", Theme::GOLD, 2);

        Graphics::RasterFont::drawStringCentered(fb, cx, 120, "Play together from multiple laptops across LAN or Wi-Fi!", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawStringCentered(fb, cx, 140, "No split-screen - each racer has a dedicated full-screen view!", Theme::CYAN_UPGRADE, 1);

        m_btnHostMode.render(fb, 2);
        m_btnJoinMode.render(fb, 2);

        Graphics::RasterFont::drawStringCentered(fb, cx, m_height - 50, "HOST creates room; other laptops join using Host IP address.", Theme::TEXT_MUTED, 1);
    }
    else if (m_mode == LobbyMode::HOST_LOBBY) {
        // Top Header with Host IP banner
        std::string ip = server ? server->getLocalIpAddress().toStdString() : "127.0.0.1";
        std::string titleStr = "HOST LOBBY - SERVER IP: " + ip + ":7777";

        UIComponents::drawArcadePanel(fb, cx - 340, 16, 680, 42, 0xF00A1322, Theme::CARD_BORDER, Theme::GREEN_GAS, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 28, titleStr, Theme::GREEN_GAS, 2);

        int leftX = 40;
        int leftW = 400;
        int panY = 70;
        int panH = 390;

        // --- LEFT PANEL: ROOM CONFIGURATION ---
        UIComponents::drawArcadePanel(fb, leftX, panY, leftW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 2, true, true);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 16, "MATCH SETTINGS", Theme::GOLD, 2);

        // 1. Track selector
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 52, "RACE TRACK / BIOME:", Theme::TEXT_WHITE, 1);
        fb.fillRect(leftX + 68, panY + 75, 264, 34, 0xFF0E1724);
        fb.drawRect(leftX + 68, panY + 75, 264, 34, Theme::CYAN_UPGRADE);

        const auto& spec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
        Graphics::RasterFont::drawStringCentered(fb, leftX + 200, panY + 84, spec.name, Theme::TEXT_WHITE, 1);

        m_btnPrevTrack.setPosition(leftX + 20, panY + 75);
        m_btnNextTrack.setPosition(leftX + 338, panY + 75);
        m_btnPrevTrack.render(fb, 1);
        m_btnNextTrack.render(fb, 1);

        // 2. Host Vehicle selector
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 122, "YOUR VEHICLE:", Theme::TEXT_WHITE, 1);
        fb.fillRect(leftX + 68, panY + 145, 264, 34, 0xFF0E1724);
        fb.drawRect(leftX + 68, panY + 145, 264, 34, Theme::GOLD);

        const auto& hostVeh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(m_selectedVehicle));
        Graphics::RasterFont::drawStringCentered(fb, leftX + 200, panY + 154, hostVeh.name, Theme::TEXT_WHITE, 1);

        m_btnHostPrevVehicle.setPosition(leftX + 20, panY + 145);
        m_btnHostNextVehicle.setPosition(leftX + 338, panY + 145);
        m_btnHostPrevVehicle.render(fb, 1);
        m_btnHostNextVehicle.render(fb, 1);

        // 3. Grand Prix Toggle
        m_btnToggleGP.setPosition(leftX + 30, panY + 195);
        m_btnToggleGP.render(fb, 1);

        // 4. Connection instructions
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 245, "CONNECT INSTRUCTIONS:", Theme::GOLD, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 268, "1. Connect all laptops to same Wi-Fi / LAN.", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 290, "2. On other laptops, enter IP: " + ip, Theme::CYAN_UPGRADE, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 312, "3. When racers are ready, click LAUNCH below!", Theme::TEXT_MUTED, 1);

        // --- RIGHT PANEL: CONNECTED RACERS ROSTER ---
        int rightX = leftX + leftW + 25;
        int rightW = m_width - rightX - 40;
        UIComponents::drawArcadePanel(fb, rightX, panY, rightW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

        int playerCount = server ? server->getPlayerCount() : 1;
        std::string rosterTitle = "CONNECTED RACERS (" + std::to_string(playerCount) + " IN ROOM)";
        Graphics::RasterFont::drawString(fb, rightX + 20, panY + 16, rosterTitle, Theme::GOLD, 2);

        // 1. Host card
        fb.fillRect(rightX + 16, panY + 50, rightW - 32, 44, 0xDD121D2C);
        fb.drawRect(rightX + 16, panY + 50, rightW - 32, 44, 0xFFFF3D00);
        fb.fillCircle(rightX + 36, panY + 72, 8, 0xFFFF3D00);
        Graphics::RasterFont::drawString(fb, rightX + 54, panY + 58, "P1: YOU (HOST)", 0xFFFFFFFF, 2);
        Graphics::RasterFont::drawString(fb, rightX + 54, panY + 78, "[" + hostVeh.name + "]", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawString(fb, rightX + rightW - 120, panY + 64, "[HOST READY]", Theme::GREEN_GAS, 1);

        // 2. Connected client cards
        if (server) {
            int slotY = panY + 102;
            for (const auto& c : server->getClients()) {
                fb.fillRect(rightX + 16, slotY, rightW - 32, 44, 0xDD121D2C);
                fb.drawRect(rightX + 16, slotY, rightW - 32, 44, c.assignedColor);
                fb.fillCircle(rightX + 36, slotY + 22, 8, c.assignedColor);

                std::string pTag = "P" + std::to_string(c.id + 1) + ": " + c.name;
                Graphics::RasterFont::drawString(fb, rightX + 54, slotY + 8, pTag, 0xFFFFFFFF, 2);

                const auto& cVeh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(c.vehicleType));
                Graphics::RasterFont::drawString(fb, rightX + 54, slotY + 28, "[" + cVeh.name + "]", Theme::TEXT_MUTED, 1);

                std::string rdyStr = c.isReady ? "[READY]" : "[WAITING...]";
                uint32_t rdyCol = c.isReady ? Theme::GREEN_GAS : Theme::GOLD;
                Graphics::RasterFont::drawString(fb, rightX + rightW - 120, slotY + 14, rdyStr, rdyCol, 1);

                slotY += 52;
            }
        }

        m_btnStartRace.setPosition(rightX + (rightW - 260) / 2, panY + panH - 58);
        m_btnStartRace.render(fb, 2);
    }
    else if (m_mode == LobbyMode::JOIN_LOBBY) {
        bool connected = (client && client->isConnected());

        // Top Header
        std::string titleStr = connected ? "CONNECTED TO HOST ROOM" : "JOIN REMOTE RACE VIA HOST IP";
        uint32_t titleCol = connected ? Theme::GREEN_GAS : Theme::CYAN_UPGRADE;
        UIComponents::drawArcadePanel(fb, cx - 300, 16, 600, 42, 0xF00A1322, Theme::CARD_BORDER, titleCol, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 28, titleStr, titleCol, 2);

        int leftX = 40;
        int leftW = 400;
        int panY = 70;
        int panH = 390;

        // --- LEFT PANEL: CLIENT SETTINGS ---
        UIComponents::drawArcadePanel(fb, leftX, panY, leftW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 2, true, true);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 16, "JOIN SETTINGS", Theme::GOLD, 2);

        // Host IP Address input
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 52, "ENTER HOST IP ADDRESS:", Theme::GOLD, 1);
        fb.fillRect(leftX + 20, panY + 75, 218, 36, 0xFF0E1624);
        fb.drawRect(leftX + 20, panY + 75, 218, 36, Theme::CYAN_UPGRADE);

        std::string ipDisp = m_hostIpInput;
        if (!connected && static_cast<int>(m_animTime * 2.5f) % 2 == 0) ipDisp += "_";
        Graphics::RasterFont::drawString(fb, leftX + 28, panY + 85, ipDisp, 0xFFFFFFFF, 2);

        // Connect / Disconnect button
        m_btnConnect.setText(connected ? "DISCONNECT" : "CONNECT");
        m_btnConnect.setBaseColor(connected ? 0xFFC62828 : Theme::CYAN_UPGRADE);
        m_btnConnect.setPosition(leftX + 250, panY + 75);
        m_btnConnect.render(fb, 1);

        // Vehicle Picker
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 130, "CHOOSE YOUR VEHICLE:", Theme::TEXT_WHITE, 1);
        fb.fillRect(leftX + 68, panY + 155, 264, 34, 0xFF0E1624);
        fb.drawRect(leftX + 68, panY + 155, 264, 34, Theme::GOLD);

        const auto& veh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(m_selectedVehicle));
        Graphics::RasterFont::drawStringCentered(fb, leftX + 200, panY + 164, veh.name, Theme::TEXT_WHITE, 1);

        m_btnPrevVehicle.setPosition(leftX + 20, panY + 155);
        m_btnNextVehicle.setPosition(leftX + 338, panY + 155);
        m_btnPrevVehicle.render(fb, 1);
        m_btnNextVehicle.render(fb, 1);

        // Ready toggle button
        m_btnToggleReady.setPosition(leftX + 90, panY + 215);
        m_btnToggleReady.render(fb, 2);

        // Status banner inside left panel
        fb.fillRect(leftX + 20, panY + 285, leftW - 40, 75, 0xDD0D1624);
        fb.drawRect(leftX + 20, panY + 285, leftW - 40, 75, Theme::CARD_BORDER);

        Graphics::RasterFont::drawString(fb, leftX + 30, panY + 295, "STATUS:", Theme::GOLD, 1);
        if (!m_statusMessage.empty()) {
            Graphics::RasterFont::drawString(fb, leftX + 30, panY + 315, m_statusMessage, Theme::CYAN_UPGRADE, 1);
        } else if (connected) {
            Graphics::RasterFont::drawString(fb, leftX + 30, panY + 315, "Connected! Waiting for host launch...", Theme::GREEN_GAS, 1);
        } else {
            Graphics::RasterFont::drawString(fb, leftX + 30, panY + 315, "Type Host IP and click CONNECT.", Theme::TEXT_MUTED, 1);
            Graphics::RasterFont::drawString(fb, leftX + 30, panY + 335, "Port 7777 is used automatically.", Theme::TEXT_MUTED, 1);
        }

        // --- RIGHT PANEL: ROOM TRACK & CONNECTED ROSTER ---
        int rightX = leftX + leftW + 25;
        int rightW = m_width - rightX - 40;
        UIComponents::drawArcadePanel(fb, rightX, panY, rightW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

        if (connected && !m_clientLobbyPlayers.empty()) {
            // Selected Track Info Banner from Host
            const auto& trackSpec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
            std::string trackBanner = "ROOM TRACK: " + trackSpec.name;
            UIComponents::drawArcadePanel(fb, rightX + 16, panY + 14, rightW - 32, 34, 0xEE121E2E, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 1, false, false);
            Graphics::RasterFont::drawStringCentered(fb, rightX + rightW / 2, panY + 24, trackBanner, Theme::CYAN_UPGRADE, 1);

            int count = static_cast<int>(m_clientLobbyPlayers.size());
            std::string rTitle = "CONNECTED RACERS (" + std::to_string(count) + " IN ROOM)";
            Graphics::RasterFont::drawString(fb, rightX + 20, panY + 58, rTitle, Theme::GOLD, 1);

            int slotY = panY + 80;
            for (const auto& p : m_clientLobbyPlayers) {
                fb.fillRect(rightX + 16, slotY, rightW - 32, 44, 0xDD121D2C);
                fb.drawRect(rightX + 16, slotY, rightW - 32, 44, p.color);
                fb.fillCircle(rightX + 36, slotY + 22, 8, p.color);

                std::string tag = (p.isHost ? "HOST: " : ("P" + std::to_string(p.id + 1) + ": ")) + p.name;
                if (p.id == client->getLocalPlayerId()) tag += " (YOU)";
                Graphics::RasterFont::drawString(fb, rightX + 54, slotY + 8, tag, 0xFFFFFFFF, 2);

                const auto& pVeh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(p.vehicleType));
                Graphics::RasterFont::drawString(fb, rightX + 54, slotY + 28, "[" + pVeh.name + "]", Theme::TEXT_MUTED, 1);

                std::string rdyStr = p.isReady ? "[READY]" : "[WAITING...]";
                uint32_t rdyCol = p.isReady ? Theme::GREEN_GAS : Theme::GOLD;
                Graphics::RasterFont::drawString(fb, rightX + rightW - 120, slotY + 14, rdyStr, rdyCol, 1);

                slotY += 52;
            }
        } else {
            // Not connected yet - display connection guide
            Graphics::RasterFont::drawString(fb, rightX + 20, panY + 16, "HOW TO CONNECT OVER WI-FI", Theme::GOLD, 2);

            int guideY = panY + 56;
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY, "1. Make sure both laptops are on the same Wi-Fi.", Theme::TEXT_WHITE, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 30, "2. Have Player 1 click 'CREATE HOST ROOM'.", Theme::TEXT_WHITE, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 60, "3. Note the IP shown on Player 1's screen.", Theme::CYAN_UPGRADE, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 90, "4. Enter that IP into the box on the left.", Theme::TEXT_WHITE, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 120, "5. Click CONNECT and choose your car.", Theme::TEXT_WHITE, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 150, "6. Click SET READY when you are set!", Theme::GREEN_GAS, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 200, "Once the Host starts the race, both laptops", Theme::TEXT_MUTED, 1);
            Graphics::RasterFont::drawString(fb, rightX + 20, guideY + 220, "launch seamlessly into the same race track!", Theme::GOLD, 1);
        }
    }
}

} // namespace UI
