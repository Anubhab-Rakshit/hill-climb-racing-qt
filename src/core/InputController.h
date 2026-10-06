#pragma once

#include <QKeyEvent>
#include <QMouseEvent>

namespace Core {

/**
 * @brief Low-latency non-blocking Input Controller.
 * Eliminates OS key-repeat delays and combines keyboard + touch/mouse pedal states.
 */
class InputController {
public:
    struct State {
        bool gas = false;
        bool brake = false;
        bool pause = false;
        bool restart = false;
        int mouseX = 0;
        int mouseY = 0;
        bool mouseDown = false;
        bool mouseJustPressed = false;
        bool mouseJustReleased = false;
        bool mouseMoved = false;
    };

    InputController();

    void onKeyPress(QKeyEvent* event);
    void onKeyRelease(QKeyEvent* event);
    void onMouseMove(QMouseEvent* event, int virtualW, int virtualH, int widgetW, int widgetH);
    void onMousePress(QMouseEvent* event, int virtualW, int virtualH, int widgetW, int widgetH);
    void onMouseRelease(QMouseEvent* event, int virtualW, int virtualH, int widgetW, int widgetH);

    const State& state() const { return m_state; }
    State& state() { return m_state; }

    void resetTransientActions();

private:
    State m_state;
};

} // namespace Core
