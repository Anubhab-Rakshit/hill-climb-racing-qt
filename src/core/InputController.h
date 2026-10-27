#pragma once

#include <QKeyEvent>
#include <QMouseEvent>

namespace Core {

/**
 * @brief Low-latency non-blocking Input Controller.
 * Eliminates OS key-repeat delays and combines keyboard + touch/mouse pedal states.
 * Supports dual-player split keyboard controls: P1 (WASD) vs P2 (Arrow Keys).
 */
class InputController {
public:
    struct PlayerControls {
        bool gas = false;
        bool brake = false;
        bool reverse = false;
        bool tiltLeft = false;
        bool tiltRight = false;
    };

    struct State {
        // Player 1 controls (W = gas, S = brake/rev, A = tilt left, D = tilt right)
        PlayerControls p1;

        // Player 2 controls (Up = gas, Down = brake/rev, Left = tilt left, Right = tilt right)
        PlayerControls p2;

        // Single Player combined convenience flags
        bool gas = false;
        bool brake = false;
        bool reverse = false;

        bool pause = false;
        bool restart = false;
        bool enter = false;
        bool multiplayer = false;
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

    void setOnKeyTyped(std::function<void(int key, const std::string& text)> cb) { m_keyTypedCallback = cb; }

    void resetTransientActions();

private:
    State m_state;
    std::function<void(int, const std::string&)> m_keyTypedCallback;

    void updateCombinedInputs();
};

} // namespace Core
