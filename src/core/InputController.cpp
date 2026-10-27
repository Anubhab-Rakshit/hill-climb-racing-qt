#include "InputController.h"
#include <algorithm>

namespace Core {

InputController::InputController() {
}

void InputController::resetTransientActions() {
    m_state.pause = false;
    m_state.restart = false;
    m_state.enter = false;
    m_state.multiplayer = false;
    m_state.mouseJustPressed = false;
    m_state.mouseJustReleased = false;
    m_state.mouseMoved = false;
}

void InputController::updateCombinedInputs() {
    m_state.gas = m_state.p1.gas || m_state.p2.gas || m_state.p1.tiltRight || m_state.p2.tiltRight;
    m_state.brake = m_state.p1.brake || m_state.p2.brake || m_state.p1.tiltLeft || m_state.p2.tiltLeft;
    m_state.reverse = m_state.p1.reverse || m_state.p2.reverse;
}

void InputController::onKeyPress(QKeyEvent* event) {
    if (m_keyTypedCallback) {
        m_keyTypedCallback(event->key(), event->text().toStdString());
    }

    if (event->isAutoRepeat()) return;

    int k = event->key();

    // Player 1 (WASD)
    if (k == Qt::Key_W) {
        m_state.p1.gas = true;
    } else if (k == Qt::Key_S) {
        m_state.p1.brake = true;
        m_state.p1.reverse = true;
    } else if (k == Qt::Key_A) {
        m_state.p1.tiltLeft = true;
    } else if (k == Qt::Key_D) {
        m_state.p1.tiltRight = true;
    }

    // Player 2 (Arrow Keys)
    if (k == Qt::Key_Up) {
        m_state.p2.gas = true;
    } else if (k == Qt::Key_Down) {
        m_state.p2.brake = true;
        m_state.p2.reverse = true;
    } else if (k == Qt::Key_Left) {
        m_state.p2.tiltLeft = true;
    } else if (k == Qt::Key_Right) {
        m_state.p2.tiltRight = true;
    }

    // Navigation / Global Shortcuts
    if (k == Qt::Key_Escape || k == Qt::Key_P) {
        m_state.pause = true;
    } else if (k == Qt::Key_R) {
        m_state.restart = true;
    } else if (k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_Space) {
        m_state.enter = true;
    } else if (k == Qt::Key_M) {
        m_state.multiplayer = true;
    }

    updateCombinedInputs();
}

void InputController::onKeyRelease(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;

    int k = event->key();

    // Player 1 (WASD)
    if (k == Qt::Key_W) {
        m_state.p1.gas = false;
    } else if (k == Qt::Key_S) {
        m_state.p1.brake = false;
        m_state.p1.reverse = false;
    } else if (k == Qt::Key_A) {
        m_state.p1.tiltLeft = false;
    } else if (k == Qt::Key_D) {
        m_state.p1.tiltRight = false;
    }

    // Player 2 (Arrow Keys)
    if (k == Qt::Key_Up) {
        m_state.p2.gas = false;
    } else if (k == Qt::Key_Down) {
        m_state.p2.brake = false;
        m_state.p2.reverse = false;
    } else if (k == Qt::Key_Left) {
        m_state.p2.tiltLeft = false;
    } else if (k == Qt::Key_Right) {
        m_state.p2.tiltRight = false;
    }

    updateCombinedInputs();
}

static void mapMouseCoords(int rawX, int rawY, int virtW, int virtH, int widW, int widH, int& outX, int& outY) {
    if (widW <= 0 || widH <= 0) {
        outX = rawX;
        outY = rawY;
        return;
    }
    // Calculate aspect ratio scaling
    float scaleX = static_cast<float>(widW) / virtW;
    float scaleY = static_cast<float>(widH) / virtH;
    float scale = std::min(scaleX, scaleY);
    if (scale <= 0.0001f) scale = 1.0f;

    int viewportW = static_cast<int>(virtW * scale);
    int viewportH = static_cast<int>(virtH * scale);
    int offsetX = (widW - viewportW) / 2;
    int offsetY = (widH - viewportH) / 2;

    outX = static_cast<int>((rawX - offsetX) / scale);
    outY = static_cast<int>((rawY - offsetY) / scale);
}

void InputController::onMouseMove(QMouseEvent* event, int virtualW, int virtualH, int widgetW, int widgetH) {
    m_state.mouseMoved = true;
    mapMouseCoords(event->pos().x(), event->pos().y(), virtualW, virtualH, widgetW, widgetH, m_state.mouseX, m_state.mouseY);
}

void InputController::onMousePress(QMouseEvent* event, int virtualW, int virtualH, int widgetW, int widgetH) {
    if (event->button() == Qt::LeftButton) {
        m_state.mouseDown = true;
        m_state.mouseJustPressed = true;
        mapMouseCoords(event->pos().x(), event->pos().y(), virtualW, virtualH, widgetW, widgetH, m_state.mouseX, m_state.mouseY);
    }
}

void InputController::onMouseRelease(QMouseEvent* event, int virtualW, int virtualH, int widgetW, int widgetH) {
    if (event->button() == Qt::LeftButton) {
        m_state.mouseDown = false;
        m_state.mouseJustReleased = true;
        mapMouseCoords(event->pos().x(), event->pos().y(), virtualW, virtualH, widgetW, widgetH, m_state.mouseX, m_state.mouseY);
    }
}

} // namespace Core
