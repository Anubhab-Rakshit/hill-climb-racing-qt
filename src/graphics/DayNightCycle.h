#pragma once

#include <cstdint>
#include <string>
#include <algorithm>
#include <cmath>

namespace Graphics {

enum class TimePhase {
    NIGHT = 0,
    DAWN,
    MORNING,
    NOON,
    AFTERNOON,
    SUNSET,
    DUSK
};

struct LightingState {
    uint32_t skyTopColor = 0xFF42A5F5;
    uint32_t skyBottomColor = 0xFFBBDEFB;
    uint32_t horizonGlowColor = 0xFFFFD54F;
    uint32_t sunColor = 0xFFFFF176;
    uint32_t moonColor = 0xFFECEFF1;
    uint32_t ambientColor = 0xFFFFFFFF;

    float ambientBrightness = 1.0f; // [0.55, 1.0f] High readability guarantee
    float nightFactor = 0.0f;       // 0.0 at noon, 1.0 at midnight
    float starsOpacity = 0.0f;      // 0.0 to 1.0
    float sunOpacity = 1.0f;        // 0.0 to 1.0
    float moonOpacity = 0.0f;       // 0.0 to 1.0
    float auroraOpacity = 0.0f;     // 0.0 to 1.0 (Arctic polar night)
    float windowLightOpacity = 0.0f;// 0.0 at day, 1.0 at night
    float fireflyOpacity = 0.0f;    // 0.0 at day, 1.0 at night

    float sunScreenX = 0.0f;
    float sunScreenY = 0.0f;
    float moonScreenX = 0.0f;
    float moonScreenY = 0.0f;
};

/**
 * @brief Realistic periodic Day-Night cycle system for Hill Climb Racing.
 * Governs in-game time progression, astronomical celestial trajectories,
 * smooth multi-phase palette transitions, and atmospheric lighting.
 */
class DayNightCycle {
public:
    explicit DayNightCycle(float durationSeconds = 1200.0f, float initialTimeOfDay = 0.40f);

    // Frame update (explicitly respects gameplay pause state)
    void update(float dt, bool isGamePaused = false);

    // Time controls
    void setTimeOfDay(float normalizedTime); // 0.0 to 1.0 (0.0 = 00:00, 0.5 = 12:00)
    float timeOfDay() const { return m_timeOfDay; }

    void setDayDurationSeconds(float sec) { m_dayDurationSeconds = std::max(10.0f, sec); }
    float dayDurationSeconds() const { return m_dayDurationSeconds; }

    void setSpeedMultiplier(float mult) { m_speedMultiplier = std::max(0.0f, mult); }
    float speedMultiplier() const { return m_speedMultiplier; }

    TimePhase phase() const;
    std::string phaseName() const;
    std::string clockString() const; // Format "HH:MM"

    // Lighting evaluation for a given biome and camera viewport
    LightingState getLighting(const std::string& biomeId, int screenW, int screenH, float camX) const;

    // Fast color blending utilities
    static uint32_t blendColor(uint32_t c1, uint32_t c2, float t);
    static uint32_t applyAmbient(uint32_t baseColor, uint32_t ambientColor, float brightness);

private:
    float m_dayDurationSeconds;
    float m_timeOfDay; // 0.0 to 1.0
    float m_speedMultiplier;
};

} // namespace Graphics
