#include "DayNightCycle.h"
#include <iomanip>
#include <sstream>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Graphics {

DayNightCycle::DayNightCycle(float durationSeconds, float initialTimeOfDay)
    : m_dayDurationSeconds(std::max(10.0f, durationSeconds))
    , m_timeOfDay(std::clamp(initialTimeOfDay, 0.0f, 1.0f))
    , m_speedMultiplier(1.0f)
{
}

void DayNightCycle::update(float dt, bool isGamePaused) {
    if (isGamePaused) return; // Explicitly pauses with gameplay
    float deltaNorm = (dt * m_speedMultiplier) / m_dayDurationSeconds;
    m_timeOfDay += deltaNorm;
    if (m_timeOfDay >= 1.0f) {
        m_timeOfDay = std::fmod(m_timeOfDay, 1.0f);
    }
}

void DayNightCycle::setTimeOfDay(float normalizedTime) {
    m_timeOfDay = std::fmod(std::abs(normalizedTime), 1.0f);
}

TimePhase DayNightCycle::phase() const {
    if (m_timeOfDay >= 0.20f && m_timeOfDay < 0.28f) return TimePhase::DAWN;
    if (m_timeOfDay >= 0.28f && m_timeOfDay < 0.42f) return TimePhase::MORNING;
    if (m_timeOfDay >= 0.42f && m_timeOfDay < 0.58f) return TimePhase::NOON;
    if (m_timeOfDay >= 0.58f && m_timeOfDay < 0.72f) return TimePhase::AFTERNOON;
    if (m_timeOfDay >= 0.72f && m_timeOfDay < 0.80f) return TimePhase::SUNSET;
    if (m_timeOfDay >= 0.80f && m_timeOfDay < 0.88f) return TimePhase::DUSK;
    return TimePhase::NIGHT;
}

std::string DayNightCycle::phaseName() const {
    switch (phase()) {
        case TimePhase::DAWN: return "Dawn";
        case TimePhase::MORNING: return "Morning";
        case TimePhase::NOON: return "Noon";
        case TimePhase::AFTERNOON: return "Afternoon";
        case TimePhase::SUNSET: return "Sunset";
        case TimePhase::DUSK: return "Dusk";
        case TimePhase::NIGHT: return "Night";
    }
    return "Day";
}

std::string DayNightCycle::clockString() const {
    int totalMinutes = static_cast<int>(m_timeOfDay * 1440.0f) % 1440;
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes;
    return oss.str();
}

