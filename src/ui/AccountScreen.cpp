#include "AccountScreen.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include <QKeyEvent>
#include <cmath>
#include <algorithm>

namespace UI {

AccountScreen::AccountScreen()
    : m_btnToggleMode(0, 0, 180, 36, "SWITCH TO LOGIN", 0xFF2A3A4E)
    , m_btnSubmit(0, 0, 180, 42, "CREATE ACCOUNT", Theme::GREEN_GAS)
    , m_btnGuest(0, 0, 180, 36, "PLAY AS GUEST", 0xFF37474F)
    , m_btnClose(0, 0, 100, 36, "BACK", 0xFF263238)
{
    m_usernameInput = "SpeedRacer";
    m_passwordInput = "1234";

    for (int i = 0; i < 4; ++i) {
        m_btnSuggestions[i] = RetroButton(0, 0, 170, 30, "", 0xFF192535);
        m_btnSuggestions[i].setOnClick([this, i]() {
            applySuggestion(i);
        });
    }

    m_btnToggleMode.setOnClick([this]() {
        m_isRegisterMode = !m_isRegisterMode;
        m_btnToggleMode.setText(m_isRegisterMode ? "SWITCH TO LOGIN" : "SWITCH TO REGISTER");
        m_btnSubmit.setText(m_isRegisterMode ? "CREATE ACCOUNT" : "LOGIN");
        m_statusMessage = "";
        m_suggestions.clear();
    });

    m_btnSubmit.setOnClick([this]() {
        std::string err;
        if (m_isRegisterMode) {
            std::vector<std::string> sugs;
            if (Core::AccountManager::instance().registerAccount(m_usernameInput, m_passwordInput, err, sugs)) {
                m_statusMessage = "ACCOUNT CREATED SUCCESSFULLY!";
                m_statusColor = Theme::GREEN_GAS;
                m_suggestions.clear();
                if (m_onLoggedIn) m_onLoggedIn(m_usernameInput);
            } else {
                m_statusMessage = err;
                m_statusColor = Theme::RED_BRAKE;
                m_suggestions = sugs;
                for (size_t i = 0; i < 4; ++i) {
                    if (i < m_suggestions.size()) {
                        m_btnSuggestions[i].setText(m_suggestions[i]);
                    } else {
                        m_btnSuggestions[i].setText("");
                    }
                }
            }
        } else {
            if (Core::AccountManager::instance().loginAccount(m_usernameInput, m_passwordInput, err)) {
                m_statusMessage = "WELCOME BACK, " + m_usernameInput + "!";
                m_statusColor = Theme::GREEN_GAS;
                m_suggestions.clear();
                if (m_onLoggedIn) m_onLoggedIn(m_usernameInput);
            } else {
                m_statusMessage = err;
                m_statusColor = Theme::RED_BRAKE;
            }
        }
    });

    m_btnGuest.setOnClick([this]() {
        Core::AccountManager::instance().logout();
        if (m_onLoggedIn) m_onLoggedIn("GuestRacer");
    });

    m_btnClose.setOnClick([this]() {
        if (m_onClose) m_onClose();
    });

    setDimensions(m_width, m_height);
}

void AccountScreen::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;

    int panelW = 480;
    int panelH = 430;
    int px = (m_width - panelW) / 2;
    int py = (m_height - panelH) / 2;

    m_btnClose.setPosition(px + 20, py + panelH - 52);
    m_btnToggleMode.setPosition(px + panelW / 2 - 90, py + panelH - 52);
    m_btnSubmit.setPosition(px + panelW - 200, py + panelH - 56);
    m_btnGuest.setPosition(px + panelW / 2 - 90, py + 230);

    for (int i = 0; i < 4; ++i) {
        int sx = (i % 2 == 0) ? px + 40 : px + 250;
        int sy = py + 320 + (i / 2) * 36;
        m_btnSuggestions[i].setPosition(sx, sy);
        m_btnSuggestions[i].setSize(190, 30);
    }
}

void AccountScreen::setButtonSound(std::function<void()> cb) {
    m_btnToggleMode.setOnSound(cb);
    m_btnSubmit.setOnSound(cb);
    m_btnGuest.setOnSound(cb);
    m_btnClose.setOnSound(cb);
    for (int i = 0; i < 4; ++i) {
        m_btnSuggestions[i].setOnSound(cb);
    }
}

void AccountScreen::applySuggestion(int idx) {
    if (idx >= 0 && idx < static_cast<int>(m_suggestions.size())) {
        m_usernameInput = m_suggestions[idx];
        m_statusMessage = "NAME APPLIED: " + m_usernameInput;
        m_statusColor = Theme::CYAN_UPGRADE;
    }
}

void AccountScreen::update(float dt) {
    m_animTime += dt;
}

void AccountScreen::onMouseMove(int px, int py) {
    m_btnToggleMode.onMouseMove(px, py);
    m_btnSubmit.onMouseMove(px, py);
    m_btnGuest.onMouseMove(px, py);
    m_btnClose.onMouseMove(px, py);
    for (int i = 0; i < 4; ++i) {
        if (i < static_cast<int>(m_suggestions.size())) {
            m_btnSuggestions[i].onMouseMove(px, py);
        }
    }
}

