#pragma once

#include "Framebuffer.h"
#include <string>
#include <vector>
#include <cstdint>

namespace UI {

struct RadarRacer {
    std::string name;
    float distance = 0.0f;
    uint32_t color = 0xFF00E5FF;
    bool isLocal = false;
    bool isCrashed = false;

    RadarRacer() = default;
    RadarRacer(const std::string& n, float d, uint32_t c, bool loc, bool cr)
        : name(n), distance(d), color(c), isLocal(loc), isCrashed(cr) {}
};

/**
 * @brief Contracted Minimap / Race Progress Radar.
 * Displays relative positions of all racers along the track timeline with leader markers.
 */
class MinimapRadar {
public:
    MinimapRadar();

    void setDimensions(int x, int y, int width, int height);
    void setCourseLength(float finishDistanceMeters);

    void render(Graphics::Framebuffer& fb, const std::vector<RadarRacer>& racers);

private:
    int m_x = 270;
    int m_y = 10;
    int m_width = 420;
    int m_height = 24;
    float m_courseLength = 1500.0f;
};

} // namespace UI
