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
    , m_btnToggleGP(0, 0, 240, 36, "MODE: SINGLE RACE", 0xFF2A3A4E)
    , m_btnStartRace(0, 0, 260, 48, "LAUNCH RACE", Theme::GREEN_GAS)
    , m_btnConnect(0, 0, 140, 38, "CONNECT", Theme::CYAN_UPGRADE)
    , m_btnToggleReady(0, 0, 180, 44, "SET READY", Theme::GOLD)
    , m_btnPrevVehicle(0, 0, 42, 34, "<", 0xFF263238)
    , m_btnNextVehicle(0, 0, 42, 34, ">", 0xFF263238)
{
    m_btnBack.setKeyHint("ESC");
    m_btnStartRace.setKeyHint("ENTER");

    m_btnHostMode.setOnClick([this]() {
        setMode(LobbyMode::HOST_LOBBY);
    });

    m_btnJoinMode.setOnClick([this]() {
        setMode(LobbyMode::JOIN_LOBBY);
    });

    m_btnBack.setOnClick([this]() {
        if (m_mode != LobbyMode::CHOOSE_ROLE) {
            setMode(LobbyMode::CHOOSE_ROLE);
        } else if (m_onBack) {
            m_onBack();
        }
    });

    m_btnPrevTrack.setOnClick([this]() { cycleTrack(-1); });
    m_btnNextTrack.setOnClick([this]() { cycleTrack(1); });

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

    m_btnToggleReady.setOnClick([this]() {
        m_isLocalReady = !m_isLocalReady;
        m_btnToggleReady.setText(m_isLocalReady ? "READY (WAITING...)" : "NOT READY");
        m_btnToggleReady.setBaseColor(m_isLocalReady ? Theme::GREEN_GAS : Theme::GOLD);
    });

    m_btnPrevVehicle.setOnClick([this]() { cycleVehicle(-1); });
    m_btnNextVehicle.setOnClick([this]() { cycleVehicle(1); });

    setDimensions(m_width, m_height);
}

void LobbyScreen::setMode(LobbyMode mode) {
    m_mode = mode;
    m_statusMessage = "";
    m_isLocalReady = false;
    m_btnToggleReady.setText("SET READY");
    m_btnToggleReady.setBaseColor(Theme::GOLD);
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

    // Host screen
    int hostPanX = cx - 360;
    int hostPanY = 85;
    m_btnPrevTrack.setPosition(hostPanX + 20, hostPanY + 95);
    m_btnNextTrack.setPosition(hostPanX + 250, hostPanY + 95);
    m_btnToggleGP.setPosition(hostPanX + 20, hostPanY + 150);
    m_btnStartRace.setPosition(cx + 80, m_height - 75);

    // Join screen
    int joinPanX = cx - 360;
    int joinPanY = 85;
    m_btnConnect.setPosition(joinPanX + 220, joinPanY + 80);
    m_btnPrevVehicle.setPosition(joinPanX + 20, joinPanY + 185);
    m_btnNextVehicle.setPosition(joinPanX + 250, joinPanY + 185);
    m_btnToggleReady.setPosition(joinPanX + 40, joinPanY + 250);
}

void LobbyScreen::setButtonSound(std::function<void()> cb) {
    m_btnHostMode.setOnSound(cb);
    m_btnJoinMode.setOnSound(cb);
    m_btnBack.setOnSound(cb);
    m_btnPrevTrack.setOnSound(cb);
    m_btnNextTrack.setOnSound(cb);
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
}

