#pragma once

namespace UI {

/**
 * @brief Automated UI layout regression helper.
 * Renders key screens at 960x540 and 1280x720, saves 12 reference captures,
 * and programmatically verifies geometric bounding invariants and non-overlap constraints.
 */
class UILayoutRegression {
public:
    static int run();
};

} // namespace UI
