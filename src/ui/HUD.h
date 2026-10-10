#pragma once

#include "Framebuffer.h"
#include "AnalogGauge.h"
#include "FuelBar.h"
#include "StuntBanner.h"
#include "RetroButton.h"

namespace UI {

/**
 * @brief Master in-game telemetry HUD.
 * Manages virtual touch/mouse pedals (GAS / BRAKE), analog gauges, fuel bar,
 * odometer, coin balances, and stunt banners.
 */
class HUD {
public:
    HUD();

    void setDimensions(int width, int height);

    void setFuel(float fuelPercent) { m_fuelBar.setFuel(fuelPercent); }
    void setSpeed(float kmh) { m_speedometer.setValue(kmh); }
    void setRpm(float rpm) { m_tachometer.setValue(rpm); }
    void setDistance(float meters) { m_distance = meters; }
    void setRecord(float meters) { m_recordDistance = meters; }
    void setCoins(int coins) { m_coins = coins; }
    void setCheckpointProgress(int cleared, int total) { m_clearedCheckpoints = cleared; m_totalCheckpoints = total; }
    void setNextCheckpoint(const std::string& name, float dist) { m_nextCheckpointName = name; m_nextCheckpointDist = dist; }

    void triggerStunt(const std::string& title, int bonusCoins, uint32_t color = 0xFFFFB300) {
        m_stuntBanner.trigger(title, bonusCoins, color);
    }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    bool isBrakePressed() const { return m_brakePedal.isPressed(); }
    bool isGasPressed() const { return m_gasPedal.isPressed(); }

    void setBrakeVirtualPressed(bool pressed);
    void setGasVirtualPressed(bool pressed);

    void setOnPauseClicked(std::function<void()> cb) { m_pauseButton.setOnClick(cb); }
    void setButtonSound(std::function<void()> cb) { m_pauseButton.setOnSound(cb); }

    void addFloatingText(const std::string& text, float x, float y, uint32_t color = 0xFFFFB300);

    void update(float dt);
    void render(Graphics::Framebuffer& fb);

private:
    int m_width;
    int m_height;
    float m_animTime;

    FuelBar m_fuelBar;
    AnalogGauge m_speedometer;
    AnalogGauge m_tachometer;
    StuntBanner m_stuntBanner;

    RetroButton m_brakePedal;
    RetroButton m_gasPedal;
    RetroButton m_pauseButton;

    float m_distance;
    float m_recordDistance;
    int m_coins;
    int m_clearedCheckpoints;
    int m_totalCheckpoints;
    std::string m_nextCheckpointName;
    float m_nextCheckpointDist;

    struct FloatingText {
        std::string text;
        float x;
        float y;
        float life;
        float maxLife;
        uint32_t color;
    };
    std::vector<FloatingText> m_floatingTexts;
};

} // namespace UI
