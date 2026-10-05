#pragma once

#include "Framebuffer.h"
#include <string>

namespace UI {

/**
 * @brief Circular Analog Gauge (Tachometer & Speedometer) rendered entirely with raster algorithms.
 * Includes dial tick marks, redline arc, swept needle, center hub, and digital readout.
 */
class AnalogGauge {
public:
    AnalogGauge(int xc = 0, int yc = 0, int radius = 45,
                float minVal = 0.0f, float maxVal = 100.0f,
                const std::string& label = "RPM", const std::string& unit = "x1000",
                float redlineFrac = 0.8f);

    void setCenter(int xc, int yc) { m_xc = xc; m_yc = yc; }
    void setValue(float targetVal);
    void update(float dt); // Smooth needle damping
    void render(Graphics::Framebuffer& fb);

    float currentValue() const { return m_currentVal; }

private:
    int m_xc;
    int m_yc;
    int m_radius;
    float m_minVal;
    float m_maxVal;
    float m_currentVal;
    float m_targetVal;
    std::string m_label;
    std::string m_unit;
    float m_redlineFrac;
};

} // namespace UI
