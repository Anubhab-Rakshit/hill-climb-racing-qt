#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include <string>
#include <functional>
#include <vector>

namespace Physics {
    class PhysicsWorld;
}

namespace Audio {
    class SoundManager;
}

namespace Core {
    class LocalizationManager;
}

namespace UI {

enum class DevTab {
    PHYSICS = 0,
    RACE_CONFIG = 1,
    AUDIO_LANG = 2
};

/**
 * @brief Interactive Developer Mode Console (F1).
 * Allows real-time parameter tweaking of physics, multiplayer rules,
 * audio tracks, and language on the running simulation without restarting.
 */
class DeveloperOverlay {
public:
    DeveloperOverlay();

    void setDimensions(int width, int height);

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py, Physics::PhysicsWorld& physics, Audio::SoundManager& audio);
    void onMouseUp(int px, int py);

    void render(Graphics::Framebuffer& fb, const Physics::PhysicsWorld& physics, const Audio::SoundManager& audio);

    void setOnClose(std::function<void()> cb) { m_onClose = cb; }

    bool isPointInside(int px, int py) const;

private:
    int m_width = 960;
    int m_height = 540;
    int m_panelX = 200;
    int m_panelY = 40;
    int m_panelW = 560;
    int m_panelH = 460;

    DevTab m_currentTab = DevTab::PHYSICS;

    // Navigation Tabs
    RetroButton m_tabPhysics;
    RetroButton m_tabRace;
    RetroButton m_tabAudio;
    RetroButton m_btnClose;

    // Presets
    RetroButton m_btnPresetDefault;
    RetroButton m_btnPresetLunar;
    RetroButton m_btnPresetHyper;
    RetroButton m_btnPresetDrift;
    RetroButton m_btnPresetTank;

    // Steppers (+/- buttons)
    RetroButton m_btnGravMinus;
    RetroButton m_btnGravPlus;
    RetroButton m_btnSuspKMinus;
    RetroButton m_btnSuspKPlus;
    RetroButton m_btnSuspCMinus;
    RetroButton m_btnSuspCPlus;
    RetroButton m_btnTorqueMinus;
    RetroButton m_btnTorquePlus;
    RetroButton m_btnGripMinus;
    RetroButton m_btnGripPlus;
    RetroButton m_btnGyroMinus;
    RetroButton m_btnGyroPlus;

    // Race Config Buttons
    RetroButton m_btnFinish500;
    RetroButton m_btnFinish1000;
    RetroButton m_btnFinish1500;
    RetroButton m_btnFinish3000;
    RetroButton m_btnFinishInf;

    RetroButton m_btnFuelInf;
    RetroButton m_btnFuelHalf;
    RetroButton m_btnFuelNormal;
    RetroButton m_btnFuelDouble;

    RetroButton m_btnRubberbanding;

    // Audio & Lang Buttons
    RetroButton m_btnMusicPokemon;
    RetroButton m_btnMusicSoothing;
    RetroButton m_btnMusicEdgy;
    RetroButton m_btnMusicEnergetic;

    RetroButton m_btnToggleLang;
    RetroButton m_btnGodMode;

    std::function<void()> m_onClose;

    void updateLayout();
};

} // namespace UI
