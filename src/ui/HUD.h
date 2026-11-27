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
    void triggerFuelFlash() { m_fuelBar.triggerPickupFlash(); }
    void setNitro(float nitroPercent) { m_nitro = nitroPercent; }
    void setBoosting(bool boosting) { m_isBoosting = boosting; }
    void setSpeed(float kmh) { m_speedometer.setValue(kmh); }
    void setRpm(float rpm) { m_tachometer.setValue(rpm); }
    void setDistance(float meters) { m_distance = meters; }
    void setRecord(float meters) { m_recordDistance = meters; }
    void setCoins(int coins) { m_coins = coins; }
    void setCheckpointProgress(int cleared, int total) { m_clearedCheckpoints = cleared; m_totalCheckpoints = total; }
    void setNextCheckpoint(const std::string& name, float dist) { m_nextCheckpointName = name; m_nextCheckpointDist = dist; }

    void setSurfaceInfo(const std::string& name, uint32_t col) { m_surfaceName = name; m_surfaceColor = col; }
    void setRolloverWarning(bool warn) { m_rolloverWarning = warn; }
    void setBountyInfo(const std::string& title, const std::string& progress, int reward) {
        m_bountyTitle = title;
        m_bountyProgress = progress;
        m_bountyReward = reward;
    }

    void triggerStunt(const std::string& title, int bonusCoins, uint32_t color = 0xFFFFB300) {
        m_stuntBanner.trigger(title, bonusCoins, color);
    }

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py);
    void onMouseUp(int px, int py);

    bool isBrakePressed() const { return m_brakePedal.isPressed(); }
    bool isReversePressed() const { return m_reversePedal.isPressed(); }
    bool isGasPressed() const { return m_gasPedal.isPressed(); }
    bool isBoostPressed() const { return m_boostPedal.isPressed(); }

    void setBrakeVirtualPressed(bool pressed);
    void setReverseVirtualPressed(bool pressed);
    void setGasVirtualPressed(bool pressed);
    void setBoostVirtualPressed(bool pressed);

    void setOnPauseClicked(std::function<void()> cb) { m_pauseButton.setOnClick(cb); }
    void setButtonSound(std::function<void()> cb) { m_pauseButton.setOnSound(cb); }

    void addFloatingText(const std::string& text, float x, float y, uint32_t color = 0xFFFFB300);

    const RetroButton& brakePedal() const { return m_brakePedal; }
    const RetroButton& reversePedal() const { return m_reversePedal; }
    const RetroButton& gasPedal() const { return m_gasPedal; }
    const RetroButton& boostPedal() const { return m_boostPedal; }
    const RetroButton& pauseButton() const { return m_pauseButton; }

    int distanceCardX() const { return (m_width - distanceCardWidth()) / 2; }
    int distanceCardY() const { return (m_height >= 700) ? 42 : 38; }
    int distanceCardWidth() const { return (m_width >= 1200) ? 380 : 320; }
    int distanceCardHeight() const { return 48; }

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
    RetroButton m_reversePedal;
    RetroButton m_gasPedal;
    RetroButton m_boostPedal;
    RetroButton m_pauseButton;

    float m_distance;
    float m_recordDistance;
    int m_coins;
    int m_clearedCheckpoints;
    int m_totalCheckpoints;
    std::string m_nextCheckpointName;
    float m_nextCheckpointDist;

    // Nitro & Dynamic Telemetry
    float m_nitro = 100.0f;
    bool m_isBoosting = false;
    std::string m_surfaceName;
    uint32_t m_surfaceColor = 0xFFFFFFFF;
    bool m_rolloverWarning = false;

    // In-Run Bounties
    std::string m_bountyTitle;
    std::string m_bountyProgress;
    int m_bountyReward = 0;

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