void LobbyScreen::cycleVehicle(int direction) {
    int totalVehicles = static_cast<int>(Physics::VehicleType::COUNT);
    m_selectedVehicle = (m_selectedVehicle + direction + totalVehicles) % totalVehicles;
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

        int leftX = 60;
        int leftW = 380;
        int panY = 74;
        int panH = 370;

        // --- LEFT PANEL: ROOM CONFIGURATION ---
        UIComponents::drawArcadePanel(fb, leftX, panY, leftW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 2, true, true);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 18, "MATCH SETTINGS", Theme::GOLD, 2);

        // Track selector
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 60, "RACE TRACK / BIOME:", Theme::TEXT_WHITE, 1);
        fb.fillRect(leftX + 68, panY + 95, 176, 34, 0xFF0E1724);
        fb.drawRect(leftX + 68, panY + 95, 176, 34, Theme::CYAN_UPGRADE);

        const auto& spec = Physics::BiomeRegistry::getBiome(m_selectedBiome);
        Graphics::RasterFont::drawStringCentered(fb, leftX + 156, panY + 104, spec.name, Theme::TEXT_WHITE, 1);

        m_btnPrevTrack.setPosition(leftX + 20, panY + 95);
        m_btnNextTrack.setPosition(leftX + 250, panY + 95);
        m_btnPrevTrack.render(fb, 1);
        m_btnNextTrack.render(fb, 1);

        // Grand Prix Toggle
        m_btnToggleGP.setPosition(leftX + 20, panY + 145);
        m_btnToggleGP.render(fb, 1);

        // Connection instruction
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 200, "CONNECT INSTRUCTIONS:", Theme::GOLD, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 220, "1. Connect all laptops to same Wi-Fi / LAN.", Theme::TEXT_MUTED, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 240, "2. Tell other players to enter IP: " + ip, Theme::CYAN_UPGRADE, 1);
        Graphics::RasterFont::drawString(fb, leftX + 20, panY + 260, "3. When ready, click LAUNCH RACE below!", Theme::TEXT_MUTED, 1);

        // --- RIGHT PANEL: CONNECTED RACERS ROSTER ---
        int rightX = leftX + leftW + 30;
        int rightW = m_width - rightX - 60;
        UIComponents::drawArcadePanel(fb, rightX, panY, rightW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::GOLD, 2, true, true);

        int playerCount = server ? server->getPlayerCount() : 1;
        std::string rosterTitle = "CONNECTED RACERS (" + std::to_string(playerCount) + " LOBBY)";
        Graphics::RasterFont::drawString(fb, rightX + 20, panY + 18, rosterTitle, Theme::GOLD, 2);

        // 1. Host card
        fb.fillRect(rightX + 16, panY + 54, rightW - 32, 40, 0xDD121D2C);
        fb.drawRect(rightX + 16, panY + 54, rightW - 32, 40, 0xFFFF3D00);
        fb.fillCircle(rightX + 34, panY + 74, 8, 0xFFFF3D00);
        Graphics::RasterFont::drawString(fb, rightX + 50, panY + 64, "P1: YOU (HOST)", 0xFFFFFFFF, 2);
        Graphics::RasterFont::drawString(fb, rightX + rightW - 100, panY + 66, "[HOST READY]", Theme::GREEN_GAS, 1);

        // 2. Connected client cards
        if (server) {
            int slotY = panY + 104;
            for (const auto& c : server->getClients()) {
                fb.fillRect(rightX + 16, slotY, rightW - 32, 40, 0xDD121D2C);
                fb.drawRect(rightX + 16, slotY, rightW - 32, 40, c.assignedColor);
                fb.fillCircle(rightX + 34, slotY + 20, 8, c.assignedColor);

                std::string pTag = "P" + std::to_string(c.id + 1) + ": " + c.name;
                Graphics::RasterFont::drawString(fb, rightX + 50, slotY + 10, pTag, 0xFFFFFFFF, 2);

                std::string rdyStr = c.isReady ? "[READY]" : "[WAITING...]";
                uint32_t rdyCol = c.isReady ? Theme::GREEN_GAS : Theme::GOLD;
                Graphics::RasterFont::drawString(fb, rightX + rightW - 100, slotY + 12, rdyStr, rdyCol, 1);

                slotY += 48;
            }
        }

        m_btnStartRace.render(fb, 2);
    }
    else if (m_mode == LobbyMode::JOIN_LOBBY) {
        // Join Screen
        UIComponents::drawArcadePanel(fb, cx - 300, 16, 600, 42, 0xF00A1322, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 3, true, true);
        Graphics::RasterFont::drawStringCentered(fb, cx, 28, "JOIN REMOTE RACE VIA HOST IP", Theme::CYAN_UPGRADE, 2);

        int panW = 540;
        int panH = 390;
        int panX = cx - panW / 2;
        int panY = 75;

        UIComponents::drawArcadePanel(fb, panX, panY, panW, panH, 0xEE0B1422, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 2, true, true);

        // Host IP Address input
        Graphics::RasterFont::drawString(fb, panX + 40, panY + 30, "ENTER HOST LAPTOP IP ADDRESS:", Theme::GOLD, 1);
        fb.fillRect(panX + 40, panY + 54, 280, 36, 0xFF0E1624);
        fb.drawRect(panX + 40, panY + 54, 280, 36, Theme::CYAN_UPGRADE);

        std::string ipDisp = m_hostIpInput;
        if (static_cast<int>(m_animTime * 2.5f) % 2 == 0) ipDisp += "_";
        Graphics::RasterFont::drawString(fb, panX + 52, panY + 64, ipDisp, 0xFFFFFFFF, 2);

        m_btnConnect.setPosition(panX + 340, panY + 54);
        m_btnConnect.render(fb, 1);

        // Vehicle Picker
        Graphics::RasterFont::drawString(fb, panX + 40, panY + 115, "CHOOSE YOUR VEHICLE:", Theme::TEXT_WHITE, 1);
        fb.fillRect(panX + 90, panY + 140, 260, 36, 0xFF0E1624);
        fb.drawRect(panX + 90, panY + 140, 260, 36, Theme::GOLD);

        const auto& veh = Physics::VehicleRegistry::getConfig(static_cast<Physics::VehicleType>(m_selectedVehicle));
        Graphics::RasterFont::drawStringCentered(fb, panX + 220, panY + 150, veh.name, Theme::TEXT_WHITE, 1);

        m_btnPrevVehicle.setPosition(panX + 40, panY + 140);
        m_btnNextVehicle.setPosition(panX + 360, panY + 140);
        m_btnPrevVehicle.render(fb, 1);
        m_btnNextVehicle.render(fb, 1);

        // Ready toggle
        m_btnToggleReady.setPosition(panX + panW / 2 - 90, panY + 200);
        m_btnToggleReady.render(fb, 2);

        // Status
        if (!m_statusMessage.empty()) {
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 270, m_statusMessage, Theme::GOLD, 1);
        } else if (client && client->isConnected()) {
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 270, "CONNECTED TO HOST! WAITING FOR RACE LAUNCH...", Theme::GREEN_GAS, 1);
        } else {
            Graphics::RasterFont::drawStringCentered(fb, cx, panY + 270, "Type Host IP and click CONNECT.", Theme::TEXT_MUTED, 1);
        }
    }
}

} // namespace UI