uint32_t DayNightCycle::blendColor(uint32_t c1, uint32_t c2, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float a1 = static_cast<float>((c1 >> 24) & 0xFF);
    float r1 = static_cast<float>((c1 >> 16) & 0xFF);
    float g1 = static_cast<float>((c1 >> 8) & 0xFF);
    float b1 = static_cast<float>(c1 & 0xFF);

    float a2 = static_cast<float>((c2 >> 24) & 0xFF);
    float r2 = static_cast<float>((c2 >> 16) & 0xFF);
    float g2 = static_cast<float>((c2 >> 8) & 0xFF);
    float b2 = static_cast<float>(c2 & 0xFF);

    uint32_t a = static_cast<uint32_t>(std::clamp(a1 + (a2 - a1) * t, 0.0f, 255.0f));
    uint32_t r = static_cast<uint32_t>(std::clamp(r1 + (r2 - r1) * t, 0.0f, 255.0f));
    uint32_t g = static_cast<uint32_t>(std::clamp(g1 + (g2 - g1) * t, 0.0f, 255.0f));
    uint32_t b = static_cast<uint32_t>(std::clamp(b1 + (b2 - b1) * t, 0.0f, 255.0f));

    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t DayNightCycle::applyAmbient(uint32_t baseColor, uint32_t ambientColor, float brightness) {
    brightness = std::clamp(brightness, 0.52f, 1.15f);

    uint32_t a = (baseColor >> 24) & 0xFF;
    uint32_t br = (baseColor >> 16) & 0xFF;
    uint32_t bg = (baseColor >> 8) & 0xFF;
    uint32_t bb = baseColor & 0xFF;

    uint32_t ar = (ambientColor >> 16) & 0xFF;
    uint32_t ag = (ambientColor >> 8) & 0xFF;
    uint32_t ab = ambientColor & 0xFF;

    float normAr = (ar / 255.0f) * brightness;
    float normAg = (ag / 255.0f) * brightness;
    float normAb = (ab / 255.0f) * brightness;

    uint32_t r = static_cast<uint32_t>(std::clamp(br * normAr, 0.0f, 255.0f));
    uint32_t g = static_cast<uint32_t>(std::clamp(bg * normAg, 0.0f, 255.0f));
    uint32_t b = static_cast<uint32_t>(std::clamp(bb * normAb, 0.0f, 255.0f));

    return (a << 24) | (r << 16) | (g << 8) | b;
}

LightingState DayNightCycle::getLighting(const std::string& biomeId, int screenW, int screenH, float camX) const {
    LightingState s;
    float t = m_timeOfDay;

    // 1. Calculate Night Factor [0.0 = Noon, 1.0 = Midnight]
    if (t >= 0.50f) {
        // Afternoon -> Sunset -> Midnight
        s.nightFactor = std::clamp((t - 0.50f) / 0.40f, 0.0f, 1.0f);
    } else {
        // Midnight -> Dawn -> Noon
        s.nightFactor = std::clamp(1.0f - (t - 0.10f) / 0.40f, 0.0f, 1.0f);
    }

    // 2. Stars Opacity (fades in after sunset, fades out at sunrise)
    if (biomeId == "moon") {
        s.starsOpacity = 1.0f; // Moon is in permanent space vacuum
    } else {
        if (t >= 0.70f && t < 0.85f) {
            s.starsOpacity = (t - 0.70f) / 0.15f;
        } else if (t >= 0.85f || t < 0.18f) {
            s.starsOpacity = 1.0f;
        } else if (t >= 0.18f && t < 0.30f) {
            s.starsOpacity = 1.0f - (t - 0.18f) / 0.12f;
        } else {
            s.starsOpacity = 0.0f;
        }
    }

    // 3. Sun and Moon Celestial Positions & Opacity
    // Daytime arc: t in [0.20, 0.80]
    if (t >= 0.20f && t <= 0.80f) {
        float uSun = (t - 0.20f) / 0.60f;
        s.sunScreenX = std::fmod((screenW * 0.10f + screenW * 0.80f * uSun) - camX * 0.012f + screenW * 4.0f, static_cast<float>(screenW + 200)) - 100.0f;
        s.sunScreenY = screenH * 0.72f - std::sin(static_cast<float>(M_PI * uSun)) * (screenH * 0.55f);

        if (uSun < 0.15f) s.sunOpacity = uSun / 0.15f;
        else if (uSun > 0.85f) s.sunOpacity = (1.0f - uSun) / 0.15f;
        else s.sunOpacity = 1.0f;
    } else {
        s.sunOpacity = 0.0f;
        s.sunScreenX = -200.0f;
        s.sunScreenY = screenH + 200.0f;
    }

    // Nighttime arc: t in [0.75, 1.0] and [0.0, 0.25]
    if (t >= 0.74f || t <= 0.26f) {
        float uMoon = (t >= 0.74f) ? (t - 0.74f) / 0.52f : (t + 0.26f) / 0.52f;
        s.moonScreenX = std::fmod((screenW * 0.12f + screenW * 0.76f * uMoon) - camX * 0.010f + screenW * 4.0f, static_cast<float>(screenW + 200)) - 100.0f;
        s.moonScreenY = screenH * 0.66f - std::sin(static_cast<float>(M_PI * uMoon)) * (screenH * 0.48f);

        if (uMoon < 0.12f) s.moonOpacity = uMoon / 0.12f;
        else if (uMoon > 0.88f) s.moonOpacity = (1.0f - uMoon) / 0.12f;
        else s.moonOpacity = 1.0f;
    } else {
        s.moonOpacity = 0.0f;
        s.moonScreenX = -200.0f;
        s.moonScreenY = screenH + 200.0f;
    }

    // 4. Nighttime Details & Atmospheric Effects
    s.auroraOpacity = (biomeId == "arctic") ? std::clamp((s.nightFactor - 0.25f) / 0.75f, 0.0f, 1.0f) : 0.0f;
    s.fireflyOpacity = (biomeId == "countryside") ? std::clamp((s.nightFactor - 0.30f) / 0.70f, 0.0f, 1.0f) : 0.0f;
    s.windowLightOpacity = std::clamp((s.nightFactor - 0.20f) / 0.80f, 0.0f, 1.0f);

    // 5. Ambient Brightness and Color Shading
    // Clamping guarantees terrain readability at all hours
    s.ambientBrightness = 1.0f - (s.nightFactor * 0.42f); // Range [0.58f, 1.0f]

    if (s.nightFactor > 0.65f) {
        // Moonlit nocturnal ambiance
        s.ambientColor = 0xFF88A8D8; // Cool soft blue moonlight
    } else if (t >= 0.70f && t < 0.82f) {
        // Sunset golden hour
        s.ambientColor = 0xFFFFD2A0; // Warm amber sunlight
    } else if (t >= 0.20f && t < 0.32f) {
        // Dawn glow
        s.ambientColor = 0xFFFFE0B2; // Peach dawn light
    } else {
        // Crisp full daylight
        s.ambientColor = 0xFFFFFFFF;
    }

    // 6. Multi-Phase Sky Palette Keyframe Interpolation
    struct SkyKeyframe {
        float time;
        uint32_t top;
        uint32_t bot;
        uint32_t glow;
    };

    std::vector<SkyKeyframe> keys;
    if (biomeId == "desert") {
        keys = {
            {0.00f, 0xFF080C1E, 0xFF181C30, 0xFF2A2840}, // Midnight
            {0.20f, 0xFF14182C, 0xFF352B3C, 0xFF654448}, // Pre-dawn
            {0.26f, 0xFF2B2544, 0xFFFF8A65, 0xFFFFCC80}, // Dawn
            {0.36f, 0xFF1E88E5, 0xFFFFCC80, 0xFFFFE082}, // Morning
            {0.50f, 0xFF0D47A1, 0xFFFFB74D, 0xFFFFF176}, // Noon
            {0.68f, 0xFF1565C0, 0xFFFFA726, 0xFFFFD54F}, // Afternoon
            {0.76f, 0xFF4A148C, 0xFFFF5722, 0xFFFF9800}, // Sunset
            {0.83f, 0xFF1A0C2E, 0xFF880E4F, 0xFFB71C1C}, // Dusk
            {0.90f, 0xFF080C1E, 0xFF181C30, 0xFF2A2840}, // Night
            {1.00f, 0xFF080C1E, 0xFF181C30, 0xFF2A2840}
        };
    } else if (biomeId == "arctic") {
        keys = {
            {0.00f, 0xFF030714, 0xFF0C1628, 0xFF182A40}, // Midnight Polar Night
            {0.20f, 0xFF0B1424, 0xFF1E3048, 0xFF385570},
            {0.26f, 0xFF1C2D44, 0xFFFF8FA3, 0xFFFFC1D0}, // Pink Arctic Dawn
            {0.36f, 0xFF375870, 0xFFCFD8DC, 0xFFECEFF1}, // Morning Frost
            {0.50f, 0xFF456882, 0xFFE0F2F1, 0xFFFFFFFF}, // Noon
            {0.68f, 0xFF375870, 0xFFB0BEC5, 0xFFECEFF1}, // Afternoon
            {0.76f, 0xFF38224C, 0xFFFF80AB, 0xFFFF4081}, // Polar Sunset
            {0.83f, 0xFF16102C, 0xFF3F1D56, 0xFF5C2A78}, // Dusk
            {0.90f, 0xFF030714, 0xFF0C1628, 0xFF182A40}, // Night
            {1.00f, 0xFF030714, 0xFF0C1628, 0xFF182A40}
        };
    } else if (biomeId == "mountain") {
        keys = {
            {0.00f, 0xFF040A18, 0xFF0F1B30, 0xFF1C2C44}, // Midnight Alpine
            {0.20f, 0xFF0F182C, 0xFF283448, 0xFF4A5668},
            {0.26f, 0xFF24223E, 0xFFFF8A65, 0xFFFFB74D}, // Alpine Dawn
            {0.36f, 0xFF1E5B94, 0xFF90CAF9, 0xFFBBDEFB}, // Morning
            {0.50f, 0xFF1565C0, 0xFFBBDEFB, 0xFFE3F2FD}, // Noon High Peak
            {0.68f, 0xFF1E5B94, 0xFF90CAF9, 0xFFBBDEFB},
            {0.76f, 0xFF4A148C, 0xFFFF6E40, 0xFFFFAB40}, // Alpenglow Sunset
            {0.83f, 0xFF1E1035, 0xFF512DA8, 0xFF7E57C2}, // Dusk
            {0.90f, 0xFF040A18, 0xFF0F1B30, 0xFF1C2C44}, // Night
            {1.00f, 0xFF040A18, 0xFF0F1B30, 0xFF1C2C44}
        };
    } else if (biomeId == "volcano") {
        keys = {
            {0.00f, 0xFF0C0202, 0xFF240606, 0xFF4E0C0C}, // Pitch Volcanic Night
            {0.20f, 0xFF180505, 0xFF350A0A, 0xFF651212},
            {0.26f, 0xFF320808, 0xFFFF5722, 0xFFFF8A65}, // Dawn Cinder
            {0.36f, 0xFF2A1010, 0xFF6E2818, 0xFFBF360C}, // Morning
            {0.50f, 0xFF261010, 0xFF792D1D, 0xFFD84315}, // Noon Ash Sky
            {0.68f, 0xFF2A1010, 0xFF6E2818, 0xFFBF360C},
            {0.76f, 0xFF3E0A0A, 0xFFFF3D00, 0xFFFF9100}, // Fiery Sunset
            {0.83f, 0xFF240606, 0xFFBF360C, 0xFFFF5722}, // Dusk
            {0.90f, 0xFF0C0202, 0xFF240606, 0xFF4E0C0C}, // Night
            {1.00f, 0xFF0C0202, 0xFF240606, 0xFF4E0C0C}
        };
    } else if (biomeId == "moon") {
        // Lunar vacuum sky: deep cosmic black space with faint nebula dust
        keys = {
            {0.00f, 0xFF000206, 0xFF030610, 0xFF080C1A},
            {0.25f, 0xFF000206, 0xFF040712, 0xFF0A0F20},
            {0.50f, 0xFF000206, 0xFF050814, 0xFF0C1224},
            {0.75f, 0xFF000206, 0xFF040712, 0xFF0A0F20},
            {1.00f, 0xFF000206, 0xFF030610, 0xFF080C1A}
        };
    } else {
        // Default: COUNTRYSIDE
        keys = {
            {0.00f, 0xFF050B18, 0xFF0F1A30, 0xFF1C2C48}, // Midnight
            {0.20f, 0xFF0E1830, 0xFF25324E, 0xFF455575}, // Pre-dawn
            {0.26f, 0xFF22284E, 0xFFFF9E80, 0xFFFFD180}, // Dawn Peach/Gold
            {0.36f, 0xFF2196F3, 0xFF90CAF9, 0xFFE3F2FD}, // Morning Blue
            {0.50f, 0xFF1E88E5, 0xFFBBDEFB, 0xFFFFF9C4}, // Midday Azure
            {0.68f, 0xFF1976D2, 0xFF90CAF9, 0xFFFFE082}, // Afternoon
            {0.76f, 0xFF4A148C, 0xFFFF6F00, 0xFFFFB300}, // Vibrant Sunset
            {0.83f, 0xFF1A1035, 0xFF6A1B9A, 0xFF8E24AA}, // Dusk Twilight
            {0.90f, 0xFF050B18, 0xFF0F1A30, 0xFF1C2C48}, // Night
            {1.00f, 0xFF050B18, 0xFF0F1A30, 0xFF1C2C48}
        };
    }

    // Interpolate between surrounding keyframes
    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (t >= keys[i].time && t <= keys[i + 1].time) {
            float frac = (t - keys[i].time) / (keys[i + 1].time - keys[i].time);
            s.skyTopColor = blendColor(keys[i].top, keys[i + 1].top, frac);
            s.skyBottomColor = blendColor(keys[i].bot, keys[i + 1].bot, frac);
            s.horizonGlowColor = blendColor(keys[i].glow, keys[i + 1].glow, frac);
            break;
        }
    }

    return s;
}

} // namespace Graphics
