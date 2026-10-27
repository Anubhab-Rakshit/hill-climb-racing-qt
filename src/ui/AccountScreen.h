#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "AccountManager.h"
#include <string>
#include <vector>
#include <functional>

namespace UI {

/**
 * @brief Interactive Account Registration & Login Screen.
 * Enforces unique names with real-time feedback and smart AI name suggestions.
 */
class AccountScreen {
public:
    AccountScreen();

    void setDimensions(int width, int height);

    void setOnClose(std::function<void()> cb) { m_onClose = cb; }
    void setOnLoggedIn(std::function<void(const std::string& username)> cb) { m_onLoggedIn = cb; }
    void setButtonSound(std::function<void()> cb);

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);
    void onKeyPress(int key, const std::string& text);

    void update(float dt);
    void render(Graphics::Framebuffer& fb);

private:
    int m_width = 960;
    int m_height = 540;
    float m_animTime = 0.0f;

    bool m_isRegisterMode = true; // true = Register, false = Login
    int m_activeInputIndex = 0;   // 0 = Username, 1 = Password

    std::string m_usernameInput;
    std::string m_passwordInput;
    std::string m_statusMessage;
    uint32_t m_statusColor = 0xFFFFFFFF;
    std::vector<std::string> m_suggestions;

    RetroButton m_btnToggleMode;
    RetroButton m_btnSubmit;
    RetroButton m_btnGuest;
    RetroButton m_btnClose;
    RetroButton m_btnSuggestions[4];

    std::function<void()> m_onClose;
    std::function<void(const std::string&)> m_onLoggedIn;

    void applySuggestion(int idx);
};

} // namespace UI