bool AccountScreen::onMouseDown(int px, int py) {
    if (m_btnToggleMode.onMouseDown(px, py)) return true;
    if (m_btnSubmit.onMouseDown(px, py)) return true;
    if (m_btnGuest.onMouseDown(px, py)) return true;
    if (m_btnClose.onMouseDown(px, py)) return true;

    for (int i = 0; i < 4; ++i) {
        if (i < static_cast<int>(m_suggestions.size())) {
            if (m_btnSuggestions[i].onMouseDown(px, py)) return true;
        }
    }

    // Input box clicking
    int panelW = 480;
    int panelH = 430;
    int panX = (m_width - panelW) / 2;
    int panY = (m_height - panelH) / 2;

    int boxX = panX + 40;
    int boxW = panelW - 80;

    // Username box
    if (px >= boxX && px <= boxX + boxW && py >= panY + 90 && py <= panY + 128) {
        m_activeInputIndex = 0;
        return true;
    }
    // Password box
    if (px >= boxX && px <= boxX + boxW && py >= panY + 155 && py <= panY + 193) {
        m_activeInputIndex = 1;
        return true;
    }

    return false;
}

void AccountScreen::onMouseUp(int px, int py) {
    m_btnToggleMode.onMouseUp(px, py);
    m_btnSubmit.onMouseUp(px, py);
    m_btnGuest.onMouseUp(px, py);
    m_btnClose.onMouseUp(px, py);
    for (int i = 0; i < 4; ++i) {
        m_btnSuggestions[i].onMouseUp(px, py);
    }
}

void AccountScreen::onKeyPress(int key, const std::string& text) {
    if (key == Qt::Key_Tab) {
        m_activeInputIndex = (m_activeInputIndex == 0) ? 1 : 0;
        return;
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        m_btnSubmit.triggerClick();
        return;
    }
    if (key == Qt::Key_Backspace) {
        if (m_activeInputIndex == 0 && !m_usernameInput.empty()) {
            m_usernameInput.pop_back();
        } else if (m_activeInputIndex == 1 && !m_passwordInput.empty()) {
            m_passwordInput.pop_back();
        }
        return;
    }

    // Text characters
    if (!text.empty() && (std::isalnum(text[0]) || text[0] == '_' || text[0] == '-')) {
        if (m_activeInputIndex == 0 && m_usernameInput.length() < 16) {
            m_usernameInput += text[0];
        } else if (m_activeInputIndex == 1 && m_passwordInput.length() < 20) {
            m_passwordInput += text[0];
        }
    }
}

void AccountScreen::render(Graphics::Framebuffer& fb) {
    // Dim background curtain
    fb.fillRect(0, 0, m_width, m_height, 0xCC060A12);

    int panelW = 480;
    int panelH = 430;
    int px = (m_width - panelW) / 2;
    int py = (m_height - panelH) / 2;

    // Center Arcade Panel
    UIComponents::drawArcadePanel(fb, px, py, panelW, panelH, 0xF40B1320, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 4, true, true);

    // Title
    std::string title = m_isRegisterMode ? "CREATE RACER PROFILE" : "LOGIN TO PROFILE";
    Graphics::RasterFont::drawStringCentered(fb, px + panelW / 2, py + 22, title, Theme::GOLD, 2);

    // Subtitle
    std::string sub = m_isRegisterMode ? "Choose a unique racer callsign" : "Enter your credentials";
    Graphics::RasterFont::drawStringCentered(fb, px + panelW / 2, py + 48, sub, Theme::TEXT_MUTED, 1);

    int boxX = px + 40;
    int boxW = panelW - 80;

    // --- 1. USERNAME FIELD ---
    Graphics::RasterFont::drawString(fb, boxX, py + 72, "CALLSIGN / USERNAME:", Theme::TEXT_WHITE, 1);
    uint32_t userBorder = (m_activeInputIndex == 0) ? Theme::CYAN_UPGRADE : 0xFF2A3A4E;
    fb.fillRect(boxX, py + 88, boxW, 36, 0xFF0E1622);
    fb.drawRect(boxX, py + 88, boxW, 36, userBorder);

    std::string displayUser = m_usernameInput;
    if (m_activeInputIndex == 0 && static_cast<int>(m_animTime * 2.5f) % 2 == 0) {
        displayUser += "_";
    }
    Graphics::RasterFont::drawString(fb, boxX + 12, py + 98, displayUser, 0xFFFFFFFF, 2);

    // --- 2. PASSWORD FIELD ---
    Graphics::RasterFont::drawString(fb, boxX, py + 138, "PASSWORD (OPTIONAL):", Theme::TEXT_WHITE, 1);
    uint32_t passBorder = (m_activeInputIndex == 1) ? Theme::CYAN_UPGRADE : 0xFF2A3A4E;
    fb.fillRect(boxX, py + 154, boxW, 36, 0xFF0E1622);
    fb.drawRect(boxX, py + 154, boxW, 36, passBorder);

    std::string displayPass(m_passwordInput.length(), '*');
    if (m_activeInputIndex == 1 && static_cast<int>(m_animTime * 2.5f) % 2 == 0) {
        displayPass += "_";
    }
    Graphics::RasterFont::drawString(fb, boxX + 12, py + 164, displayPass, 0xFFFFFFFF, 2);

    // --- 3. STATUS / ERROR MESSAGE ---
    if (!m_statusMessage.empty()) {
        Graphics::RasterFont::drawStringCentered(fb, px + panelW / 2, py + 202, m_statusMessage, m_statusColor, 1);
    }

    // --- 4. SUGGESTIONS BOX (If duplicate name) ---
    if (!m_suggestions.empty()) {
        Graphics::RasterFont::drawStringCentered(fb, px + panelW / 2, py + 300, "NAME TAKEN! TRY ONE OF THESE:", Theme::GOLD, 1);
        for (size_t i = 0; i < 4 && i < m_suggestions.size(); ++i) {
            m_btnSuggestions[i].render(fb, 1);
        }
    }

    // Buttons
    m_btnSubmit.render(fb, 2);
    m_btnToggleMode.render(fb, 1);
    m_btnGuest.render(fb, 1);
    m_btnClose.render(fb, 1);
}

} // namespace UI
