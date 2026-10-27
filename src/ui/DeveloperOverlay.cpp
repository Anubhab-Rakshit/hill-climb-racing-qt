#include "DeveloperOverlay.h"
#include "UITheme.h"
#include "UIComponents.h"
#include "RasterFont.h"
#include "PhysicsWorld.h"
#include "SoundManager.h"
#include "LocalizationManager.h"
#include <iomanip>
#include <sstream>

namespace UI {

DeveloperOverlay::DeveloperOverlay()
    : m_tabPhysics(0, 0, 110, 30, "PHYSICS", Theme::CYAN_UPGRADE)
    , m_tabRace(0, 0, 130, 30, "RACE & TROPHY", Theme::CARD_BORDER)
    , m_tabAudio(0, 0, 130, 30, "AUDIO & LANG", Theme::CARD_BORDER)
    , m_btnClose(0, 0, 90, 30, "CLOSE [F1]", Theme::RED_BRAKE)
    // Presets
    , m_btnPresetDefault(0, 0, 96, 26, "DEFAULT", 0xFF37474F)
    , m_btnPresetLunar(0, 0, 96, 26, "LUNAR", 0xFF5E35B1)
    , m_btnPresetHyper(0, 0, 96, 26, "HYPER", 0xFFD84315)
    , m_btnPresetDrift(0, 0, 96, 26, "DRIFT", 0xFF00897B)
    , m_btnPresetTank(0, 0, 96, 26, "TANK", 0xFF4E342E)
    // Steppers
    , m_btnGravMinus(0, 0, 28, 24, "-", Theme::CARD_BORDER)
    , m_btnGravPlus(0, 0, 28, 24, "+", Theme::CARD_BORDER)
    , m_btnSuspKMinus(0, 0, 28, 24, "-", Theme::CARD_BORDER)
    , m_btnSuspKPlus(0, 0, 28, 24, "+", Theme::CARD_BORDER)
    , m_btnSuspCMinus(0, 0, 28, 24, "-", Theme::CARD_BORDER)
    , m_btnSuspCPlus(0, 0, 28, 24, "+", Theme::CARD_BORDER)
    , m_btnTorqueMinus(0, 0, 28, 24, "-", Theme::CARD_BORDER)
    , m_btnTorquePlus(0, 0, 28, 24, "+", Theme::CARD_BORDER)
    , m_btnGripMinus(0, 0, 28, 24, "-", Theme::CARD_BORDER)
    , m_btnGripPlus(0, 0, 28, 24, "+", Theme::CARD_BORDER)
    , m_btnGyroMinus(0, 0, 28, 24, "-", Theme::CARD_BORDER)
    , m_btnGyroPlus(0, 0, 28, 24, "+", Theme::CARD_BORDER)
    // Race Config
    , m_btnFinish500(0, 0, 90, 26, "500 M", Theme::CARD_BORDER)
    , m_btnFinish1000(0, 0, 90, 26, "1000 M", Theme::CARD_BORDER)
    , m_btnFinish1500(0, 0, 90, 26, "1500 M", Theme::CYAN_UPGRADE)
    , m_btnFinish3000(0, 0, 90, 26, "3000 M", Theme::CARD_BORDER)
    , m_btnFinishInf(0, 0, 90, 26, "INFINITE", Theme::CARD_BORDER)
    , m_btnFuelInf(0, 0, 110, 26, "INFINITE (0X)", Theme::CARD_BORDER)
    , m_btnFuelHalf(0, 0, 110, 26, "HALF (0.5X)", Theme::CARD_BORDER)
    , m_btnFuelNormal(0, 0, 110, 26, "NORMAL (1.0X)", Theme::GOLD)
    , m_btnFuelDouble(0, 0, 110, 26, "DOUBLE (2.0X)", Theme::CARD_BORDER)
    , m_btnRubberbanding(0, 0, 220, 28, "RUBBERBANDING: OFF", Theme::CARD_BORDER)
    // Audio & Lang
    , m_btnMusicPokemon(0, 0, 220, 28, "POKEMON 8-BIT GB", 0xFFE91E63)
    , m_btnMusicSoothing(0, 0, 220, 28, "SOOTHING PASTORAL", 0xFF009688)
    , m_btnMusicEdgy(0, 0, 220, 28, "EDGY SYNTHWAVE", 0xFF673AB7)
    , m_btnMusicEnergetic(0, 0, 220, 28, "ENERGETIC ARCADE", 0xFFFF9800)
    , m_btnToggleLang(0, 0, 220, 32, "LANGUAGE: ENGLISH", Theme::GOLD)
    , m_btnGodMode(0, 0, 220, 32, "INFINITE FUEL: OFF", Theme::CARD_BORDER)
{
    updateLayout();
}

void DeveloperOverlay::setDimensions(int width, int height) {
    m_width = width;
    m_height = height;
    m_panelW = 600;
    m_panelH = 450;
    m_panelX = (m_width - m_panelW) / 2;
    m_panelY = (m_height - m_panelH) / 2;
    updateLayout();
}

void DeveloperOverlay::updateLayout() {
    int topY = m_panelY + 12;
    m_tabPhysics.setPosition(m_panelX + 16, topY);
    m_tabRace.setPosition(m_panelX + 132, topY);
    m_tabAudio.setPosition(m_panelX + 268, topY);
    m_btnClose.setPosition(m_panelX + m_panelW - 106, topY);

    // Presets Row
    int prY = m_panelY + 54;
    int prX = m_panelX + 24;
    int prW = 104;
    m_btnPresetDefault.setPosition(prX, prY);
    m_btnPresetLunar.setPosition(prX + prW + 6, prY);
    m_btnPresetHyper.setPosition(prX + (prW + 6) * 2, prY);
    m_btnPresetDrift.setPosition(prX + (prW + 6) * 3, prY);
    m_btnPresetTank.setPosition(prX + (prW + 6) * 4, prY);

    // Physics Steppers
    int stY = m_panelY + 100;
    int dy = 38;
    int plusX = m_panelX + m_panelW - 60;
    int minusX = plusX - 100;

    m_btnGravMinus.setPosition(minusX, stY);
    m_btnGravPlus.setPosition(plusX, stY);

    m_btnSuspKMinus.setPosition(minusX, stY + dy);
    m_btnSuspKPlus.setPosition(plusX, stY + dy);

    m_btnSuspCMinus.setPosition(minusX, stY + dy * 2);
    m_btnSuspCPlus.setPosition(plusX, stY + dy * 2);

    m_btnTorqueMinus.setPosition(minusX, stY + dy * 3);
    m_btnTorquePlus.setPosition(plusX, stY + dy * 3);

    m_btnGripMinus.setPosition(minusX, stY + dy * 4);
    m_btnGripPlus.setPosition(plusX, stY + dy * 4);

    m_btnGyroMinus.setPosition(minusX, stY + dy * 5);
    m_btnGyroPlus.setPosition(plusX, stY + dy * 5);

    // Race Config layout
    int rY = m_panelY + 100;
    m_btnFinish500.setPosition(m_panelX + 30, rY + 30);
    m_btnFinish1000.setPosition(m_panelX + 136, rY + 30);
    m_btnFinish1500.setPosition(m_panelX + 242, rY + 30);
    m_btnFinish3000.setPosition(m_panelX + 348, rY + 30);
    m_btnFinishInf.setPosition(m_panelX + 454, rY + 30);

    m_btnFuelInf.setPosition(m_panelX + 30, rY + 120);
    m_btnFuelHalf.setPosition(m_panelX + 160, rY + 120);
    m_btnFuelNormal.setPosition(m_panelX + 290, rY + 120);
    m_btnFuelDouble.setPosition(m_panelX + 420, rY + 120);

    m_btnRubberbanding.setPosition(m_panelX + 180, rY + 200);

    // Audio & Lang layout
    int aY = m_panelY + 90;
    m_btnMusicPokemon.setPosition(m_panelX + 40, aY + 30);
    m_btnMusicSoothing.setPosition(m_panelX + 300, aY + 30);
    m_btnMusicEdgy.setPosition(m_panelX + 40, aY + 74);
    m_btnMusicEnergetic.setPosition(m_panelX + 300, aY + 74);

    m_btnToggleLang.setPosition(m_panelX + 170, aY + 160);
    m_btnGodMode.setPosition(m_panelX + 170, aY + 210);
}

bool DeveloperOverlay::isPointInside(int px, int py) const {
    return px >= m_panelX && px <= m_panelX + m_panelW &&
           py >= m_panelY && py <= m_panelY + m_panelH;
}

void DeveloperOverlay::onMouseMove(int px, int py) {
    m_tabPhysics.onMouseMove(px, py);
    m_tabRace.onMouseMove(px, py);
    m_tabAudio.onMouseMove(px, py);
    m_btnClose.onMouseMove(px, py);

    if (m_currentTab == DevTab::PHYSICS) {
        m_btnPresetDefault.onMouseMove(px, py);
        m_btnPresetLunar.onMouseMove(px, py);
        m_btnPresetHyper.onMouseMove(px, py);
        m_btnPresetDrift.onMouseMove(px, py);
        m_btnPresetTank.onMouseMove(px, py);

        m_btnGravMinus.onMouseMove(px, py);
        m_btnGravPlus.onMouseMove(px, py);
        m_btnSuspKMinus.onMouseMove(px, py);
        m_btnSuspKPlus.onMouseMove(px, py);
        m_btnSuspCMinus.onMouseMove(px, py);
        m_btnSuspCPlus.onMouseMove(px, py);
        m_btnTorqueMinus.onMouseMove(px, py);
        m_btnTorquePlus.onMouseMove(px, py);
        m_btnGripMinus.onMouseMove(px, py);
        m_btnGripPlus.onMouseMove(px, py);
        m_btnGyroMinus.onMouseMove(px, py);
        m_btnGyroPlus.onMouseMove(px, py);
    } else if (m_currentTab == DevTab::RACE_CONFIG) {
        m_btnFinish500.onMouseMove(px, py);
        m_btnFinish1000.onMouseMove(px, py);
        m_btnFinish1500.onMouseMove(px, py);
        m_btnFinish3000.onMouseMove(px, py);
        m_btnFinishInf.onMouseMove(px, py);
        m_btnFuelInf.onMouseMove(px, py);
        m_btnFuelHalf.onMouseMove(px, py);
        m_btnFuelNormal.onMouseMove(px, py);
        m_btnFuelDouble.onMouseMove(px, py);
        m_btnRubberbanding.onMouseMove(px, py);
    } else if (m_currentTab == DevTab::AUDIO_LANG) {
        m_btnMusicPokemon.onMouseMove(px, py);
        m_btnMusicSoothing.onMouseMove(px, py);
        m_btnMusicEdgy.onMouseMove(px, py);
        m_btnMusicEnergetic.onMouseMove(px, py);
        m_btnToggleLang.onMouseMove(px, py);
        m_btnGodMode.onMouseMove(px, py);
    }
}

bool DeveloperOverlay::onMouseDown(int px, int py, Physics::PhysicsWorld& physics, Audio::SoundManager& audio) {
    if (m_btnClose.onMouseDown(px, py)) {
        if (m_onClose) m_onClose();
        return true;
    }

    if (m_tabPhysics.onMouseDown(px, py)) {
        m_currentTab = DevTab::PHYSICS;
        m_tabPhysics.setBaseColor(Theme::CYAN_UPGRADE);
        m_tabRace.setBaseColor(Theme::CARD_BORDER);
        m_tabAudio.setBaseColor(Theme::CARD_BORDER);
        return true;
    }
    if (m_tabRace.onMouseDown(px, py)) {
        m_currentTab = DevTab::RACE_CONFIG;
        m_tabPhysics.setBaseColor(Theme::CARD_BORDER);
        m_tabRace.setBaseColor(Theme::CYAN_UPGRADE);
        m_tabAudio.setBaseColor(Theme::CARD_BORDER);
        return true;
    }
    if (m_tabAudio.onMouseDown(px, py)) {
        m_currentTab = DevTab::AUDIO_LANG;
        m_tabPhysics.setBaseColor(Theme::CARD_BORDER);
        m_tabRace.setBaseColor(Theme::CARD_BORDER);
        m_tabAudio.setBaseColor(Theme::CYAN_UPGRADE);
        return true;
    }

    if (m_currentTab == DevTab::PHYSICS) {
        // Presets
        if (m_btnPresetDefault.onMouseDown(px, py)) {
            physics.setCustomGravity(-9.81f);
            physics.setSuspStiffnessMultiplier(1.0f);
            physics.setSuspDampingMultiplier(1.0f);
            physics.setCustomTorqueMultiplier(1.0f);
            physics.setCustomFrictionMultiplier(1.0f);
            physics.setCustomAirGyroMultiplier(1.0f);
            return true;
        }
        if (m_btnPresetLunar.onMouseDown(px, py)) {
            physics.setCustomGravity(-2.5f);
            physics.setSuspStiffnessMultiplier(1.4f);
            physics.setSuspDampingMultiplier(0.5f);
            physics.setCustomTorqueMultiplier(1.2f);
            physics.setCustomFrictionMultiplier(0.9f);
            physics.setCustomAirGyroMultiplier(2.2f);
            return true;
        }
        if (m_btnPresetHyper.onMouseDown(px, py)) {
            physics.setCustomGravity(-12.0f);
            physics.setSuspStiffnessMultiplier(1.3f);
            physics.setSuspDampingMultiplier(1.4f);
            physics.setCustomTorqueMultiplier(2.6f);
            physics.setCustomFrictionMultiplier(1.6f);
            physics.setCustomAirGyroMultiplier(1.6f);
            return true;
        }
        if (m_btnPresetDrift.onMouseDown(px, py)) {
            physics.setCustomGravity(-9.81f);
            physics.setSuspStiffnessMultiplier(0.9f);
            physics.setSuspDampingMultiplier(0.8f);
            physics.setCustomTorqueMultiplier(1.6f);
            physics.setCustomFrictionMultiplier(0.40f);
            physics.setCustomAirGyroMultiplier(1.8f);
            return true;
        }
        if (m_btnPresetTank.onMouseDown(px, py)) {
            physics.setCustomGravity(-15.0f);
            physics.setSuspStiffnessMultiplier(2.2f);
            physics.setSuspDampingMultiplier(2.5f);
            physics.setCustomTorqueMultiplier(2.0f);
            physics.setCustomFrictionMultiplier(1.8f);
            physics.setCustomAirGyroMultiplier(0.5f);
            return true;
        }

        // Steppers
        if (m_btnGravMinus.onMouseDown(px, py)) {
            float g = physics.customGravity() - 1.0f;
            if (g < -25.0f) g = -25.0f;
            physics.setCustomGravity(g);
            return true;
        }
        if (m_btnGravPlus.onMouseDown(px, py)) {
            float g = physics.customGravity() + 1.0f;
            if (g > -2.0f) g = -2.0f;
            physics.setCustomGravity(g);
            return true;
        }
        if (m_btnSuspKMinus.onMouseDown(px, py)) {
            float m = std::max(0.2f, physics.suspStiffnessMultiplier() - 0.1f);
            physics.setSuspStiffnessMultiplier(m);
            return true;
        }
        if (m_btnSuspKPlus.onMouseDown(px, py)) {
            float m = std::min(3.0f, physics.suspStiffnessMultiplier() + 0.1f);
            physics.setSuspStiffnessMultiplier(m);
            return true;
        }
        if (m_btnSuspCMinus.onMouseDown(px, py)) {
            float m = std::max(0.2f, physics.suspDampingMultiplier() - 0.1f);
            physics.setSuspDampingMultiplier(m);
            return true;
        }
        if (m_btnSuspCPlus.onMouseDown(px, py)) {
            float m = std::min(3.0f, physics.suspDampingMultiplier() + 0.1f);
            physics.setSuspDampingMultiplier(m);
            return true;
        }
        if (m_btnTorqueMinus.onMouseDown(px, py)) {
            float m = std::max(0.2f, physics.customTorqueMultiplier() - 0.1f);
            physics.setCustomTorqueMultiplier(m);
            return true;
        }
        if (m_btnTorquePlus.onMouseDown(px, py)) {
            float m = std::min(4.0f, physics.customTorqueMultiplier() + 0.1f);
            physics.setCustomTorqueMultiplier(m);
            return true;
        }
        if (m_btnGripMinus.onMouseDown(px, py)) {
            float m = std::max(0.2f, physics.customFrictionMultiplier() - 0.1f);
            physics.setCustomFrictionMultiplier(m);
            return true;
        }
        if (m_btnGripPlus.onMouseDown(px, py)) {
            float m = std::min(3.0f, physics.customFrictionMultiplier() + 0.1f);
            physics.setCustomFrictionMultiplier(m);
            return true;
        }
        if (m_btnGyroMinus.onMouseDown(px, py)) {
            float m = std::max(0.2f, physics.customAirGyroMultiplier() - 0.2f);
            physics.setCustomAirGyroMultiplier(m);
            return true;
        }
        if (m_btnGyroPlus.onMouseDown(px, py)) {
            float m = std::min(5.0f, physics.customAirGyroMultiplier() + 0.2f);
            physics.setCustomAirGyroMultiplier(m);
            return true;
        }
    } else if (m_currentTab == DevTab::RACE_CONFIG) {
        if (m_btnFinish500.onMouseDown(px, py)) { physics.setFinishDistance(500.0f); return true; }
        if (m_btnFinish1000.onMouseDown(px, py)) { physics.setFinishDistance(1000.0f); return true; }
        if (m_btnFinish1500.onMouseDown(px, py)) { physics.setFinishDistance(1500.0f); return true; }
        if (m_btnFinish3000.onMouseDown(px, py)) { physics.setFinishDistance(3000.0f); return true; }
        if (m_btnFinishInf.onMouseDown(px, py)) { physics.setFinishDistance(99999.0f); return true; }

        if (m_btnFuelInf.onMouseDown(px, py)) { physics.setInfiniteFuel(true); return true; }
        if (m_btnFuelHalf.onMouseDown(px, py)) { physics.setInfiniteFuel(false); physics.setFuelBurnMultiplier(0.5f); return true; }
        if (m_btnFuelNormal.onMouseDown(px, py)) { physics.setInfiniteFuel(false); physics.setFuelBurnMultiplier(1.0f); return true; }
        if (m_btnFuelDouble.onMouseDown(px, py)) { physics.setInfiniteFuel(false); physics.setFuelBurnMultiplier(2.0f); return true; }

        if (m_btnRubberbanding.onMouseDown(px, py)) {
            physics.setRubberbanding(!physics.isRubberbanding());
            m_btnRubberbanding.setText(physics.isRubberbanding() ? "RUBBERBANDING: ON" : "RUBBERBANDING: OFF");
            m_btnRubberbanding.setBaseColor(physics.isRubberbanding() ? Theme::GREEN_GAS : Theme::CARD_BORDER);
            return true;
        }
    } else if (m_currentTab == DevTab::AUDIO_LANG) {
        if (m_btnMusicPokemon.onMouseDown(px, py)) {
            audio.setTargetMusic(Audio::MusicTrack::POKEMON_CHIPTUNE);
            return true;
        }
        if (m_btnMusicSoothing.onMouseDown(px, py)) {
            audio.setTargetMusic(Audio::MusicTrack::SOOTHING_PASTORAL);
            return true;
        }
        if (m_btnMusicEdgy.onMouseDown(px, py)) {
            audio.setTargetMusic(Audio::MusicTrack::EDGY_SYNTHWAVE);
            return true;
        }
        if (m_btnMusicEnergetic.onMouseDown(px, py)) {
            audio.setTargetMusic(Audio::MusicTrack::ENERGETIC_ARCADE);
            return true;
        }

        if (m_btnToggleLang.onMouseDown(px, py)) {
            Core::LocalizationManager::instance().toggleLanguage();
            bool isBn = Core::LocalizationManager::instance().isBengali();
            m_btnToggleLang.setText(isBn ? "LANGUAGE: BANGLA (BANGLI)" : "LANGUAGE: ENGLISH");
            m_btnToggleLang.setBaseColor(isBn ? Theme::GOLD : Theme::CYAN_UPGRADE);
            return true;
        }

        if (m_btnGodMode.onMouseDown(px, py)) {
            physics.setInfiniteFuel(!physics.isInfiniteFuel());
            m_btnGodMode.setText(physics.isInfiniteFuel() ? "INFINITE FUEL: ON" : "INFINITE FUEL: OFF");
            m_btnGodMode.setBaseColor(physics.isInfiniteFuel() ? Theme::GREEN_GAS : Theme::CARD_BORDER);
            return true;
        }
    }

    return isPointInside(px, py);
}

void DeveloperOverlay::onMouseUp(int px, int py) {
    m_tabPhysics.onMouseUp(px, py);
    m_tabRace.onMouseUp(px, py);
    m_tabAudio.onMouseUp(px, py);
    m_btnClose.onMouseUp(px, py);

    m_btnPresetDefault.onMouseUp(px, py);
    m_btnPresetLunar.onMouseUp(px, py);
    m_btnPresetHyper.onMouseUp(px, py);
    m_btnPresetDrift.onMouseUp(px, py);
    m_btnPresetTank.onMouseUp(px, py);

    m_btnGravMinus.onMouseUp(px, py);
    m_btnGravPlus.onMouseUp(px, py);
    m_btnSuspKMinus.onMouseUp(px, py);
    m_btnSuspKPlus.onMouseUp(px, py);
    m_btnSuspCMinus.onMouseUp(px, py);
    m_btnSuspCPlus.onMouseUp(px, py);
    m_btnTorqueMinus.onMouseUp(px, py);
    m_btnTorquePlus.onMouseUp(px, py);
    m_btnGripMinus.onMouseUp(px, py);
    m_btnGripPlus.onMouseUp(px, py);
    m_btnGyroMinus.onMouseUp(px, py);
    m_btnGyroPlus.onMouseUp(px, py);

    m_btnFinish500.onMouseUp(px, py);
    m_btnFinish1000.onMouseUp(px, py);
    m_btnFinish1500.onMouseUp(px, py);
    m_btnFinish3000.onMouseUp(px, py);
    m_btnFinishInf.onMouseUp(px, py);
    m_btnFuelInf.onMouseUp(px, py);
    m_btnFuelHalf.onMouseUp(px, py);
    m_btnFuelNormal.onMouseUp(px, py);
    m_btnFuelDouble.onMouseUp(px, py);
    m_btnRubberbanding.onMouseUp(px, py);

    m_btnMusicPokemon.onMouseUp(px, py);
    m_btnMusicSoothing.onMouseUp(px, py);
    m_btnMusicEdgy.onMouseUp(px, py);
    m_btnMusicEnergetic.onMouseUp(px, py);
    m_btnToggleLang.onMouseUp(px, py);
    m_btnGodMode.onMouseUp(px, py);
}

void DeveloperOverlay::render(Graphics::Framebuffer& fb, const Physics::PhysicsWorld& physics, const Audio::SoundManager& audio) {
    (void)audio;
    // Dark transparent backing scrim
    fb.fillRect(0, 0, m_width, m_height, 0xAA040810);

    // Main Cyber/Terminal Panel Frame
    UIComponents::drawArcadePanel(fb, m_panelX, m_panelY, m_panelW, m_panelH, 0xFA0A1220, Theme::CARD_BORDER, Theme::CYAN_UPGRADE, 3, true, true);

    // Header Title
    std::string title = Core::LocalizationManager::instance().isBengali() ?
        "ENGINEER BABUR CONSOLE (DEVELOPER MODE)" : "DEVELOPER CONSOLE & LIVE TWEAKER";
    Graphics::RasterFont::drawString(fb, m_panelX + 20, m_panelY + 16, title, Theme::GOLD, 1);

    // Render Tabs & Close
    m_tabPhysics.render(fb, 1);
    m_tabRace.render(fb, 1);
    m_tabAudio.render(fb, 1);
    m_btnClose.render(fb, 1);

    int contentY = m_panelY + 54;

    if (m_currentTab == DevTab::PHYSICS) {
        // Quick Presets
        Graphics::RasterFont::drawString(fb, m_panelX + 24, contentY - 8, "PHYSICS PRESETS:", Theme::TEXT_MUTED, 1);
        m_btnPresetDefault.render(fb, 1);
        m_btnPresetLunar.render(fb, 1);
        m_btnPresetHyper.render(fb, 1);
        m_btnPresetDrift.render(fb, 1);
        m_btnPresetTank.render(fb, 1);

        // Divider
        fb.drawLine(m_panelX + 20, contentY + 34, m_panelX + m_panelW - 20, contentY + 34, 0xFF1C2B3E);

        // Parameters table
        auto drawParamRow = [&](int y, const std::string& label, const std::string& valStr, RetroButton& bMinus, RetroButton& bPlus) {
            Graphics::RasterFont::drawString(fb, m_panelX + 30, y + 6, label, Theme::TEXT_WHITE, 1);
            bMinus.render(fb, 1);
            Graphics::RasterFont::drawStringCentered(fb, bMinus.x() + 50, y + 6, valStr, Theme::GOLD, 1);
            bPlus.render(fb, 1);
        };

        int stY = m_panelY + 100;
        int dy = 38;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);

        ss.str(""); ss << physics.customGravity() << " m/s2";
        drawParamRow(stY, "GRAVITY ACCELERATION:", ss.str(), m_btnGravMinus, m_btnGravPlus);

        ss.str(""); ss << physics.suspStiffnessMultiplier() << "x";
        drawParamRow(stY + dy, "SUSPENSION STIFFNESS (k):", ss.str(), m_btnSuspKMinus, m_btnSuspKPlus);

        ss.str(""); ss << physics.suspDampingMultiplier() << "x";
        drawParamRow(stY + dy * 2, "SUSPENSION DAMPING (c):", ss.str(), m_btnSuspCMinus, m_btnSuspCPlus);

        ss.str(""); ss << physics.customTorqueMultiplier() << "x";
        drawParamRow(stY + dy * 3, "ENGINE TORQUE MULTIPLIER:", ss.str(), m_btnTorqueMinus, m_btnTorquePlus);

        ss.str(""); ss << physics.customFrictionMultiplier() << "x";
        drawParamRow(stY + dy * 4, "TYRE GRIP / FRICTION (mu):", ss.str(), m_btnGripMinus, m_btnGripPlus);

        ss.str(""); ss << physics.customAirGyroMultiplier() << "x";
        drawParamRow(stY + dy * 5, "AIR ATTITUDE GYRO TORQUE:", ss.str(), m_btnGyroMinus, m_btnGyroPlus);

        Graphics::RasterFont::drawStringCentered(fb, m_panelX + m_panelW / 2, m_panelY + m_panelH - 24,
            "Parameters apply instantly to running vehicle without restart", Theme::TEXT_MUTED, 1);
    } else if (m_currentTab == DevTab::RACE_CONFIG) {
        int rY = m_panelY + 80;
        Graphics::RasterFont::drawString(fb, m_panelX + 30, rY, "MULTIPLAYER FINISH LINE DISTANCE:", Theme::TEXT_WHITE, 1);

        float finDist = physics.finishDistance();
        m_btnFinish500.setBaseColor(finDist == 500.0f ? Theme::CYAN_UPGRADE : Theme::CARD_BORDER);
        m_btnFinish1000.setBaseColor(finDist == 1000.0f ? Theme::CYAN_UPGRADE : Theme::CARD_BORDER);
        m_btnFinish1500.setBaseColor(finDist == 1500.0f ? Theme::CYAN_UPGRADE : Theme::CARD_BORDER);
        m_btnFinish3000.setBaseColor(finDist == 3000.0f ? Theme::CYAN_UPGRADE : Theme::CARD_BORDER);
        m_btnFinishInf.setBaseColor(finDist >= 90000.0f ? Theme::CYAN_UPGRADE : Theme::CARD_BORDER);

        m_btnFinish500.render(fb, 1);
        m_btnFinish1000.render(fb, 1);
        m_btnFinish1500.render(fb, 1);
        m_btnFinish3000.render(fb, 1);
        m_btnFinishInf.render(fb, 1);

        int fY = rY + 80;
        Graphics::RasterFont::drawString(fb, m_panelX + 30, fY, "FUEL CONSUMPTION RATE:", Theme::TEXT_WHITE, 1);
        m_btnFuelInf.setBaseColor(physics.isInfiniteFuel() ? Theme::GOLD : Theme::CARD_BORDER);
        m_btnFuelHalf.setBaseColor(!physics.isInfiniteFuel() && physics.fuelBurnMultiplier() == 0.5f ? Theme::GOLD : Theme::CARD_BORDER);
        m_btnFuelNormal.setBaseColor(!physics.isInfiniteFuel() && physics.fuelBurnMultiplier() == 1.0f ? Theme::GOLD : Theme::CARD_BORDER);
        m_btnFuelDouble.setBaseColor(!physics.isInfiniteFuel() && physics.fuelBurnMultiplier() == 2.0f ? Theme::GOLD : Theme::CARD_BORDER);

        m_btnFuelInf.render(fb, 1);
        m_btnFuelHalf.render(fb, 1);
        m_btnFuelNormal.render(fb, 1);
        m_btnFuelDouble.render(fb, 1);

        int rubY = fY + 80;
        Graphics::RasterFont::drawString(fb, m_panelX + 30, rubY, "MULTIPLAYER CATCH-UP SLIPSTREAM:", Theme::TEXT_WHITE, 1);
        m_btnRubberbanding.render(fb, 1);

        Graphics::RasterFont::drawStringCentered(fb, m_panelX + m_panelW / 2, m_panelY + m_panelH - 24,
            "Trophy Competition Rules synchronize with remote LAN opponents", Theme::TEXT_MUTED, 1);
    } else if (m_currentTab == DevTab::AUDIO_LANG) {
        int aY = m_panelY + 80;
        Graphics::RasterFont::drawString(fb, m_panelX + 40, aY, "PROCEDURAL BACKGROUND SOUNDTRACKS:", Theme::TEXT_WHITE, 1);
        m_btnMusicPokemon.render(fb, 1);
        m_btnMusicSoothing.render(fb, 1);
        m_btnMusicEdgy.render(fb, 1);
        m_btnMusicEnergetic.render(fb, 1);

        int lY = aY + 140;
        Graphics::RasterFont::drawStringCentered(fb, m_panelX + m_panelW / 2, lY, "LANGUAGE / BANGLA MODE TOGGLE (KEY: B):", Theme::TEXT_WHITE, 1);
        m_btnToggleLang.render(fb, 1);

        m_btnGodMode.render(fb, 1);

        Graphics::RasterFont::drawStringCentered(fb, m_panelX + m_panelW / 2, m_panelY + m_panelH - 24,
            "Press B to toggle Bangla mode | Press F2 to cycle sound tracks", Theme::TEXT_MUTED, 1);
    }
}

} // namespace UI
