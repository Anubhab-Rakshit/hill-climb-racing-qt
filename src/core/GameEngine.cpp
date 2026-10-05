#include "GameEngine.h"
#include "UITheme.h"
#include <algorithm>

namespace Core {

GameEngine::GameEngine(QWidget* parent)
    : QMainWindow(parent)
    , m_framebuffer(UI::Theme::VIRTUAL_WIDTH, UI::Theme::VIRTUAL_HEIGHT)
    , m_canvas(new Graphics::PixelCanvas(m_framebuffer, m_input, this))
    , m_accumulator(0.0f)
{
    setWindowTitle("Hill Climb Racing (Qt C++ Pure-Raster Edition)");
    resize(1280, 720); // Default 16:9 window size
    setCentralWidget(m_canvas);

    connect(&m_tickTimer, &QTimer::timeout, this, &GameEngine::onTick);
}

GameEngine::~GameEngine() {
    m_tickTimer.stop();
}

void GameEngine::start() {
    m_elapsedTimer.start();
    // 60 FPS display tick interval (~16 ms)
    m_tickTimer.start(16);
}

void GameEngine::onTick() {
    float realDt = m_elapsedTimer.restart() / 1000.0f;
    // Prevent spiral of death on window hitches
    if (realDt > 0.25f) realDt = 0.25f;

    m_accumulator += realDt;

    // Fixed timestep physics tick at 120 Hz
    const float FIXED_DT = 1.0f / 120.0f;
    while (m_accumulator >= FIXED_DT) {
        m_stateManager.handleInput(m_input.state());
        m_input.resetTransientActions();
        m_stateManager.fixedUpdate(FIXED_DT);
        m_accumulator -= FIXED_DT;
    }

    // Render state with interpolation
    float alpha = m_accumulator / FIXED_DT;
    m_stateManager.render(m_framebuffer, alpha);

    // Request Qt Repaint
    m_canvas->update();
}

} // namespace Core
