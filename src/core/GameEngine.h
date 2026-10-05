#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

#include "Framebuffer.h"
#include "PixelCanvas.h"
#include "InputController.h"
#include "GameStateManager.h"

namespace Core {

/**
 * @brief Master Game Engine coordinating the 120Hz physics accumulator loop and 60Hz display refresh.
 */
class GameEngine : public QMainWindow {
    Q_OBJECT
public:
    explicit GameEngine(QWidget* parent = nullptr);
    ~GameEngine() override;

    void start();

private slots:
    void onTick();

private:
    Graphics::Framebuffer m_framebuffer;
    InputController m_input;
    GameStateManager m_stateManager;
    Graphics::PixelCanvas* m_canvas;

    QTimer m_tickTimer;
    QElapsedTimer m_elapsedTimer;
    float m_accumulator;
};

} // namespace Core
