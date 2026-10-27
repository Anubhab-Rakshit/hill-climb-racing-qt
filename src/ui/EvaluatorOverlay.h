#pragma once

#include "Framebuffer.h"
#include <string>
#include <functional>

namespace UI {

/**
 * @brief Evaluator & Reviewer Showcase Overlay.
 * Provides on-the-fly vehicle hot-swapping, track switching, infinite fuel, and physics inspection tools.
 */
class EvaluatorOverlay {
public:
    EvaluatorOverlay();

    void setDimensions(int width, int height);

    bool isInfiniteFuel() const { return m_infiniteFuel; }
    void toggleInfiniteFuel() { m_infiniteFuel = !m_infiniteFuel; }

    bool isSlowMotion() const { return m_slowMotion; }
    void toggleSlowMotion() { m_slowMotion = !m_slowMotion; }

    bool isTurboBoost() const { return m_turboBoost; }
    void setTurboBoost(bool b) { m_turboBoost = b; }

    void render(Graphics::Framebuffer& fb, const std::string& currentVehicleName,
                const std::string& currentBiomeName);

private:
    int m_width = 960;
    int m_height = 540;
    bool m_infiniteFuel = true;
    bool m_slowMotion = false;
    bool m_turboBoost = false;
};

} // namespace UI
